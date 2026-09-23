package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.time.Clock;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.HexFormat;
import java.util.List;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.config.ClockConfig;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.persistence.AtomicJsonFile;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecovery;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecoveryReport;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecoveryReport.Conclusion;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.dto.sync.RoundActionRequest;
import top.zhcmqtt.ewf.backend.dto.sync.SettingsCommandRequest;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.dto.sync.SyncReportRequest;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.HistoryStatsService;
import top.zhcmqtt.ewf.backend.service.IdentityStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.service.ProgressSyncService;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;
import top.zhcmqtt.ewf.backend.ws.RealtimePushPort;

/**
 * Story 5.7：业务态「写 → 等价重启 → 读回 / 幂等重放」验收门禁。
 *
 * <p><b>裁决登记（create-story A–G，本类实现约束）：</b>
 * <ul>
 *   <li>A：验证优先；本类默认不改 Store/基元/契约，仅验收既有写路径。</li>
 *   <li>B：「重启」= 丢弃旧 {@link ProgressSyncService}/Store，同一 {@code @TempDir} data-dir
 *       新建实例 + {@link PersistenceRecovery#recover()}；不是独立 OS 进程 fork。</li>
 *   <li>C：半写注入 = 权威完整字节旁植入冲突数值的 {@code .progress-*.tmp}，禁止用部分
 *       {@code progress.json} 伪装原子写成功路径。</li>
 *   <li>D：一条主剧本覆盖 report 推进、末字完成、restart、settings→report 收敛。</li>
 *   <li>E：篇章「最后一次」重放时，其 {@code action_id} 须仍等于落盘 {@code progress.action_id}；
 *       {@code consumedRoundActionIds} 不跨重启（见 deferred-work 5-7 / 5-2）。</li>
 *   <li>F：证据写入 {@code @TempDir/evidence/}，禁止污染仓库 {@code cloud/backend/data/}。</li>
 *   <li>G：只钉业务写路径重启后幂等；不扩展信封/401 单飞。</li>
 * </ul>
 *
 * <p><b>本 Story 不修改契约：</b>{@code docs/contracts/sync-contract.md} 与 schema 只读引用。
 *
 * <p><b>未证明面：</b>真 JVM fork+kill、掉电、多进程文件锁、生产磁盘 fsync/挂载。
 */
class BusinessStateRestartRecoveryTest {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private static final String DEVICE_ID = "ewf-restart-device";

    @TempDir
    Path dataDir;

