package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Clock;
import java.time.LocalDate;
import java.util.Optional;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.config.ClockConfig;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.dto.sync.RoundActionRequest;
import top.zhcmqtt.ewf.backend.dto.sync.SettingsCommandRequest;
import top.zhcmqtt.ewf.backend.dto.sync.SyncReportRequest;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.HistoryStatsService;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.service.ProgressSyncService;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;
import top.zhcmqtt.ewf.backend.ws.RealtimePushPort;

/**
 * 高水位幂等推进的单元覆盖（{@code @TempDir} + 既有 Store 直接构造，不启 Spring）。
 *
 * <p>只写临时目录，不触碰仓库工作区 {@code cloud/backend/data/}。
 */
class ProgressSyncServiceTest {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private static final String DEVICE_ID = "ewf-sync-device";

    @TempDir
    Path dataDir;

    @Test
    @DisplayName("local_total 高于 cloud → acked_total 推进一次；同值重复 → 权威不变")
    void 推进与同值noOp() {
        seedCloud(120, 120, 3, 96, false, "act-1");
        ProgressSyncService service = service();

        ProgressSyncService.SyncOutcome first = service.report(DEVICE_ID,
                report(128, 120, 3, 96, false, "act-2"));
        assertEquals(ApiResponse.SUCCESS_CODE, first.code());
        assertEquals(128, first.snapshot().ackedTotal());
        assertEquals(128, first.snapshot().snapshotSeq());

        String afterAdvance = readProgress();
        ProgressSyncService.SyncOutcome noop = service.report(DEVICE_ID,
                report(128, 128, 3, 96, false, "act-3"));
        assertEquals(ApiResponse.SUCCESS_CODE, noop.code());
        assertEquals(128, noop.snapshot().ackedTotal());
        assertEquals(128, readAcked());
        assertTrue(readProgress().contains("act-2"),
                "同值 no-op 不得覆盖已确认 action_id");

        ProgressSyncService.SyncOutcome replay = service.report(DEVICE_ID,
                report(128, 128, 3, 96, false, "act-2"));
        assertEquals(128, replay.snapshot().ackedTotal());
        assertEquals(afterAdvance.contains("\"acked_total\" : 128")
                || afterAdvance.contains("\"acked_total\":128"), true);
    }

    @Test
    @DisplayName("local_total 低于 cloud → 20003，文件不被回退")
    void 重置冲突不回退() {
        seedCloud(200, 200, 3, 96, false, "act-1");
        String before = readProgress();
        ProgressSyncService.SyncOutcome outcome = service().report(DEVICE_ID,
                report(150, 150, 3, 96, false, "act-2"));
        assertEquals(ErrorCode.DEVICE_RESET_CONFLICT, outcome.code());
        assertEquals(200, outcome.snapshot().ackedTotal());
        assertEquals(before, readProgress());
    }

    @Test
    @DisplayName("scripture_version 错误 → 20005 且不写盘")
    void 版本不一致不写盘() {
        Path empty = dataDir.resolve("empty");
        ProgressSyncService service = serviceOver(empty);
        SyncReportRequest bad = new SyncReportRequest(DEVICE_ID, "HS-9.9.9", 10, 0, 1, "in_progress",
                10, false, 0, 80, "connected", 1, "1.0.0", "act-v");
        ProgressSyncService.SyncOutcome outcome = service.report(DEVICE_ID, bad);
        assertEquals(ErrorCode.SCRIPTURE_VERSION_MISMATCH, outcome.code());
        assertTrue(Files.notExists(empty.resolve(ProgressStore.fileName())));
    }

    @Test
    @DisplayName("积压差 ≥ 1000 → 20004 且 acked_total 仍推进")
    void 积压达上限仍确认() {
        seedCloud(0, 0, 1, 0, false, "act-0");
        ProgressSyncService.SyncOutcome outcome = service().report(DEVICE_ID,
                report(1000, 0, 1, 10, false, "act-q"));
        assertEquals(ErrorCode.QUEUE_FULL, outcome.code());
        assertEquals(1000, outcome.snapshot().ackedTotal());
        assertEquals(1000, readAcked());
    }

    @Test
    @DisplayName("同一 action_id 重复提交返回首次确认后的权威结果")
    void actionId幂等() {
        seedCloud(100, 100, 3, 50, false, "act-same");
        ProgressSyncService service = service();
        ProgressSyncService.SyncOutcome first = service.report(DEVICE_ID,
                report(110, 100, 3, 60, false, "act-new"));
        assertEquals(110, first.snapshot().ackedTotal());
        ProgressSyncService.SyncOutcome replay = service.report(DEVICE_ID,
                report(999, 100, 3, 60, false, "act-new"));
        assertEquals(110, replay.snapshot().ackedTotal(), "同 action_id 不得再次推进");
        assertEquals(110, readAcked());
    }

