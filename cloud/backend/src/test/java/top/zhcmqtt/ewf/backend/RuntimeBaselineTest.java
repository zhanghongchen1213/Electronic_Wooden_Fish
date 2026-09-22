package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.boot.Banner;
import org.springframework.boot.WebApplicationType;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.builder.SpringApplicationBuilder;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.core.env.Environment;
import org.yaml.snakeyaml.Yaml;

import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * 运行期配置基线门禁：profile 激活面、端口、data-dir、时区与前后端占位值对齐。
 *
 * <p><b>静态校验无法覆盖的边界：</b>本测试只校验配置文件与前端环境文件的取值一致性，
 * 不替换任何部署占位值（生产域名、AppID、密钥属部署 Deferred）；也不证明真实部署环境的取值。
 */
class RuntimeBaselineTest {

    private static final String PRODUCTION_APP_ID = "replace-with-ewf-wechat-app-id";
    private static final String LOCAL_APP_ID = "replace-with-ewf-local-wechat-app-id";

    private static ConfigurableApplicationContext defaultContext;
    private static ConfigurableApplicationContext localContext;

    @BeforeAll
    static void 启动基线上下文() {
        defaultContext = new SpringApplication(EwfBackendApplication.class)
                .run("--spring.main.web-application-type=none", "--spring.main.banner-mode=off");
        localContext = new SpringApplicationBuilder(EwfBackendApplication.class)
                .web(WebApplicationType.NONE)
                .bannerMode(Banner.Mode.OFF)
                .profiles("local")
                .run();
    }

    @AfterAll
    static void 关闭基线上下文() {
        if (defaultContext != null) {
            defaultContext.close();
        }
        if (localContext != null) {
            localContext.close();
        }
    }

    @Test
    @DisplayName("默认 profile 不得激活 local")
    void 默认profile不得激活local() {
        String[] activeProfiles = defaultContext.getEnvironment().getActiveProfiles();

        assertFalse(Arrays.asList(activeProfiles).contains("local"),
                "默认 profile 不应激活 local，实际为 " + Arrays.toString(activeProfiles));
    }

    @Test
    @DisplayName("默认（生产）profile 下端口、data-dir 与 Jackson 时区取基线值")
    void 默认profile基线取值() {
        Environment env = defaultContext.getEnvironment();

        assertEquals("9218", env.getProperty("server.port"), "server.port 应为 9218");
        assertEquals("./data", env.getProperty("app.data-dir"), "app.data-dir 应为 ./data（落点约定，本 Story 不建目录）");
        assertEquals("Asia/Shanghai", env.getProperty("spring.jackson.time-zone"), "Jackson 时区应为 Asia/Shanghai");
    }

    @Test
    @DisplayName("local profile 下端口、data-dir 与 Jackson 时区取基线值")
    void local_profile基线取值() {
        Environment env = localContext.getEnvironment();

        assertTrue(Arrays.asList(env.getActiveProfiles()).contains("local"), "local profile 应处于激活态");
        assertEquals("9218", env.getProperty("server.port"), "server.port 应为 9218");
        assertEquals("./data", env.getProperty("app.data-dir"), "app.data-dir 应为 ./data（落点约定，本 Story 不建目录）");
        assertEquals("Asia/Shanghai", env.getProperty("spring.jackson.time-zone"), "Jackson 时区应为 Asia/Shanghai");
        assertEquals(LOCAL_APP_ID, env.getProperty("wechat.mini.app-id"), "local profile 应取本地 AppID 占位值");

        for (String key : List.of("wechat.mini.app-secret", "jwt.secret",
                "jwt.access-token-expiration", "jwt.refresh-token-expiration")) {
            assertNotNull(env.getProperty(key), "缺配置键 " + key + "（本 Story 只校验存在，不实现其语义）");
        }
    }

