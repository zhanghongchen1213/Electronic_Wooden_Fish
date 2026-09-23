package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Clock;
import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneId;
import java.time.ZoneOffset;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.config.ClockConfig;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.dto.sync.HistoryStatsResponse;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.HistoryStatsService;
import top.zhcmqtt.ewf.backend.service.ProgressStore;

/**
 * 日桶归档 / catch-up / 聚合查询单元覆盖（{@code @TempDir} + 可注入 {@link Clock#fixed}）。
 */
class HistoryStatsServiceTest {

    private static final ObjectMapper MAPPER = new ObjectMapper();
    private static final ZoneId SHANGHAI = ClockConfig.ZONE;

    @TempDir
    Path dataDir;

    @Test
    @DisplayName("确认差量归档到今日桶；同额 catch-up no-op 不加")
    void 推进归档与幂等() {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        HistoryStatsService service = service(clock);
        seedProgress(10, 10);

        service.archiveAndCatchUp(18);
        ObjectNode buckets = daily().read().orElseThrow();
        assertEquals(18, buckets.path("2026-09-23").path("confirmed_taps").asInt());
        assertEquals(18, service.sumConfirmed(buckets));

        service.archiveAndCatchUp(18);
        assertEquals(18, daily().read().orElseThrow().path("2026-09-23").path("confirmed_taps").asInt());
    }

    @Test
    @DisplayName("UTC 16:30 → 上海已跨日；差量落入确认日 2026-09-23")
    void 跨上海日界按确认日归档() {
        // 2026-09-22T16:30:00Z → Asia/Shanghai = 2026-09-23 00:30
        Clock clock = Clock.fixed(Instant.parse("2026-09-22T16:30:00Z"), SHANGHAI);
        assertEquals(LocalDate.of(2026, 9, 23), LocalDate.now(clock));
        HistoryStatsService service = service(clock);
        seedProgress(0, 0);

        service.archiveAndCatchUp(5);
        ObjectNode buckets = daily().read().orElseThrow();
        assertTrue(buckets.has("2026-09-23"));
        assertFalse(buckets.has("2026-09-22"), "不得按 UTC 日或设备日拆桶");
        assertEquals(5, buckets.path("2026-09-23").path("confirmed_taps").asInt());
    }

    @Test
    @DisplayName("负例：非上海时区 Clock 构造即失败")
    void 禁止非上海时区() {
        Clock utc = Clock.fixed(Instant.parse("2026-09-23T04:00:00Z"), ZoneOffset.UTC);
        assertThrows(IllegalStateException.class, () -> service(utc));
    }

    @Test
    @DisplayName("acked 前进但日统计写失败 → 再次 catch-up 收敛")
    void 派生失败后catchUp() {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        ProgressStore progress = new ProgressStore(MAPPER, dataDir.toString());
        FailingDailyStatsStore daily = new FailingDailyStatsStore(MAPPER, dataDir.toString());
        HistoryStatsService service = new HistoryStatsService(daily, progress, MAPPER, clock);
        seedProgress(0, 0);

        daily.failNextWrite = true;
        service.archiveAndCatchUp(7);
        assertTrue(daily.failedOnce);
        assertTrue(daily().read().isEmpty(), "写失败时文件应仍缺失");

        daily.failNextWrite = false;
        seedProgress(7, 7);
        service.archiveAndCatchUp(7);
        assertEquals(7, daily().read().orElseThrow().path("2026-09-23").path("confirmed_taps").asInt());
        assertEquals(7, service.sumConfirmed(daily().read().orElseThrow()));
    }

