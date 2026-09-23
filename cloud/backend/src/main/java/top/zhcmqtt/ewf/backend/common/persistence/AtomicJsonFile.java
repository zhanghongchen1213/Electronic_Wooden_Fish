package top.zhcmqtt.ewf.backend.common.persistence;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * 唯一的 JSON 原子写基元：同目录临时文件 + 写入 + {@code FileChannel.force(true)}（等价 fsync）
 * + {@link Files#move(Path, Path, java.nio.file.CopyOption...) Files.move(ATOMIC_MOVE)} 原子替换。
 *
 * <p>契约 {@code docs/contracts/sync-contract.md} §11：每个文件独立原子写，单次重命名即一次
 * 持久化事务边界。本类是 backend 全部落盘的唯一实现——
 * {@code cloud/backend/src/main/java/.../service/} 下不得再出现第二份 temp+force+move 逻辑。
 *
 * <p><b>失败路径纪律：</b>任一环节失败都在 {@code finally} 中删除临时文件，不留 {@code .tmp} 残留；
 * 不提供 {@code Files.writeString(target, ...)} 这类非原子覆盖回退分支——一旦提供，调用方就可能在
 * 崩溃后观察到半写文件。
 *
 * <p><b>已知边界（不得写成已验证）：</b>
 * <ul>
 *   <li>{@code ATOMIC_MOVE} 只保证「观察到的要么是旧文件、要么是新文件」，<b>不保证</b> rename 本身
 *       已持久化；文件系统不支持原子 move 时会抛 {@code AtomicMoveNotSupportedException}。</li>
 *   <li>本基元不提供跨进程/跨 JVM 互斥与文件锁；原子写是崩溃保护，不能替代调用方在单实例内的互斥
 *       （AD-16 单实例单进程前提）。</li>
 *   <li>文件级 {@code force(true)} 加 best-effort 目录 fsync 仍不能证明掉电安全：临时目录下的测试
 *       不能证明生产磁盘的写缓存、挂载选项与权限配置。</li>
 * </ul>
 */
public final class AtomicJsonFile {

    private static final Logger LOG = LoggerFactory.getLogger(AtomicJsonFile.class);

    private AtomicJsonFile() {
    }

    /**
     * 原子替换写入：按需创建父目录（**仅写路径**创建，读路径不得调用本方法）→ 同目录临时文件 →
     * 写入并 fsync → 原子 move 到目标 → 成功后 best-effort fsync 父目录。
     *
     * <p>父目录 fsync 是业界补全步骤：只 fsync 临时文件并不使 rename 的目录项持久化。该步骤没有
     * 跨平台 API（Windows 上把目录打开为通道会失败），因此失败只记 WARN，**既不改写写入结论**，
     * 也不把失败当成功：写入成功与否只由「临时文件写入 + fsync + move」决定。
     *
     * @param target  目标文件；文件名必须以 {@code .json} 结尾以复用统一的临时文件命名规则
     * @param payload 完整文件字节（含调用方决定的尾换行等格式细节）
     * @throws IOException 目录创建、临时文件写入、fsync 或 move 失败；此时临时文件已被清理。
     *                     {@link java.nio.file.FileAlreadyExistsException} 可能由 move 抛出，调用方
     *                     可将其用作并发建号哨兵。
     */
    public static void write(Path target, byte[] payload) throws IOException {
        Path fileName = target.getFileName();
        if (fileName == null) {
            throw new IllegalArgumentException("原子写目标必须是文件路径");
        }
        if (payload.length > PersistenceLimits.MAX_JSON_BYTES) {
            // 上限集中一处、在落盘之前检查：超限即拒绝，不创建目录、不创建临时文件、不落盘。
            throw new IOException("载荷超出单文件字节上限: bytes=" + payload.length);
        }
        // 命名规则先于任何文件系统副作用求值：目标名不合法属调用方错误，不得留下已创建的目录。
        String temporaryPrefix = PersistenceLimits.tempPrefix(fileName.toString());
        Path parent = target.getParent() != null ? target.getParent() : Path.of(".");
        try {
            Files.createDirectories(parent);
        } catch (FileAlreadyExistsException ex) {
            // 目录路径被常规文件占用：这不是并发建号，翻译成普通 IO 失败，
            // 以免调用方把目录创建失败误当成 FileAlreadyExistsException 哨兵。
            throw new IOException("数据目录路径被非目录占用: " + parent, ex);
        }

        Path temporary = Files.createTempFile(parent, temporaryPrefix, PersistenceLimits.TEMP_SUFFIX);
        boolean moved = false;
        try {
            try (FileChannel channel = FileChannel.open(temporary, StandardOpenOption.WRITE)) {
                ByteBuffer buffer = ByteBuffer.wrap(payload);
                while (buffer.hasRemaining()) {
                    channel.write(buffer);
                }
                channel.force(true);
            }
            Files.move(temporary, target, StandardCopyOption.ATOMIC_MOVE);
            moved = true;
            forceDirectory(parent);
        } finally {
            if (!moved) {
                Files.deleteIfExists(temporary);
            }
        }
    }

    /** best-effort 父目录 fsync：失败只记 WARN，不改变已完成的原子写结论。 */
    private static void forceDirectory(Path directory) {
        try (FileChannel channel = FileChannel.open(directory, StandardOpenOption.READ)) {
            channel.force(true);
        } catch (IOException | UnsupportedOperationException ex) {
            LOG.warn("父目录 fsync 在该平台不可用，已完成的原子写不受影响: dir={}", directory, ex);
        }
    }
}
