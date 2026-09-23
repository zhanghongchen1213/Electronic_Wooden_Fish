package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;
import top.zhcmqtt.ewf.backend.common.persistence.VersionedJsonFile;

/**
 * 严格读基元的直接单测：真实文件系统 + {@code @TempDir}，不启动 Spring。
 *
 * <p>每一条非法输入都必须回 {@code 50000} 且**原文件字节不变**——绝不改写、绝不返回默认值。
 *
 * <p><b>静态校验无法覆盖的边界：</b>本测试只证明单进程下「非法字节被拒绝、文件未被改写」，
 * 不证明并发写入期间读取的行为，也不证明生产数据目录下的权限与挂载配置。
 */
class VersionedJsonFileTest {

    private static final Set<String> ENVELOPE =
            PersistenceLimits.envelopeFields(Set.of("acked_total"));

    @TempDir
    Path tempDir;

    @Test
    @DisplayName("合法文件可读回，且不修改文件")
    void 合法文件可读回() throws Exception {
        Path file = tempDir.resolve("progress.json");
        String content = "{\"schema_version\":1,\"acked_total\":7}";
        Files.writeString(file, content, StandardCharsets.UTF_8);

        Optional<ObjectNode> root = VersionedJsonFile.read(file, ENVELOPE);

        assertTrue(root.isPresent(), "合法文件必须可读");
        assertEquals(7, root.get().path("acked_total").asInt());
        assertEquals(content, Files.readString(file, StandardCharsets.UTF_8), "读取不得改写文件");
    }

    @Test
    @DisplayName("文件缺失返回「未初始化」而非异常")
    void 缺失文件返回未初始化() {
        Path missing = tempDir.resolve("progress.json");

        assertEquals(Optional.empty(), VersionedJsonFile.read(missing, ENVELOPE),
                "缺失必须与损坏区分：缺失是空状态，不是错误");
        assertFalse(Files.exists(missing), "读取缺失文件不得创建它");
    }

    @Test
    @DisplayName("schema_version 非 1 / 缺失 / 类型错误一律 50000 且不改写文件")
    void schema版本非法被拒绝() throws Exception {
        for (String invalid : List.of(
                "{\"schema_version\":2,\"acked_total\":1}",
                "{\"schema_version\":0,\"acked_total\":1}",
                "{\"schema_version\":\"1\",\"acked_total\":1}",
                "{\"schema_version\":1.0,\"acked_total\":1}",
                "{\"acked_total\":1}")) {
            assertRejectedWithoutRewrite(invalid);
        }
    }

    @Test
    @DisplayName("未知字段与缺失字段一律 50000 且不改写文件")
    void 字段集合精确匹配() throws Exception {
        assertRejectedWithoutRewrite("{\"schema_version\":1,\"acked_total\":1,\"extra\":1}");
        assertRejectedWithoutRewrite("{\"schema_version\":1}");
    }

    @Test
    @DisplayName("尾随内容与非 object 根一律 50000 且不改写文件")
    void 尾随内容与非对象根被拒绝() throws Exception {
        assertRejectedWithoutRewrite("{\"schema_version\":1,\"acked_total\":1}{\"schema_version\":1}");
        assertRejectedWithoutRewrite("[1,2,3]");
        assertRejectedWithoutRewrite("\"schema_version\"");
        assertRejectedWithoutRewrite("");
        assertRejectedWithoutRewrite("not json at all");
    }

    @Test
    @DisplayName("非 UTF-8 字节一律 50000 且不改写文件")
    void 非法UTF8被拒绝() throws Exception {
        Path file = tempDir.resolve("progress.json");
        byte[] invalidUtf8 = {'{', '"', (byte) 0xC3, (byte) 0x28, '"', '}'};
        Files.write(file, invalidUtf8);

        assertRejected(file, invalidUtf8);
    }

