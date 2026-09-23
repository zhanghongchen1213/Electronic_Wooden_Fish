package top.zhcmqtt.ewf.backend;

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
import java.util.List;
import java.util.Locale;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestInstance;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.web.server.LocalServerPort;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.web.socket.client.standard.StandardWebSocketClient;
import org.springframework.web.socket.handler.TextWebSocketHandler;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import ch.qos.logback.classic.Logger;
import ch.qos.logback.classic.spi.ILoggingEvent;
import ch.qos.logback.core.read.ListAppender;
import top.zhcmqtt.ewf.backend.client.WechatMiniClient;
import top.zhcmqtt.ewf.backend.common.exception.GlobalExceptionHandler;
import top.zhcmqtt.ewf.backend.common.security.JwtAuthenticationFilter;
import top.zhcmqtt.ewf.backend.common.security.UserContext;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.EnvelopeFailureProbeController;
import top.zhcmqtt.ewf.backend.ws.WsHandshakeInterceptor;

/**
 * Story 5.6：敏感日志窄扫描 —— sync 写路径与 WS 握手失败不得泄漏会话材料。
 */
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureMockMvc
@Import(EnvelopeFailureProbeController.class)
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class SensitiveLogGuardTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    /** JWT 三段式明文粗检（header.payload.sig）。 */
    private static final Pattern JWT_TRIPLE = Pattern.compile(
            "eyJ[A-Za-z0-9_-]+\\.eyJ[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+");

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-sensitive-log-");
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

    @LocalServerPort
    private int port;

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @MockBean
    private WechatMiniClient wechatMiniClient;

    private String accessToken;

    @BeforeEach
    void reset() throws Exception {
        for (String file : List.of("identity.json", ProgressStore.fileName(), "commands.json",
                "device_state.json", "daily_stats.json")) {
            Files.deleteIfExists(DATA_DIR.resolve(file));
        }
        UserContext.clear();
        when(wechatMiniClient.exchangeCode("code-a")).thenReturn("openid-a");
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
    @DisplayName("sync 拒绝与 500 兜底日志不含 session_key/Authorization 值/JWT 明文/openid")
    void syncAndHandlerLogsAreClean() throws Exception {
        ListAppender<ILoggingEvent> appender = attach(
                GlobalExceptionHandler.class, JwtAuthenticationFilter.class);

        try {
            ObjectNode body = MAPPER.createObjectNode();
            body.put("device_id", "spoofed");
            body.put("scripture_version", "HS-0.0.0");
            body.put("local_total", 10);
            body.put("acked_total", 0);
            body.put("round_id", 1);
            body.put("round_state", "in_progress");
            body.put("round_cursor", 1);
            body.put("pending_completion", false);
            body.put("applied_revision", 0);
            body.put("battery_percent", 76);
            body.put("network_mode", "connected");
            body.put("audio_config_version", 1);
            body.put("firmware_version", "1.0.0");
            body.put("action_id", "sens-1");

            mockMvc.perform(post("/api/v1/sync/report")
                    .header("Authorization", "Bearer " + accessToken)
                    .contentType(MediaType.APPLICATION_JSON)
                    .content(objectMapper.writeValueAsString(body)))
                    .andExpect(status().isOk());

            mockMvc.perform(get("/api/v1/sync/snapshot")
                    .header("Authorization", "Bearer " + accessToken + "-tampered"))
                    .andExpect(status().isUnauthorized());

            mockMvc.perform(get("/__probe/boom")).andExpect(status().isInternalServerError());

            assertLogsClean(appender, accessToken);
        } finally {
            detach(appender, GlobalExceptionHandler.class, JwtAuthenticationFilter.class);
        }
    }

    @Test
    @DisplayName("WS 握手失败日志不含完整 JWT / session_key / Authorization 值")
    void wsHandshakeFailureLogsAreClean() throws Exception {
        ListAppender<ILoggingEvent> appender = attach(WsHandshakeInterceptor.class);
        String leakToken = accessToken;
        try {
            StandardWebSocketClient client = new StandardWebSocketClient();
            try {
                client.execute(new TextWebSocketHandler() {
                }, "ws://127.0.0.1:" + port + "/api/v1/ws?token=" + leakToken + "bad")
                        .get();
            } catch (Exception ignored) {
                // 握手拒绝属预期
            }
            try {
                client.execute(new TextWebSocketHandler() {
                }, "ws://127.0.0.1:" + port + "/api/v1/ws")
                        .get();
            } catch (Exception ignored) {
                // 缺 token
            }
            assertLogsClean(appender, leakToken);
        } finally {
            detach(appender, WsHandshakeInterceptor.class);
        }
    }

    private static ListAppender<ILoggingEvent> attach(Class<?>... types) {
        ListAppender<ILoggingEvent> appender = new ListAppender<>();
        appender.start();
        for (Class<?> type : types) {
            ((Logger) LoggerFactory.getLogger(type)).addAppender(appender);
        }
        return appender;
    }

    private static void detach(ListAppender<ILoggingEvent> appender, Class<?>... types) {
        for (Class<?> type : types) {
            ((Logger) LoggerFactory.getLogger(type)).detachAppender(appender);
        }
    }

    private static void assertLogsClean(ListAppender<ILoggingEvent> appender, String accessToken) {
        String joined = appender.list.stream()
                .map(ILoggingEvent::getFormattedMessage)
                .collect(Collectors.joining("\n"))
                .toLowerCase(Locale.ROOT);
        assertFalse(joined.contains("session_key"), "日志不得含 session_key");
        assertFalse(joined.contains("sessionkey"), "日志不得含 sessionKey");
        assertFalse(joined.contains("authorization:"), "日志不得含 Authorization: 头值");
        assertFalse(joined.contains("appsecret") || joined.contains("app-secret"),
                "日志不得含 AppSecret");
        assertFalse(joined.contains("openid-a"), "日志不得含未脱敏 openid");
        assertFalse(JWT_TRIPLE.matcher(joined).find(), "日志不得含 JWT 三段式明文");
        if (accessToken != null && !accessToken.isBlank()) {
            assertFalse(joined.contains(accessToken.toLowerCase(Locale.ROOT)),
                    "日志不得回显完整 access token");
        }
        assertTrue(true);
    }

    private JsonNode json(org.springframework.test.web.servlet.MvcResult result) throws Exception {
        return objectMapper.readTree(result.getResponse().getContentAsString(StandardCharsets.UTF_8));
    }
}