    @Test
    @DisplayName("跨轮无法归属 → 20001 且不写权威")
    void 跨轮无法归属() {
        seedCloud(120, 120, 5, 10, false, "act-1");
        String before = readProgress();
        ProgressSyncService.SyncOutcome outcome = service().report(DEVICE_ID,
                report(130, 120, 3, 20, false, "act-2"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, outcome.code());
        assertEquals(before, readProgress());
    }

    @Test
    @DisplayName("pending_completion 时提交更大 round_id → 20001")
    void 待完成时跨轮拒绝() {
        seedCloud(120, 120, 3, 260, true, "act-1");
        String before = readProgress();
        ProgressSyncService.SyncOutcome outcome = service().report(DEVICE_ID,
                report(130, 120, 4, 0, false, "act-2"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, outcome.code());
        assertEquals(before, readProgress());
    }

    @Test
    @DisplayName("round_cursor 超过 consumableHan → 20001 且不写盘")
    void 游标越界拒绝() {
        seedCloud(50, 50, 2, 10, false, "act-1");
        String before = readProgress();
        ProgressSyncService service = service();
        ProgressSyncService.SyncOutcome outcome = service.report(DEVICE_ID,
                report(60, 50, 2, service.consumableHan() + 1, false, "act-bad-cursor"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, outcome.code());
        assertEquals(before, readProgress());
    }

    @Test
    @DisplayName("applied_revision 单调不减；command_revision 不递增；设备镜像等于请求")
    void 派生镜像内容与单调() {
        seedCloud(50, 50, 2, 10, false, "act-1");
        ProgressSyncService.SyncOutcome outcome = service().report(DEVICE_ID,
                new SyncReportRequest(DEVICE_ID, service().canonicalScriptureVersion(), 60, 50, 2,
                        "in_progress", 20, false, 0, 88, "no_signal", 2, "2.0.0", "act-2"));
        assertEquals(ApiResponse.SUCCESS_CODE, outcome.code());
        ObjectNode commands = new CommandsStore(MAPPER, dataDir.toString()).read().orElseThrow();
        assertEquals(4, commands.get("applied_revision").asInt(), "请求更小不得压低 applied_revision");
        assertEquals(4, commands.get("command_revision").asInt(), "同步不得递增 command_revision");
        assertEquals(88, outcome.snapshot().batteryPercent());
        assertEquals("no_signal", outcome.snapshot().networkMode());
        assertEquals(2, outcome.snapshot().audioConfigVersion());
        assertEquals("2.0.0", outcome.snapshot().firmwareVersion());
        assertEquals(20, outcome.snapshot().roundCursor(), "成功路径游标等于请求，不得盲推");
    }

    @Test
    @DisplayName("写序：权威 progress 先于派生；派生写失败不回滚权威；同 action_id 重放可收敛")
    void 权威先于派生且派生失败不回滚() {
        seedCloud(50, 50, 2, 10, false, "act-1");
        ProgressStore progress = new ProgressStore(MAPPER, dataDir.toString());
        FailingCommandsStore commands = new FailingCommandsStore(MAPPER, dataDir.toString());
        DeviceStateStore deviceState = new DeviceStateStore(MAPPER, dataDir.toString());
        StateSnapshotService snapshot = new StateSnapshotService(progress, commands, deviceState);
        ProgressSyncService service = new ProgressSyncService(progress, commands, deviceState, snapshot, MAPPER,
                history(progress), RealtimePushPort.NOOP);

        commands.failNextWrite = true;
        ProgressSyncService.SyncOutcome first = service.report(DEVICE_ID,
                report(60, 50, 2, 20, false, "act-2"));
        assertEquals(ApiResponse.SUCCESS_CODE, first.code());
        assertEquals(60, readAcked(), "派生写失败不得回滚已确认权威");
        assertTrue(commands.failedOnce, "用例必须真正注入派生写失败");

        commands.failNextWrite = false;
        ProgressSyncService.SyncOutcome replay = service.report(DEVICE_ID,
                report(60, 50, 2, 20, false, "act-2"));
        assertEquals(60, replay.snapshot().ackedTotal());
        assertTrue(new CommandsStore(MAPPER, dataDir.toString()).read().isPresent(),
                "同 action_id 重放应收敛派生 commands.json");
    }

    @Test
    @DisplayName("负例护栏：同值重复不得推进；降低不得静默回退；不得盲推游标；20004 不阻断；拒绝带 data")
    void 负例矩阵可失败() {
        seedCloud(100, 100, 3, 40, false, "act-1");
        ProgressSyncService service = service();

        // ① 同值重复不得推进
        service.report(DEVICE_ID, report(100, 100, 3, 40, false, "act-a"));
        assertEquals(100, readAcked(), "同值重复提交却推进了 acked_total");

        // ② local_total 低于 cloud 不得静默回退
        ProgressSyncService.SyncOutcome reset = service.report(DEVICE_ID,
                report(50, 50, 3, 10, false, "act-b"));
        assertEquals(ErrorCode.DEVICE_RESET_CONFLICT, reset.code());
        assertEquals(100, readAcked(), "local_total 低于 cloud 却静默回退");

        // ③ 成功上报后游标等于请求值，跨轮拒绝不得改写
        ProgressSyncService.SyncOutcome ok = service.report(DEVICE_ID,
                report(105, 100, 3, 41, false, "act-c"));
        assertEquals(ApiResponse.SUCCESS_CODE, ok.code());
        assertEquals(41, readCursor(), "成功路径不得盲推 round_cursor");
        ProgressSyncService.SyncOutcome cross = service.report(DEVICE_ID,
                report(110, 105, 1, 260, false, "act-d"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, cross.code());
        assertEquals(41, readCursor(), "跨轮拒绝后不得改写 round_cursor");

        // ④ 20004 不阻断确认
        seedCloud(0, 0, 1, 0, false, "act-z");
        ProgressSyncService.SyncOutcome queue = service().report(DEVICE_ID,
                report(1000, 0, 1, 1, false, "act-q2"));
        assertEquals(ErrorCode.QUEUE_FULL, queue.code());
        assertEquals(1000, queue.snapshot().ackedTotal(), "20004 阻断了确认");

        // ⑤ 拒绝响应必须带完整基准
        assertFalse(reset.snapshot() == null, "拒绝响应退化为只回 code/message");
        assertEquals(100, reset.snapshot().ackedTotal());
        assertEquals(DEVICE_ID, reset.snapshot().deviceId());
    }

    @Test
    @DisplayName("末字 + pending → completed 且 pending_completion=false")
    void 末字确认完成() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCloud(100, 100, 3, end - 1, false, "act-1");
        ProgressSyncService.SyncOutcome outcome = service.report(DEVICE_ID,
                report(110, 100, 3, end, true, "act-done"));
        assertEquals(ApiResponse.SUCCESS_CODE, outcome.code());
        assertEquals("completed", outcome.snapshot().roundState());
        assertFalse(outcome.snapshot().pendingCompletion());
        assertEquals(end, outcome.snapshot().roundCursor());
        assertEquals(3, outcome.snapshot().roundId());
    }

    @Test
    @DisplayName("确认前更大 round_id → 20001")
    void 确认前新轮次拒绝() {
        ProgressSyncService service = service();
        seedCloud(120, 120, 3, service.consumableHan(), true, "act-1");
        String before = readProgress();
        ProgressSyncService.SyncOutcome outcome = service.report(DEVICE_ID,
                report(130, 120, 4, 0, false, "act-2"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, outcome.code());
        assertEquals(before, readProgress());
        assertFalse(outcome.snapshot() == null);
    }

    @Test
    @DisplayName("完成后改游标 → 20001；同轮高水位推进后游标仍为 consumableHan")
    void 完成后游标冻结且高水位可推进() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCompleted(200, 200, 5, end, "act-done");

        ProgressSyncService.SyncOutcome mutate = service.report(DEVICE_ID,
                report(210, 200, 5, end - 1, false, "act-mut"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, mutate.code());
        assertEquals(end, readCursor());

        ProgressSyncService.SyncOutcome advance = service.report(DEVICE_ID,
                report(220, 200, 5, end, false, "act-hw", "completed"));
        assertEquals(ApiResponse.SUCCESS_CODE, advance.code());
        assertEquals(220, advance.snapshot().ackedTotal());
        assertEquals(end, advance.snapshot().roundCursor());
        assertEquals("completed", advance.snapshot().roundState());
        assertFalse(advance.snapshot().pendingCompletion());
    }

    @Test
    @DisplayName("矛盾组合：已 completed 再报 pending → 20001；未达末字却 completed/pending → 20001")
    void 矛盾组合拒绝() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCompleted(100, 100, 2, end, "act-c");
        String before = readProgress();
        ProgressSyncService.SyncOutcome pendingOnCompleted = service.report(DEVICE_ID,
                report(110, 100, 2, end, true, "act-p", "completed"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, pendingOnCompleted.code());
        assertEquals(before, readProgress());

        seedCloud(50, 50, 2, 10, false, "act-m");
        before = readProgress();
        ProgressSyncService.SyncOutcome earlyComplete = service.report(DEVICE_ID,
                report(60, 50, 2, 10, false, "act-early", "completed"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, earlyComplete.code());
        assertEquals(before, readProgress());
        assertFalse(earlyComplete.snapshot() == null);

        seedCloud(50, 50, 2, 10, false, "act-m2");
        before = readProgress();
        ProgressSyncService.SyncOutcome earlyPending = service.report(DEVICE_ID,
                report(60, 50, 2, 10, true, "act-early-p"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, earlyPending.code());
        assertEquals(before, readProgress());
    }

    @Test
    @DisplayName("末字 + round_state=completed → 确认完成；末字无意图保持 in_progress；完成后改回 in_progress → 20001")
    void 完成意图分支与锁定改态() {
        ProgressSyncService service = service();
        int end = service.consumableHan();

        seedCloud(100, 100, 3, end - 1, false, "act-1");
        ProgressSyncService.SyncOutcome byState = service.report(DEVICE_ID,
                report(110, 100, 3, end, false, "act-done-state", "completed"));
        assertEquals(ApiResponse.SUCCESS_CODE, byState.code());
        assertEquals("completed", byState.snapshot().roundState());
        assertFalse(byState.snapshot().pendingCompletion());

        seedCloud(100, 100, 4, end - 1, false, "act-2");
        ProgressSyncService.SyncOutcome noIntent = service.report(DEVICE_ID,
                report(110, 100, 4, end, false, "act-end-only"));
        assertEquals(ApiResponse.SUCCESS_CODE, noIntent.code());
        assertEquals("in_progress", noIntent.snapshot().roundState());
        assertFalse(noIntent.snapshot().pendingCompletion());

        seedCompleted(200, 200, 5, end, "act-done");
        ProgressSyncService.SyncOutcome reopen = service.report(DEVICE_ID,
                report(210, 200, 5, end, false, "act-reopen"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, reopen.code());
        assertEquals("completed", readRoundState());
    }

    @Test
    @DisplayName("进行中 /report 更大 round_id → 20001；restart 后推进再同键重放不二次开轮")
    void 跨轮仅restart且乱序幂等() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCloud(50, 50, 2, 10, false, "act-1");
        ProgressSyncService.SyncOutcome jump = service.report(DEVICE_ID,
                report(60, 50, 3, 0, false, "act-jump"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, jump.code());
        assertEquals(2, readRoundId());

        seedCompleted(300, 300, 7, end, "act-prev");
        service.roundAction(DEVICE_ID, new RoundActionRequest("restart", "restart-late"));
        assertEquals(8, readRoundId());
        service.report(DEVICE_ID, report(310, 300, 8, 5, false, "act-adv"));
        assertTrue(readProgress().contains("act-adv"));
        ProgressSyncService.SyncOutcome complete = service.report(DEVICE_ID,
                report(320, 310, 8, end, true, "act-done"));
        assertEquals("completed", complete.snapshot().roundState());
        ProgressSyncService.SyncOutcome lateReplay = service.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "restart-late"));
        assertEquals(ApiResponse.SUCCESS_CODE, lateReplay.code());
        assertEquals(8, lateReplay.snapshot().roundId(), "推进覆盖单槽后同键不得二次开轮");
    }

    @Test
    @DisplayName("restart 创建 round_id+1；同 action_id 不二次递增；exit 保留完成态；未完成 restart → 20001")
    void 篇章动作幂等与门禁() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCompleted(300, 300, 7, end, "act-prev");

        ProgressSyncService.SyncOutcome restart = service.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "restart-1"));
        assertEquals(ApiResponse.SUCCESS_CODE, restart.code());
        assertEquals(8, restart.snapshot().roundId());
        assertEquals(0, restart.snapshot().roundCursor());
        assertEquals("in_progress", restart.snapshot().roundState());
        assertFalse(restart.snapshot().pendingCompletion());
        assertEquals(300, restart.snapshot().ackedTotal());
        assertEquals(300, restart.snapshot().localTotal());

        ProgressSyncService.SyncOutcome replay = service.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "restart-1"));
        assertEquals(8, replay.snapshot().roundId());

        // 新轮次进行中：exit / restart 均 20001
        ProgressSyncService.SyncOutcome exitEarly = service.roundAction(DEVICE_ID,
                new RoundActionRequest("exit", "exit-early"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, exitEarly.code());
        ProgressSyncService.SyncOutcome restartEarly = service.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "restart-2"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, restartEarly.code());

        // 再完成 → exit 保留 → 再 restart
        ProgressSyncService.SyncOutcome complete = service.report(DEVICE_ID,
                report(310, 300, 8, end, true, "act-done2"));
        assertEquals("completed", complete.snapshot().roundState());
        ProgressSyncService.SyncOutcome exit = service.roundAction(DEVICE_ID,
                new RoundActionRequest("exit", "exit-1"));
        assertEquals(ApiResponse.SUCCESS_CODE, exit.code());
        assertEquals(8, exit.snapshot().roundId());
        assertEquals("completed", exit.snapshot().roundState());
        ProgressSyncService.SyncOutcome exitReplay = service.roundAction(DEVICE_ID,
                new RoundActionRequest("exit", "exit-1"));
        assertEquals(8, exitReplay.snapshot().roundId());

        ProgressSyncService.SyncOutcome restartAgain = service.roundAction(DEVICE_ID,
                new RoundActionRequest("restart", "restart-3"));
        assertEquals(9, restartAgain.snapshot().roundId());
        assertEquals(0, restartAgain.snapshot().roundCursor());

        // restart 后可再推进游标
        ProgressSyncService.SyncOutcome advance = service.report(DEVICE_ID,
                report(320, 310, 9, 5, false, "act-adv"));
        assertEquals(ApiResponse.SUCCESS_CODE, advance.code());
        assertEquals(5, advance.snapshot().roundCursor());
    }

