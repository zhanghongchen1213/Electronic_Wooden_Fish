package top.zhcmqtt.ewf.backend.service;

import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Stream;

import org.springframework.stereotype.Service;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;

/**
 * 权威状态快照的**只读**装配、{@code snapshot_seq} 导出与恢复基准校验。
 *
 * <p><b>只读保证（AC2 / 关键回归风险 3）：</b>本类的全部路径只调用既有四个 Store 的 `read()`，
 * 不创建数据目录、不创建或改写任何文件、不做孤儿临时文件清理。查询前后 `app.data-dir` 的目录内容
 * 与文件字节必须完全不变；目录原本不存在时查询后仍不存在。
 *
 * <p><b>权威来源（AD-2 / 4.5 跨文件写序裁决）：</b>
 * <ul>
 *   <li>`acked_total`、`local_total`、`round_id`、`round_state`、`round_cursor`、
 *       `pending_completion` **只**取自 {@code progress.json} 的权威单事务组；</li>
 *   <li>`command_revision`、`applied_revision`、`volume`、`brightness`、`timeout` 取自
 *       {@code commands.json}（派生文件，只保留最新修订）；</li>
 *   <li>`battery_percent`、`network_mode`、`audio_config_version`、`firmware_version` 取自
 *       {@code device_state.json}（**非权威镜像，可整体重建**，不得据此反向覆盖权威）；
 *       缺失时按未初始化表达，不伪造（见 {@link StateSnapshotResponse} 的 `NO_*` 常量）。</li>
 * </ul>
 *
 * <p><b>存在性口径（沿用 4.5）：</b>「确定缺失」由 {@code VersionedJsonFile} 内部的
 * `Files.notExists` 判定并返回 `Optional.empty()`，那是唯一的「未初始化」判据；派生文件缺失即取
 * 契约默认值而**不是** `50000`。文件存在时不合法（损坏、超限、字段集合不符、字段值类型不符）一律
 * fail closed 抛 `50000` 且不改写文件——不得把「不可判定/不合法」静默降级成默认高水位。
 *
 * <p><b>本 Story 不做（明确不做）：</b>不实现任何写路径（不推进 `acked_total`、不改轮次、不改游标、
 * 不改命令修订、不落盘任何文件）；不实现 WebSocket 与断线补齐（Epic 5）；不暴露
 * {@code PersistenceRecoveryReport}（FR-C-006 的「恢复基准」是查询响应冻结的快照水位，不是恢复报告）；
 * 不依赖 `PersistenceRecovery` / `PersistenceRecoveryReport`。
 */
@Service
public class StateSnapshotService {

    /** 契约 §2 的权威已确认高水位字段名。 */
    private static final String ACKED_TOTAL = "acked_total";

    /** 契约 §2 的 `round_state` 冻结取值域。 */
    private static final Set<String> ROUND_STATES = Set.of("in_progress", "completed");

    private final ProgressStore progressStore;
    private final CommandsStore commandsStore;
    private final DeviceStateStore deviceStateStore;

    public StateSnapshotService(ProgressStore progressStore, CommandsStore commandsStore,
            DeviceStateStore deviceStateStore) {
        this.progressStore = progressStore;
        this.commandsStore = commandsStore;
        this.deviceStateStore = deviceStateStore;
    }

    /**
     * 只读装配一次权威快照（AC1、AC2、AC5）。
     *
     * <p>不做任何恢复基准校验，故不会返回业务拒绝码；这是「只读装配」与「恢复基准校验」两条职责中
     * 可单独测试的一条。
     */
    public StateSnapshotResponse assemble(String deviceId) {
        return readAuthoritativeState().toResponse(requireDeviceId(deviceId));
    }

