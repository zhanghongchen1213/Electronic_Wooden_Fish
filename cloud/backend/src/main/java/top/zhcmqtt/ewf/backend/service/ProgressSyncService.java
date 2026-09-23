package top.zhcmqtt.ewf.backend.service;

import java.io.IOException;
import java.io.InputStream;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import org.springframework.core.io.ClassPathResource;
import org.springframework.stereotype.Service;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.dto.sync.RoundActionRequest;
import top.zhcmqtt.ewf.backend.dto.sync.SettingsCommandRequest;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.dto.sync.SyncReportRequest;
import top.zhcmqtt.ewf.backend.ws.RealtimePushPort;

/**
 * 高水位幂等同步与轮次完成/跨轮动作的唯一写路径入口。
 *
 * <p>业务规则不散落在控制器；只读装配委托 {@link StateSnapshotService#assemble(String)}，
 * 本类负责权威推进、完成确认、篇章动作与派生镜像写入。
 *
 * <h2>裁决 A（Story 5.1）：高水位推进操作数</h2>
 * <ul>
 *   <li>推进条件：{@code request.local_total > cloud.acked_total}，新权威
 *       {@code acked_total := request.local_total}，同时镜像 {@code local_total}；</li>
 *   <li>{@code request.acked_total} 只作差量/积压判定输入，不得写成权威 {@code acked_total}；</li>
 *   <li>{@code local_total == cloud.acked_total} → 成功 no-op（可更新轮次事务组与派生镜像；
 *       已存 {@code action_id} 不得被新键覆盖，以保持重放幂等）；</li>
 *   <li>{@code local_total < cloud.acked_total} → {@code 20003}，不改盘；</li>
 *   <li>首启 / {@code progress.json} 缺失：视 {@code acked_total=0}，首次合法上报创建权威文件。</li>
 * </ul>
 *
 * <h2>裁决 A（Story 5.2）：完成确认触发条件</h2>
 * <p><b>选择：</b>当且仅当同时满足：① 请求 {@code round_id} 等于权威 {@code round_id}
 * （权威尚无轮次时允许首轮建立）；② {@code round_cursor == consumableHan}；③ 请求表达完成意图
 * （{@code pending_completion==true} <b>或</b> {@code round_state==completed}）；④ 高水位校验未命中硬冲突
 * ——则权威写入 {@code round_state=completed}、{@code pending_completion=false}。
 * <b>理由：</b>完成反馈只由 backend 确认触发（FR-C-016 / AD-19），禁止仅凭末字盲推完成态。
 * <b>约束：</b>未达末字却报 {@code completed}/{@code pending_completion=true} → {@code 20001}；
 * 确认成功后响应可读完成态；Story 5.5 在写锁外经 {@link RealtimePushPort} 推送已确认帧。
 *
 * <h2>裁决 B（Story 5.2）：完成锁定与高水位</h2>
 * <p><b>选择：</b>{@code round_state==completed} 时冻结轮次四字段；同步上报若试图修改
 * {@code round_id}/{@code round_cursor}/{@code round_state}/{@code pending_completion} → {@code 20001}。
 * 同轮次且轮次字段与权威一致时，允许按 5.1 推进/no-op 高水位与派生镜像。
 * {@code pending_completion==true} 且尚未确认：任意 {@code round_id} 不一致均 {@code 20001}；
 * 同轮可将游标推到末字（请求 {@code pending=false} 时不清完成意图以外的权威约束见裁决 A）。
 * <b>理由：</b>FR-C-004 完成锁定期间正式输入不推进游标；累计高水位仍可收敛；跨轮仅 restart。
 *
 * <h2>裁决 C（Story 5.2）：exit 在非完成态</h2>
 * <p><b>选择：</b>非 {@code completed} 时 exit → {@code 20001}（与 restart 对称）。
 * 已 {@code completed} 时 exit 写入/保留篇章动作 {@code action_id}，轮次字段不变；同键重放返回同一快照。
 * <b>理由：</b>避免静默 no-op 掩盖客户端状态机错误。
 *
 * <h2>裁决 D（Story 5.2）：篇章动作路径</h2>
 * <p>见 {@link top.zhcmqtt.ewf.backend.controller.SyncController} 与 {@link RoundActionRequest}；
 * 本类 {@link #roundAction} 与 {@link #report} 共用 {@code deviceLocks}。
 *
 * <h2>裁决 E（Story 5.2）：restart 与累计</h2>
 * <p><b>选择：</b>{@code round_id' = round_id + 1}，{@code round_cursor=0}，{@code in_progress}，
 * {@code pending_completion=false}；{@code local_total}/{@code acked_total} 原样保留。
 * 仅权威已 {@code completed} 时允许；同 {@code action_id} 已执行过 restart → 返回已创建的那一个
 * {@code round_id}，不递增。
 *
 * <h2>裁决 C（Story 5.1）：{@code 20004} 与确认同事务</h2>
 * <p>先算积压标志，再执行可执行的幂等推进；积压达上限以 {@code 20004} 返回且 snapshot 已确认。
 *
 * <h2>裁决 E（Story 5.1）：{@code scripture_version}</h2>
 * <p>启动时从 classpath canonical 读取并缓存；不一致 → {@code 20005} 整单不写盘。
 *
 * <h2>裁决 F（Story 5.1）：派生镜像</h2>
 * <p>同步成功后写 {@code commands.json}/{@code device_state.json}；写序先权威再派生；派生失败不回滚。
 * 篇章动作 {@link #roundAction} <b>不</b>写派生、不改 commands 的 {@code action_id}。
 *
 * <h2>裁决 A/E（Story 5.3）：日统计派生</h2>
 * <p><b>选择：</b>确认差量成功写完 {@code progress.json} 后，以及 {@code action_id} 短路径 /
 * 高水位 no-op，均调用 {@link HistoryStatsService#archiveAndCatchUp(int)} 尝试归档/catch-up。
 * 写序：先权威 {@code progress.json}，再 {@code daily_stats.json}（派生），再
 * {@code commands}/{@code device_state}。日统计写失败不回滚权威；桶合计大于 {@code acked_total}
 * → {@code 50000} fail closed。{@link #roundAction} <b>不</b>改 {@code acked_total}、不写日统计。
 * <b>约束：</b>不得用设备日期拆差量；不得改契约字节。
 *
 * <h2>裁决 A/C/D/E/F（Story 5.4）：设置命令修订与待设备应用收敛</h2>
 * <p><b>选择 A（路径入口）：</b>{@link #submitSettings} 是设置下发唯一写路径入口；与
 * {@link #report}/{@link #roundAction} 共用同一 {@code deviceLocks}（键 = deviceId），禁止第二套
 * {@code ConcurrentHashMap} 锁。控制器只做鉴权与信封，不散落修订算法。
 * <b>理由：</b>NFR4 / AD-16——commands 与 progress 交错撕裂会误报已生效。
 * <b>约束：</b>本路径只写 {@code commands.json}，不改 {@code progress.json}/
 * {@code daily_stats.json}/{@code device_state.json}，不调用 {@link HistoryStatsService}。
 *
 * <p><b>选择 C（修订递增与只留最新）：</b>文件缺失视 {@code command_revision=0}、
 * {@code applied_revision=0}、载荷=§9.1 默认；新 {@code action_id} →
 * {@code command_revision := current + 1} 并整体替换六字段；旧修订不得以更低修订号写回。
 * 同 {@code action_id} → 纯读返回，不写盘。
 *
 * <p><b>选择 D（{@code 20006}）：</b>（1）{@code base_revision != null && base_revision != current}；
 * （2）任何将写入的 {@code command_revision} 严格小于磁盘当前值。拒绝时不改盘，HTTP 200 + 完整快照。
 * 同值高水位 report、同 action_id 重放 <b>不是</b> {@code 20006}。
 * 判定顺序：先同键幂等，再 CAS——丢失响应后的同 {@code action_id}+过期 {@code base_revision}
 * 重试必须成功返回权威快照，不得误拒。
 *
 * <p><b>选择 E（与 report 收敛）：</b>不新增 ACK 端点；收敛唯一输入仍是 report 的
 * {@code applied_revision}（5.1 单调不减镜像）。本 Story 补「先抬 command_revision，再等 applied 追上」。
 *
 * <p><b>Story 5.4 交付：</b>设置命令修订递增、{@code 20006} CAS、与 report 收敛为已生效。
 *
 * <h2>裁决 A–G（Story 5.5）：WebSocket 差量推送钩子</h2>
 * <p><b>选择 A（传输栈）：</b>原生 WebSocket 文本帧，见 {@code WebSocketConfig} / {@code SyncWebSocketHandler}。
 * <b>选择 B（序号）：</b>{@code delta.seq :=} 确认后 {@code acked_total}（= {@link StateSnapshotService#exportSnapshotSeq}）。
 * <b>选择 C（粒度）：</b>每次 acked 前进推送 1 帧 {@code delta}（合并水位，不拆物理敲击）。
 * <b>选择 D（鉴权）：</b>握手 query {@code token}，本类不参与。
 * <b>选择 E（时机）：</b>权威落盘成功且快照可读后、写锁释放后调用 {@link RealtimePushPort}；推送失败不影响 HTTP 确认。
 * <b>选择 F/G：</b>会话与心跳在 Handler。拒绝路径（如 {@code 20003}）不推送；{@code 20004} 已确认前进则仍推 {@code delta}。
 * <b>约束：</b>禁止 Store 层推送；无订阅者 no-op；不改契约、不新增 {@code ErrorCode}、不做历史帧缓冲。
 * <b>仍不做：</b>「立即同步」、改契约字节、小程序消费（Epic 6）、真机 WSS（Epic 7）。
 */