    @Test
    @DisplayName("5.2 负例：①未达末字写成 completed ②确认前新 round 覆盖 ③完成后游标推进 ④同 action_id 双轮次 ⑤拒绝缺快照")
    void 完成与跨轮负例矩阵() {
        ProgressSyncService service = service();
        int end = service.consumableHan();

        // ① 未达末字却被写成 completed
        seedCloud(10, 10, 1, 5, false, "n1");
        String before = readProgress();
        ProgressSyncService.SyncOutcome early = service.report(DEVICE_ID,
                report(20, 10, 1, 5, false, "n1a", "completed"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, early.code());
        assertEquals(before, readProgress());
        assertFalse("completed".equals(readRoundState()));

        // ② 确认前新 round_id 覆盖成功（必须失败）
        seedCloud(10, 10, 1, end, true, "n2");
        before = readProgress();
        ProgressSyncService.SyncOutcome cover = service.report(DEVICE_ID,
                report(20, 10, 2, 0, false, "n2a"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, cover.code());
        assertEquals(before, readProgress());

        // ③ 完成后游标被推进（必须失败）
        seedCompleted(10, 10, 1, end, "n3");
        ProgressSyncService.SyncOutcome pushCursor = service.report(DEVICE_ID,
                report(20, 10, 1, 0, false, "n3b", "completed"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, pushCursor.code());
        assertEquals(end, readCursor());
        assertEquals(10, readAcked());

        // ④ 同 action_id restart 产生两个新轮次（必须失败）
        seedCompleted(10, 10, 4, end, "n4");
        service.roundAction(DEVICE_ID, new RoundActionRequest("restart", "same-restart"));
        assertEquals(5, readRoundId());
        service.roundAction(DEVICE_ID, new RoundActionRequest("restart", "same-restart"));
        assertEquals(5, readRoundId(), "同 action_id 不得二次递增 round_id");

        // ⑤ 拒绝响应缺完整快照
        assertFalse(early.snapshot() == null);
        assertEquals(DEVICE_ID, early.snapshot().deviceId());
        assertEquals(10, early.snapshot().ackedTotal());
    }

    @Test
    @DisplayName("确认差量后写入 daily_stats；同 action_id 不双计；派生失败后 catch-up")
    void 日统计归档与幂等catchUp() {
        seedCloud(50, 50, 2, 10, false, "act-1");
        ProgressSyncService service = service();

        ProgressSyncService.SyncOutcome first = service.report(DEVICE_ID,
                report(60, 50, 2, 20, false, "act-day"));
        assertEquals(ApiResponse.SUCCESS_CODE, first.code());
        ObjectNode buckets = new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow();
        int todayBeforeReplay = sumConfirmed(buckets);
        assertEquals(60, todayBeforeReplay);

        ProgressSyncService.SyncOutcome replay = service.report(DEVICE_ID,
                report(999, 50, 2, 20, false, "act-day"));
        assertEquals(60, replay.snapshot().ackedTotal());
        assertEquals(60, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()),
                "同 action_id 不得二次累加日桶");

        // 注入日统计写失败 → 权威仍推进 → 再 report catch-up
        ProgressStore progress = new ProgressStore(MAPPER, dataDir.toString());
        CommandsStore commands = new CommandsStore(MAPPER, dataDir.toString());
        DeviceStateStore deviceState = new DeviceStateStore(MAPPER, dataDir.toString());
        FailingDailyStatsStore daily = new FailingDailyStatsStore(MAPPER, dataDir.toString());
        HistoryStatsService history = new HistoryStatsService(daily, progress, MAPPER,
                Clock.system(ClockConfig.ZONE));
        StateSnapshotService snapshot = new StateSnapshotService(progress, commands, deviceState);
        ProgressSyncService failing = new ProgressSyncService(progress, commands, deviceState, snapshot, MAPPER,
                history, RealtimePushPort.NOOP);

        daily.failNextWrite = true;
        ProgressSyncService.SyncOutcome advance = failing.report(DEVICE_ID,
                report(70, 60, 2, 25, false, "act-fail-day"));
        assertEquals(ApiResponse.SUCCESS_CODE, advance.code());
        assertEquals(70, readAcked(), "日统计写失败不得回滚权威");
        assertTrue(daily.failedOnce);
        assertEquals(60, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()),
                "写失败时桶合计应仍为失败前值");

        daily.failNextWrite = false;
        ProgressSyncService.SyncOutcome catchUp = failing.report(DEVICE_ID,
                report(70, 60, 2, 25, false, "act-fail-day"));
        assertEquals(70, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()),
                "同 action_id 重放应 catch-up 收敛 Σ==acked_total");

        // 再注入写失败后，用新 action_id 的同值高水位 no-op 也应 catch-up（非短路径）
        daily.failNextWrite = true;
        ProgressSyncService.SyncOutcome advance2 = failing.report(DEVICE_ID,
                report(80, 70, 2, 30, false, "act-fail-day-2"));
        assertEquals(ApiResponse.SUCCESS_CODE, advance2.code());
        assertEquals(80, readAcked());
        assertEquals(70, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()));

