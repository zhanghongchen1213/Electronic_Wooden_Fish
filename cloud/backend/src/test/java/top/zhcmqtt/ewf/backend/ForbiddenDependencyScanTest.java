package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.stream.Stream;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * 禁用依赖与禁用配置 fail-closed 扫描：backend 不得读取 miaowu 的数据库 / 支付 / 对象存储配置。
 *
 * <p>可失败性由实测证明：向 {@code pom.xml} 加入 {@code spring-boot-starter-jdbc} 即使本类红。
 *
 * <p><b>静态校验无法覆盖的边界：</b>本扫描是字面量匹配，只能证明这些键名未出现在
 * {@code pom.xml} 与 {@code src/main/resources/*.yml} 中；它不能证明运行期不会通过环境变量、
 * 外部配置文件或反射引入等价的依赖与配置。
 */
class ForbiddenDependencyScanTest {

    /** 禁用键：数据库、ORM、迁移、支付与对象存储、公网隧道。 */
    private static final List<String> FORBIDDEN_TOKENS = List.of(
            "jdbc", "jpa", "mybatis", "hibernate", "flyway", "liquibase", "sqlite",
            "postgresql", "mysql", "druid", "h2", "jeepay", "obs", "huaweicloud", "cloudflared");

    /** 禁用初始化脚本：JSON 零库基线不引入任何数据库。 */
    private static final List<String> FORBIDDEN_SQL_FILES = List.of("schema.sql", "data.sql");

    @Test
    @DisplayName("pom.xml 与 application*.yml 不得命中禁用键")
    void 不得命中禁用键() throws IOException {
        Path resources = TestWorkspace.repoRoot().resolve("cloud/backend/src/main/resources");
        List<Path> scanned = new ArrayList<>();
        scanned.add(TestWorkspace.repoRoot().resolve("cloud/backend/pom.xml"));
        try (Stream<Path> candidates = Files.list(resources)) {
            candidates.filter(path -> path.getFileName().toString().endsWith(".yml")).forEach(scanned::add);
        }

        assertTrue(scanned.size() >= 3, "应扫描 pom.xml 与两份 application 配置，实际 " + scanned);

        for (Path path : scanned) {
            assertTrue(Files.isRegularFile(path), "缺少被扫描文件：" + path);
            String content = Files.readString(path, StandardCharsets.UTF_8).toLowerCase(Locale.ROOT);
            for (String token : FORBIDDEN_TOKENS) {
                assertFalse(content.contains(token),
                        path + " 命中禁用键「" + token + "」：backend 不得引入数据库、支付、对象存储或公网隧道依赖");
            }
        }
    }

    @Test
    @DisplayName("src/main/resources 不得存在 schema.sql / data.sql")
    void 不得存在初始化SQL() {
        Path resources = TestWorkspace.repoRoot().resolve("cloud/backend/src/main/resources");

        for (String sqlFile : FORBIDDEN_SQL_FILES) {
            assertFalse(Files.exists(resources.resolve(sqlFile)),
                    "JSON 零库基线不允许数据库初始化脚本：" + resources.resolve(sqlFile));
        }
    }
}
