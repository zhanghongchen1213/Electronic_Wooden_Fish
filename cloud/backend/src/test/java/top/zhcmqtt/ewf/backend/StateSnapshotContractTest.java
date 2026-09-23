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
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * 权威状态查询端点的契约门禁：响应字段闭包、鉴权保护、恢复基准选码、拒绝仍带基准、查询只读。
 *
 * <p><b>字段闭包断言不手抄第三份字段表（Task 5.3）：</b>期望集合由
 * {@code docs/contracts/sync-contract.schema.json} 的 {@code ws_frames[snapshot].payload_fields}
 * ∪ {@code samples[https_sync_response].required} **机械提取**（经 {@link TestWorkspace#repoRoot()}
 * 定位，fail-closed），再与端点实际返回的 {@code data} 键集合和响应 DTO 的 Jackson 序列化键集合
 * 逐字比对。注册表本身是契约正文的派生品（`docs/contracts/README.md`），不是第二真源；
 * §6.2 必填清单与注册表的一致性由 `docs/contracts/tests/run_sync_contract_tests.py` 另行锁定。
 *
 * <p><b>测试落点：</b>只写 {@link DynamicPropertySource} 注入的临时目录，**不创建、读取或修改**
 * 仓库工作区的 {@code cloud/backend/data/}。
 *
 * <p><b>静态校验无法覆盖的边界（不得写成已验证）：</b>
 * <ul>
 *   <li>只证明**单进程内**的查询响应、字段闭包与只读性；不证明多进程/跨 JVM 并发下的水位一致。</li>
 *   <li>不证明 WebSocket 侧 {@code seq} 空间与 {@code snapshot_seq} 一致（属 Epic 5）。</li>
 *   <li>不证明生产数据目录的权限、挂载与磁盘写缓存配置。</li>
 *   <li>字段闭包只证明本端点的响应形状与 Java DTO 一致，不能证明运行期不存在其他响应出口
 *       （容器级 {@code /error} 仍然存在，见 {@code deferred-work.md} 4-3 段）。</li>
 * </ul>
 */
@SpringBootTest
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class StateSnapshotContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final String SNAPSHOT_PATH = "/api/v1/sync/snapshot";

    /** 契约注册表中发出快照的帧类型（§10.1）。 */
    private static final String SNAPSHOT_FRAME_TYPE = "snapshot";

    /** 契约注册表中 §6.2 响应样例的 id。 */
    private static final String RESPONSE_SAMPLE_ID = "https_sync_response";

    private static final ObjectMapper MAPPER = new ObjectMapper();

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-state-snapshot-");
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
    @DisplayName("响应 data 的字段集合恰为契约 §6.2 必填 ∪ snapshot 帧 payload_fields，且 DTO 序列化形状一致")
    void 响应字段闭包与契约注册表逐值一致() throws Exception {
        seedAuthoritativeState();
        Set<String> expected = contractResponseFields();

        JsonNode body = json(call(120, 120));
        assertEquals(0, body.path("code").asInt(), "已初始化且基准一致时应成功");
        Set<String> wireFields = fieldNames(body.path("data"));

        assertEquals(expected, wireFields,
                "响应 data 字段集合必须与契约注册表逐值相等，不得新增 delta/high_watermark/replay_cursor 等字段");
        assertEquals(Set.of("code", "message", "data"), fieldNames(body), "信封恒为三键");

        StateSnapshotResponse dto = new StateSnapshotResponse(deviceId, 120, 128, 3, "in_progress", 96,
                false, 4, 4, 120, 76, "connected", 1, "1.0.0", 50, "mid", 15);
        Set<String> dtoFields = fieldNames(objectMapper.readTree(objectMapper.writeValueAsString(dto)));
        assertEquals(expected, dtoFields, "响应 DTO 的 wire 字段名必须与契约注册表逐值相等");
    }

    @Test
    @DisplayName("新端点默认受 Bearer 保护：无令牌 40103，令牌主体与落盘身份不符 40300")
    void 新端点受Bearer保护() throws Exception {
        JsonNode missing = json(mockMvc.perform(get(SNAPSHOT_PATH))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(ErrorCode.TOKEN_MISSING, missing.path("code").asInt());

        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(Map.of("code", "code-b"))))
                .andExpect(status().isOk());

        JsonNode foreign = json(mockMvc.perform(get(SNAPSHOT_PATH)
                .header("Authorization", "Bearer " + accessToken))
                .andExpect(status().isForbidden()).andReturn());
        assertEquals(ErrorCode.IDENTITY_MISMATCH, foreign.path("code").asInt());
    }

    @Test
    @DisplayName("越域/非法基准参数是协议级错误：回 400 + 40000，不得落进业务拒绝")
    void 非法基准参数回参数错误() throws Exception {
        JsonNode negative = json(mockMvc.perform(get(SNAPSHOT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .param("acked_total", "-1"))
                .andExpect(status().isBadRequest()).andReturn());
        assertEquals(ErrorCode.PARAM_INVALID, negative.path("code").asInt(),
                "越域声明必须回参数错误，不得被判成 20003「请按云端基准重建」");

        JsonNode nonNumeric = json(mockMvc.perform(get(SNAPSHOT_PATH)
                .header("Authorization", "Bearer " + accessToken)
                .param("snapshot_seq", "abc"))
                .andExpect(status().isBadRequest()).andReturn());
        assertEquals(ErrorCode.PARAM_INVALID, nonNumeric.path("code").asInt(),
                "非数值参数必须回参数错误，不得进入装配与选码路径");
    }

    @Test
    @DisplayName("未携带可验证基准回 20002，且响应携带完整权威基准")
    void 无基准返回20002并携带权威基准() throws Exception {
        seedAuthoritativeState();

        JsonNode body = json(call(null, null));

        assertEquals(ErrorCode.BASELINE_HIGH_WATER_MISSING, body.path("code").asInt(),
                "未携带基准高水位应回契约 §12 的 20002");
        assertFalse(body.path("message").asText().isBlank());
        assertFullAuthoritativeBaseline(body.path("data"));
    }

    @Test
    @DisplayName("越前基准回 20002、落后基准回 20003，两者都返回完整权威快照而不是会跳字的局部差量")
    void 越前与落后基准返回补齐基准() throws Exception {
        seedAuthoritativeState();

        JsonNode ahead = json(call(121, null));
        assertEquals(ErrorCode.BASELINE_HIGH_WATER_MISSING, ahead.path("code").asInt(),
                "声明高于已确认水位属「该声明从未被确认」，不构成有效基准");
        assertFullAuthoritativeBaseline(ahead.path("data"));

        JsonNode behind = json(call(119, null));
        assertEquals(ErrorCode.DEVICE_RESET_CONFLICT, behind.path("code").asInt(),
                "基准低于 cloud 已确认水位应回可恢复的 20003");
        assertFullAuthoritativeBaseline(behind.path("data"));

        JsonNode inSync = json(call(120, 120));
        assertEquals(0, inSync.path("code").asInt(), "与已确认水位一致的基准应被接受");
        assertFullAuthoritativeBaseline(inSync.path("data"));
    }

    @Test
    @DisplayName("权威轮次标识落在契约取值域外时回 20001，不把累计高水位盲推到新轮次")
    void 轮次无法归属返回20001() throws Exception {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", 128);
        progress.put("acked_total", 120);
        progress.put("round_id", 0);
        progress.put("round_cursor", 96);
        progress.put("round_state", "in_progress");
        progress.put("pending_completion", false);
        progress.put("action_id", "sync-20260923-0001");
        progressStore.write(progress);

        JsonNode body = json(call(120, null));

        assertEquals(ErrorCode.ROUND_UNASSIGNED, body.path("code").asInt());
        assertEquals(StateSnapshotResponse.NO_CONFIRMED_ROUND_ID, body.path("data").path("round_id").asInt());
        assertEquals(120, body.path("data").path("acked_total").asInt(),
                "拒绝仍须携带权威累计高水位");
    }

    @Test
    @DisplayName("查询路径严格只读：调用前后数据目录的文件集合与字节完全不变")
    void 查询不落盘任何文件() throws Exception {
        seedAuthoritativeState();
        Map<String, String> before = directoryContent();

        call(null, null);
        call(120, null);
        call(121, null);
        call(119, null);

        assertEquals(before, directoryContent(),
                "查询不得创建、改写或删除 app.data-dir 下的任何文件（snapshot_seq 绝不落盘）");
        assertTrue(before.containsKey(ProgressStore.fileName()), "权威文件应在场");
        assertTrue(before.keySet().stream().noneMatch(name -> name.contains("snapshot_seq")),
                "不得新增承载 snapshot_seq 的文件");
    }

    @Test
    @DisplayName("轮次与高水位只来自 progress.json：篡改派生/镜像文件不影响这些字段")
    void 派生文件不影响权威字段() throws Exception {
        seedAuthoritativeState();
        JsonNode before = json(call(120, null)).path("data");

        ObjectNode commands = MAPPER.createObjectNode();
        commands.put("command_revision", 9);
        commands.put("applied_revision", 9);
        commands.put("action_id", "cmd-20260923-0002");
        commands.put("volume", 10);
        commands.put("brightness", "low");
        commands.put("timeout", 5);
        commandsStore.write(commands);

        ObjectNode deviceState = MAPPER.createObjectNode();
        deviceState.put("battery_percent", 5);
        deviceState.put("network_mode", "no_signal");
        deviceState.put("audio_config_version", 7);
        deviceState.put("firmware_version", "9.9.9");
        deviceStateStore.write(deviceState);

        JsonNode after = json(call(120, null)).path("data");

        for (String authoritative : List.of("acked_total", "local_total", "round_id", "round_state",
                "round_cursor", "pending_completion", "snapshot_seq")) {
            assertEquals(before.path(authoritative), after.path(authoritative),
                    authoritative + " 只取自 progress.json，不得被派生文件改写");
        }
        assertEquals(9, after.path("command_revision").asInt());
        assertEquals(5, after.path("battery_percent").asInt());
    }


    private void seedAuthoritativeState() {
        ObjectNode progress = MAPPER.createObjectNode();
        progress.put("local_total", 128);
        progress.put("acked_total", 120);
        progress.put("round_id", 3);
        progress.put("round_cursor", 96);
        progress.put("round_state", "in_progress");
        progress.put("pending_completion", false);
        progress.put("action_id", "sync-20260923-0001");
        progressStore.write(progress);

        ObjectNode commands = MAPPER.createObjectNode();
        commands.put("command_revision", 4);
        commands.put("applied_revision", 4);
        commands.put("action_id", "cmd-20260923-0001");
        commands.put("volume", 50);
        commands.put("brightness", "mid");
        commands.put("timeout", 15);
        commandsStore.write(commands);

        ObjectNode deviceState = MAPPER.createObjectNode();
        deviceState.put("battery_percent", 76);
        deviceState.put("network_mode", "connected");
        deviceState.put("audio_config_version", 1);
        deviceState.put("firmware_version", "1.0.0");
        deviceStateStore.write(deviceState);
    }

    private MvcResult call(Integer ackedTotal, Integer snapshotSeq) throws Exception {
        var request = get(SNAPSHOT_PATH).header("Authorization", "Bearer " + accessToken);
        if (ackedTotal != null) {
            request = request.param("acked_total", ackedTotal.toString());
        }
        if (snapshotSeq != null) {
            request = request.param("snapshot_seq", snapshotSeq.toString());
        }
        return mockMvc.perform(request).andExpect(status().isOk()).andReturn();
    }

    /** 契约 §12：业务拒绝仍是一次响应，§6.2 的响应必填字段照常回传。 */
    private void assertFullAuthoritativeBaseline(JsonNode data) {
        assertEquals(deviceId, data.path("device_id").asText());
        assertEquals(120, data.path("acked_total").asInt());
        assertEquals(128, data.path("local_total").asInt());
        assertEquals(3, data.path("round_id").asInt());
        assertEquals(96, data.path("round_cursor").asInt());
        assertEquals(120, data.path("snapshot_seq").asInt(), "拒绝响应必须携带冻结的 snapshot_seq");
        assertEquals(4, data.path("command_revision").asInt());
        assertEquals(50, data.path("volume").asInt());
        assertFalse(data.isMissingNode(), "拒绝响应的 data 不得缺席");
    }

    /** 机械提取契约 §6.2 响应必填 ∪ snapshot 帧 payload_fields，不手抄第三份字段表。 */
    private static Set<String> contractResponseFields() throws IOException {
        JsonNode registry = contractRegistry();
        Set<String> fields = new TreeSet<>();

        JsonNode frames = registry.get("ws_frames");
        assertNotNull(frames, "契约注册表缺少 ws_frames");
        for (JsonNode frame : frames) {
            if (SNAPSHOT_FRAME_TYPE.equals(frame.path("type").asText())) {
                frame.path("payload_fields").forEach(field -> fields.add(field.asText()));
            }
        }

        JsonNode samples = registry.get("samples");
        assertNotNull(samples, "契约注册表缺少 samples");
        for (JsonNode sample : samples) {
            if (RESPONSE_SAMPLE_ID.equals(sample.path("id").asText())) {
                sample.path("required").forEach(field -> fields.add(field.asText()));
            }
        }

        assertFalse(fields.isEmpty(),
                "未能从注册表提取响应字段：ws_frames[snapshot].payload_fields 与 samples[https_sync_response].required 至少其一应命中");
        return fields;
    }

    private static JsonNode contractRegistry() throws IOException {
        Path schema = TestWorkspace.repoRoot().resolve("docs/contracts/sync-contract.schema.json");
        assertTrue(Files.isRegularFile(schema), "缺少契约注册表：" + schema);
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