    /**
     * 恢复基准校验 + 装配（AC3、AC4、AC5；裁决 B/C/D）。
     *
     * <p>无论是否被拒绝，返回值的 {@code snapshot} 都是**完整的**权威快照：契约 §12 要求业务拒绝
     * 「§6.2 的响应必填字段照常回传」，使客户端能据此重建本地基准（裁决 C）。调用方据此决定信封是
     * 成功还是业务拒绝形状。
     */
    public SnapshotOutcome resolve(String deviceId, Integer declaredAckedTotal, Integer declaredSnapshotSeq) {
        String identity = requireDeviceId(deviceId);
        AuthoritativeState state = readAuthoritativeState();
        int code = decisionCode(state, declaredAckedTotal, declaredSnapshotSeq);
        return new SnapshotOutcome(code, messageOf(code), state.toResponse(identity));
    }

    /**
     * 裁决 A 的**单一导出入口**：{@code snapshot_seq := progress.json 的权威已确认高水位 acked_total}；
     * 文件缺失（未初始化）时取 {@link StateSnapshotResponse#NO_COUNT}；文件存在而该字段缺失或不是整数时
     * **fail closed** 抛 {@code 50000}（见 {@link #intField}），绝不静默回退到默认水位。
     *
     * <p>理由与约束证明：
     * <ol>
     *   <li>{@code snapshot_seq} 的所有者与权威都是 backend（契约 §2），故不能取前端值；</li>
     *   <li>backend **无法持久化**它：契约 §11.1 把 {@code snapshot_seq} 的持久化作用域冻结为
     *       `frontend`，`persistence_scopes[backend].fields` 恰好不含它；新增第六个状态文件被
     *       {@code PersistenceFileContractTest} 直接拒绝（`STORES.size() == persistence_files.size()`），
     *       改 `progress.json` 字段列又超出本 Story 面且需先改契约。因此**导出是唯一在契约内可行的口径**；</li>
     *   <li>{@code acked_total} 单调不减、backend 权威、落在 `progress.json` 单事务组，
     *       **重启后仍单调**；</li>
     *   <li>它与契约 §8「从 {@code snapshot_seq + 1} 续订」、{@code delta} 帧「带单调 {@code seq}」
     *       同处一个「已确认敲击」序号空间，前端 `seq <= last_applied_seq` 的丢弃规则语义自洽；</li>
     *   <li>与注册表样例 {@code https_sync_response}（{@code acked_total: 128} 与
     *       {@code snapshot_seq: 128} 相等）一致。注意：样例数值之间**不存在**被门禁校验的恒等关系，
     *       故这里只把它当一致性佐证，不当推导依据。</li>
     * </ol>
     *
     * <p><b>已登记的张力：</b>契约 §2 对 {@code snapshot_seq} 写「单调递增」，对 {@code acked_total}
     * 写「单调不减」。在无新承载文件的前提下 backend 只能保证**不后退**，无法保证严格递增。
     * 本 Story 不改契约、按「不后退」实现，并把该措辞层张力登记在 `deferred-work.md`。
     * 为了「严格递增」而新增计数器文件、改 `progress.json` 字段列或递增 `contract_version`
     * 都超出本 Story 面且需先改契约，故明确不做。
     *
     * <p><b>交给 Epic 5：</b>WebSocket 帧的 {@code seq} 空间必须与本导出的 {@code snapshot_seq}
     * 空间对齐（Story 5.5）；本 Story 只登记该契约缺口。
     */
    public static int exportSnapshotSeq(Optional<ObjectNode> progress) {
        return intField(progress, ACKED_TOTAL, StateSnapshotResponse.NO_COUNT);
    }

    /**
     * 契约 §9 的已生效判定：{@code applied_revision ≥ command_revision} 即「已生效」。
     *
     * <p>该判定**不是** wire 字段：本 Story 的响应字段闭包（17 字段）不含 §5.2 的命令维度
     * {@code pending_apply}/{@code applied}，命令维度状态由客户端按响应携带的两个修订自行派生。
     * 这里把规则实现成唯一一处谓词，避免同一条规则在文档与多段实现之间漂移。
     */
    public static boolean commandApplied(int appliedRevision, int commandRevision) {
        return appliedRevision >= commandRevision;
    }