    @Test
    @DisplayName("超出字节上限一律 50000，且不解析、不改写文件")
    void 超出字节上限被拒绝() throws Exception {
        Path file = tempDir.resolve("progress.json");
        StringBuilder filler = new StringBuilder("{\"schema_version\":1,\"acked_total\":0,\"pad\":\"");
        filler.append("a".repeat(PersistenceLimits.MAX_JSON_BYTES));
        filler.append("\"}");
        byte[] tooLarge = filler.toString().getBytes(StandardCharsets.UTF_8);
        assertTrue(tooLarge.length > PersistenceLimits.MAX_JSON_BYTES, "前置条件：载荷应超过上限");
        Files.write(file, tooLarge);

        assertRejected(file, tooLarge);
    }

    @Test
    @DisplayName("非常规文件（目录）一律 50000")
    void 非常规文件被拒绝() throws Exception {
        Path directory = tempDir.resolve("progress.json");
        Files.createDirectories(directory);

        BusinessException ex = assertThrows(BusinessException.class,
                () -> VersionedJsonFile.read(directory, ENVELOPE));
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode(), "非常规文件必须是可诊断的 50000");
        assertTrue(Files.isDirectory(directory), "读取失败不得改动该路径");
    }

    @Test
    @DisplayName("信封读取：容器必须是 object，字段集合只认 schema_version 与容器")
    void 信封读取只覆盖信封() throws Exception {
        Path file = tempDir.resolve("daily_stats.json");
        Files.writeString(file, "{\"schema_version\":1,\"buckets\":{\"2026-09-23\":{\"acked\":3}}}",
                StandardCharsets.UTF_8);

        Optional<ObjectNode> root = VersionedJsonFile.readEnvelope(file, "buckets");
        assertTrue(root.isPresent(), "信封合法时必须可读，且不对桶内键名做白名单断言");
        assertEquals(3, root.get().path("buckets").path("2026-09-23").path("acked").asInt());

        for (String invalid : List.of(
                "{\"schema_version\":1,\"buckets\":[]}",
                "{\"schema_version\":1,\"buckets\":3}",
                "{\"schema_version\":1,\"buckets\":{},\"extra\":1}",
                "{\"schema_version\":1}",
                "{\"schema_version\":2,\"buckets\":{}}")) {
            Files.writeString(file, invalid, StandardCharsets.UTF_8);
            BusinessException ex = assertThrows(BusinessException.class,
                    () -> VersionedJsonFile.readEnvelope(file, "buckets"), "非法信封必须被拒绝: " + invalid);
            assertEquals(ErrorCode.SERVER_ERROR, ex.getCode(), "非法信封应回 50000: " + invalid);
            assertEquals(invalid, Files.readString(file, StandardCharsets.UTF_8),
                    "读取失败不得改写文件: " + invalid);
        }
    }

    /** 写入非法内容 → 断言 50000 且字节不变。 */
    private void assertRejectedWithoutRewrite(String invalid) throws Exception {
        Path file = tempDir.resolve("progress.json");
        Files.writeString(file, invalid, StandardCharsets.UTF_8);

        BusinessException ex = assertThrows(BusinessException.class,
                () -> VersionedJsonFile.read(file, ENVELOPE), "非法内容必须被拒绝: " + invalid);
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode(), "非法内容应回 50000: " + invalid);
        assertEquals(invalid, Files.readString(file, StandardCharsets.UTF_8),
                "读取失败绝不改写原文件: " + invalid);
    }

    /** 断言给定字节被拒绝且字节不变。 */
    private void assertRejected(Path file, byte[] original) throws Exception {
        BusinessException ex = assertThrows(BusinessException.class,
                () -> VersionedJsonFile.read(file, ENVELOPE));
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode());
        assertEquals(original.length, Files.size(file), "读取失败不得改写原文件");
        assertArrayEquals(original, Files.readAllBytes(file), "读取失败不得改写原文件字节");
    }
}