    @Test
    @DisplayName("主剧本：写→重建实例+recover→状态自洽；半写 orphan 不污染；证据落盘")
    void 业务态重启后一致性与半写隔离() throws Exception {
        Path evidenceRoot = Files.createDirectories(dataDir.resolve("evidence"));
        Fixture fixture = new Fixture(dataDir);
        ProgressSyncService writer = fixture.service();
        int end = writer.consumableHan();

        // (1) report 抬 acked_total 并归档日统计
        seedCloud(dataDir, 100, 100, 3, end - 2, false, "seed-1");
        ProgressSyncService.SyncOutcome advance = writer.report(DEVICE_ID,
                report(writer, 110, 100, 3, end - 1, false, "act-advance"));
        assertEquals(ApiResponse.SUCCESS_CODE, advance.code());
        assertEquals(110, advance.snapshot().ackedTotal());

        // (2) 达末字完成
        ProgressSyncService.SyncOutcome complete = writer.report(DEVICE_ID,
                report(writer, 120, 110, 3, end, true, "act-complete"));
        assertEquals(ApiResponse.SUCCESS_CODE, complete.code());
        assertEquals("completed", complete.snapshot().roundState());
        assertFalse(complete.snapshot().pendingCompletion());

        // (3) restart 新 round_id（保留在单槽，供后续幂等场景；本主剧本随后会被 settings/report 覆盖 progress.action_id）
        ProgressSyncService.SyncOutcome restart = writer.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "act-restart-main"));
        assertEquals(ApiResponse.SUCCESS_CODE, restart.code());
        assertEquals(4, restart.snapshot().roundId());
        assertEquals(0, restart.snapshot().roundCursor());

        // (4) settings 抬 command_revision，再 report 收敛 applied_revision
        ProgressSyncService.SyncOutcome settings = writer.submitSettings(DEVICE_ID,
                settings(70, "high", 30, "cmd-settings-1", null));
        assertEquals(ApiResponse.SUCCESS_CODE, settings.code());
        int commandRev = settings.snapshot().commandRevision();
        assertTrue(commandRev > 4, "settings 后 command_revision 应抬升");
        assertFalse(StateSnapshotService.commandApplied(settings.snapshot().appliedRevision(), commandRev));

        SyncReportRequest lastReport = new SyncReportRequest(DEVICE_ID, writer.canonicalScriptureVersion(),
                125, 120, 4, "in_progress", 5, false, commandRev, 88, "connected", 2, "2.0.0",
                "act-apply-rev");
        ProgressSyncService.SyncOutcome applied = writer.report(DEVICE_ID, lastReport);
        assertEquals(ApiResponse.SUCCESS_CODE, applied.code());
        assertEquals(125, applied.snapshot().ackedTotal());
        assertEquals(commandRev, applied.snapshot().appliedRevision());
        assertTrue(StateSnapshotService.commandApplied(applied.snapshot().appliedRevision(),
                applied.snapshot().commandRevision()));
        assertEquals("act-apply-rev", readProgress(dataDir).get("action_id").asText(),
                "推进路径须覆盖 progress.action_id，供重启后同键重放");

        SettingsCommandRequest lastSettings = settings(70, "high", 30, "cmd-settings-1", null);
        writeJson(evidenceRoot.resolve("inputs/last-report.json"), lastReport);
        writeJson(evidenceRoot.resolve("inputs/last-settings.json"), lastSettings);
        writeOutcome(evidenceRoot.resolve("outputs/pre-restart-applied.json"), applied);

        byte[] progressDigest = sha256(dataDir.resolve(ProgressStore.fileName()));
        ObjectNode progressBefore = readProgress(dataDir);
        int statsSumBefore = sumConfirmed(readDaily(dataDir));
        assertEquals(125, progressBefore.get("acked_total").asInt());
        assertTrue(statsSumBefore <= 125, "日统计桶合计不得大于权威 acked_total");
        assertEquals(125, statsSumBefore, "写路径 catch-up 后 Σ confirmed_taps 应等于权威 acked_total");

        // 半写 orphan：结构合法但假高水位
        ObjectNode orphanPayload = MAPPER.createObjectNode();
        orphanPayload.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        orphanPayload.put("local_total", 9999);
        orphanPayload.put("acked_total", 9999);
        orphanPayload.put("round_id", 99);
        orphanPayload.put("round_cursor", 0);
        orphanPayload.put("round_state", "in_progress");
        orphanPayload.put("pending_completion", false);
        orphanPayload.put("action_id", "orphan-fake");
        Path orphan = dataDir.resolve(PersistenceLimits.tempPrefix(ProgressStore.fileName()) + "crash.tmp");
        Files.writeString(orphan, MAPPER.writeValueAsString(orphanPayload), StandardCharsets.UTF_8);
        assertTrue(Files.exists(orphan));
        // AC1：orphan 仍在时读取只认精确文件名，不得读到假高水位
        assertEquals(125, readAcked(dataDir), "orphan 并存时权威 progress 仍可读为最终水位");
        assertArrayEquals(progressDigest, sha256(dataDir.resolve(ProgressStore.fileName())),
                "orphan 并存不得改写权威 progress 字节");

        // 裁决 B：丢弃旧实例，同 data-dir 重建 + recover
        Fixture restarted = new Fixture(dataDir);
        PersistenceRecoveryReport recovery = new PersistenceRecovery(dataDir.toString()).recover();
        assertEquals(Conclusion.OK, fileConclusion(recovery, ProgressStore.fileName()));
        assertEquals(Conclusion.OK, fileConclusion(recovery, CommandsStore.fileName()));
        assertEquals(Conclusion.OK, fileConclusion(recovery, DailyStatsStore.fileName()));
        assertEquals(Conclusion.OK, fileConclusion(recovery, DeviceStateStore.fileName()));
        assertEquals(Conclusion.MISSING, fileConclusion(recovery, IdentityStore.fileName()),
                "本剧本未建 identity；缺失须记为 MISSING 且合法");
        assertFalse(Files.exists(orphan), "恢复后 orphan 应被 best-effort 清理");

        StateSnapshotResponse snap = restarted.snapshot.assemble(DEVICE_ID);
        ObjectNode progressAfter = readProgress(dataDir);
        ObjectNode commandsAfter = readCommands(dataDir);
        ObjectNode deviceAfter = restarted.deviceState.read().orElseThrow();

        assertEquals(125, progressAfter.get("acked_total").asInt());
        assertEquals(125, progressAfter.get("local_total").asInt());
        assertEquals(4, progressAfter.get("round_id").asInt());
        assertEquals(5, progressAfter.get("round_cursor").asInt());
        assertEquals("in_progress", progressAfter.get("round_state").asText());
        assertFalse(progressAfter.get("pending_completion").asBoolean());
        assertEquals("act-apply-rev", progressAfter.get("action_id").asText());

        assertEquals(commandRev, commandsAfter.get("command_revision").asInt());
        assertEquals(commandRev, commandsAfter.get("applied_revision").asInt());
        assertEquals("cmd-settings-1", commandsAfter.get("action_id").asText());
        assertEquals(70, commandsAfter.get("volume").asInt());
        assertEquals("high", commandsAfter.get("brightness").asText());
        assertEquals(30, commandsAfter.get("timeout").asInt());
        assertTrue(StateSnapshotService.commandApplied(commandsAfter.get("applied_revision").asInt(),
                commandsAfter.get("command_revision").asInt()));

        assertEquals(88, deviceAfter.get("battery_percent").asInt());
        assertEquals("connected", deviceAfter.get("network_mode").asText());
        assertEquals(2, deviceAfter.get("audio_config_version").asInt());
        assertEquals("2.0.0", deviceAfter.get("firmware_version").asText());

        // AC1 / Task 2.4：快照 17 字段与文件读回逐项对账（非仅 DTO 组件数）
        assertEquals(17, StateSnapshotResponse.class.getRecordComponents().length);
        assertEquals(DEVICE_ID, snap.deviceId());
        assertEquals(progressAfter.get("acked_total").asInt(), snap.ackedTotal());
        assertEquals(progressAfter.get("local_total").asInt(), snap.localTotal());
        assertEquals(progressAfter.get("round_id").asInt(), snap.roundId());
        assertEquals(progressAfter.get("round_state").asText(), snap.roundState());
        assertEquals(progressAfter.get("round_cursor").asInt(), snap.roundCursor());
        assertEquals(progressAfter.get("pending_completion").asBoolean(), snap.pendingCompletion());
        assertEquals(commandsAfter.get("command_revision").asInt(), snap.commandRevision());
        assertEquals(commandsAfter.get("applied_revision").asInt(), snap.appliedRevision());
        assertEquals(snap.ackedTotal(), snap.snapshotSeq());
        assertEquals(deviceAfter.get("battery_percent").asInt(), snap.batteryPercent());
        assertEquals(deviceAfter.get("network_mode").asText(), snap.networkMode());
        assertEquals(deviceAfter.get("audio_config_version").asInt(), snap.audioConfigVersion());
        assertEquals(deviceAfter.get("firmware_version").asText(), snap.firmwareVersion());
        assertEquals(commandsAfter.get("volume").asInt(), snap.volume());
        assertEquals(commandsAfter.get("brightness").asText(), snap.brightness());
        assertEquals(commandsAfter.get("timeout").asInt(), snap.timeout());

        int statsSumAfter = sumConfirmed(readDaily(dataDir));
        assertTrue(statsSumAfter <= snap.ackedTotal(), "重启后日统计仍不得大于 acked_total");
        assertEquals(statsSumBefore, statsSumAfter);
        assertEquals(snap.ackedTotal(), statsSumAfter, "重启后 Σ confirmed_taps 须与权威水位对齐");
        var historyView = restarted.history.query();
        assertEquals(snap.ackedTotal(), historyView.totalTaps(),
                "HistoryStatsService 读回的 total_taps 须等于权威 acked_total");
        assertTrue(historyView.last7DaysTaps() >= 0);
        assertTrue(historyView.last30DaysTaps() >= 0);

        // device_state / identity 可读（缺失合法）且不改写高水位
        assertTrue(restarted.deviceState.read().isPresent());
        assertTrue(IdentityStore.strictRead(dataDir.resolve(IdentityStore.fileName())).isEmpty(),
                "identity 缺失合法");
        assertArrayEquals(progressDigest, sha256(dataDir.resolve(ProgressStore.fileName())),
                "半写 orphan 不得改写权威 progress 字节");

        writeFileDigest(evidenceRoot.resolve("files/progress.sha256"), ProgressStore.fileName(), progressDigest);
        writeFileDigest(evidenceRoot.resolve("files/commands.sha256"), CommandsStore.fileName(),
                sha256(dataDir.resolve(CommandsStore.fileName())));
        writeFileDigest(evidenceRoot.resolve("files/daily_stats.sha256"), DailyStatsStore.fileName(),
                sha256(dataDir.resolve(DailyStatsStore.fileName())));
        writeFileDigest(evidenceRoot.resolve("files/device_state.sha256"), DeviceStateStore.fileName(),
                sha256(dataDir.resolve(DeviceStateStore.fileName())));
        writeIdentityEvidence(evidenceRoot.resolve("files/identity.status"), dataDir);
        writeJson(evidenceRoot.resolve("outputs/post-restart-snapshot.json"), snap);
        writeJson(evidenceRoot.resolve("files/progress-fields.json"), progressAfter);
        writeJson(evidenceRoot.resolve("files/commands-fields.json"), commandsAfter);
        writeJson(evidenceRoot.resolve("files/device_state-fields.json"), deviceAfter);

        assertTrue(Files.exists(evidenceRoot.resolve("inputs/last-report.json")));
        assertTrue(Files.exists(evidenceRoot.resolve("outputs/post-restart-snapshot.json")));
        assertTrue(Files.exists(evidenceRoot.resolve("files/progress.sha256")));
        assertTrue(Files.exists(evidenceRoot.resolve("files/identity.status")));
        assertTrue(Files.exists(evidenceRoot.resolve("files/device_state.sha256")));
        assertTrue(Files.exists(evidenceRoot.resolve("files/device_state-fields.json")));

        // AC2：重启后重放最后一次 report / settings —— 不抬水位、不二次递增修订
        ProgressSyncService.SyncOutcome reportReplay = restarted.service().report(DEVICE_ID, lastReport);
        assertEquals(ApiResponse.SUCCESS_CODE, reportReplay.code());
        assertEquals(125, reportReplay.snapshot().ackedTotal());
        assertEquals(125, readAcked(dataDir));
        assertEquals(commandRev, reportReplay.snapshot().commandRevision());
        assertEquals(commandRev, reportReplay.snapshot().appliedRevision());

        ProgressSyncService.SyncOutcome settingsReplay = restarted.service().submitSettings(DEVICE_ID, lastSettings);
        assertEquals(ApiResponse.SUCCESS_CODE, settingsReplay.code());
        assertEquals(commandRev, settingsReplay.snapshot().commandRevision(),
                "同 settings action_id 不得二次递增");
        assertEquals(70, settingsReplay.snapshot().volume());

        writeOutcome(evidenceRoot.resolve("outputs/post-replay-report.json"), reportReplay);
        writeOutcome(evidenceRoot.resolve("outputs/post-replay-settings.json"), settingsReplay);

        // 负例护栏（Task 4.3）：若断言失败即说明不变量被破坏
        assertNotEqualsOrphanWatermark(dataDir, 9999);
        assertEquals(125, readAcked(dataDir), "负例②：同 action_id report 不得再推进 acked_total");
        assertEquals(commandRev, readCommands(dataDir).get("command_revision").asInt(),
                "负例③：同 settings action_id 不得二次递增修订");
        assertArrayEquals(progressDigest, sha256(dataDir.resolve(ProgressStore.fileName())),
                "负例④：半写后完整快照字节不得被改写");
    }

    @Test
    @DisplayName("篇章动作：最后一次 restart 仍在单槽时，重启后同键幂等不二次开轮")
    void 重启后篇章动作单槽幂等() throws Exception {
        Path evidenceRoot = Files.createDirectories(dataDir.resolve("evidence-round"));
        Fixture fixture = new Fixture(dataDir);
        ProgressSyncService writer = fixture.service();
        int end = writer.consumableHan();
        seedCompleted(dataDir, 200, 200, 5, end, "act-done");

        RoundActionRequest lastRestart = new RoundActionRequest("restart", "act-restart-last");
        ProgressSyncService.SyncOutcome first = writer.roundAction(DEVICE_ID, lastRestart);
        assertEquals(ApiResponse.SUCCESS_CODE, first.code());
        assertEquals(6, first.snapshot().roundId());
        assertEquals("act-restart-last", readProgress(dataDir).get("action_id").asText(),
                "裁决 E：最后一次篇章动作须仍落在 progress.action_id 单槽");

        writeJson(evidenceRoot.resolve("inputs/last-round-action.json"), lastRestart);
        writeOutcome(evidenceRoot.resolve("outputs/pre-restart-round.json"), first);

        // 进程内集合清空：重建后只能靠单槽
        Fixture restarted = new Fixture(dataDir);
        new PersistenceRecovery(dataDir.toString()).recover();
        ProgressSyncService.SyncOutcome replay = restarted.service().roundAction(DEVICE_ID, lastRestart);
        assertEquals(ApiResponse.SUCCESS_CODE, replay.code());
        assertEquals(6, replay.snapshot().roundId(), "同 action_id 不得二次开轮");
        assertEquals(200, replay.snapshot().ackedTotal());
        writeOutcome(evidenceRoot.resolve("outputs/post-replay-round.json"), replay);

        // AC2：篇章幂等场景亦落盘五文件摘要/缺失证据
        writeFileDigest(evidenceRoot.resolve("files/progress.sha256"), ProgressStore.fileName(),
                sha256(dataDir.resolve(ProgressStore.fileName())));
        writeFileDigest(evidenceRoot.resolve("files/commands.sha256"), CommandsStore.fileName(),
                sha256(dataDir.resolve(CommandsStore.fileName())));
        writePresenceDigest(evidenceRoot.resolve("files/daily_stats.status"), dataDir, DailyStatsStore.fileName());
        writePresenceDigest(evidenceRoot.resolve("files/device_state.status"), dataDir, DeviceStateStore.fileName());
        writeIdentityEvidence(evidenceRoot.resolve("files/identity.status"), dataDir);
        writeJson(evidenceRoot.resolve("files/progress-fields.json"), readProgress(dataDir));
        writeJson(evidenceRoot.resolve("files/commands-fields.json"), readCommands(dataDir));
    }

    @Test
    @DisplayName("负例⑤：日统计桶合计 > acked_total 经写路径 fail-closed，不得 code=0")
    void 日统计越权不得当作成功() throws Exception {
        seedCloud(dataDir, 50, 50, 2, 10, false, "act-1");
        ProgressSyncService service = new Fixture(dataDir).service();
        service.report(DEVICE_ID, report(service, 60, 50, 2, 20, false, "act-ok"));

        ObjectNode inflated = MAPPER.createObjectNode();
        ObjectNode day = MAPPER.createObjectNode();
        day.put(HistoryStatsService.CONFIRMED_TAPS, 999);
        inflated.set(LocalDate.now(Clock.system(ClockConfig.ZONE)).toString(), day);
        new DailyStatsStore(MAPPER, dataDir.toString()).write(inflated);

        // 重建实例后仍须 fail-closed
        ProgressSyncService afterRestart = new Fixture(dataDir).service();
        new PersistenceRecovery(dataDir.toString()).recover();
        BusinessException ex = assertThrows(BusinessException.class,
                () -> afterRestart.report(DEVICE_ID, report(afterRestart, 60, 60, 2, 20, false, "act-tamper")));
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode());
        assertEquals(60, readAcked(dataDir));
    }

    @Test
    @DisplayName("写序钉扎：report=progress→daily_stats→commands→device_state；settings 只写 commands；roundAction 不写日统计")
    void 写序与写面隔离() {
        RecordingStores stores = new RecordingStores(dataDir);
        ProgressSyncService service = stores.service();

        seedCloud(dataDir, 40, 40, 2, 5, false, "seed");
        stores.clear();
        ProgressSyncService.SyncOutcome report = service.report(DEVICE_ID,
                report(service, 50, 40, 2, 8, false, "act-order"));
        assertEquals(ApiResponse.SUCCESS_CODE, report.code());
        assertEquals(List.of("progress", "daily_stats", "commands", "device_state"), stores.writes,
                "report 写序必须为 progress → daily_stats → commands → device_state");

        stores.clear();
        ProgressSyncService.SyncOutcome settings = service.submitSettings(DEVICE_ID,
                settings(55, "mid", 15, "cmd-order", null));
        assertEquals(ApiResponse.SUCCESS_CODE, settings.code());
        assertEquals(List.of("commands"), stores.writes, "settings 只写 commands");

        int end = service.consumableHan();
        seedCompleted(dataDir, 50, 50, 2, end, "done-for-restart");
        stores.clear();
        ProgressSyncService.SyncOutcome restart = service.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "restart-order"));
        assertEquals(ApiResponse.SUCCESS_CODE, restart.code());
        assertEquals(List.of("progress"), stores.writes, "roundAction 只写 progress，不写日统计/commands");
        assertFalse(stores.writes.contains("daily_stats"));
        assertFalse(stores.writes.contains("commands"));
    }

    @Test
    @DisplayName("AtomicJsonFile 回归：成功无残留；失败路径零 .tmp")
    void 原子写基元回归() throws Exception {
        Path target = dataDir.resolve("progress.json");
        byte[] payload = "{\"schema_version\":1,\"acked_total\":7}".getBytes(StandardCharsets.UTF_8);
        AtomicJsonFile.write(target, payload);
        assertArrayEquals(payload, Files.readAllBytes(target));
        assertEquals(0, countTmp(dataDir));

        // 与 AtomicJsonFileTest 同构：目标被非空目录占用 → move 失败 → 零 .tmp
        Path failTarget = dataDir.resolve("commands.json");
        Files.createDirectories(failTarget);
        Files.writeString(failTarget.resolve("occupied.txt"), "x");
        assertThrows(Exception.class,
                () -> AtomicJsonFile.write(failTarget, "{}".getBytes(StandardCharsets.UTF_8)));
        assertEquals(0, countTmp(dataDir), "失败路径不得残留 .tmp");
    }

    // --- helpers ---

    private static void assertNotEqualsOrphanWatermark(Path dir, int orphanValue) {
        assertFalse(readAcked(dir) == orphanValue, "负例①：不得读到 orphan .tmp 的假高水位");
    }

    private static long countTmp(Path dir) throws Exception {
        if (!Files.isDirectory(dir)) {
            return 0;
        }
        try (var stream = Files.list(dir)) {
            return stream.filter(p -> PersistenceLimits.isTemporaryFileName(p.getFileName().toString())).count();
        }
    }

    private static PersistenceRecoveryReport.FileStatus fileStatus(PersistenceRecoveryReport report,
            String fileName) {
        return report.files().stream().filter(f -> fileName.equals(f.fileName())).findFirst().orElseThrow();
    }

    private static Conclusion fileConclusion(PersistenceRecoveryReport report, String fileName) {
        return fileStatus(report, fileName).conclusion();
    }

    private static void seedCloud(Path dir, int local, int acked, int roundId, int cursor, boolean pending,
            String actionId) {
        seedProgress(dir, local, acked, roundId, cursor, "in_progress", pending, actionId);
    }

    private static void seedCompleted(Path dir, int local, int acked, int roundId, int cursor, String actionId) {
        seedProgress(dir, local, acked, roundId, cursor, "completed", false, actionId);
    }

    private static void seedProgress(Path dir, int local, int acked, int roundId, int cursor, String roundState,
            boolean pending, String actionId) {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", local);
        progress.put("acked_total", acked);
        progress.put("round_id", roundId);
        progress.put("round_cursor", cursor);
        progress.put("round_state", roundState);
        progress.put("pending_completion", pending);
        progress.put("action_id", actionId);
        new ProgressStore(MAPPER, dir.toString()).write(progress);

        ObjectNode commands = MAPPER.createObjectNode();
        commands.put("command_revision", 4);
        commands.put("applied_revision", 4);
        commands.put("action_id", "cmd-1");
        commands.put("volume", 50);
        commands.put("brightness", "mid");
        commands.put("timeout", 15);
        new CommandsStore(MAPPER, dir.toString()).write(commands);
    }

    private static SyncReportRequest report(ProgressSyncService svc, int local, int acked, int roundId,
            int cursor, boolean pending, String actionId) {
        return new SyncReportRequest(DEVICE_ID, svc.canonicalScriptureVersion(), local, acked, roundId,
                "in_progress", cursor, pending, 4, 76, "connected", 1, "1.0.0", actionId);
    }

    private static SettingsCommandRequest settings(int volume, String brightness, int timeout, String actionId,
            Integer baseRevision) {
        return new SettingsCommandRequest(volume, brightness, timeout, actionId, baseRevision);
    }

    private static ObjectNode readProgress(Path dir) {
        return new ProgressStore(MAPPER, dir.toString()).read().orElseThrow();
    }

    private static ObjectNode readCommands(Path dir) {
        return new CommandsStore(MAPPER, dir.toString()).read().orElseThrow();
    }

    private static ObjectNode readDaily(Path dir) {
        return new DailyStatsStore(MAPPER, dir.toString()).read().orElseGet(MAPPER::createObjectNode);
    }

    private static int readAcked(Path dir) {
        return readProgress(dir).get("acked_total").asInt();
    }

    private static int sumConfirmed(ObjectNode buckets) {
        int sum = 0;
        var it = buckets.fields();
        while (it.hasNext()) {
            var entry = it.next();
            if (entry.getValue().isObject()) {
                sum += entry.getValue().path(HistoryStatsService.CONFIRMED_TAPS).asInt(0);
            }
        }
        return sum;
    }

    private static byte[] sha256(Path file) throws Exception {
        byte[] bytes = Files.readAllBytes(file);
        return MessageDigest.getInstance("SHA-256").digest(bytes);
    }

    private static void writeJson(Path path, Object value) throws Exception {
        Files.createDirectories(path.getParent());
        MAPPER.writerWithDefaultPrettyPrinter().writeValue(path.toFile(), value);
    }

    private static void writeOutcome(Path path, ProgressSyncService.SyncOutcome outcome) throws Exception {
        ObjectNode node = MAPPER.createObjectNode();
        node.put("code", outcome.code());
        node.put("message", outcome.message());
        node.set("snapshot", MAPPER.valueToTree(outcome.snapshot()));
        writeJson(path, node);
    }

    private static void writeFileDigest(Path path, String fileName, byte[] digest) throws Exception {
        Files.createDirectories(path.getParent());
        Files.writeString(path, fileName + " sha256=" + HexFormat.of().formatHex(digest) + "\n",
                StandardCharsets.UTF_8);
    }

    /** 五文件证据：存在则写摘要，缺失则记 MISSING（identity/派生缺失合法）。 */
    private static void writeIdentityEvidence(Path path, Path dir) throws Exception {
        writePresenceDigest(path, dir, IdentityStore.fileName());
    }

    private static void writePresenceDigest(Path path, Path dir, String fileName) throws Exception {
        Path file = dir.resolve(fileName);
        if (Files.exists(file)) {
            writeFileDigest(path, fileName, sha256(file));
        } else {
            Files.createDirectories(path.getParent());
            Files.writeString(path, fileName + " MISSING (合法)\n", StandardCharsets.UTF_8);
        }
    }

    /** 同一 data-dir 上可丢弃/重建的服务束（裁决 B）。 */
    private static final class Fixture {
        final ProgressStore progress;
        final CommandsStore commands;
        final DeviceStateStore deviceState;
        final StateSnapshotService snapshot;
        final HistoryStatsService history;
        final Path dir;

        Fixture(Path dir) {
            this.dir = dir;
            this.progress = new ProgressStore(MAPPER, dir.toString());
            this.commands = new CommandsStore(MAPPER, dir.toString());
            this.deviceState = new DeviceStateStore(MAPPER, dir.toString());
            this.snapshot = new StateSnapshotService(progress, commands, deviceState);
            DailyStatsStore daily = new DailyStatsStore(MAPPER, dir.toString());
            this.history = new HistoryStatsService(daily, progress, MAPPER, Clock.system(ClockConfig.ZONE));
        }

        ProgressSyncService service() {
            return new ProgressSyncService(progress, commands, deviceState, snapshot, MAPPER, history,
                    RealtimePushPort.NOOP);
        }
    }

    /** 记录写序的 Store 束。 */
    private static final class RecordingStores {
        final List<String> writes = new ArrayList<>();
        final Path dir;
        final ProgressStore progress;
        final CommandsStore commands;
        final DeviceStateStore deviceState;
        final DailyStatsStore daily;
        final StateSnapshotService snapshot;
        final HistoryStatsService history;

        RecordingStores(Path dir) {
            this.dir = dir;
            this.progress = new ProgressStore(MAPPER, dir.toString()) {
                @Override
                public void write(ObjectNode payload) {
                    writes.add("progress");
                    super.write(payload);
                }
            };
            this.commands = new CommandsStore(MAPPER, dir.toString()) {
                @Override
                public void write(ObjectNode payload) {
                    writes.add("commands");
                    super.write(payload);
                }
            };
            this.deviceState = new DeviceStateStore(MAPPER, dir.toString()) {
                @Override
                public void write(ObjectNode payload) {
                    writes.add("device_state");
                    super.write(payload);
                }
            };
            this.daily = new DailyStatsStore(MAPPER, dir.toString()) {
                @Override
                public void write(ObjectNode buckets) {
                    writes.add("daily_stats");
                    super.write(buckets);
                }
            };
            this.snapshot = new StateSnapshotService(progress, commands, deviceState);
            this.history = new HistoryStatsService(daily, progress, MAPPER, Clock.system(ClockConfig.ZONE));
        }

        ProgressSyncService service() {
            return new ProgressSyncService(progress, commands, deviceState, snapshot, MAPPER, history,
                    RealtimePushPort.NOOP);
        }

        void clear() {
            writes.clear();
        }
    }
}