        daily.failNextWrite = false;
        ProgressSyncService.SyncOutcome noOpCatchUp = failing.report(DEVICE_ID,
                report(80, 80, 2, 30, false, "act-noop-catchup"));
        assertEquals(ApiResponse.SUCCESS_CODE, noOpCatchUp.code());
        assertEquals(80, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()),
                "新 action_id 高水位 no-op 应 catch-up 收敛 Σ==acked_total");
    }

    @Test
    @DisplayName("report 路径 Σ>acked → 50000 fail closed，不静默砍桶")
    void 日统计越权经reportFailClosed() {
        seedCloud(50, 50, 2, 10, false, "act-1");
        ProgressSyncService service = service();
        service.report(DEVICE_ID, report(60, 50, 2, 20, false, "act-ok"));

        ObjectNode inflated = MAPPER.createObjectNode();
        ObjectNode day = MAPPER.createObjectNode();
        day.put(HistoryStatsService.CONFIRMED_TAPS, 999);
        inflated.set(LocalDate.now(Clock.system(ClockConfig.ZONE)).toString(), day);
        new DailyStatsStore(MAPPER, dataDir.toString()).write(inflated);
        int before = sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow());

        BusinessException ex = assertThrows(BusinessException.class, () -> service.report(DEVICE_ID,
                report(60, 60, 2, 20, false, "act-tamper")));
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode());
        assertEquals(60, readAcked(), "fail-closed 不得改权威");
        assertEquals(before, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()),
                "不得静默砍桶");
    }

    @Test
    @DisplayName("损坏 daily_stats 后 report 可 catch-up 重建，不阻断权威")
    void 损坏日统计经reportCatchUp() throws Exception {
        seedCloud(50, 50, 2, 10, false, "act-1");
        ProgressSyncService service = service();
        service.report(DEVICE_ID, report(60, 50, 2, 20, false, "act-before-corrupt"));
        assertEquals(60, readAcked());

        Path statsFile = dataDir.resolve(DailyStatsStore.FILE_NAME);
        Files.writeString(statsFile, "{not-json", StandardCharsets.UTF_8);

        ProgressSyncService.SyncOutcome again = service.report(DEVICE_ID,
                report(70, 60, 2, 25, false, "act-after-corrupt"));
        assertEquals(ApiResponse.SUCCESS_CODE, again.code());
        assertEquals(70, readAcked());
        assertEquals(70, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()),
                "损坏派生面应经写路径 catch-up 重建为 Σ==acked_total");
    }

    @Test
    @DisplayName("restart/exit 不改日统计与 acked_total")
    void 篇章动作不改日统计() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCompleted(100, 100, 3, end, "act-done");
        ProgressSyncService.SyncOutcome seed = service.report(DEVICE_ID,
                report(100, 100, 3, end, false, "act-seed-day", "completed"));
        assertEquals(ApiResponse.SUCCESS_CODE, seed.code());
        assertEquals(100, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()));
        assertEquals(100, readAcked());

        service.roundAction(DEVICE_ID, new RoundActionRequest("exit", "exit-stats"));
        assertEquals(100, readAcked());
        assertEquals(100, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()));

        service.roundAction(DEVICE_ID, new RoundActionRequest("restart", "restart-stats"));
        assertEquals(100, readAcked());
        assertEquals(100, sumConfirmed(new DailyStatsStore(MAPPER, dataDir.toString()).read().orElseThrow()));
    }

    @Test
    @DisplayName("新 action_id 递增 command_revision；同键幂等；载荷域合法")
    void 设置命令递增与同键幂等() {
        seedCloud(50, 50, 2, 10, false, "prog-1");
        ProgressSyncService service = service();

        ProgressSyncService.SyncOutcome first = service.submitSettings(DEVICE_ID,
                settings(70, "high", 30, "cmd-a", null));
        assertEquals(ApiResponse.SUCCESS_CODE, first.code());
        assertEquals(5, first.snapshot().commandRevision());
        assertEquals(4, first.snapshot().appliedRevision());
        assertEquals(70, first.snapshot().volume());
        assertEquals("high", first.snapshot().brightness());
        assertEquals(30, first.snapshot().timeout());
        assertFalse(StateSnapshotService.commandApplied(first.snapshot().appliedRevision(),
                first.snapshot().commandRevision()));

        ProgressSyncService.SyncOutcome replay = service.submitSettings(DEVICE_ID,
                settings(10, "low", 5, "cmd-a", null));
        assertEquals(ApiResponse.SUCCESS_CODE, replay.code());
        assertEquals(5, replay.snapshot().commandRevision(), "同 action_id 不得二次递增");
        assertEquals(70, replay.snapshot().volume(), "同 action_id 不得改载荷");

        ObjectNode onDisk = new CommandsStore(MAPPER, dataDir.toString()).read().orElseThrow();
        assertEquals(5, onDisk.get("command_revision").asInt());
        assertEquals("cmd-a", onDisk.get("action_id").asText());
    }

    @Test
    @DisplayName("base_revision 过期 → 20006 不写盘；null 跳过 CAS；同键+过期基线仍幂等成功")
    void 设置命令Cas拒绝() {
        seedCloud(50, 50, 2, 10, false, "prog-1");
        ProgressSyncService service = service();
        service.submitSettings(DEVICE_ID, settings(60, "mid", 15, "cmd-fresh", null));
        String before = readCommands();

        ProgressSyncService.SyncOutcome stale = service.submitSettings(DEVICE_ID,
                settings(80, "low", 5, "cmd-2", 4));
        assertEquals(ErrorCode.COMMAND_STALE_REVISION, stale.code());
        assertEquals(5, stale.snapshot().commandRevision());
        assertEquals(before, readCommands());

        // 丢失响应重试：同 action_id + 首次提交时的过期 base_revision → 幂等成功，非 20006
        ProgressSyncService.SyncOutcome sameKeyRetry = service.submitSettings(DEVICE_ID,
                settings(99, "low", 5, "cmd-fresh", 4));
        assertEquals(ApiResponse.SUCCESS_CODE, sameKeyRetry.code());
        assertEquals(5, sameKeyRetry.snapshot().commandRevision());
        assertEquals(60, sameKeyRetry.snapshot().volume());
        assertEquals(before, readCommands());

        ProgressSyncService.SyncOutcome ok = service.submitSettings(DEVICE_ID,
                settings(80, "low", 5, "cmd-2", 5));
        assertEquals(ApiResponse.SUCCESS_CODE, ok.code());
        assertEquals(6, ok.snapshot().commandRevision());
        assertEquals(80, ok.snapshot().volume());
    }

    @Test
    @DisplayName("连续两次不同 action_id → 修订 +2；旧 applied 未追上保持未生效；report 追上后已生效")
    void 设置命令与report收敛() {
        seedCloud(50, 50, 2, 10, false, "prog-1");
        ProgressSyncService service = service();

        ProgressSyncService.SyncOutcome first = service.submitSettings(DEVICE_ID,
                settings(40, "low", 5, "set-1", null));
        assertEquals(5, first.snapshot().commandRevision());
        assertFalse(StateSnapshotService.commandApplied(first.snapshot().appliedRevision(),
                first.snapshot().commandRevision()));

        ProgressSyncService.SyncOutcome second = service.submitSettings(DEVICE_ID,
                settings(90, "high", 30, "set-2", null));
        assertEquals(6, second.snapshot().commandRevision());
        assertEquals(90, second.snapshot().volume());
        assertEquals(4, second.snapshot().appliedRevision());
        assertFalse(StateSnapshotService.commandApplied(second.snapshot().appliedRevision(),
                second.snapshot().commandRevision()));

        ProgressSyncService.SyncOutcome report = service.report(DEVICE_ID,
                new SyncReportRequest(DEVICE_ID, service.canonicalScriptureVersion(), 50, 50, 2,
                        "in_progress", 10, false, 6, 76, "connected", 1, "1.0.0", "prog-report"));
        assertEquals(ApiResponse.SUCCESS_CODE, report.code());
        assertEquals(6, report.snapshot().commandRevision(), "report 不得抬升 command_revision");
        assertEquals(6, report.snapshot().appliedRevision());
        assertEquals(90, report.snapshot().volume());
        assertTrue(StateSnapshotService.commandApplied(report.snapshot().appliedRevision(),
                report.snapshot().commandRevision()));

        ProgressSyncService.SyncOutcome reportAgain = service.report(DEVICE_ID,
                new SyncReportRequest(DEVICE_ID, service.canonicalScriptureVersion(), 50, 50, 2,
                        "in_progress", 10, false, 6, 76, "connected", 1, "1.0.0", "prog-report"));
        assertEquals(6, reportAgain.snapshot().commandRevision());
        assertEquals(6, reportAgain.snapshot().appliedRevision());
        assertEquals(90, reportAgain.snapshot().volume());
    }

    @Test
    @DisplayName("设置族与篇章族 action_id 互不覆盖；非法 timeout → 40000")
    void 设置族与篇章族隔离及载荷域() {
        ProgressSyncService service = service();
        int end = service.consumableHan();
        seedCompleted(100, 100, 3, end, "round-act-keep");
        service.submitSettings(DEVICE_ID, settings(55, "mid", 15, "settings-only", null));

        ObjectNode progress = new ProgressStore(MAPPER, dataDir.toString()).read().orElseThrow();
        assertEquals("round-act-keep", progress.get("action_id").asText());
        ObjectNode commands = new CommandsStore(MAPPER, dataDir.toString()).read().orElseThrow();
        assertEquals("settings-only", commands.get("action_id").asText());

        BusinessException ex = assertThrows(BusinessException.class,
                () -> service.submitSettings(DEVICE_ID, settings(50, "mid", 10, "bad-to", null)));
        assertEquals(ErrorCode.PARAM_INVALID, ex.getCode());
    }

    @Test
    @DisplayName("command_revision 已达 MAX → 下一次递增防御为 20006，不可静默降修订")
    void 修订单调不减防御() {
        seedCloud(50, 50, 2, 10, false, "prog-1");
        ObjectNode commands = MAPPER.createObjectNode();
        commands.put("command_revision", Integer.MAX_VALUE);
        commands.put("applied_revision", 0);
        commands.put("action_id", "old");
        commands.put("volume", 50);
        commands.put("brightness", "mid");
        commands.put("timeout", 15);
        new CommandsStore(MAPPER, dataDir.toString()).write(commands);
        String before = readCommands();

        ProgressSyncService.SyncOutcome outcome = service().submitSettings(DEVICE_ID,
                settings(60, "high", 30, "overflow", null));
        assertEquals(ErrorCode.COMMAND_STALE_REVISION, outcome.code());
        assertEquals(before, readCommands());
        assertEquals(Integer.MAX_VALUE, outcome.snapshot().commandRevision());
    }

    private ProgressSyncService service() {
        return serviceOver(dataDir);
    }

    private ProgressSyncService serviceOver(Path directory) {
        ProgressStore progress = new ProgressStore(MAPPER, directory.toString());
        CommandsStore commands = new CommandsStore(MAPPER, directory.toString());
        DeviceStateStore deviceState = new DeviceStateStore(MAPPER, directory.toString());
        StateSnapshotService snapshot = new StateSnapshotService(progress, commands, deviceState);
        return new ProgressSyncService(progress, commands, deviceState, snapshot, MAPPER, history(progress),
                RealtimePushPort.NOOP);
    }

    private HistoryStatsService history(ProgressStore progress) {
        DailyStatsStore daily = new DailyStatsStore(MAPPER, progress.dataDirectory().toString());
        return new HistoryStatsService(daily, progress, MAPPER, Clock.system(ClockConfig.ZONE));
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

    private void seedCloud(int local, int acked, int roundId, int cursor, boolean pending, String actionId) {
        seedProgress(local, acked, roundId, cursor, "in_progress", pending, actionId);
    }

    private void seedCompleted(int local, int acked, int roundId, int cursor, String actionId) {
        seedProgress(local, acked, roundId, cursor, "completed", false, actionId);
    }

    private void seedProgress(int local, int acked, int roundId, int cursor, String roundState,
            boolean pending, String actionId) {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", local);
        progress.put("acked_total", acked);
        progress.put("round_id", roundId);
        progress.put("round_cursor", cursor);
        progress.put("round_state", roundState);
        progress.put("pending_completion", pending);
        progress.put("action_id", actionId);
        new ProgressStore(MAPPER, dataDir.toString()).write(progress);

        ObjectNode commands = MAPPER.createObjectNode();
        commands.put("command_revision", 4);
        commands.put("applied_revision", 4);
        commands.put("action_id", "cmd-1");
        commands.put("volume", 50);
        commands.put("brightness", "mid");
        commands.put("timeout", 15);
        new CommandsStore(MAPPER, dataDir.toString()).write(commands);
    }

    private SyncReportRequest report(int local, int acked, int roundId, int cursor, boolean pending,
            String actionId) {
        return report(local, acked, roundId, cursor, pending, actionId, "in_progress");
    }

    private SyncReportRequest report(int local, int acked, int roundId, int cursor, boolean pending,
            String actionId, String roundState) {
        ProgressSyncService svc = service();
        return new SyncReportRequest(DEVICE_ID, svc.canonicalScriptureVersion(), local, acked, roundId,
                roundState, cursor, pending, 4, 76, "connected", 1, "1.0.0", actionId);
    }

    private String readProgress() {
        try {
            return Files.readString(dataDir.resolve(ProgressStore.fileName()), StandardCharsets.UTF_8);
        } catch (Exception ex) {
            throw new IllegalStateException(ex);
        }
    }

    private String readCommands() {
        try {
            return Files.readString(dataDir.resolve(CommandsStore.fileName()), StandardCharsets.UTF_8);
        } catch (Exception ex) {
            throw new IllegalStateException(ex);
        }
    }

    private static SettingsCommandRequest settings(int volume, String brightness, int timeout,
            String actionId, Integer baseRevision) {
        return new SettingsCommandRequest(volume, brightness, timeout, actionId, baseRevision);
    }

    private int readAcked() {
        Optional<ObjectNode> node = new ProgressStore(MAPPER, dataDir.toString()).read();
        return node.map(n -> n.get("acked_total").asInt()).orElse(-1);
    }

    private int readCursor() {
        Optional<ObjectNode> node = new ProgressStore(MAPPER, dataDir.toString()).read();
        return node.map(n -> n.get("round_cursor").asInt()).orElse(-1);
    }

    private int readRoundId() {
        Optional<ObjectNode> node = new ProgressStore(MAPPER, dataDir.toString()).read();
        return node.map(n -> n.get("round_id").asInt()).orElse(-1);
    }

    private String readRoundState() {
        Optional<ObjectNode> node = new ProgressStore(MAPPER, dataDir.toString()).read();
        return node.map(n -> n.get("round_state").asText()).orElse("");
    }

    /** 可注入一次写失败的 CommandsStore，避免依赖 POSIX 只读位。 */
    private static final class FailingCommandsStore extends CommandsStore {
        boolean failNextWrite;
        boolean failedOnce;

        FailingCommandsStore(ObjectMapper mapper, String dataDir) {
            super(mapper, dataDir);
        }

        @Override
        public void write(ObjectNode payload) {
            if (failNextWrite) {
                failedOnce = true;
                failNextWrite = false;
                throw new RuntimeException("injected derived write failure");
            }
            super.write(payload);
        }
    }

    /** 可注入一次写失败的 DailyStatsStore。 */
    private static final class FailingDailyStatsStore extends DailyStatsStore {
        boolean failNextWrite;
        boolean failedOnce;

        FailingDailyStatsStore(ObjectMapper mapper, String dataDir) {
            super(mapper, dataDir);
        }

        @Override
        public void write(ObjectNode buckets) {
            if (failNextWrite) {
                failedOnce = true;
                failNextWrite = false;
                throw new RuntimeException("injected daily_stats write failure");
            }
            super.write(buckets);
        }
    }
}