    /**
     * 裁决 D：拒绝码选择。**以契约 §12 的 `condition` 文案为唯一选择依据**，且不新增
     * {@link ErrorCode} 常量（`ErrorCodeContractTest` 是精确集合门禁）：
     *
     * <ul>
     *   <li>权威轮次标识不可用（见 {@link AuthoritativeState#roundAttributable()}）→
     *       {@link ErrorCode#ROUND_UNASSIGNED `20001`}（「轮次无法归属」；§7「无法归属轮次或缺基准
     *       高水位时拒绝确认」）。此时 backend 无法把请求归入任何一个已确认轮次，
     *       故拒绝而不是把一个累计高水位盲推到新轮次。</li>
     *   <li>未携带任何可验证基准 → {@link ErrorCode#BASELINE_HIGH_WATER_MISSING `20002`}
     *       （「缺少基准高水位」，字面吻合）。</li>
     *   <li>声明值**高于**已确认水位（越前/空洞）→ 同样归 `20002`：该声明从未被确认，
     *       因此不构成有效基准。**这是本 Story 的选择**（契约 §12 未为「客户端声明高于已确认水位」
     *       单列条件，已登记为契约缺口），必须配测试。</li>
     *   <li>声明值**低于**已确认水位（落后）→ {@link ErrorCode#DEVICE_RESET_CONFLICT `20003`}：
     *       §8「设备重置（{@code local_total} 低于 cloud 基准）→ 返回可恢复的冲突状态；
     *       设备必须按 cloud 基准重建本地高水位后再继续」。</li>
     * </ul>
     *
     * <p>判定顺序固定为「轮次归属 → 无基准 → 越前 → 落后 → 成功」：轮次归属是 backend 自身权威状态
     * 的性质，与请求参数无关，故最先判定；越前优先于落后，因为越前是「基准不可协调」而落后是
     * 「基准可协调但需重建」。AC4 要求两种情形都返回完整权威快照 + 冻结的 {@code snapshot_seq}
     * 作为补齐基准，绝不返回会跳字的「局部差量」——本方法只决定码值，`data` 恒为完整快照。
     */
    static int decisionCode(AuthoritativeState state, Integer declaredAckedTotal, Integer declaredSnapshotSeq) {
        if (!state.roundAttributable()) {
            return ErrorCode.ROUND_UNASSIGNED;
        }
        List<Integer> declared = Stream.of(declaredAckedTotal, declaredSnapshotSeq)
                .filter(Objects::nonNull)
                .toList();
        if (declared.isEmpty()) {
            return ErrorCode.BASELINE_HIGH_WATER_MISSING;
        }
        int confirmed = state.snapshotSeq();
        if (declared.stream().anyMatch(value -> value > confirmed)) {
            return ErrorCode.BASELINE_HIGH_WATER_MISSING;
        }
        if (declared.stream().anyMatch(value -> value < confirmed)) {
            return ErrorCode.DEVICE_RESET_CONFLICT;
        }
        return ApiResponse.SUCCESS_CODE;
    }

    /**
     * 业务拒绝的展示文案（产品面向，不含内部推理或调试信息）。
     * 成功时返回信封固定文案，保持与 {@link ApiResponse#SUCCESS_MESSAGE} 单一真源。
     */
    private static String messageOf(int code) {
        return switch (code) {
            case ErrorCode.ROUND_UNASSIGNED -> "轮次无法归属，请按本次响应重建本地基准";
            case ErrorCode.BASELINE_HIGH_WATER_MISSING -> "缺少可核实的基准高水位，请按本次响应重建本地基准";
            case ErrorCode.DEVICE_RESET_CONFLICT -> "本地基准低于云端已确认基准，请按云端基准重建后继续";
            default -> ApiResponse.SUCCESS_MESSAGE;
        };
    }

