package top.zhcmqtt.ewf.backend;

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
import org.springframework.boot.test.web.server.LocalServerPort;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;
import org.springframework.test.web.servlet.MockMvc;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import ch.qos.logback.classic.Logger;
import ch.qos.logback.classic.spi.ILoggingEvent;
import ch.qos.logback.core.read.ListAppender;
import top.zhcmqtt.ewf.backend.common.exception.GlobalExceptionHandler;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.EnvelopeFailureProbeController;

/**
 * Story 5.6：敏感日志窄扫描 —— sync 写路径与 500 兜底不得泄漏会话材料。
 * 2026-10-02 裁决：鉴权链路已移除；session_key/openid/JWT 明文的日志卫生守卫保留作回归防线。
 */
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureMockMvc
@Import(EnvelopeFailureProbeController.class)
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class SensitiveLogGuardTest {

    private static final Path DATA_DIR;

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
    }

    @LocalServerPort
    private int port;

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @BeforeEach
    void reset() throws Exception {
        for (String file : List.of("identity.json", ProgressStore.fileName(), "commands.json",
                "device_state.json", "daily_stats.json")) {
            Files.deleteIfExists(DATA_DIR.resolve(file));
        }
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
        ListAppender<ILoggingEvent> appender = attach(GlobalExceptionHandler.class);

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
                    .contentType(MediaType.APPLICATION_JSON)
                    .content(objectMapper.writeValueAsString(body)))
                    .andExpect(status().isOk());

            mockMvc.perform(get("/__probe/boom")).andExpect(status().isInternalServerError());

            assertLogsClean(appender);
        } finally {
            detach(appender, GlobalExceptionHandler.class);
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

    private static void assertLogsClean(ListAppender<ILoggingEvent> appender) {
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
        assertTrue(true);
    }

    private static void assertFalse(boolean condition, String message) {
        org.junit.jupiter.api.Assertions.assertFalse(condition, message);
    }

    private static void assertTrue(boolean condition) {
        org.junit.jupiter.api.Assertions.assertTrue(condition);
    }
}
