package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

import java.io.IOException;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.PosixFilePermission;
import java.util.Set;
import java.util.stream.Stream;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import top.zhcmqtt.ewf.backend.common.persistence.AtomicJsonFile;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;

/**
 * 原子写基元的直接单测：真实文件系统 + {@code @TempDir}，不启动 Spring。
 *
 * <p><b>静态校验无法覆盖的边界（不得写成已验证）：</b>
 * <ul>
 *   <li>单进程 {@code ATOMIC_MOVE} 不能证明多进程/跨 JVM 竞态下的语义，也不提供文件锁。</li>
 *   <li>文件级 {@code force(true)} 加 best-effort 父目录 fsync 不能证明掉电安全。</li>
 *   <li>{@code @TempDir} 下的测试不能证明生产数据目录的权限、挂载与磁盘写缓存配置。</li>
 * </ul>
 */
class AtomicJsonFileTest {

    @TempDir
    Path tempDir;

    @Test
    @DisplayName("原子替换：写入可读回，重复写入整体覆盖，无 .tmp 残留")
    void 原子替换与整体覆盖() throws Exception {
        Path target = tempDir.resolve("progress.json");
        byte[] first = "{\"schema_version\":1,\"acked_total\":1}".getBytes(StandardCharsets.UTF_8);
        byte[] second = "{\"schema_version\":1,\"acked_total\":2}".getBytes(StandardCharsets.UTF_8);

        AtomicJsonFile.write(target, first);
        assertArrayEquals(first, Files.readAllBytes(target), "首次写入的内容应可逐字节读回");

        AtomicJsonFile.write(target, second);
        assertArrayEquals(second, Files.readAllBytes(target), "重复写入必须是整体替换，不得追加或残留旧内容");

        assertEquals(0, temporaryFileCount(tempDir), "成功的原子写不得残留 .tmp 文件");
    }

    @Test
    @DisplayName("按需创建父目录（仅写路径）")
    void 按需创建父目录() throws Exception {
        Path nested = tempDir.resolve("a/b/c");
        Path target = nested.resolve("device_state.json");
        assertFalse(Files.exists(nested), "前置条件：父目录不应存在");

        AtomicJsonFile.write(target, "{}".getBytes(StandardCharsets.UTF_8));

        assertTrue(Files.isDirectory(nested), "原子写应按需创建父目录");
        assertEquals("{}", Files.readString(target, StandardCharsets.UTF_8));
    }

    @Test
    @DisplayName("超字节上限的载荷被拒绝，且完全不落盘")
    void 超上限拒绝且不落盘() throws Exception {
        Path nested = tempDir.resolve("oversize");
        Path target = nested.resolve("daily_stats.json");
        byte[] tooLarge = new byte[PersistenceLimits.MAX_JSON_BYTES + 1];

        assertThrows(IOException.class, () -> AtomicJsonFile.write(target, tooLarge),
                "超出上限的载荷必须被拒绝，而不是先落盘再由读取面发现");

        assertFalse(Files.exists(target), "被拒绝的载荷不得落盘");
        assertFalse(Files.exists(nested), "被拒绝的载荷不得触发目录创建");
        assertEquals(0, temporaryFileCount(tempDir), "被拒绝的载荷不得残留 .tmp 文件");
    }

    @Test
    @DisplayName("move 失败时 fail closed，且失败路径零 .tmp 残留")
    void 失败路径零临时残留() throws Exception {
        // 让 rename 的目标被一个非空目录占用：临时文件会写出，但 move 必然失败。
        Path target = tempDir.resolve("commands.json");
        Files.createDirectories(target);
        Files.writeString(target.resolve("occupied.txt"), "x");

        assertThrows(IOException.class, () -> AtomicJsonFile.write(target, "{}".getBytes(StandardCharsets.UTF_8)));

        assertTrue(Files.isDirectory(target), "失败路径不得改动占用目标的目录");
        assertEquals(0, temporaryFileCount(tempDir), "失败的原子写不得残留 .tmp 文件");
    }

    @Test
    @DisplayName("父目录不可打开时容忍目录 fsync 失败，已成功的写入不被判为失败")
    void 父目录fsync失败被容忍() throws Exception {
        Path parent = tempDir.resolve("no-read");
        Files.createDirectories(parent);
        Set<PosixFilePermission> original = Files.getPosixFilePermissions(parent);
        try {
            // 只给写 + 执行权限：可在其中创建/重命名文件，但无法把目录打开为通道。
            Files.setPosixFilePermissions(parent,
                    Set.of(PosixFilePermission.OWNER_WRITE, PosixFilePermission.OWNER_EXECUTE));
            assumeTrue(!directoryOpenable(parent), "本机仍可打开该目录，无法构造目录 fsync 的 tolerated 分支");

            Path target = parent.resolve("progress.json");
            AtomicJsonFile.write(target, "{\"schema_version\":1}".getBytes(StandardCharsets.UTF_8));

            assertEquals("{\"schema_version\":1}", Files.readString(target, StandardCharsets.UTF_8),
                    "目录 fsync 失败只记 WARN，不得把已成功的写入判为失败");
        } finally {
            Files.setPosixFilePermissions(parent, original);
        }
    }

    @Test
    @DisplayName("父目录 fsync 分支不改变写入结论")
    void 父目录fsync分支不改变写入结论() throws Exception {
        Path target = tempDir.resolve("fsync-conclusion.json");
        byte[] payload = "{\"schema_version\":1}".getBytes(StandardCharsets.UTF_8);

        AtomicJsonFile.write(target, payload);

        assertArrayEquals(payload, Files.readAllBytes(target),
                "无论目录 fsync 是否可用，写入结论都应由「临时文件 + fsync + move」决定");
        assertEquals(0, temporaryFileCount(tempDir), "目录 fsync 分支不得留下 .tmp 残留");
    }

    private static boolean directoryOpenable(Path directory) {
        try (FileChannel channel = FileChannel.open(directory, StandardOpenOption.READ)) {
            channel.force(true);
            return true;
        } catch (IOException | UnsupportedOperationException ex) {
            return false;
        }
    }

    /** 统计目录树中残留的 .tmp 文件数（含子目录，避免遗漏）。 */
    private static long temporaryFileCount(Path root) throws IOException {
        try (Stream<Path> paths = Files.walk(root)) {
            return paths.filter(Files::isRegularFile)
                    .filter(path -> PersistenceLimits.isTemporaryFileName(path.getFileName().toString()))
                    .count();
        }
    }
}
