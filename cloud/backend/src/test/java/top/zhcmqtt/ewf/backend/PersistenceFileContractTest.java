package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.IdentityStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * 契约一致性门禁：五个 Store 的「文件名 + {@code schema_version} + 字段集合」必须与
 * {@code docs/contracts/sync-contract.schema.json#persistence_files} 逐值一致，且五份文件的字段并集
 * 必须等于 {@code #persistence_scopes} 中 {@code backend} 作用域的字段集合。
 *
 * <p>期望集合**机械提取**自注册表，断言内不手抄第三份字段表——手抄会在契约与实现之外再产生一个真源
 * （沿用 {@code ErrorCodeContractTest} 的反第三真源口径）。注册表本身是契约正文的派生品
 * （{@code docs/contracts/README.md}），不是第二真源。
 *
 * <p><b>静态校验无法覆盖的边界（不得写成已验证）：</b>本测试只能证明 Java 常量与契约注册表逐值一致，
 * **不能证明运行期不存在其他落盘路径**，也不能证明写入顺序、原子性或恢复语义。它也不校验字段类型与
 * {@code single_transaction} 等本 Story 未实现语义的对象。
 */
class PersistenceFileContractTest {

    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

    /** 契约文件名 → 「文件名 + 字段集合 + schema_version」的读取入口，与 §11 文件表一一对应。 */
    private static final Map<String, StoreFacade> STORES = storeFacades();

    @Test
    @DisplayName("五个 Store 的文件名与字段集合与契约 §11 逐值一致")
    void 文件名与字段集合逐值一致() throws IOException {
        Map<String, JsonNode> contract = contractFiles();

        assertEquals(STORES.size(), contract.size(),
                "本 Story 只建立契约 §11 的五份文件；不得新增第六个后端状态文件");

        for (Map.Entry<String, JsonNode> entry : contract.entrySet()) {
            StoreFacade facade = STORES.get(entry.getKey());
            assertNotNull(facade, "Store 未覆盖契约文件 " + entry.getKey());
            assertEquals(PersistenceLimits.SCHEMA_VERSION, entry.getValue().get("schema_version").asInt(),
                    entry.getKey() + " 的 schema_version 应为契约冻结值");
            assertEquals(toSet(entry.getValue().get("fields")), facade.fields(),
                    entry.getKey() + " 的字段集合应逐值等于契约 §11 文件表");
            assertEquals("app.data-dir", entry.getValue().get("root").asText(),
                    entry.getKey() + " 必须落在 app.data-dir 下，不得引入第二个数据目录");
        }
    }

    @Test
    @DisplayName("五份文件的字段并集等于 backend 作用域的字段集合")
    void 字段并集等于backend作用域() throws IOException {
        JsonNode scopes = registry().get("persistence_scopes");
        assertNotNull(scopes, "契约注册表缺少 persistence_scopes");
        JsonNode backend = null;
        for (JsonNode scope : scopes) {
            if ("backend".equals(scope.get("scope").asText())) {
                backend = scope;
            }
        }
        assertNotNull(backend, "persistence_scopes 缺少 backend 作用域");

        Set<String> union = new TreeSet<>();
        for (StoreFacade facade : STORES.values()) {
            union.addAll(facade.fields());
        }

        assertEquals(toSet(backend.get("fields")), union,
                "backend 作用域的承载字段集合必须等于 §11 五份文件字段列的并集");
    }

    @Test
    @DisplayName("daily_stats.json 的字段列为「无」，桶结构留给 Story 5.3")
    void 日统计不冻结桶字段() throws IOException {
        JsonNode dailyStats = contractFiles().get(DailyStatsStore.fileName());
        assertNotNull(dailyStats, "契约 §11 应包含 daily_stats.json");
        assertEquals(0, dailyStats.get("fields").size(), "契约把该文件字段列冻结为「无」");
        assertEquals(Set.of(), DailyStatsStore.allowedFields(),
                "本 Story 不得为按日桶发明第二套字段白名单（属 Story 5.3）");
    }

    private static Map<String, StoreFacade> storeFacades() {
        Map<String, StoreFacade> stores = new LinkedHashMap<>();
        stores.put(ProgressStore.fileName(), new StoreFacade(ProgressStore.fileName(), ProgressStore.allowedFields()));
        stores.put(IdentityStore.fileName(), new StoreFacade(IdentityStore.fileName(), IdentityStore.allowedFields()));
        stores.put(CommandsStore.fileName(), new StoreFacade(CommandsStore.fileName(), CommandsStore.allowedFields()));
        stores.put(DeviceStateStore.fileName(),
                new StoreFacade(DeviceStateStore.fileName(), DeviceStateStore.allowedFields()));
        stores.put(DailyStatsStore.fileName(),
                new StoreFacade(DailyStatsStore.fileName(), DailyStatsStore.allowedFields()));
        return stores;
    }

    private static Map<String, JsonNode> contractFiles() throws IOException {
        JsonNode files = registry().get("persistence_files");
        assertNotNull(files, "契约注册表缺少 persistence_files");
        assertTrue(files.isArray() && !files.isEmpty(), "persistence_files 应为非空数组");

        Map<String, JsonNode> byName = new LinkedHashMap<>();
        for (JsonNode file : files) {
            byName.put(file.get("name").asText(), file);
        }
        return byName;
    }

    private static JsonNode registry() throws IOException {
        Path schema = TestWorkspace.repoRoot().resolve("docs/contracts/sync-contract.schema.json");
        assertTrue(Files.isRegularFile(schema), "缺少契约注册表：" + schema);
        JsonNode root = OBJECT_MAPPER.readTree(Files.readString(schema, StandardCharsets.UTF_8));
        assertNotNull(root, "契约注册表不可解析：" + schema);
        return root;
    }

    private static Set<String> toSet(JsonNode array) {
        assertTrue(array != null && array.isArray(), "契约字段列应为数组");
        Set<String> fields = new TreeSet<>();
        for (JsonNode field : array) {
            if (!fields.add(field.asText())) {
                fail("契约字段列存在重复项: " + field.asText());
            }
        }
        return fields;
    }

    /** Store 对外暴露的契约面：文件名 + 契约字段列（不含信封 {@code schema_version}）。 */
    private record StoreFacade(String fileName, Set<String> fields) {
    }
}