@Service
public class ProgressSyncService {

    /** 契约 §12 / 设备 Story 2.2：积压差达到 1000 即达上限。 */
    public static final int QUEUE_FULL_THRESHOLD = 1000;

    private static final String STATE_IN_PROGRESS = "in_progress";
    private static final String STATE_COMPLETED = "completed";
    private static final String ACTION_RESTART = "restart";
    private static final String ACTION_EXIT = "exit";

    private static final Set<String> ROUND_STATES = Set.of(STATE_IN_PROGRESS, STATE_COMPLETED);

    /** 契约 §9.1：熄屏超时秒数取值域。 */
    private static final Set<Integer> ALLOWED_TIMEOUTS = Set.of(5, 15, 30);

    private final ProgressStore progressStore;
    private final CommandsStore commandsStore;
    private final DeviceStateStore deviceStateStore;
    private final StateSnapshotService stateSnapshotService;
    private final HistoryStatsService historyStatsService;
    private final RealtimePushPort realtimePush;
    private final ObjectMapper objectMapper;
    private final String canonicalScriptureVersion;
    private final int consumableHan;
    private final ConcurrentHashMap<String, Object> deviceLocks = new ConcurrentHashMap<>();
    /**
     * 篇章动作 {@code action_id} 已消费集合（进程内）。
     * <p>{@code progress.json} 仅单槽 {@code action_id}，高水位推进会覆盖该槽；本集合保证
     * 同键 restart/exit 在覆盖后仍幂等，不二次开轮（AC #3）。进程重启后回退为单槽语义。
     */
    private final ConcurrentHashMap<String, Set<String>> consumedRoundActionIds = new ConcurrentHashMap<>();

