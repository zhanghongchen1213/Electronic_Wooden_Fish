package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Stream;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestInstance;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.MediaType;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.MvcResult;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.client.WechatMiniClient;
import top.zhcmqtt.ewf.backend.dto.sync.HistoryStatsResponse;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.HistoryStatsService;
import top.zhcmqtt.ewf.backend.service.ProgressStore;

/**
 * {@code GET /api/v1/sync/stats} 契约门禁：鉴权、字段闭包、空态/待同步、查询只读、重启后稳定。
 */
@SpringBootTest
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class HistoryStatsContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final String STATS_PATH = "/api/v1/sync/stats";
    private static final String REPORT_PATH = "/api/v1/sync/report";
    private static final String SNAPSHOT_PATH = "/api/v1/sync/snapshot";

    private static final Set<String> STATS_FIELDS = Set.of(
            "today_taps", "last_7_days_taps", "last_30_days_taps", "total_taps",
            "streak_days", "empty", "pending_sync");

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-history-stats-");
        } catch (Exception ex) {
            throw new ExceptionInInitializerError(ex);
        }
    }

    @DynamicPropertySource
    static void properties(DynamicPropertyRegistry registry) {
        registry.add("app.data-dir", () -> DATA_DIR.toString());
        registry.add("jwt.secret", () -> TEST_SECRET);
        registry.add("jwt.access-token-expiration", () -> "7200000");
        registry.add("jwt.refresh-token-expiration", () -> "2592000000");
    }

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @Autowired
    private ProgressStore progressStore;

    @Autowired
    private DailyStatsStore dailyStatsStore;

    @Autowired
    private HistoryStatsService historyStatsService;

    @MockBean
    private WechatMiniClient wechatMiniClient;

    private String accessToken;

    @BeforeEach
    void 重置临时数据目录并登录() throws Exception {
        for (String file : List.of("identity.json", ProgressStore.fileName(), CommandsStore.fileName(),
                DeviceStateStore.fileName(), DailyStatsStore.fileName())) {
            Files.deleteIfExists(DATA_DIR.resolve(file));
        }
        when(wechatMiniClient.exchangeCode("code-stats")).thenReturn("openid-stats");

        JsonNode login = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(Map.of("code", "code-stats"))))
                .andExpect(status().isOk()).andReturn());
        accessToken = login.path("data").path("accessToken").asText();
    }

    @AfterAll
    void 清理临时数据目录() throws Exception {
        if (Files.exists(DATA_DIR)) {
            try (Stream<Path> paths = Files.walk(DATA_DIR)) {
                paths.sorted(Comparator.reverseOrder()).forEach(path -> {
                    try {
                        Files.deleteIfExists(path);
                    } catch (Exception ignored) {
                        // best-effort
                    }
                });
            }
        }
    }

    @Test
    @DisplayName("无 JWT → 401；字段闭包恰为裁决 C 七字段")
    void 鉴权与字段闭包() throws Exception {
        mockMvc.perform(get(STATS_PATH)).andExpect(status().isUnauthorized());

        JsonNode body = json(mockMvc.perform(get(STATS_PATH)
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk()).andReturn());
        assertEquals(0, body.path("code").asInt());
        JsonNode data = body.path("data");
        assertEquals(STATS_FIELDS, fieldNames(data));
        assertTrue(data.path("empty").asBoolean());
        assertEquals(0, data.path("today_taps").asInt());
        assertEquals(0, data.path("total_taps").asInt());
    }

    @Test
    @DisplayName("确认后正式数字更新；pending_sync 反映 local>acked；查询不写盘")
    void 空态待同步与查询只读() throws Exception {
        assertTrue(Files.notExists(DATA_DIR.resolve(DailyStatsStore.fileName())));

        JsonNode empty = json(mockMvc.perform(get(STATS_PATH)
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk()).andReturn()).path("data");
        assertTrue(empty.path("empty").asBoolean());
        assertTrue(Files.notExists(DATA_DIR.resolve(DailyStatsStore.fileName())),
                "负例护栏：查询路径不得创建 daily_stats.json");

        json(report(10, 0, 1, 2, false, "hs-1"));
        assertTrue(Files.exists(DATA_DIR.resolve(DailyStatsStore.fileName())));
        long sizeBefore = Files.size(DATA_DIR.resolve(DailyStatsStore.fileName()));
        String contentBefore = Files.readString(DATA_DIR.resolve(DailyStatsStore.fileName()),
                StandardCharsets.UTF_8);

        // 制造待同步镜像：直接写 progress local>acked，不经 report（字段列不含 schema_version）
        ObjectNode current = progressStore.read().orElseThrow();
        ObjectNode progress = objectMapper.createObjectNode();
        progress.put("local_total", 15);
        progress.put("acked_total", current.get("acked_total").asInt());
        progress.put("round_id", current.get("round_id").asInt());
        progress.put("round_cursor", current.get("round_cursor").asInt());
        progress.put("round_state", current.get("round_state").asText());
        progress.put("pending_completion", current.get("pending_completion").asBoolean());
        progress.put("action_id", current.get("action_id").asText());
        progressStore.write(progress);

        JsonNode pending = json(mockMvc.perform(get(STATS_PATH)
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk()).andReturn()).path("data");
        assertTrue(pending.path("pending_sync").asBoolean());
        assertEquals(10, pending.path("today_taps").asInt(), "负例：未确认差不得计入 today_taps");
        assertEquals(10, pending.path("total_taps").asInt());
        assertFalse(pending.path("empty").asBoolean());

        assertEquals(contentBefore, Files.readString(DATA_DIR.resolve(DailyStatsStore.fileName()),
                StandardCharsets.UTF_8), "查询不得 catch-up 写盘");
        assertEquals(sizeBefore, Files.size(DATA_DIR.resolve(DailyStatsStore.fileName())));
    }

    @Test
    @DisplayName("重建 Store（重启近似）后查询稳定")
    void 重启后稳定() throws Exception {
        json(report(8, 0, 1, 1, false, "hs-stable"));
        JsonNode first = json(mockMvc.perform(get(STATS_PATH)
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk()).andReturn()).path("data");

        DailyStatsStore rebuiltDaily = new DailyStatsStore(objectMapper, DATA_DIR.toString());
        ProgressStore rebuiltProgress = new ProgressStore(objectMapper, DATA_DIR.toString());
        HistoryStatsService rebuilt = new HistoryStatsService(rebuiltDaily, rebuiltProgress, objectMapper,
                java.time.Clock.system(top.zhcmqtt.ewf.backend.common.config.ClockConfig.ZONE));
        HistoryStatsResponse second = rebuilt.query();

        assertEquals(first.path("today_taps").asInt(), second.todayTaps());
        assertEquals(first.path("total_taps").asInt(), second.totalTaps());
        assertEquals(first.path("last_7_days_taps").asInt(), second.last7DaysTaps());
        assertEquals(first.path("streak_days").asInt(), second.streakDays());
        assertEquals(first.path("empty").asBoolean(), second.empty());
    }

    @Test
    @DisplayName("负例：统计字段不得进入 snapshot 17 闭包")
    void 快照闭包不含统计字段() throws Exception {
        json(report(3, 0, 1, 1, false, "hs-snap"));
        JsonNode snap = json(mockMvc.perform(get(SNAPSHOT_PATH)
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk()).andReturn()).path("data");
        Set<String> names = fieldNames(snap);
        assertEquals(17, names.size());
        for (String statsField : STATS_FIELDS) {
            assertFalse(names.contains(statsField), "snapshot 不得含 " + statsField);
        }
        // DTO 序列化闭包也钉死 17
        String wire = objectMapper.writeValueAsString(new StateSnapshotResponse(
                "d", 0, 0, 1, "in_progress", 0, false, 0, 0, 0, 0, "no_signal", 0, "", 50, "mid", 15));
        Set<String> dtoKeys = fieldNames(objectMapper.readTree(wire));
        assertEquals(17, dtoKeys.size());
        assertFalse(dtoKeys.contains("today_taps"));
    }

    private MvcResult report(int local, int acked, int roundId, int cursor, boolean pending, String actionId)
            throws Exception {
        ObjectNode body = MAPPER.createObjectNode();
        body.put("device_id", "spoofed");
        body.put("scripture_version", "HS-1.0.0");
        body.put("local_total", local);
        body.put("acked_total", acked);
        body.put("round_id", roundId);
        body.put("round_state", "in_progress");
        body.put("round_cursor", cursor);
        body.put("pending_completion", pending);
        body.put("applied_revision", 0);
        body.put("battery_percent", 76);
        body.put("network_mode", "connected");
        body.put("audio_config_version", 1);
        body.put("firmware_version", "1.0.0");
        body.put("action_id", actionId);
        return mockMvc.perform(post(REPORT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(body)))
                .andExpect(status().isOk()).andReturn();
    }

    private JsonNode json(MvcResult result) throws Exception {
        return objectMapper.readTree(result.getResponse().getContentAsString(StandardCharsets.UTF_8));
    }

    private static Set<String> fieldNames(JsonNode node) {
        Set<String> names = new LinkedHashSet<>();
        Iterator<String> it = node.fieldNames();
        while (it.hasNext()) {
            names.add(it.next());
        }
        return names;
    }
}