    @Test
    @DisplayName("application.yml 不得写死 spring.profiles.active；application-local.yml 必须声明 on-profile: local")
    void profile声明基线() throws IOException {
        Map<String, Object> production = readYaml("cloud/backend/src/main/resources/application.yml");
        assertFalse(nestedHasKey(production, "spring", "profiles", "active"),
                "本地 profile 只应由 start-local-test-backend.sh 的 --spring.profiles.active=local 激活");

        Map<String, Object> local = readYaml("cloud/backend/src/main/resources/application-local.yml");
        assertEquals("local", nestedGet(local, "spring", "config", "activate", "on-profile"),
                "application-local.yml 必须声明 on-profile: local");
    }

    @Test
    @DisplayName("前端环境文件与 backend 配置对齐：API 前缀与 AppID")
    void 前端环境文件与backend对齐() throws IOException {
        Path repoRoot = TestWorkspace.repoRoot();
        Map<String, String> production = readEnvFile(repoRoot.resolve("cloud/frontend/.env"));
        Map<String, String> local = readEnvFile(repoRoot.resolve("cloud/frontend/.env.development"));

        assertTrue(production.get("VITE_API_BASE_URL").endsWith("/api/v1"),
                "VITE_API_BASE_URL 应以 /api/v1 结尾，实际为 " + production.get("VITE_API_BASE_URL"));
        assertTrue(local.get("VITE_API_BASE_URL").endsWith("/api/v1"),
                "本地 VITE_API_BASE_URL 应以 /api/v1 结尾，实际为 " + local.get("VITE_API_BASE_URL"));

        assertEquals(PRODUCTION_APP_ID, defaultContext.getEnvironment().getProperty("wechat.mini.app-id"));
        assertEquals(PRODUCTION_APP_ID, production.get("VITE_WX_APPID"),
                "application.yml 的 wechat.mini.app-id 应与 cloud/frontend/.env 的 AppID 一致");
        assertEquals(LOCAL_APP_ID, local.get("VITE_WX_APPID"),
                "application-local.yml 的 wechat.mini.app-id 应与 cloud/frontend/.env.development 的 AppID 一致");
    }

    private static Map<String, Object> readYaml(String relativePath) throws IOException {
        Path path = TestWorkspace.repoRoot().resolve(relativePath);
        assertTrue(Files.isRegularFile(path), "缺少配置文件：" + path);
        Object loaded = new Yaml().load(Files.readString(path, StandardCharsets.UTF_8));
        assertTrue(loaded instanceof Map, "配置文件不是 YAML 映射：" + path);
        @SuppressWarnings("unchecked")
        Map<String, Object> map = (Map<String, Object>) loaded;
        return map;
    }

    private static Object nestedGet(Map<String, Object> root, String... keys) {
        Object current = root;
        for (String key : keys) {
            if (!(current instanceof Map)) {
                return null;
            }
            current = ((Map<?, ?>) current).get(key);
        }
        return current;
    }

    private static boolean nestedHasKey(Map<String, Object> root, String... keys) {
        Object parent = nestedGet(root, Arrays.copyOf(keys, keys.length - 1));
        return parent instanceof Map && ((Map<?, ?>) parent).containsKey(keys[keys.length - 1]);
    }

    private static Map<String, String> readEnvFile(Path path) throws IOException {
        assertTrue(Files.isRegularFile(path), "缺少前端环境文件：" + path);
        Map<String, String> values = new LinkedHashMap<>();
        for (String line : Files.readAllLines(path, StandardCharsets.UTF_8)) {
            String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            int separator = trimmed.indexOf('=');
            if (separator <= 0) {
                continue;
            }
            values.put(trimmed.substring(0, separator).trim(), trimmed.substring(separator + 1).trim());
        }
        for (String key : List.of("VITE_API_BASE_URL", "VITE_WX_APPID")) {
            if (!values.containsKey(key)) {
                fail(path + " 缺少键 " + key);
            }
        }
        return values;
    }
}