    public ProgressSyncService(ProgressStore progressStore, CommandsStore commandsStore,
            DeviceStateStore deviceStateStore, StateSnapshotService stateSnapshotService,
            ObjectMapper objectMapper, HistoryStatsService historyStatsService,
            RealtimePushPort realtimePush) {
        this.progressStore = progressStore;
        this.commandsStore = commandsStore;
        this.deviceStateStore = deviceStateStore;
        this.stateSnapshotService = stateSnapshotService;
        this.objectMapper = objectMapper;
        this.historyStatsService = historyStatsService;
        this.realtimePush = realtimePush == null ? RealtimePushPort.NOOP : realtimePush;
        CanonicalBaseline baseline = loadCanonicalBaseline(objectMapper);
        this.canonicalScriptureVersion = baseline.scriptureVersion();
        this.consumableHan = baseline.consumableHan();
    }

    /**
     * 执行一次幂等同步。调用方必须传入已认证的 {@code deviceId}（来自 {@code UserContext}），
     * 不得使用请求体中的 {@code device_id}。
     */
    public SyncOutcome report(String deviceId, SyncReportRequest request) {
        String identity = requireDeviceId(deviceId);
        Object lock = deviceLocks.computeIfAbsent(identity, key -> new Object());
        LockedResult locked;
        synchronized (lock) {
            locked = reportLocked(identity, request);
        }
        dispatchPush(locked.push());
        return locked.outcome();
    }

