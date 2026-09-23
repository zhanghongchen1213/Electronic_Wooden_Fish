package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
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
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.dto.sync.SyncReportRequest;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * {@code POST /api/v1/sync/report} 契约门禁：鉴权、字段闭包、重复/乱序、冲突码、拒绝带基准。
 *
 * <p>字段闭包经 {@link TestWorkspace#repoRoot()} 机械提取契约注册表，不手抄第三份字段表。
 * 只写临时 {@code app.data-dir}，不触碰仓库 {@code cloud/backend/data/}。
 */
@SpringBootTest
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class ProgressSyncContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final String REPORT_PATH = "/api/v1/sync/report";

    private static final String SNAPSHOT_PATH = "/api/v1/sync/snapshot";

    private static final String RESPONSE_SAMPLE_ID = "https_sync_response";

    private static final String SNAPSHOT_FRAME_TYPE = "snapshot";

    private static final String REPORT_SAMPLE_ID = "https_sync_report";

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-progress-sync-");
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
    private CommandsStore commandsStore;

    @Autowired
    private DeviceStateStore deviceStateStore;

    @MockBean
    private WechatMiniClient wechatMiniClient;

    private String accessToken;

    private String deviceId;

    @BeforeEach
    void 重置临时数据目录并登录() throws Exception {
        for (String file : List.of("identity.json", ProgressStore.fileName(), CommandsStore.fileName(),
                DeviceStateStore.fileName(), "daily_stats.json")) {
            Files.deleteIfExists(DATA_DIR.resolve(file));
        }
        when(wechatMiniClient.exchangeCode("code-a")).thenReturn("openid-a");
        when(wechatMiniClient.exchangeCode("code-b")).thenReturn("openid-b");

        JsonNode login = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(Map.of("code", "code-a"))))
                .andExpect(status().isOk()).andReturn());
        deviceId = login.path("data").path("deviceId").asText();
        accessToken = login.path("data").path("accessToken").asText();
    }

    @AfterAll
    void 清理临时数据目录() throws Exception {
        if (Files.exists(DATA_DIR)) {
            try (Stream<Path> paths = Files.walk(DATA_DIR)) {
                paths.sorted(Comparator.reverseOrder()).forEach(path -> {
                    try {
                        Files.deleteIfExists(path);
                    } catch (IOException ex) {
                        throw new java.io.UncheckedIOException(ex);
                    }
                });
            }
        }
    }

    @Test
    @DisplayName("新端点默认受 Bearer 保护：无令牌 40103，身份不匹配 40300")
    void 鉴权保护() throws Exception {
        JsonNode missing = json(mockMvc.perform(post(REPORT_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(reportBody(10, 0, 1, 1, false, "a1"))))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(ErrorCode.TOKEN_MISSING, missing.path("code").asInt());

        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(Map.of("code", "code-b"))))
                .andExpect(status().isOk());

        JsonNode foreign = json(mockMvc.perform(post(REPORT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(reportBody(10, 0, 1, 1, false, "a2"))))
                .andExpect(status().isForbidden()).andReturn());
        assertEquals(ErrorCode.IDENTITY_MISMATCH, foreign.path("code").asInt());
    }

    @Test
    @DisplayName("成功响应 17 字段闭包与契约注册表逐字相等，无 delta/high_watermark/scripture_version")
    void 响应字段闭包() throws Exception {
        Set<String> expected = contractResponseFields();
        JsonNode body = json(report(128, 0, 3, 96, false, "sync-1"));
        assertEquals(0, body.path("code").asInt());
        Set<String> wire = fieldNames(body.path("data"));
        assertEquals(expected, wire);
        assertFalse(wire.contains("delta"));
        assertFalse(wire.contains("high_watermark"));
        assertFalse(wire.contains("scripture_version"));
        assertEquals(expected, fieldNames(objectMapper.readTree(objectMapper.writeValueAsString(
                new StateSnapshotResponse(deviceId, 128, 128, 3, "in_progress", 96, false, 0, 0, 128,
                        76, "connected", 1, "1.0.0", 50, "mid", 15)))));
    }

    @Test
    @DisplayName("请求体字段闭包恰为契约 §6.1 的 14 字段（DTO + helper）")
    void 请求字段闭包() throws Exception {
        Set<String> expected = contractReportFields();
        ObjectNode body = reportBody(10, 0, 1, 1, false, "req-1");
        assertEquals(expected, fieldNames(body));
        SyncReportRequest dto = new SyncReportRequest("d", "HS-1.0.0", 1, 0, 1, "in_progress",
                1, false, 0, 76, "connected", 1, "1.0.0", "a1");
        assertEquals(expected, fieldNames(objectMapper.readTree(objectMapper.writeValueAsString(dto))),
                "SyncReportRequest 序列化字段必须对齐契约 14 字段");
    }

    @Test
    @DisplayName("重复同值 no-op；同 action_id 乱序重试返回同一权威结果")
    void 重复与乱序幂等() throws Exception {
        JsonNode first = json(report(50, 0, 1, 10, false, "idem-1"));
        assertEquals(0, first.path("code").asInt());
        assertEquals(50, first.path("data").path("acked_total").asInt());
        assertEquals(deviceId, first.path("data").path("device_id").asText(),
                "响应身份必须来自 JWT，不得信任请求体 spoofed device_id");

        JsonNode noop = json(report(50, 50, 1, 10, false, "idem-2"));
        assertEquals(0, noop.path("code").asInt());
        assertEquals(50, noop.path("data").path("acked_total").asInt());

        // no-op 夹在中间后，原 action_id 仍须幂等（不得被新键覆盖）。
        JsonNode replay = json(report(999, 50, 1, 10, false, "idem-1"));
        assertEquals(50, replay.path("data").path("acked_total").asInt());
        assertEquals(0, replay.path("code").asInt());
    }

    @Test
    @DisplayName("battery_percent > 100 → 协议校验拒绝")
    void 电量越界拒绝() throws Exception {
        ObjectNode body = reportBody(1, 0, 1, 1, false, "bat-1");
        body.put("battery_percent", 101);
        JsonNode response = json(mockMvc.perform(post(REPORT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(body)))
                .andExpect(status().isBadRequest()).andReturn());
        assertEquals(ErrorCode.PARAM_INVALID, response.path("code").asInt());
    }

    @Test
    @DisplayName("local_total 降低触发 20003，且拒绝仍带完整基准")
    void 降低高水位() throws Exception {
        json(report(80, 0, 1, 10, false, "r1"));
        JsonNode conflict = json(report(40, 40, 1, 5, false, "r2"));
        assertEquals(ErrorCode.DEVICE_RESET_CONFLICT, conflict.path("code").asInt());
        assertEquals(Set.of("code", "message", "data"), fieldNames(conflict));
        assertEquals(80, conflict.path("data").path("acked_total").asInt());
        assertEquals(80, conflict.path("data").path("snapshot_seq").asInt());
    }

    @Test
    @DisplayName("跨轮无法归属触发 20001")
    void 跨轮() throws Exception {
        json(report(20, 0, 5, 10, false, "c1"));
        JsonNode body = json(report(30, 20, 3, 5, false, "c2"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, body.path("code").asInt());
        assertNotNull(body.path("data"));
        assertEquals(20, body.path("data").path("acked_total").asInt());
    }

    @Test
    @DisplayName("积压达上限 20004 仍确认；版本不一致 20005 不推进")
    void 积压与版本() throws Exception {
        JsonNode queue = json(report(1000, 0, 1, 1, false, "q1"));
        assertEquals(ErrorCode.QUEUE_FULL, queue.path("code").asInt());
        assertEquals(1000, queue.path("data").path("acked_total").asInt());

        ObjectNode badVersion = reportBody(1100, 1000, 1, 2, false, "q2");
        badVersion.put("scripture_version", "HS-0.0.0");
        JsonNode mismatch = json(mockMvc.perform(post(REPORT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(badVersion)))
                .andExpect(status().isOk()).andReturn());
        assertEquals(ErrorCode.SCRIPTURE_VERSION_MISMATCH, mismatch.path("code").asInt());
        assertEquals(1000, mismatch.path("data").path("acked_total").asInt());
        assertEquals(1000, progressStore.read().get().get("acked_total").asInt());
    }

    @Test
    @DisplayName("GET /snapshot 语义不受本 Story 影响")
    void 既有查询端点仍可用() throws Exception {
        json(report(12, 0, 1, 3, false, "s1"));
        JsonNode snap = json(mockMvc.perform(get(SNAPSHOT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .param("acked_total", "12")
                .param("snapshot_seq", "12"))
                .andExpect(status().isOk()).andReturn());
        assertEquals(0, snap.path("code").asInt());
        assertEquals(12, snap.path("data").path("acked_total").asInt());
    }

    @Test
    @DisplayName("确认差量后写入 daily_stats.json；仅确认/catch-up 可写")
    void 确认后写日统计() throws Exception {
        assertTrue(Files.notExists(DATA_DIR.resolve("daily_stats.json")));
        JsonNode body = json(report(5, 0, 1, 1, false, "d1"));
        assertEquals(0, body.path("code").asInt());
        assertTrue(Files.exists(DATA_DIR.resolve("daily_stats.json")),
                "确认差量成功后应写入 daily_stats.json");
        ObjectNode buckets = new top.zhcmqtt.ewf.backend.service.DailyStatsStore(
                objectMapper, DATA_DIR.toString()).read().orElseThrow();
        int sum = 0;
        var it = buckets.fields();
        while (it.hasNext()) {
            sum += it.next().getValue().path("confirmed_taps").asInt(0);
        }
        assertEquals(5, sum);
        assertTrue(directoryContent().keySet().contains("daily_stats.json"));
    }

    @Test
    @DisplayName("report 镜像 applied_revision 时不抬升 command_revision、不改设置载荷")
    void report不抬升命令修订() throws Exception {
        ObjectNode commands = MAPPER.createObjectNode();
        commands.put("command_revision", 7);
        commands.put("applied_revision", 3);
        commands.put("action_id", "settings-keep");
        commands.put("volume", 66);
        commands.put("brightness", "high");
        commands.put("timeout", 30);
        commandsStore.write(commands);

        JsonNode body = json(report(8, 0, 1, 2, false, "r-cmd"));
        assertEquals(0, body.path("code").asInt());
        assertEquals(7, body.path("data").path("command_revision").asInt());
        assertEquals(3, body.path("data").path("applied_revision").asInt(),
                "applied_revision 请求更小不得压低；本例 report 携带 0，磁盘保持 3");
        assertEquals(66, body.path("data").path("volume").asInt());
        assertEquals("high", body.path("data").path("brightness").asText());
        assertEquals(30, body.path("data").path("timeout").asInt());
        assertEquals("settings-keep", commandsStore.read().orElseThrow().get("action_id").asText());
    }

    private MvcResult report(int local, int acked, int roundId, int cursor, boolean pending, String actionId)
            throws Exception {
        return mockMvc.perform(post(REPORT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(
                        reportBody(local, acked, roundId, cursor, pending, actionId))))
                .andExpect(status().isOk()).andReturn();
    }

    private ObjectNode reportBody(int local, int acked, int roundId, int cursor, boolean pending,
            String actionId) {
        ObjectNode body = MAPPER.createObjectNode();
        body.put("device_id", "spoofed-must-be-ignored");
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
        return body;
    }

    private static Set<String> contractResponseFields() throws IOException {
        JsonNode registry = contractRegistry();
        Set<String> fields = new TreeSet<>();
        for (JsonNode frame : registry.get("ws_frames")) {
            if (SNAPSHOT_FRAME_TYPE.equals(frame.path("type").asText())) {
                frame.path("payload_fields").forEach(field -> fields.add(field.asText()));
            }
        }
        for (JsonNode sample : registry.get("samples")) {
            if (RESPONSE_SAMPLE_ID.equals(sample.path("id").asText())) {
                sample.path("required").forEach(field -> fields.add(field.asText()));
            }
        }
        assertFalse(fields.isEmpty());
        return fields;
    }

    private static Set<String> contractReportFields() throws IOException {
        JsonNode registry = contractRegistry();
        Set<String> fields = new TreeSet<>();
        for (JsonNode sample : registry.get("samples")) {
            if (REPORT_SAMPLE_ID.equals(sample.path("id").asText())) {
                sample.path("required").forEach(field -> fields.add(field.asText()));
            }
        }
        assertEquals(14, fields.size());
        return fields;
    }

    private static JsonNode contractRegistry() throws IOException {
        Path schema = TestWorkspace.repoRoot().resolve("docs/contracts/sync-contract.schema.json");
        assertTrue(Files.isRegularFile(schema));
        return MAPPER.readTree(Files.readString(schema, StandardCharsets.UTF_8));
    }

    private Map<String, String> directoryContent() throws IOException {
        Map<String, String> content = new LinkedHashMap<>();
        try (Stream<Path> paths = Files.list(DATA_DIR)) {
            for (Path path : paths.sorted().toList()) {
                content.put(path.getFileName().toString(), Files.readString(path, StandardCharsets.UTF_8));
            }
        }
        return content;
    }

    private JsonNode json(MvcResult result) throws Exception {
        return objectMapper.readTree(result.getResponse().getContentAsString(StandardCharsets.UTF_8));
    }

    private static Set<String> fieldNames(JsonNode node) {
        Set<String> names = new TreeSet<>();
        node.fieldNames().forEachRemaining(names::add);
        return names;
    }
}
