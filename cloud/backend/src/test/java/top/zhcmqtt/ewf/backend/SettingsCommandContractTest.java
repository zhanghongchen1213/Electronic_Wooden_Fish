package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
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
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;
import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * {@code POST /api/v1/sync/command} 契约门禁：鉴权、字段闭包、待应用→已生效、旧修订拒绝。
 *
 * <p>只写临时 {@code app.data-dir}，不触碰仓库 {@code cloud/backend/data/}。
 */
@SpringBootTest
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class SettingsCommandContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final String COMMAND_PATH = "/api/v1/sync/command";

    private static final String REPORT_PATH = "/api/v1/sync/report";

    private static final String RESPONSE_SAMPLE_ID = "https_sync_response";

    private static final String SNAPSHOT_FRAME_TYPE = "snapshot";

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-settings-cmd-");
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

    @MockBean
    private WechatMiniClient wechatMiniClient;

    private String accessToken;

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
    @DisplayName("command 默认受 Bearer 保护：无令牌 40103")
    void 鉴权保护() throws Exception {
        JsonNode missing = json(mockMvc.perform(post(COMMAND_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(commandBody(50, "mid", 15, "x", null))))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(ErrorCode.TOKEN_MISSING, missing.path("code").asInt());
    }

    @Test
    @DisplayName("成功下发后快照 17 字段闭包；待应用→report 追上→已生效")
    void 待应用至已生效收敛() throws Exception {
        seedProgress();
        JsonNode first = json(command(70, "high", 30, "set-1", null));
        assertEquals(0, first.path("code").asInt());
        JsonNode data = first.path("data");
        assertEquals(contractResponseFields(), fieldNames(data));
        assertEquals(1, data.path("command_revision").asInt());
        assertEquals(0, data.path("applied_revision").asInt());
        assertFalse(StateSnapshotService.commandApplied(
                data.path("applied_revision").asInt(), data.path("command_revision").asInt()));

        JsonNode reportBody = json(report(10, 0, 1, 1, false, "r1", 1));
        assertEquals(0, reportBody.path("code").asInt());
        assertEquals(1, reportBody.path("data").path("command_revision").asInt());
        assertEquals(1, reportBody.path("data").path("applied_revision").asInt());
        assertEquals(70, reportBody.path("data").path("volume").asInt());
        assertTrue(StateSnapshotService.commandApplied(
                reportBody.path("data").path("applied_revision").asInt(),
                reportBody.path("data").path("command_revision").asInt()));

        JsonNode reportAgain = json(report(10, 10, 1, 1, false, "r1", 1));
        assertEquals(1, reportAgain.path("data").path("command_revision").asInt());
        assertEquals(70, reportAgain.path("data").path("volume").asInt());
    }

    @Test
    @DisplayName("base_revision 过期 → 20006 + 完整快照，磁盘不变")
    void 旧修订拒绝带完整快照() throws Exception {
        seedProgress();
        json(command(50, "mid", 15, "set-1", null));
        String before = Files.readString(DATA_DIR.resolve(CommandsStore.fileName()), StandardCharsets.UTF_8);

        JsonNode rejected = json(command(90, "low", 5, "set-2", 0));
        assertEquals(ErrorCode.COMMAND_STALE_REVISION, rejected.path("code").asInt());
        assertEquals(contractResponseFields(), fieldNames(rejected.path("data")));
        assertEquals(1, rejected.path("data").path("command_revision").asInt());
        assertEquals(50, rejected.path("data").path("volume").asInt());
        assertEquals(before, Files.readString(DATA_DIR.resolve(CommandsStore.fileName()), StandardCharsets.UTF_8));
    }

    @Test
    @DisplayName("负例：同 action_id 二次提交不得递增；新 action_id 必须递增")
    void 负例修订行为() throws Exception {
        seedProgress();
        JsonNode first = json(command(40, "low", 5, "same-id", null));
        assertEquals(1, first.path("data").path("command_revision").asInt());

        JsonNode replay = json(command(99, "high", 30, "same-id", null));
        assertEquals(1, replay.path("data").path("command_revision").asInt(),
                "负例②：同 action_id 不得二次递增");
        assertEquals(40, replay.path("data").path("volume").asInt());

        JsonNode next = json(command(55, "mid", 15, "other-id", null));
        assertEquals(2, next.path("data").path("command_revision").asInt(),
                "负例①：新 action_id 必须递增修订");
    }

    @Test
    @DisplayName("负例：report 不得抬升 command_revision；20006 不得只回两键")
    void 负例Report与拒绝信封() throws Exception {
        seedProgress();
        json(command(60, "mid", 15, "cmd-x", null));
        int revision = commandsStore.read().orElseThrow().get("command_revision").asInt();

        JsonNode reportBody = json(report(5, 0, 1, 1, false, "rp", revision));
        assertEquals(revision, reportBody.path("data").path("command_revision").asInt(),
                "负例④：report 不得抬升 command_revision");

        JsonNode rejected = json(command(10, "low", 5, "cmd-y", revision - 1));
        assertEquals(ErrorCode.COMMAND_STALE_REVISION, rejected.path("code").asInt());
        assertTrue(rejected.path("data").isObject(), "负例⑤：20006 必须携带完整快照，不得只回 {code,message}");
        assertEquals(contractResponseFields(), fieldNames(rejected.path("data")));
    }

    @Test
    @DisplayName("非法载荷 → 40000")
    void 非法载荷() throws Exception {
        JsonNode badBrightness = json(mockMvc.perform(post(COMMAND_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(commandBody(50, "medium", 15, "x", null))))
                .andExpect(status().isBadRequest()).andReturn());
        assertEquals(ErrorCode.PARAM_INVALID, badBrightness.path("code").asInt());

        JsonNode badTimeout = json(mockMvc.perform(post(COMMAND_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(commandBody(50, "mid", 10, "y", null))))
                .andExpect(status().isBadRequest()).andReturn());
        assertEquals(ErrorCode.PARAM_INVALID, badTimeout.path("code").asInt());
    }

    private void seedProgress() {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", 0);
        progress.put("acked_total", 0);
        progress.put("round_id", 1);
        progress.put("round_cursor", 0);
        progress.put("round_state", "in_progress");
        progress.put("pending_completion", false);
        progress.put("action_id", "seed");
        progressStore.write(progress);
    }

    private MvcResult command(int volume, String brightness, int timeout, String actionId,
            Integer baseRevision) throws Exception {
        return mockMvc.perform(post(COMMAND_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(
                        commandBody(volume, brightness, timeout, actionId, baseRevision))))
                .andExpect(status().isOk()).andReturn();
    }

    private ObjectNode commandBody(int volume, String brightness, int timeout, String actionId,
            Integer baseRevision) {
        ObjectNode body = MAPPER.createObjectNode();
        body.put("volume", volume);
        body.put("brightness", brightness);
        body.put("timeout", timeout);
        body.put("action_id", actionId);
        if (baseRevision != null) {
            body.put("base_revision", baseRevision);
        }
        return body;
    }

    private MvcResult report(int local, int acked, int roundId, int cursor, boolean pending,
            String actionId, int appliedRevision) throws Exception {
        ObjectNode body = MAPPER.createObjectNode();
        body.put("device_id", "spoofed");
        body.put("scripture_version", "HS-1.0.0");
        body.put("local_total", local);
        body.put("acked_total", acked);
        body.put("round_id", roundId);
        body.put("round_state", "in_progress");
        body.put("round_cursor", cursor);
        body.put("pending_completion", pending);
        body.put("applied_revision", appliedRevision);
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

    private static JsonNode contractRegistry() throws IOException {
        Path schema = TestWorkspace.repoRoot().resolve("docs/contracts/sync-contract.schema.json");
        assertTrue(Files.isRegularFile(schema));
        return MAPPER.readTree(Files.readString(schema, StandardCharsets.UTF_8));
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