    /**
     * 篇章动作（从头开始 / 退出）。与 {@link #report} 共用同一 {@code deviceId} 锁，
     * 避免完成确认与 restart 交错读改写。
     */
    public SyncOutcome roundAction(String deviceId, RoundActionRequest request) {
        String identity = requireDeviceId(deviceId);
        Object lock = deviceLocks.computeIfAbsent(identity, key -> new Object());
        LockedResult locked;
        synchronized (lock) {
            locked = roundActionLocked(identity, request);
        }
        dispatchPush(locked.push());
        return locked.outcome();
    }

    /**
     * 设置命令下发（裁决 A/C/D/F / Story 5.4）。与 {@link #report}/{@link #roundAction}
     * 共用同一 {@code deviceId} 锁，只写 {@code commands.json}。
     */
    public SyncOutcome submitSettings(String deviceId, SettingsCommandRequest request) {
        validateSettingsPayload(request);
        String identity = requireDeviceId(deviceId);
        Object lock = deviceLocks.computeIfAbsent(identity, key -> new Object());
        LockedResult locked;
        synchronized (lock) {
            locked = submitSettingsLocked(identity, request);
        }
        dispatchPush(locked.push());
        return locked.outcome();
    }

    /** 供测试读取的 canonical 版本单一来源。 */
    public String canonicalScriptureVersion() {
        return canonicalScriptureVersion;
    }

    /** 供测试读取的进度分母（consumableHan）。 */
    public int consumableHan() {
        return consumableHan;
    }

