package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

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

import top.zhcmqtt.ewf.backend.client.WechatMiniClient;
import top.zhcmqtt.ewf.backend.common.config.JwtProperties;
import top.zhcmqtt.ewf.backend.common.security.JwtTokenProvider;
import top.zhcmqtt.ewf.backend.common.security.UserContext;

@SpringBootTest
@AutoConfigureMockMvc
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class AuthContractTest {

    private static final Path DATA_DIR;

    private static final String TEST_SECRET =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    private static final long TEST_ACCESS_EXPIRATION = 7200000L;

    static {
        try {
            DATA_DIR = Files.createTempDirectory("ewf-auth-contract-");
        } catch (Exception ex) {
            throw new ExceptionInInitializerError(ex);
        }
    }

    @DynamicPropertySource
    static void properties(DynamicPropertyRegistry registry) {
        registry.add("app.data-dir", () -> DATA_DIR.toString());
        registry.add("jwt.secret", () -> TEST_SECRET);
        registry.add("jwt.access-token-expiration", () -> String.valueOf(TEST_ACCESS_EXPIRATION));
        registry.add("jwt.refresh-token-expiration", () -> "2592000000");
    }

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @MockBean
    private WechatMiniClient wechatMiniClient;

    @BeforeEach
    void cleanIdentity() throws Exception {
        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        when(wechatMiniClient.exchangeCode("code-a")).thenReturn("openid-a");
        when(wechatMiniClient.exchangeCode("code-b")).thenReturn("openid-b");
        when(wechatMiniClient.exchangeCode("same")).thenReturn("openid-same");
        UserContext.clear();
    }

    @AfterAll
    void deleteDataDirectory() throws Exception {
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
    @DisplayName("首登、重复登录与 refresh 返回固定 deviceId，且 refresh 不重新调用微信")
    void loginRepeatAndRefresh() throws Exception {
        JsonNode first = login("code-a");
        String deviceId = first.path("data").path("deviceId").asText();
        String access = first.path("data").path("accessToken").asText();
        String refresh = first.path("data").path("refreshToken").asText();
        assertTrue(deviceId.startsWith("ewf-"));
        assertFalse(first.toString().contains("openid"));
        assertFalse(first.toString().contains("session_key"));
        assertFalse(access.isBlank());
        assertFalse(refresh.isBlank());
        assertEquals(TEST_ACCESS_EXPIRATION / 1000, first.path("data").path("expiresIn").asLong(),
                "expiresIn 必须是配置的 access 生命周期秒数；前端据此计算本地过期时间");

        JsonNode repeated = login("code-a");
        assertEquals(deviceId, repeated.path("data").path("deviceId").asText());

        JsonNode refreshed = json(mockMvc.perform(post("/api/v1/auth/refresh")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(java.util.Map.of("refreshToken", refresh))))
                .andExpect(status().isOk()).andReturn());
        assertEquals(deviceId, refreshed.path("data").path("deviceId").asText());
        assertEquals(TEST_ACCESS_EXPIRATION / 1000, refreshed.path("data").path("expiresIn").asLong(),
                "refresh 响应同样必须携带 expiresIn");
        org.mockito.Mockito.verify(wechatMiniClient, org.mockito.Mockito.times(2)).exchangeCode("code-a");
        try (var files = Files.list(DATA_DIR)) {
            assertEquals(1, files.filter(path -> path.getFileName().toString().equals("identity.json")).count());
        }
    }

    @Test
    @DisplayName("不同微信身份返回 40300 且不覆盖已有身份")
    void differentOpenIdRejected() throws Exception {
        JsonNode first = login("code-a");
        String original = Files.readString(DATA_DIR.resolve("identity.json"));
        JsonNode denied = json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType("application/json")
                .content("{\"code\":\"code-b\"}"))
                .andExpect(status().isForbidden()).andReturn());
        assertEquals(40300, denied.path("code").asInt());
        assertFalse(denied.path("message").asText().isBlank());
        assertEquals(original, Files.readString(DATA_DIR.resolve("identity.json")));
        assertEquals(first.path("data").path("deviceId").asText(),
                objectMapper.readTree(original).path("device_id").asText());
    }

    @Test
    @DisplayName("并发同 openid 首登只产生一个身份，受保护资源读取 UserContext")
    void concurrentSameOpenIdAndProtectedProbe() throws Exception {
        ExecutorService executor = Executors.newFixedThreadPool(6);
        try {
            List<Callable<String>> tasks = new ArrayList<>();
            for (int i = 0; i < 6; i++) {
                tasks.add(() -> login("same").path("data").path("deviceId").asText());
            }
            List<String> devices = new ArrayList<>();
            for (var result : executor.invokeAll(tasks)) {
                devices.add(result.get());
            }
            assertEquals(1, devices.stream().distinct().count());
        } finally {
            executor.shutdownNow();
        }
        JsonNode login = login("same");
        String access = login.path("data").path("accessToken").asText();
        String expectedDevice = login.path("data").path("deviceId").asText();
        JsonNode probe = json(mockMvc.perform(get("/api/v1/__probe/identity").header("Authorization", "Bearer " + access))
                .andExpect(status().isOk()).andReturn());
        assertEquals(expectedDevice, probe.path("data").path("deviceId").asText());
        assertNull(UserContext.current(), "请求 finally 后 ThreadLocal 必须清理");
    }

    @Test
    @DisplayName("缺失/非法/refresh 误用 Bearer 令牌均返回统一 401")
    void tokenPathIsolation() throws Exception {
        JsonNode missing = json(mockMvc.perform(get("/api/v1/__probe/identity"))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(40103, missing.path("code").asInt());
        JsonNode invalid = json(mockMvc.perform(get("/api/v1/__probe/identity")
                .header("Authorization", "Bearer malformed"))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(40102, invalid.path("code").asInt());

        JsonNode login = login("code-a");
        String refresh = login.path("data").path("refreshToken").asText();
        JsonNode refreshAsAccess = json(mockMvc.perform(get("/api/v1/__probe/identity")
                .header("Authorization", "Bearer " + refresh))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(40102, refreshAsAccess.path("code").asInt());
    }

    @Test
    @DisplayName("过期与签名被篡改的 access token 分别返回 401 + 40101 / 40102")
    void expiredAndTamperedAccessTokenRejected() throws Exception {
        String deviceId = login("code-a").path("data").path("deviceId").asText();
        JwtProperties shortLived = new JwtProperties();
        shortLived.setSecret(TEST_SECRET);
        shortLived.setAccessTokenExpiration(1);
        shortLived.setRefreshTokenExpiration(1);
        String expired = new JwtTokenProvider(shortLived).issueTokenPair(deviceId).accessToken();
        Thread.sleep(20);

        JsonNode expiredBody = json(mockMvc.perform(get("/api/v1/__probe/identity")
                .header("Authorization", "Bearer " + expired))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(40101, expiredBody.path("code").asInt(), "过期 access token 应回冻结 40101");
        assertNull(UserContext.current(), "认证失败后 ThreadLocal 也必须清理");

        String valid = login("code-a").path("data").path("accessToken").asText();
        String tampered = valid.substring(0, valid.lastIndexOf('.') + 1) + "AAAA";
        JsonNode tamperedBody = json(mockMvc.perform(get("/api/v1/__probe/identity")
                .header("Authorization", "Bearer " + tampered))
                .andExpect(status().isUnauthorized()).andReturn());
        assertEquals(40102, tamperedBody.path("code").asInt(),
                "结构合法但签名被篡改的令牌必须回 40102，而不是被接受");
    }

    @Test
    @DisplayName("身份 A 的合法 access token 在落盘身份为 B 时访问受保护资源返回 403 + 40300")
    void protectedResourceRejectsForeignIdentity() throws Exception {
        String accessOfA = login("code-a").path("data").path("accessToken").asText();
        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        String deviceB = login("code-b").path("data").path("deviceId").asText();

        JsonNode denied = json(mockMvc.perform(get("/api/v1/__probe/identity")
                .header("Authorization", "Bearer " + accessOfA))
                .andExpect(status().isForbidden()).andReturn());
        assertEquals(40300, denied.path("code").asInt(), "令牌身份与落盘设备不匹配应回 40300");
        assertFalse(denied.path("message").asText().isBlank());
        assertEquals(deviceB, objectMapper.readTree(Files.readString(DATA_DIR.resolve("identity.json")))
                .path("device_id").asText(), "拒绝受保护资源不得改写既有身份文件");
        assertNull(UserContext.current(), "身份不匹配被拒后 ThreadLocal 必须清理");
    }

    @Test
    @DisplayName("身份 A 的 refresh token 在落盘身份为 B 时返回 403 + 40300 且不签发新令牌")
    void refreshRejectsForeignIdentity() throws Exception {
        String refreshOfA = login("code-a").path("data").path("refreshToken").asText();
        Files.deleteIfExists(DATA_DIR.resolve("identity.json"));
        String deviceB = login("code-b").path("data").path("deviceId").asText();

        MvcResult denied = mockMvc.perform(post("/api/v1/auth/refresh")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(java.util.Map.of("refreshToken", refreshOfA))))
                .andExpect(status().isForbidden()).andReturn();
        JsonNode body = json(denied);
        assertEquals(40300, body.path("code").asInt(), "跨设备 refresh 应回 40300");
        assertTrue(body.path("data").isMissingNode(), "被拒绝的 refresh 不得签发新令牌对");
        assertEquals(deviceB, objectMapper.readTree(Files.readString(DATA_DIR.resolve("identity.json")))
                .path("device_id").asText());
    }

    private JsonNode login(String code) throws Exception {
        return json(mockMvc.perform(post("/api/v1/auth/login/wechat-mini")
                .contentType("application/json")
                .content(objectMapper.writeValueAsString(java.util.Map.of("code", code))))
                .andExpect(status().isOk()).andReturn());
    }

    private JsonNode json(MvcResult result) throws Exception {
        return objectMapper.readTree(result.getResponse().getContentAsString());
    }
}
