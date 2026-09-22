package top.zhcmqtt.ewf.backend.support;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * 测试用仓库根定位。
 * 从 {@code user.dir} 逐级向上查找同时存在契约注册表与 backend pom 的目录；找不到即失败（fail-closed），
 * 不允许静默跳过，否则依赖仓库文件的断言会退化为恒真。
 */
public final class TestWorkspace {

    /** 契约注册表（机器可读派生品）在仓库内的相对路径。 */
    private static final Path CONTRACT_SCHEMA = Paths.get("docs/contracts/sync-contract.schema.json");

    /** backend 工程标识文件在仓库内的相对路径。 */
    private static final Path BACKEND_POM = Paths.get("cloud/backend/pom.xml");

    private TestWorkspace() {
    }

    /** @return 仓库根绝对路径 */
    public static Path repoRoot() {
        Path current = Paths.get(System.getProperty("user.dir")).toAbsolutePath().normalize();
        while (current != null) {
            if (Files.isRegularFile(current.resolve(CONTRACT_SCHEMA))
                    && Files.isRegularFile(current.resolve(BACKEND_POM))) {
                return current;
            }
            current = current.getParent();
        }
        throw new IllegalStateException("找不到仓库根目录：需要同时存在 " + CONTRACT_SCHEMA + " 与 " + BACKEND_POM);
    }
}
