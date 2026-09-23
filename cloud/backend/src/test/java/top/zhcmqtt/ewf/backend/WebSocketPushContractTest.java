package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

import java.io.IOException;
import java.net.URI;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
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
import org.springframework.boot.test.web.server.LocalServerPort;
import org.springframework.http.MediaType;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketHttpHeaders;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.client.standard.StandardWebSocketClient;
import org.springframework.web.socket.handler.TextWebSocketHandler;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.client.WechatMiniClient;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.ws.WsFrameFactory;

/**
 * WebSocket 差量推送契约门禁（Story 5.5）。
 *
 * <p>使用真实端口 + {@link StandardWebSocketClient}；只写临时 {@code app.data-dir}。
 * mock 客户端规则：{@code seq <= last_applied_seq} 丢弃；出现空洞不得直接播放后续帧。
 */
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class WebSocketPushContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-ws-push-");
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

    private String scriptureVersion;

    @BeforeEach
    void 重置临时数据目录并登录() throws Exception {
        for (String file : List.of("identity.json", ProgressStore.fileName(), CommandsStore.fileName(),
                DeviceStateStore.fileName(), "daily_stats.json")) {
            Files.deleteIfExists(DATA_DIR.resolve(file));
        }
        when(wechatMiniClient.exchangeCode("code-ws")).thenReturn("openid-ws");

        JsonNode login = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(Map.of("code", "code-ws"))))
                .andExpect(status().isOk()).andReturn().getResponse().getContentAsString());
        deviceId = login.path("data").path("deviceId").asText();
        accessToken = login.path("data").path("accessToken").asText();
        scriptureVersion = objectMapper.readTree(
                getClass().getResourceAsStream("/canonical/heart-sutra.json"))
                .path("scriptureVersion").asText();
        assertNotNull(deviceId);
        assertFalse(accessToken.isBlank());
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
    @DisplayName("合法 token 升级成功；report 后收到 delta 且 seq==snapshot_seq")
    void 连接鉴权与delta水位一致() throws Exception {
        try (WsProbe probe = connect(accessToken)) {
            JsonNode snapFrame = probe.awaitType("snapshot", Duration.ofSeconds(5));
            assertEquals(WsFrameFactory.CONTRACT_VERSION, snapFrame.path("contract_version").asText());
            assertEquals(0, snapFrame.path("seq").asInt());

            JsonNode report = report(10, 0, 1, 10, false, "act-ws-1");
            assertEquals(0, report.path("code").asInt());
            int snapshotSeq = report.path("data").path("snapshot_seq").asInt();
            assertEquals(10, snapshotSeq);

            JsonNode delta = probe.awaitType("delta", Duration.ofSeconds(5));
            assertEquals("delta", delta.path("type").asText());
            assertEquals(snapshotSeq, delta.path("seq").asInt());
            assertEquals(10, delta.path("acked_total").asInt());
            assertEquals(10, delta.path("local_total").asInt());
            assertEquals(1, delta.path("round_id").asInt());
            assertEquals(10, delta.path("round_cursor").asInt());
            assertFalse(delta.has("glyph") || delta.has("char"));

            JsonNode restSnap = snapshot();
            assertEquals(delta.path("seq").asInt(), restSnap.path("data").path("snapshot_seq").asInt());
            assertEquals(delta.path("acked_total").asInt(), restSnap.path("data").path("acked_total").asInt());
        }
    }

    @Test
    @DisplayName("设置下发成功 → command_state 含修订对与载荷")
    void 设置下发推commandState() throws Exception {
        try (WsProbe probe = connect(accessToken)) {
            probe.awaitType("snapshot", Duration.ofSeconds(5));
            JsonNode cmd = command(60, "high", 30, "cmd-ws-1", null);
            assertEquals(0, cmd.path("code").asInt());

            JsonNode frame = probe.awaitType("command_state", Duration.ofSeconds(5));
            assertEquals(1, frame.path("command_revision").asInt());
            assertEquals(0, frame.path("applied_revision").asInt());
            assertEquals(60, frame.path("volume").asInt());
            assertEquals("high", frame.path("brightness").asText());
            assertEquals(30, frame.path("timeout").asInt());
        }
    }

    @Test
    @DisplayName("完成确认 → completion 与快照一致")
    void 完成确认推completion() throws Exception {
        int end = objectMapper.readTree(getClass().getResourceAsStream("/canonical/heart-sutra.json"))
                .path("counts").path("consumableHan").asInt();
        try (WsProbe probe = connect(accessToken)) {
            probe.awaitType("snapshot", Duration.ofSeconds(5));
            JsonNode report = report(end, 0, 1, end, true, "act-complete");
            assertEquals(0, report.path("code").asInt());
            assertEquals("completed", report.path("data").path("round_state").asText());

            JsonNode delta = probe.awaitType("delta", Duration.ofSeconds(5));
            assertEquals(end, delta.path("acked_total").asInt());
            JsonNode completion = probe.awaitType("completion", Duration.ofSeconds(5));
            assertEquals(1, completion.path("round_id").asInt());
            assertEquals("completed", completion.path("round_state").asText());
            assertFalse(completion.path("pending_completion").asBoolean());
        }
    }

    @Test
    @DisplayName("双连接：新连接替换旧连接，仅新连接收后续推送")
    void 双连接仅新连接收推送() throws Exception {
        try (WsProbe oldProbe = connect(accessToken)) {
            oldProbe.awaitType("snapshot", Duration.ofSeconds(5));
            try (WsProbe newProbe = connect(accessToken)) {
                newProbe.awaitType("snapshot", Duration.ofSeconds(5));
                Thread.sleep(200);
                assertFalse(oldProbe.sessionOpen(), "旧连接应被关闭");
                assertTrue(newProbe.sessionOpen(), "新连接应保持打开");

                int oldBusiness = oldProbe.businessFrameCount();
                JsonNode report = report(12, 0, 1, 6, false, "act-dual");
                assertEquals(0, report.path("code").asInt());
                JsonNode delta = newProbe.awaitType("delta", Duration.ofSeconds(5));
                assertEquals(12, delta.path("acked_total").asInt());
                Thread.sleep(300);
                assertEquals(oldBusiness, oldProbe.businessFrameCount(), "旧连接不得再收业务帧");
            }
        }
    }

    @Test
    @DisplayName("负例①：无 token / 坏 token 不能升级")
    void 无token与坏token拒绝握手() {
        assertThrows(Exception.class, () -> {
            try (WsProbe ignored = connect(null)) {
                fail("应拒绝无 token 握手");
            }
        });
        assertThrows(Exception.class, () -> {
            try (WsProbe ignored = connect("not-a-jwt")) {
                fail("应拒绝坏 token 握手");
            }
        });
    }

    @Test
    @DisplayName("负例②：delta.seq 不得与同刻 snapshot_seq 矛盾")
    void deltaSeq与快照水位一致() throws Exception {
        try (WsProbe probe = connect(accessToken)) {
            probe.awaitType("snapshot", Duration.ofSeconds(5));
            report(5, 0, 1, 5, false, "act-seq-a");
            JsonNode delta = probe.awaitType("delta", Duration.ofSeconds(5));
            JsonNode snap = snapshot();
            assertEquals(snap.path("data").path("snapshot_seq").asInt(), delta.path("seq").asInt(),
                    "delta.seq 必须等于同刻 snapshot_seq");
            assertFalse(delta.path("seq").asInt() < snap.path("data").path("acked_total").asInt());
            assertFalse(delta.path("seq").asInt() > snap.path("data").path("acked_total").asInt()
                    && delta.path("acked_total").asInt() == snap.path("data").path("acked_total").asInt());
        }
    }

    @Test
    @DisplayName("负例③：拒绝路径 20003 不推送")
    void 拒绝路径不推送() throws Exception {
        report(50, 0, 1, 10, false, "act-seed");
        try (WsProbe probe = connect(accessToken)) {
            probe.awaitType("snapshot", Duration.ofSeconds(5));
            int before = probe.businessFrameCount();
            JsonNode rejected = report(40, 40, 1, 10, false, "act-reset");
            assertEquals(ErrorCode.DEVICE_RESET_CONFLICT, rejected.path("code").asInt());
            Thread.sleep(400);
            assertEquals(before, probe.businessFrameCount(), "20003 不得推送业务帧");
        }
    }

    @Test
    @DisplayName("负例④：重连后不重放 ≤ 冻结水位的历史 delta")
    void 重连不重放历史帧() throws Exception {
        try (WsProbe first = connect(accessToken)) {
            first.awaitType("snapshot", Duration.ofSeconds(5));
            report(20, 0, 1, 8, false, "act-pre");
            first.awaitType("delta", Duration.ofSeconds(5));
        }
        JsonNode frozen = snapshot();
        int frozenSeq = frozen.path("data").path("snapshot_seq").asInt();
        assertEquals(20, frozenSeq);

        try (WsProbe second = connect(accessToken)) {
            JsonNode snapFrame = second.awaitType("snapshot", Duration.ofSeconds(5));
            assertEquals(frozenSeq, snapFrame.path("seq").asInt());
            Thread.sleep(300);
            assertTrue(second.framesOfType("delta").isEmpty(), "重连不得重放历史 delta");

            // mock 客户端：seq <= last_applied 丢弃
            AtomicInteger lastApplied = new AtomicInteger(frozenSeq);
            report(25, 20, 1, 10, false, "act-post");
            JsonNode delta = second.awaitType("delta", Duration.ofSeconds(5));
            assertTrue(delta.path("seq").asInt() > lastApplied.get());
            lastApplied.set(delta.path("seq").asInt());
        }
    }

    @Test
    @DisplayName("负例⑤：仅 Bearer 头、忽略 query token → 握手失败")
    void 仅Bearer不能握手() {
        assertThrows(Exception.class, () -> {
            StandardWebSocketClient client = new StandardWebSocketClient();
            WebSocketHttpHeaders headers = new WebSocketHttpHeaders();
            headers.setBearerAuth(accessToken);
            CompletableFuture<WebSocketSession> future = client.execute(new TextWebSocketHandler() {
            }, headers, URI.create("ws://127.0.0.1:" + port + "/api/v1/ws"));
            future.get(3, TimeUnit.SECONDS);
            fail("仅 Bearer 不得升级成功");
        });
    }

    @Test
    @DisplayName("同水位二次 report 不产生前进 seq 的新 delta")
    void 同水位不重复推delta() throws Exception {
        try (WsProbe probe = connect(accessToken)) {
            probe.awaitType("snapshot", Duration.ofSeconds(5));
            report(15, 0, 1, 5, false, "act-once");
            JsonNode first = probe.awaitType("delta", Duration.ofSeconds(5));
            int seq = first.path("seq").asInt();
            int countBefore = probe.framesOfType("delta").size();
            report(15, 15, 1, 5, false, "act-noop");
            Thread.sleep(400);
            assertEquals(countBefore, probe.framesOfType("delta").size());
            assertEquals(seq, probe.framesOfType("delta").get(countBefore - 1).path("seq").asInt());
        }
    }

    private WsProbe connect(String token) throws Exception {
        String query = token == null ? "" : "?token=" + token;
        URI uri = URI.create("ws://127.0.0.1:" + port + "/api/v1/ws" + query);
        WsProbe probe = new WsProbe();
        StandardWebSocketClient client = new StandardWebSocketClient();
        WebSocketHttpHeaders headers = new WebSocketHttpHeaders();
        headers.add("Origin", "http://127.0.0.1:" + port);
        WebSocketSession session = client.execute(probe, headers, uri).get(5, TimeUnit.SECONDS);
        probe.attach(session);
        return probe;
    }

    private JsonNode report(int local, int acked, int roundId, int cursor, boolean pending, String actionId)
            throws Exception {
        var body = objectMapper.createObjectNode();
        body.put("device_id", deviceId);
        body.put("scripture_version", scriptureVersion);
        body.put("local_total", local);
        body.put("acked_total", acked);
        body.put("round_id", roundId);
        body.put("round_state", "in_progress");
        body.put("round_cursor", cursor);
        body.put("pending_completion", pending);
        body.put("applied_revision", 0);
        body.put("battery_percent", 80);
        body.put("network_mode", "connected");
        body.put("audio_config_version", 1);
        body.put("firmware_version", "1.0.0");
        body.put("action_id", actionId);
        String raw = mockMvc.perform(post("/api/v1/sync/report")
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(body)))
                .andExpect(status().isOk())
                .andReturn().getResponse().getContentAsString();
        return json(raw);
    }

    private JsonNode command(int volume, String brightness, int timeout, String actionId, Integer base)
            throws Exception {
        var node = objectMapper.createObjectNode();
        node.put("volume", volume);
        node.put("brightness", brightness);
        node.put("timeout", timeout);
        node.put("action_id", actionId);
        if (base != null) {
            node.put("base_revision", base);
        }
        String raw = mockMvc.perform(post("/api/v1/sync/command")
                .header("Authorization", "Bearer " + accessToken)
                .contentType(MediaType.APPLICATION_JSON)
                .content(objectMapper.writeValueAsString(node)))
                .andExpect(status().isOk())
                .andReturn().getResponse().getContentAsString();
        return json(raw);
    }

    private JsonNode snapshot() throws Exception {
        String raw = mockMvc.perform(get("/api/v1/sync/snapshot")
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isOk())
                .andReturn().getResponse().getContentAsString();
        return json(raw);
    }

    private static JsonNode json(String raw) throws Exception {
        return MAPPER.readTree(raw);
    }

    /** mock 客户端：收集帧；seq<=last_applied 可丢弃（由用例断言）。 */
    private static final class WsProbe extends TextWebSocketHandler implements AutoCloseable {
        private final CopyOnWriteArrayList<JsonNode> frames = new CopyOnWriteArrayList<>();
        private final Object lock = new Object();
        private final AtomicInteger readIndex = new AtomicInteger(0);
        private WebSocketSession session;

        void attach(WebSocketSession session) {
            this.session = session;
        }

        @Override
        protected void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
            JsonNode node = MAPPER.readTree(message.getPayload());
            frames.add(node);
            synchronized (lock) {
                lock.notifyAll();
            }
        }

        JsonNode awaitType(String type, Duration timeout) throws InterruptedException {
            long deadline = System.nanoTime() + timeout.toNanos();
            synchronized (lock) {
                while (true) {
                    int from = readIndex.get();
                    for (int i = from; i < frames.size(); i++) {
                        JsonNode frame = frames.get(i);
                        if (type.equals(frame.path("type").asText())) {
                            readIndex.set(i + 1);
                            return frame;
                        }
                    }
                    long remaining = deadline - System.nanoTime();
                    if (remaining <= 0) {
                        fail("等待帧类型 " + type + " 超时；已收=" + frames);
                    }
                    lock.wait(Math.max(1, TimeUnit.NANOSECONDS.toMillis(remaining)));
                }
            }
        }

        List<JsonNode> framesOfType(String type) {
            List<JsonNode> out = new ArrayList<>();
            for (JsonNode frame : frames) {
                if (type.equals(frame.path("type").asText())) {
                    out.add(frame);
                }
            }
            return out;
        }

        int businessFrameCount() {
            int n = 0;
            for (JsonNode frame : frames) {
                String t = frame.path("type").asText();
                if ("delta".equals(t) || "completion".equals(t) || "command_state".equals(t)) {
                    n++;
                }
            }
            return n;
        }

        boolean sessionOpen() {
            return session != null && session.isOpen();
        }

        @Override
        public void close() throws Exception {
            if (session != null && session.isOpen()) {
                session.close();
            }
        }
    }
}
