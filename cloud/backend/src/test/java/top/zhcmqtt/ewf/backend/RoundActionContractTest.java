package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
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
import top.zhcmqtt.ewf.backend.dto.sync.RoundActionRequest;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * {@code POST /api/v1/sync/round-action} 契约门禁：鉴权、字段闭包、完成→restart→exit、拒绝带基准。
 *
 * <p>只写临时 {@code app.data-dir}，不触碰仓库 {@code cloud/backend/data/}。
 */
@SpringBootTest
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class RoundActionContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final String REPORT_PATH = "/api/v1/sync/report";

    private static final String ROUND_ACTION_PATH = "/api/v1/sync/round-action";

    private static final String RESPONSE_SAMPLE_ID = "https_sync_response";

    private static final String SNAPSHOT_FRAME_TYPE = "snapshot";

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-round-action-");
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
    @DisplayName("round-action 默认受 Bearer 保护：无令牌 40103，身份不匹配 40300")
    void 鉴权保护() throws Exception {
        JsonNode missing = json(mockMvc.perform(post(ROUND_ACTION_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(Map.of("action", "restart", "action_id", "x"))))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(ErrorCode.TOKEN_MISSING, missing.path("code").asInt());

        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(Map.of("code", "code-b"))))
                .andExpect(status().isOk());

        JsonNode foreign = json(mockMvc.perform(post(ROUND_ACTION_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(Map.of("action", "restart", "action_id", "y"))))
                .andExpect(status().isForbidden()).andReturn());
        assertEquals(ErrorCode.IDENTITY_MISMATCH, foreign.path("code").asInt());
    }

    @Test
    @DisplayName("请求体字段闭包恰为 action + action_id")
    void 请求字段闭包() throws Exception {
        RoundActionRequest dto = new RoundActionRequest("restart", "a1");
        assertEquals(Set.of("action", "action_id"),
                fieldNames(objectMapper.readTree(objectMapper.writeValueAsString(dto))));
    }

    @Test
    @DisplayName("完成 → restart → exit happy path；同 action_id 重放；未完成 restart → 20001")
    void 完成到跨轮happyPath() throws Exception {
        json(report(259, 0, 1, 259, false, "r1"));
        JsonNode done = json(report(260, 259, 1, 260, true, "r-done"));
        assertEquals(0, done.path("code").asInt());
        assertEquals("completed", done.path("data").path("round_state").asText());
        assertFalse(done.path("data").path("pending_completion").asBoolean());

        JsonNode restart = json(roundAction("restart", "restart-1"));
        assertEquals(0, restart.path("code").asInt());
        assertEquals(2, restart.path("data").path("round_id").asInt());
        assertEquals(0, restart.path("data").path("round_cursor").asInt());
        assertEquals("in_progress", restart.path("data").path("round_state").asText());
        assertEquals(260, restart.path("data").path("acked_total").asInt());

        JsonNode replay = json(roundAction("restart", "restart-1"));
        assertEquals(2, replay.path("data").path("round_id").asInt());

        JsonNode tooEarly = json(roundAction("restart", "restart-2"));
        assertEquals(ErrorCode.ROUND_UNASSIGNED, tooEarly.path("code").asInt());
        assertEquals(Set.of("code", "message", "data"), fieldNames(tooEarly));
        assertEquals(contractResponseFields(), fieldNames(tooEarly.path("data")));

        // 再次完成后再 exit
        json(report(270, 260, 2, 260, true, "r-done2"));
        JsonNode exit = json(roundAction("exit", "exit-1"));
        assertEquals(0, exit.path("code").asInt());
        assertEquals(2, exit.path("data").path("round_id").asInt());
        assertEquals("completed", exit.path("data").path("round_state").asText());

        JsonNode exitReplay = json(roundAction("exit", "exit-1"));
        assertEquals(2, exitReplay.path("data").path("round_id").asInt());

        // 篇章动作不得改写 commands.json 的设置命令族 action_id
        assertEquals("cmd-seed", commandsStore.read().map(n -> n.get("action_id").asText()).orElse(""));
    }

    @Test
    @DisplayName("非法 action → 40000")
    void 非法action拒绝() throws Exception {
        JsonNode body = json(mockMvc.perform(post(ROUND_ACTION_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"action\":\"sync_now\",\"action_id\":\"z\"}"))
                .andExpect(status().isBadRequest()).andReturn());
        assertEquals(ErrorCode.PARAM_INVALID, body.path("code").asInt());
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
        // 首次上报时预置 commands，便于断言篇章动作不覆盖其 action_id
        if (commandsStore.read().isEmpty()) {
            ObjectNode commands = MAPPER.createObjectNode();
            commands.put("command_revision", 0);
            commands.put("applied_revision", 0);
            commands.put("action_id", "cmd-seed");
            commands.put("volume", 50);
            commands.put("brightness", "mid");
            commands.put("timeout", 15);
            commandsStore.write(commands);
        }
        return mockMvc.perform(post(REPORT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(body)))
                .andExpect(status().isOk()).andReturn();
    }

    private MvcResult roundAction(String action, String actionId) throws Exception {
        return mockMvc.perform(post(ROUND_ACTION_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(Map.of("action", action, "action_id", actionId))))
                .andExpect(status().isOk()).andReturn();
    }

    private static Set<String> contractResponseFields() throws IOException {
        JsonNode registry = MAPPER.readTree(Files.readString(
                TestWorkspace.repoRoot().resolve("docs/contracts/sync-contract.schema.json"),
                StandardCharsets.UTF_8));
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

    private JsonNode json(MvcResult result) throws Exception {
        JsonNode node = objectMapper.readTree(result.getResponse().getContentAsString(StandardCharsets.UTF_8));
        assertNotNull(node);
        return node;
    }

    private static Set<String> fieldNames(JsonNode node) {
        Set<String> names = new TreeSet<>();
        node.fieldNames().forEachRemaining(names::add);
        return names;
    }
}