    /** `device_id` 只来自已认证身份；请求体或参数中的身份声明一律不可信，缺失即不可继续。 */
    private static String requireDeviceId(String deviceId) {
        if (deviceId == null || deviceId.isBlank()) {
            throw BusinessException.serverError("设备身份不可用，请重新登录");
        }
        return deviceId;
    }

    /** 一次装配内的权威读取结果；所有缺省表达在此收敛到契约取值域内的值。 */
    private AuthoritativeState readAuthoritativeState() {
        Optional<ObjectNode> progress = progressStore.read();
        Optional<ObjectNode> commands = commandsStore.read();
        Optional<ObjectNode> deviceState = deviceStateStore.read();

        // snapshot_seq 与 acked_total 同值：裁决 A 的导出而非存储。
        final int ackedTotal = exportSnapshotSeq(progress);
        final int localTotal = intField(progress, "local_total", StateSnapshotResponse.NO_COUNT);
        final int roundId = intField(progress, "round_id", StateSnapshotResponse.NO_CONFIRMED_ROUND_ID);
        final String roundState = textField(progress, "round_state", StateSnapshotResponse.NO_ROUND_STATE);
        final int roundCursor = intField(progress, "round_cursor", StateSnapshotResponse.NO_COUNT);
        final boolean pendingCompletion = booleanField(progress, "pending_completion",
                StateSnapshotResponse.NO_PENDING_COMPLETION);

        final boolean roundAttributable = roundId >= StateSnapshotResponse.NO_CONFIRMED_ROUND_ID
                && roundCursor >= StateSnapshotResponse.NO_COUNT
                && ROUND_STATES.contains(roundState);

        final int commandRevision = intField(commands, "command_revision", StateSnapshotResponse.NO_COUNT);
        final int appliedRevision = intField(commands, "applied_revision", StateSnapshotResponse.NO_COUNT);
        final int volume = intField(commands, "volume", StateSnapshotResponse.DEFAULT_VOLUME);
        final String brightness = textField(commands, "brightness", StateSnapshotResponse.DEFAULT_BRIGHTNESS);
        final int timeout = intField(commands, "timeout", StateSnapshotResponse.DEFAULT_TIMEOUT);

        final int batteryPercent = intField(deviceState, "battery_percent",
                StateSnapshotResponse.NO_BATTERY_PERCENT);
        final String networkMode = textField(deviceState, "network_mode",
                StateSnapshotResponse.NO_NETWORK_MODE);
        final int audioConfigVersion = intField(deviceState, "audio_config_version",
                StateSnapshotResponse.NO_AUDIO_CONFIG_VERSION);
        final String firmwareVersion = textField(deviceState, "firmware_version",
                StateSnapshotResponse.NO_FIRMWARE_VERSION);

        return new AuthoritativeState(roundAttributable, ackedTotal, localTotal, roundId, roundState,
                roundCursor, pendingCompletion, commandRevision, appliedRevision, batteryPercent,
                networkMode, audioConfigVersion, firmwareVersion, volume, brightness, timeout);
    }

    /**
     * 读取整数字段：文件缺失时取 `fallback`（未初始化），文件存在而字段不是整数时 fail closed。
     *
     * <p>刻意不用 `JsonNode.asInt()` 的静默转换：`asInt()` 会对非整数返回 0，那正是
     * 「静默回退到默认高水位」——4.5 已把它登记为 AC3 禁止的行为。
     */
    private static int intField(Optional<ObjectNode> root, String field, int fallback) {
        if (root.isEmpty()) {
            return fallback;
        }
        JsonNode value = root.get().get(field);
        if (value == null || !value.isInt()) {
            throw invalidValue(field);
        }
        return value.asInt();
    }