    private LockedResult reportLocked(String deviceId, SyncReportRequest request) {
        if (!canonicalScriptureVersion.equals(request.scriptureVersion())) {
            return outcomeOnly(reject(ErrorCode.SCRIPTURE_VERSION_MISMATCH,
                    "经文版本不一致，请升级后按本次响应重建本地基准",
                    stateSnapshotService.assemble(deviceId)));
        }

        if (!ROUND_STATES.contains(request.roundState()) || request.roundCursor() > consumableHan) {
            return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                    "轮次无法归属，请按本次响应重建本地基准",
                    stateSnapshotService.assemble(deviceId)));
        }

        Optional<ObjectNode> progressOpt = progressStore.read();
        int cloudAcked = progressOpt.map(node -> intRequired(node, "acked_total")).orElse(0);
        int cloudRoundId = progressOpt.map(node -> intRequired(node, "round_id")).orElse(0);
        int cloudCursor = progressOpt.map(node -> intRequired(node, "round_cursor")).orElse(0);
        String cloudState = progressOpt.map(node -> textRequired(node, "round_state")).orElse(null);
        String storedActionId = progressOpt.map(node -> textRequired(node, "action_id")).orElse(null);
        boolean cloudCompleted = STATE_COMPLETED.equals(cloudState);
        Optional<ObjectNode> commandsBefore = commandsStore.read();
        int commandRevisionBefore = commandsBefore.map(node -> intRequired(node, "command_revision")).orElse(0);
        int appliedBefore = commandsBefore.map(node -> intRequired(node, "applied_revision")).orElse(0);
        boolean appliedBeforeFlag = StateSnapshotService.commandApplied(appliedBefore, commandRevisionBefore);

        if (storedActionId != null && storedActionId.equals(request.actionId())) {
            writeDerivedBestEffort(request, cloudAcked);
            StateSnapshotResponse snapshot = stateSnapshotService.assemble(deviceId);
            boolean commandFlip = !appliedBeforeFlag
                    && StateSnapshotService.commandApplied(snapshot.appliedRevision(), snapshot.commandRevision());
            SyncOutcome outcome = queueFull(request)
                    ? reject(ErrorCode.QUEUE_FULL, "离线积压已达上限，请继续上报收敛", snapshot)
                    : accept(snapshot);
            // 同键幂等：水位不前进；仅当派生写导致已生效翻转时推 command_state
            return acceptedWithPush(deviceId, outcome, false, false, commandFlip);
        }

        // 跨轮仅由篇章动作 restart 创建新 round_id；/report 不得自行换轮（含进行中）。
        if (cloudRoundId >= 1 && request.roundId() != cloudRoundId) {
            return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                    "轮次无法归属，请按本次响应重建本地基准",
                    stateSnapshotService.assemble(deviceId)));
        }

        // 完成锁定：权威已 completed 时冻结轮次四字段；矛盾 pending 一并拒绝。
        if (cloudCompleted) {
            if (request.pendingCompletion()
                    || request.roundCursor() != cloudCursor
                    || !STATE_COMPLETED.equals(request.roundState())) {
                return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                        "轮次无法归属，请按本次响应重建本地基准",
                        stateSnapshotService.assemble(deviceId)));
            }
        } else {
            // 确认前矛盾：未达末字却要求完成 / 置 pending。
            if (request.roundCursor() < consumableHan
                    && (request.pendingCompletion() || STATE_COMPLETED.equals(request.roundState()))) {
                return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                        "轮次无法归属，请按本次响应重建本地基准",
                        stateSnapshotService.assemble(deviceId)));
            }
        }

        if (request.localTotal() < cloudAcked) {
            return outcomeOnly(reject(ErrorCode.DEVICE_RESET_CONFLICT,
                    "本地基准低于云端已确认基准，请按云端基准重建后继续",
                    stateSnapshotService.assemble(deviceId)));
        }

        boolean queueFull = queueFull(request);
        boolean advancing = request.localTotal() > cloudAcked;
        int newAcked = advancing ? request.localTotal() : cloudAcked;
        // 完成锁定期间冻结篇章动作幂等键，避免高水位推进覆盖 exit/restart 的 action_id。
        // 进行中推进仍按 5.1 覆盖，以便同键 report 短路径；篇章动作另见 consumedRoundActionIds。
        String actionIdToStore;
        if (cloudCompleted && storedActionId != null) {
            actionIdToStore = storedActionId;
        } else if (advancing || storedActionId == null) {
            actionIdToStore = request.actionId();
        } else {
            actionIdToStore = storedActionId;
        }

        RoundWrite roundWrite = resolveRoundWrite(cloudCompleted, cloudRoundId, cloudCursor, request);
        boolean becomingCompleted = !cloudCompleted && STATE_COMPLETED.equals(roundWrite.roundState());

        ObjectNode progressPayload = objectMapper.createObjectNode();
        progressPayload.put("local_total", request.localTotal());
        progressPayload.put("acked_total", newAcked);
        progressPayload.put("round_id", roundWrite.roundId());
        progressPayload.put("round_cursor", roundWrite.roundCursor());
        progressPayload.put("round_state", roundWrite.roundState());
        progressPayload.put("pending_completion", roundWrite.pendingCompletion());
        progressPayload.put("action_id", actionIdToStore);
        progressStore.write(progressPayload);

        writeDerivedBestEffort(request, newAcked);

        StateSnapshotResponse snapshot = stateSnapshotService.assemble(deviceId);
        boolean commandFlip = !appliedBeforeFlag
                && StateSnapshotService.commandApplied(snapshot.appliedRevision(), snapshot.commandRevision());
        SyncOutcome outcome = queueFull
                ? reject(ErrorCode.QUEUE_FULL, "离线积压已达上限，请继续上报收敛", snapshot)
                : accept(snapshot);
        // 20004 已确认前进仍推 delta；硬拒绝（20003 等）不走此分支
        return acceptedWithPush(deviceId, outcome, advancing, becomingCompleted, commandFlip);
    }

    private RoundWrite resolveRoundWrite(boolean cloudCompleted, int cloudRoundId, int cloudCursor,
            SyncReportRequest request) {
        if (cloudCompleted) {
            return new RoundWrite(cloudRoundId, cloudCursor, STATE_COMPLETED, false);
        }
        boolean sameRound = cloudRoundId < 1 || request.roundId() == cloudRoundId;
        boolean atEnd = request.roundCursor() == consumableHan;
        boolean completionIntent = request.pendingCompletion()
                || STATE_COMPLETED.equals(request.roundState());
        if (sameRound && atEnd && completionIntent) {
            return new RoundWrite(request.roundId(), consumableHan, STATE_COMPLETED, false);
        }
        return new RoundWrite(request.roundId(), request.roundCursor(), STATE_IN_PROGRESS,
                request.pendingCompletion());
    }

    private LockedResult submitSettingsLocked(String deviceId, SettingsCommandRequest request) {
        Optional<ObjectNode> existing = commandsStore.read();
        int currentRevision = existing.map(node -> intRequired(node, "command_revision")).orElse(0);
        int appliedRevision = existing.map(node -> intRequired(node, "applied_revision")).orElse(0);
        String storedActionId = existing.map(node -> textRequired(node, "action_id")).orElse("");
        int volumeBefore = existing.map(node -> intRequired(node, "volume")).orElse(StateSnapshotResponse.DEFAULT_VOLUME);
        String brightnessBefore = existing.map(node -> textRequired(node, "brightness"))
                .orElse(StateSnapshotResponse.DEFAULT_BRIGHTNESS);
        int timeoutBefore = existing.map(node -> intRequired(node, "timeout")).orElse(StateSnapshotResponse.DEFAULT_TIMEOUT);

        // 同键幂等优先于 CAS：丢失响应后的同 action_id 重试（常携带首次提交时的 base_revision）
        // 必须返回成功快照，不得因修订已推进而误报 20006（裁决 D）。
        if (!storedActionId.isEmpty() && storedActionId.equals(request.actionId())) {
            StateSnapshotResponse snapshot = stateSnapshotService.assemble(deviceId);
            boolean changed = snapshot.commandRevision() != currentRevision
                    || snapshot.appliedRevision() != appliedRevision
                    || snapshot.volume() != volumeBefore
                    || !snapshot.brightness().equals(brightnessBefore)
                    || snapshot.timeout() != timeoutBefore;
            return acceptedWithPush(deviceId, accept(snapshot), false, false, changed);
        }

        if (request.baseRevision() != null && request.baseRevision() != currentRevision) {
            return outcomeOnly(reject(ErrorCode.COMMAND_STALE_REVISION,
                    "命令修订已过期，请按本次响应重建后重试",
                    stateSnapshotService.assemble(deviceId)));
        }

        int nextRevision = currentRevision + 1;
        // 防御：任何写路径算出的新修订严格小于磁盘当前值 → fail-closed 20006，不可静默降修订。
        if (nextRevision < currentRevision) {
            return outcomeOnly(reject(ErrorCode.COMMAND_STALE_REVISION,
                    "命令修订已过期，请按本次响应重建后重试",
                    stateSnapshotService.assemble(deviceId)));
        }

        ObjectNode payload = objectMapper.createObjectNode();
        payload.put("command_revision", nextRevision);
        payload.put("applied_revision", appliedRevision);
        payload.put("action_id", request.actionId());
        payload.put("volume", request.volume());
        payload.put("brightness", request.brightness());
        payload.put("timeout", request.timeout());
        commandsStore.write(payload);
        return acceptedWithPush(deviceId, accept(stateSnapshotService.assemble(deviceId)), false, false, true);
    }

    /**
     * 契约 §9.1 取值域：{@code timeout ∈ {5,15,30}}。volume/brightness 由 {@code @Valid} 覆盖；
     * timeout 用服务内校验统一错误体为 {@code 40000}（与 Story 裁决一致）。
     */
    private static void validateSettingsPayload(SettingsCommandRequest request) {
        if (request.timeout() == null || !ALLOWED_TIMEOUTS.contains(request.timeout())) {
            throw BusinessException.paramError("timeout 取值非法");
        }
    }

    private LockedResult roundActionLocked(String deviceId, RoundActionRequest request) {
        Optional<ObjectNode> progressOpt = progressStore.read();
        if (progressOpt.isEmpty()) {
            return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                    "轮次无法归属，请按本次响应重建本地基准",
                    stateSnapshotService.assemble(deviceId)));
        }

        ObjectNode current = progressOpt.get();
        String storedActionId = textRequired(current, "action_id");
        Set<String> consumed = consumedRoundActionIds.computeIfAbsent(deviceId,
                key -> ConcurrentHashMap.newKeySet());
        // 同键重放：单槽命中或进程内已消费集合命中均不得再次开轮。
        // 不区分 action 类型时，同键跨 restart/exit 碰撞视为幂等返回当前权威（客户端须换新键）。
        if (storedActionId.equals(request.actionId()) || consumed.contains(request.actionId())) {
            return outcomeOnly(accept(stateSnapshotService.assemble(deviceId)));
        }

        String cloudState = textRequired(current, "round_state");
        boolean cloudCompleted = STATE_COMPLETED.equals(cloudState);
        int localTotal = intRequired(current, "local_total");
        int ackedTotal = intRequired(current, "acked_total");
        int roundId = intRequired(current, "round_id");
        int roundCursor = intRequired(current, "round_cursor");

        if (ACTION_RESTART.equals(request.action())) {
            if (!cloudCompleted) {
                return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                        "轮次无法归属，请按本次响应重建本地基准",
                        stateSnapshotService.assemble(deviceId)));
            }
            if (roundId == Integer.MAX_VALUE) {
                return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                        "轮次无法归属，请按本次响应重建本地基准",
                        stateSnapshotService.assemble(deviceId)));
            }
            ObjectNode payload = objectMapper.createObjectNode();
            payload.put("local_total", localTotal);
            payload.put("acked_total", ackedTotal);
            payload.put("round_id", roundId + 1);
            payload.put("round_cursor", 0);
            payload.put("round_state", STATE_IN_PROGRESS);
            payload.put("pending_completion", false);
            payload.put("action_id", request.actionId());
            progressStore.write(payload);
            consumed.add(request.actionId());
            // restart 离开完成态：不发明新帧；客户端靠后续 snapshot 查询
            return outcomeOnly(accept(stateSnapshotService.assemble(deviceId)));
        }

        if (ACTION_EXIT.equals(request.action())) {
            if (!cloudCompleted) {
                return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                        "轮次无法归属，请按本次响应重建本地基准",
                        stateSnapshotService.assemble(deviceId)));
            }
            ObjectNode payload = objectMapper.createObjectNode();
            payload.put("local_total", localTotal);
            payload.put("acked_total", ackedTotal);
            payload.put("round_id", roundId);
            payload.put("round_cursor", roundCursor);
            payload.put("round_state", STATE_COMPLETED);
            payload.put("pending_completion", false);
            payload.put("action_id", request.actionId());
            progressStore.write(payload);
            consumed.add(request.actionId());
            StateSnapshotResponse snapshot = stateSnapshotService.assemble(deviceId);
            return acceptedWithPush(deviceId, accept(snapshot), false, true, false);
        }

        return outcomeOnly(reject(ErrorCode.ROUND_UNASSIGNED,
                "轮次无法归属，请按本次响应重建本地基准",
                stateSnapshotService.assemble(deviceId)));
    }

    /**
     * 派生写失败不回滚权威；同 {@code action_id} 重放会再次尝试以收敛镜像。
     *
     * <p>写序：日统计 → commands → device_state。日统计桶合计大于 {@code ackedTotal} 时
     * {@link HistoryStatsService#archiveAndCatchUp(int)} 抛 {@code 50000}（fail closed），不吞掉。
     */
    private void writeDerivedBestEffort(SyncReportRequest request, int ackedTotal) {
        historyStatsService.archiveAndCatchUp(ackedTotal);
        try {
            writeCommandsDerived(request.appliedRevision());
        } catch (RuntimeException ignored) {
            // 派生失败不回滚权威；幂等重放会收敛。
        }
        try {
            writeDeviceStateDerived(request);
        } catch (RuntimeException ignored) {
            // 同上。
        }
    }

    private void writeCommandsDerived(int requestedAppliedRevision) {
        Optional<ObjectNode> existing = commandsStore.read();
        ObjectNode payload = objectMapper.createObjectNode();
        if (existing.isEmpty()) {
            payload.put("command_revision", StateSnapshotResponse.NO_COUNT);
            payload.put("applied_revision", Math.max(0, requestedAppliedRevision));
            payload.put("action_id", "");
            payload.put("volume", StateSnapshotResponse.DEFAULT_VOLUME);
            payload.put("brightness", StateSnapshotResponse.DEFAULT_BRIGHTNESS);
            payload.put("timeout", StateSnapshotResponse.DEFAULT_TIMEOUT);
        } else {
            ObjectNode current = existing.get();
            int oldApplied = intRequired(current, "applied_revision");
            payload.put("command_revision", intRequired(current, "command_revision"));
            payload.put("applied_revision",
                    requestedAppliedRevision < oldApplied ? oldApplied : requestedAppliedRevision);
            payload.put("action_id", textRequired(current, "action_id"));
            payload.put("volume", intRequired(current, "volume"));
            payload.put("brightness", textRequired(current, "brightness"));
            payload.put("timeout", intRequired(current, "timeout"));
        }
        commandsStore.write(payload);
    }

    private void writeDeviceStateDerived(SyncReportRequest request) {
        ObjectNode payload = objectMapper.createObjectNode();
        payload.put("battery_percent", request.batteryPercent());
        payload.put("network_mode", request.networkMode());
        payload.put("audio_config_version", request.audioConfigVersion());
        payload.put("firmware_version", request.firmwareVersion());
        deviceStateStore.write(payload);
    }

    private static boolean queueFull(SyncReportRequest request) {
        return request.localTotal() - request.ackedTotal() >= QUEUE_FULL_THRESHOLD;
    }


    private void dispatchPush(PushPlan push) {
        if (push == null || !push.any()) {
            return;
        }
        realtimePush.dispatch(push.deviceId(), push.snapshot(), push.delta(), push.completion(),
                push.commandState());
    }

    private static LockedResult outcomeOnly(SyncOutcome outcome) {
        return new LockedResult(outcome, PushPlan.none());
    }

    private LockedResult acceptedWithPush(String deviceId, SyncOutcome outcome, boolean delta,
            boolean completion, boolean commandState) {
        if (!delta && !completion && !commandState) {
            return outcomeOnly(outcome);
        }
        return new LockedResult(outcome, new PushPlan(deviceId, outcome.snapshot(), delta, completion, commandState));
    }

    private static SyncOutcome accept(StateSnapshotResponse snapshot) {
        return new SyncOutcome(ApiResponse.SUCCESS_CODE, ApiResponse.SUCCESS_MESSAGE, snapshot);
    }

    private static SyncOutcome reject(int code, String message, StateSnapshotResponse snapshot) {
        return new SyncOutcome(code, message, snapshot);
    }

    private static String requireDeviceId(String deviceId) {
        if (deviceId == null || deviceId.isBlank()) {
            throw BusinessException.serverError("设备身份不可用，请重新登录");
        }
        return deviceId;
    }

    private static int intRequired(ObjectNode node, String field) {
        JsonNode value = node.get(field);
        if (value == null || !value.isInt()) {
            throw BusinessException.serverError("状态文件字段「" + field + "」取值无效，请修复后重试");
        }
        return value.asInt();
    }

    private static boolean booleanRequired(ObjectNode node, String field) {
        JsonNode value = node.get(field);
        if (value == null || !value.isBoolean()) {
            throw BusinessException.serverError("状态文件字段「" + field + "」取值无效，请修复后重试");
        }
        return value.asBoolean();
    }

    private static String textRequired(ObjectNode node, String field) {
        JsonNode value = node.get(field);
        if (value == null || !value.isTextual()) {
            throw BusinessException.serverError("状态文件字段「" + field + "」取值无效，请修复后重试");
        }
        return value.asText();
    }

    private static CanonicalBaseline loadCanonicalBaseline(ObjectMapper objectMapper) {
        try (InputStream in = new ClassPathResource("canonical/heart-sutra.json").getInputStream()) {
            JsonNode root = objectMapper.readTree(in);
            JsonNode version = root.get("scriptureVersion");
            JsonNode counts = root.get("counts");
            JsonNode consumable = counts == null ? null : counts.get("consumableHan");
            if (version == null || !version.isTextual() || consumable == null || !consumable.isInt()) {
                throw new IllegalStateException("canonical/heart-sutra.json 缺少 scriptureVersion 或 counts.consumableHan");
            }
            return new CanonicalBaseline(version.asText(), consumable.asInt());
        } catch (IOException ex) {
            throw new IllegalStateException("无法读取 classpath canonical/heart-sutra.json", ex);
        }
    }

    /** 同步结果：业务码 + 文案 + 完整权威快照（成功与拒绝均携带）。 */
    public record SyncOutcome(int code, String message, StateSnapshotResponse snapshot) {

        public boolean accepted() {
            return code == ApiResponse.SUCCESS_CODE;
        }
    }

    private record CanonicalBaseline(String scriptureVersion, int consumableHan) {
    }

    private record RoundWrite(int roundId, int roundCursor, String roundState, boolean pendingCompletion) {
    }

    private record LockedResult(SyncOutcome outcome, PushPlan push) {
    }

    private record PushPlan(String deviceId, StateSnapshotResponse snapshot, boolean delta, boolean completion,
            boolean commandState) {
        static PushPlan none() {
            return new PushPlan(null, null, false, false, false);
        }

        boolean any() {
            return deviceId != null && snapshot != null && (delta || completion || commandState);
        }
    }
}