    @Test
    @DisplayName("Σ > acked_total → 50000 fail closed，不静默砍桶")
    void 桶合计越权failClosed() {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        HistoryStatsService service = service(clock);
        ObjectNode buckets = MAPPER.createObjectNode();
        ObjectNode day = MAPPER.createObjectNode();
        day.put(HistoryStatsService.CONFIRMED_TAPS, 20);
        buckets.set("2026-09-23", day);
        daily().write(buckets);
        seedProgress(10, 10);

        BusinessException ex = assertThrows(BusinessException.class, () -> service.archiveAndCatchUp(10));
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode());
        assertEquals(20, daily().read().orElseThrow().path("2026-09-23").path("confirmed_taps").asInt());
    }

    @Test
    @DisplayName("连续天数：今日为 0 从昨日起计；今日昨日皆 0 → 0")
    void 连续天数裁决F() {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        HistoryStatsService service = service(clock);
        ObjectNode buckets = MAPPER.createObjectNode();
        putDay(buckets, "2026-09-22", 3);
        putDay(buckets, "2026-09-21", 2);
        daily().write(buckets);
        seedProgress(5, 5);

        HistoryStatsResponse stats = service.query();
        assertEquals(0, stats.todayTaps());
        assertEquals(2, stats.streakDays());

        putDay(buckets, "2026-09-23", 1);
        daily().write(buckets);
        assertEquals(3, service.query().streakDays());

        daily().write(MAPPER.createObjectNode());
        seedProgress(0, 0);
        assertEquals(0, service.query().streakDays());
    }

    @Test
    @DisplayName("空态与待同步：正式数字不含未确认差")
    void 空态与待同步() {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        HistoryStatsService service = service(clock);

        HistoryStatsResponse empty = service.query();
        assertTrue(empty.empty());
        assertFalse(empty.pendingSync());
        assertEquals(0, empty.todayTaps());
        assertEquals(0, empty.totalTaps());

        seedProgress(15, 10);
        service.archiveAndCatchUp(10);
        HistoryStatsResponse pending = service.query();
        assertFalse(pending.empty());
        assertTrue(pending.pendingSync());
        assertEquals(10, pending.todayTaps());
        assertEquals(10, pending.totalTaps(), "total_taps 以 acked_total 为准，不含未确认差 5");
        assertEquals(10, pending.last7DaysTaps());
    }

    @Test
    @DisplayName("损坏 daily_stats 写路径按空桶 catch-up；多余桶字段写回时剥离")
    void 损坏与桶形状收紧() throws Exception {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        HistoryStatsService service = service(clock);
        Path file = daily().file();
        Files.writeString(file, "{\"schema_version\":1,\"buckets\":{", StandardCharsets.UTF_8);

        seedProgress(12, 12);
        service.archiveAndCatchUp(12);
        ObjectNode rebuilt = daily().read().orElseThrow();
        assertEquals(12, rebuilt.path("2026-09-23").path(HistoryStatsService.CONFIRMED_TAPS).asInt());
        assertEquals(12, service.sumConfirmed(rebuilt));

        ObjectNode dirty = MAPPER.createObjectNode();
        ObjectNode day = MAPPER.createObjectNode();
        day.put(HistoryStatsService.CONFIRMED_TAPS, 12);
        day.put("noise", "x");
        dirty.set("2026-09-23", day);
        dirty.set("not-a-date", MAPPER.createObjectNode().put(HistoryStatsService.CONFIRMED_TAPS, 99));
        daily().write(dirty);
        assertEquals(12, service.sumConfirmed(daily().read().orElseThrow()), "非法日键不计入 Σ");

        service.archiveAndCatchUp(15);
        ObjectNode cleanedDay = (ObjectNode) daily().read().orElseThrow().get("2026-09-23");
        assertEquals(15, cleanedDay.path(HistoryStatsService.CONFIRMED_TAPS).asInt());
        assertFalse(cleanedDay.has("noise"), "写回须剥离多余桶字段");
    }

    @Test
    @DisplayName("近 7/30 日窗口含今日共 7/30 个上海自然日")
    void 窗口求和() {
        Clock clock = fixedShanghai("2026-09-23T04:00:00Z");
        HistoryStatsService service = service(clock);
        ObjectNode buckets = MAPPER.createObjectNode();
        putDay(buckets, "2026-09-23", 1);
        putDay(buckets, "2026-09-17", 2);
        putDay(buckets, "2026-09-16", 4);
        putDay(buckets, "2026-08-20", 8);
        daily().write(buckets);
        seedProgress(15, 15);

        HistoryStatsResponse stats = service.query();
        assertEquals(1, stats.todayTaps());
        assertEquals(3, stats.last7DaysTaps(), "含今日往前 7 日：23+17，不含 16");
        assertEquals(7, stats.last30DaysTaps(), "含 23+17+16，不含 8-20（超出 30 日窗）");
        assertEquals(15, stats.totalTaps());
    }

    private HistoryStatsService service(Clock clock) {
        return new HistoryStatsService(daily(), progress(), MAPPER, clock);
    }

    private ProgressStore progress() {
        return new ProgressStore(MAPPER, dataDir.toString());
    }

    private DailyStatsStore daily() {
        return new DailyStatsStore(MAPPER, dataDir.toString());
    }

    private void seedProgress(int local, int acked) {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", local);
        progress.put("acked_total", acked);
        progress.put("round_id", 1);
        progress.put("round_cursor", 0);
        progress.put("round_state", "in_progress");
        progress.put("pending_completion", false);
        progress.put("action_id", "seed");
        progress().write(progress);
    }

    private static void putDay(ObjectNode buckets, String day, int taps) {
        ObjectNode dayNode = MAPPER.createObjectNode();
        dayNode.put(HistoryStatsService.CONFIRMED_TAPS, taps);
        buckets.set(day, dayNode);
    }

    private static Clock fixedShanghai(String instant) {
        return Clock.fixed(Instant.parse(instant), SHANGHAI);
    }

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