    /** 读取字符串字段：文件缺失取 `fallback`；文件存在而字段不是字符串时 fail closed。 */
    private static String textField(Optional<ObjectNode> root, String field, String fallback) {
        if (root.isEmpty()) {
            return fallback;
        }
        JsonNode value = root.get().get(field);
        if (value == null || !value.isTextual()) {
            throw invalidValue(field);
        }
        return value.asText();
    }

    /** 读取布尔字段：文件缺失取 `fallback`；文件存在而字段不是布尔时 fail closed。 */
    private static boolean booleanField(Optional<ObjectNode> root, String field, boolean fallback) {
        if (root.isEmpty()) {
            return fallback;
        }
        JsonNode value = root.get().get(field);
        if (value == null || !value.isBoolean()) {
            throw invalidValue(field);
        }
        return value.asBoolean();
    }

    /** 只带字段名，不含文件内容、绝对路径或身份值（该文案会经异常层透出到响应）。 */
    private static BusinessException invalidValue(String field) {
        return BusinessException.serverError("状态文件字段「" + field + "」取值无效，请修复后重试");
    }

    /** 恢复基准校验的结果：业务码 + 文案 + **完整**权威快照。 */
    public record SnapshotOutcome(int code, String message, StateSnapshotResponse snapshot) {

        /** 是否被接受（成功码）。拒绝时调用方必须仍把 {@link #snapshot()} 放进响应 `data`。 */
        public boolean accepted() {
            return code == ApiResponse.SUCCESS_CODE;
        }
    }

    /**
     * 权威读取面的一次冻结结果。
     *
     * <p>{@code roundAttributable} 是本 Story 对「请求无法归入当前已确认轮次」的判定（AC5）：
     * **即将下发的三个轮次相关字段必须都落在契约冻结的取值域内**——
     * `round_id` 为 `integer ≥ 1`、`round_cursor` 为 `integer ≥ 0`、`round_state` ∈
     * `{in_progress, completed}`（契约 §2）。判定作用在**已解析值**（权威值或未初始化缺省）上：
     * 未初始化时三个缺省值本身就在域内，故一次全新部署的首次查询仍能正常应答；只有
     * `progress.json` 存在却给出域外轮次标识时判为「不可归属」，此时不得把一个累计高水位交给客户端
     * 去归属轮次，只能拒绝（`20001`）。
     *
     * <p>注意判定只覆盖**取值域**，不覆盖**类型**：字段类型不符属 wire 格式违规，由
     * {@code intField}/{@code textField}/{@code booleanField} fail closed 抛 `50000`。
     */
    record AuthoritativeState(
            boolean roundAttributable,
            int ackedTotal,
            int localTotal,
            int roundId,
            String roundState,
            int roundCursor,
            boolean pendingCompletion,
            int commandRevision,
            int appliedRevision,
            int batteryPercent,
            String networkMode,
            int audioConfigVersion,
            String firmwareVersion,
            int volume,
            String brightness,
            int timeout) {

        /** 导出的快照水位；与 `acked_total` 同值（裁决 A）。 */
        int snapshotSeq() {
            return ackedTotal;
        }

        StateSnapshotResponse toResponse(String deviceId) {
            // 轮次不可归属时只表达「暂无已确认轮次」，不给客户端任何可归属到新轮次的轮次内基准。
            int wireRoundId = roundAttributable ? roundId : StateSnapshotResponse.NO_CONFIRMED_ROUND_ID;
            String wireRoundState = roundAttributable ? roundState : StateSnapshotResponse.NO_ROUND_STATE;
            int wireRoundCursor = roundAttributable ? roundCursor : StateSnapshotResponse.NO_COUNT;
            return new StateSnapshotResponse(deviceId, ackedTotal, localTotal, wireRoundId, wireRoundState,
                    wireRoundCursor, pendingCompletion, commandRevision, appliedRevision, snapshotSeq(),
                    batteryPercent, networkMode, audioConfigVersion, firmwareVersion,
                    volume, brightness, timeout);
        }
    }
}
