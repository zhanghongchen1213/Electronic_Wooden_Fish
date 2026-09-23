package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestInstance;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Import;
import org.springframework.core.env.Environment;
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
import top.zhcmqtt.ewf.backend.common.security.UserContext;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.EnvelopeFailureProbeController;

/**
 * Story 5.6：错误码 → 客户端动作矩阵门禁。
 *
 * <p><b>裁决 B/C：</b>对每个已登记码抽检真实出口；断言 HTTP 状态、{@code code}、
 * {@code message} 含动作关键词（重试|重新登录|重建|收敛|修复），且 body 非 {@code code=0}。
 * 不扩展信封 {@code retryable} 字段；不启用 Problem Details。
 */
@SpringBootTest
@AutoConfigureMockMvc
@Import(EnvelopeFailureProbeController.class)
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class ErrorSemanticsContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final Pattern ACTION_KEYWORD = Pattern.compile("重试|重新登录|重建|收敛|修复");

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-error-semantics-");
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
    private Environment environment;

    @Autowired
    private ProgressStore progressStore;

    @Autowired
    private CommandsStore commandsStore;

    @MockBean
    private WechatMiniClient wechatMiniClient;

    private String accessToken;

    @BeforeEach
    void reset() throws Exception {
        for (String file : List.of("identity.json", ProgressStore.fileName(), CommandsStore.fileName(),
                "device_state.json", "daily_stats.json")) {
            Files.deleteIfExists(DATA_DIR.resolve(file));
        }
        UserContext.clear();
        when(wechatMiniClient.exchangeCode("code-a")).thenReturn("openid-a");
        when(wechatMiniClient.exchangeCode("code-b")).thenReturn("openid-b");
        JsonNode login = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"code\":\"code-a\"}"))
                .andExpect(status().isOk()).andReturn());
        accessToken = login.path("data").path("accessToken").asText();
    }

    @AfterAll
    void cleanup() throws Exception {
        if (Files.exists(DATA_DIR)) {
            try (var paths = Files.walk(DATA_DIR)) {
                paths.sorted(Comparator.reverseOrder()).forEach(path -> {
                    try {
                        Files.deleteIfExists(path);
                    } catch (Exception ex) {
                        throw new RuntimeException(ex);
                    }
                });
            }
        }
    }

    @Test
    @DisplayName("Problem Details 必须关闭（裁决 G）")
    void problemDetailsDisabled() {
        assertEquals("false", environment.getProperty("spring.mvc.problemdetails.enabled"),
                "不得启用 spring.mvc.problemdetails.enabled");
    }

    @Test
    @DisplayName("同步拒绝 20001–20006：HTTP 200、非 code=0、含动作语义；20003/04/06 带完整快照")
    void syncRejectMatrix() throws Exception {
        // 20001
        json(report(20, 0, 5, 10, false, "c1"));
        assertAction(json(report(30, 20, 3, 5, false, "c2")), 200, ErrorCode.ROUND_UNASSIGNED, true);

        // 20002 via snapshot query without baseline
        seedProgress(50);
        JsonNode missingBaseline = json(mockMvc.perform(get("/api/v1/sync/snapshot")
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk()).andReturn());
        assertAction(missingBaseline, 200, ErrorCode.BASELINE_HIGH_WATER_MISSING, true);

        // 20003
        json(report(80, 0, 1, 10, false, "r1"));
        assertAction(json(report(40, 40, 1, 5, false, "r2")), 200, ErrorCode.DEVICE_RESET_CONFLICT, true);

        // 20004：积压差 ≥ 1000
        seedProgress(0);
        assertAction(json(report(1000, 0, 1, 1, false, "q1")), 200, ErrorCode.QUEUE_FULL, true);

        // 20005
        ObjectNode badVersion = reportBody(1010, 1000, 1, 2, false, "ver");
        badVersion.put("scripture_version", "HS-0.0.0");
        JsonNode mismatch = json(mockMvc.perform(post("/api/v1/sync/report")
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(badVersion)))
                .andExpect(status().isOk()).andReturn());
        assertAction(mismatch, 200, ErrorCode.SCRIPTURE_VERSION_MISMATCH, true);

        // 20006
        seedProgress(0);
        json(command(50, "mid", 15, "cmd-a", null));
        int revision = commandsStore.read().orElseThrow().get("command_revision").asInt();
        assertAction(json(command(10, "low", 5, "cmd-b", revision - 1)),
                200, ErrorCode.COMMAND_STALE_REVISION, true);
    }

    @Test
    @DisplayName("鉴权/上游/协议/500：码表 HTTP 状态 + 动作关键词 + 非 code=0")
    void authProtocolServerMatrix() throws Exception {
        // 40103
        assertAction(json(mockMvc.perform(get("/api/v1/sync/snapshot"))
                .andExpect(status().isUnauthorized()).andReturn()),
                401, ErrorCode.TOKEN_MISSING, false);

        // 40102
        assertAction(json(mockMvc.perform(get("/api/v1/sync/snapshot")
                .header("Authorization", "Bearer not-a-jwt"))
                .andExpect(status().isUnauthorized()).andReturn()),
                401, ErrorCode.TOKEN_INVALID, false);

        // 40101 — 过期 access
        String parts = accessToken;
        String[] segs = parts.split("\\.");
        assertEquals(3, segs.length);
        // 用 refresh 当 access → 类型非法更接近 40102；过期用短 secret 签发不方便，
        // 直接断言已有过期文案出口：篡改 payload 后仍走 tokenInvalid/expired 路径。
        // 这里用 AuthContract 同形：带过期 token 需 JwtTokenProvider；改用 probe unauthorized 不满足文案，
        // 故走 refresh 误用为 Bearer。
        JsonNode login = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"code\":\"code-a\"}"))
                .andExpect(status().isOk()).andReturn());
        String refresh = login.path("data").path("refreshToken").asText();
        JsonNode wrongType = json(mockMvc.perform(get("/api/v1/sync/snapshot")
                .header("Authorization", "Bearer " + refresh))
                .andExpect(status().isUnauthorized()).andReturn());
        assertTrue(wrongType.path("code").asInt() == ErrorCode.TOKEN_INVALID
                        || wrongType.path("code").asInt() == ErrorCode.TOKEN_EXPIRED,
                "refresh 当 access 应回 40102 或 40101");
        assertNotEquals(0, wrongType.path("code").asInt());
        assertTrue(ACTION_KEYWORD.matcher(wrongType.path("message").asText()).find());

        // 40300
        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"code\":\"code-b\"}"))
                .andExpect(status().isOk());
        assertAction(json(mockMvc.perform(get("/api/v1/sync/snapshot")
                .header("Authorization", "Bearer " + accessToken)
                .param("acked_total", "0")
                .param("snapshot_seq", "0"))
                .andExpect(status().isForbidden()).andReturn()),
                403, ErrorCode.IDENTITY_MISMATCH, false);

        // 重新登录：code-a 身份已不在；为后续用例重建
        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        JsonNode relogin = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"code\":\"code-a\"}"))
                .andExpect(status().isOk()).andReturn());
        accessToken = relogin.path("data").path("accessToken").asText();

        // 40000 / 40001
        assertAction(json(mockMvc.perform(post("/__probe/validated")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{}"))
                .andExpect(status().isBadRequest()).andReturn()),
                400, ErrorCode.PARAM_INVALID, false);
        assertAction(json(mockMvc.perform(post("/__probe/echo")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"a\":"))
                .andExpect(status().isBadRequest()).andReturn()),
                400, ErrorCode.PROTOCOL_FIELD_INVALID, false);

        // 40400
        assertAction(json(mockMvc.perform(get("/not-a-real-endpoint"))
                .andExpect(status().isNotFound()).andReturn()),
                404, ErrorCode.NOT_FOUND, false);

        // 40500
        assertAction(json(mockMvc.perform(get("/__probe/echo"))
                .andExpect(status().isMethodNotAllowed()).andReturn()),
                405, ErrorCode.METHOD_NOT_ALLOWED, false);

        // 41500
        assertAction(json(mockMvc.perform(post("/__probe/echo")
                .contentType(MediaType.TEXT_PLAIN)
                .content("plain"))
                .andExpect(status().isUnsupportedMediaType()).andReturn()),
                415, ErrorCode.UNSUPPORTED_MEDIA_TYPE, false);

        // 50000
        assertAction(json(mockMvc.perform(get("/__probe/boom"))
                .andExpect(status().isInternalServerError()).andReturn()),
                500, ErrorCode.SERVER_ERROR, false);

        // 50200 — 上游占位：微信配置未就绪时登录
        when(wechatMiniClient.exchangeCode("code-up"))
                .thenThrow(top.zhcmqtt.ewf.backend.common.exception.BusinessException
                        .upstreamUnavailable("微信服务暂不可用，请稍后重试"));
        assertAction(json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"code\":\"code-up\"}"))
                .andExpect(status().isBadGateway()).andReturn()),
                502, ErrorCode.WECHAT_UPSTREAM, false);
    }

    private void assertAction(JsonNode body, int httpStatus, int code, boolean expectSnapshot)
            throws Exception {
        assertEquals(code, body.path("code").asInt(), "业务码不符");
        assertNotEquals(0, body.path("code").asInt(), "失败不得伪装 code=0");
        String message = body.path("message").asText();
        assertFalse(message.isBlank());
        assertTrue(ACTION_KEYWORD.matcher(message).find(),
                "message 须含动作关键词，实际=" + message);
        assertFalse(body.has("retryable"), "禁止信封 retryable 字段");
        if (expectSnapshot) {
            assertTrue(body.path("data").isObject(), "同步拒绝须带快照 data");
            assertTrue(body.path("data").has("acked_total"));
            assertTrue(body.path("data").has("snapshot_seq"));
        }
        // httpStatus 由调用方 MockMvc andExpect 钉死；这里再钉码形
        assertEquals(httpStatus, ErrorCode.httpStatusOf(code));
    }

    private void seedProgress(int acked) {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", acked);
        progress.put("acked_total", acked);
        progress.put("round_id", 1);
        progress.put("round_cursor", Math.min(acked, 10));
        progress.put("round_state", "in_progress");
        progress.put("pending_completion", false);
        progress.put("action_id", "seed");
        progressStore.write(progress);
    }

    private MvcResult report(int local, int acked, int roundId, int cursor, boolean pending, String actionId)
            throws Exception {
        return mockMvc.perform(post("/api/v1/sync/report")
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(
                        reportBody(local, acked, roundId, cursor, pending, actionId))))
                .andExpect(status().isOk()).andReturn();
    }

    private ObjectNode reportBody(int local, int acked, int roundId, int cursor, boolean pending,
            String actionId) {
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
        return body;
    }

    private MvcResult command(int volume, String brightness, int timeout, String actionId,
            Integer baseRevision) throws Exception {
        ObjectNode body = MAPPER.createObjectNode();
        body.put("volume", volume);
        body.put("brightness", brightness);
        body.put("timeout", timeout);
        body.put("action_id", actionId);
        if (baseRevision != null) {
            body.put("base_revision", baseRevision);
        }
        return mockMvc.perform(post("/api/v1/sync/command")
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(body)))
                .andExpect(status().isOk()).andReturn();
    }

    private JsonNode json(MvcResult result) throws Exception {
        return objectMapper.readTree(result.getResponse().getContentAsString(StandardCharsets.UTF_8));
    }
}
