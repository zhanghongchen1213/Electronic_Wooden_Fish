package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.service.IdentityStore;

class IdentityStoreTest {

    @TempDir
    Path tempDir;

    @Test
    @DisplayName("首登只创建 schema_version/device_id，且同身份并发幂等")
    void firstLoginAndConcurrentSameIdentity() throws Exception {
        IdentityStore store = new IdentityStore(new ObjectMapper(), tempDir.toString());
        String device = IdentityStore.deviceIdForOpenId("openid-a");
        ExecutorService executor = Executors.newFixedThreadPool(8);
        try {
            List<Callable<String>> tasks = new ArrayList<>();
            for (int i = 0; i < 8; i++) {
                tasks.add(() -> store.getOrCreate(device));
            }
            for (var result : executor.invokeAll(tasks)) {
                assertEquals(device, result.get());
            }
        } finally {
            executor.shutdownNow();
        }
        String raw = Files.readString(tempDir.resolve("identity.json"));
        assertTrue(raw.contains("schema_version"));
        assertTrue(raw.contains("device_id"));
        for (String forbidden : List.of("openid", "session_key", "progress.json", "commands.json")) {
            assertFalse(raw.contains(forbidden), "身份文件不得包含 " + forbidden);
        }
        assertFalse(Files.exists(tempDir.resolve("progress.json")));
        assertFalse(Files.exists(tempDir.resolve("commands.json")));
        IdentityStore restarted = new IdentityStore(new ObjectMapper(), tempDir.toString());
        assertEquals(device, restarted.getOrCreate(device), "重启后同 openid 派生的 device_id 必须保持不变");
    }

    @Test
    @DisplayName("不同身份被拒绝，损坏文件不静默重建")
    void mismatchAndCorruptionFailClosed() throws Exception {
        IdentityStore store = new IdentityStore(new ObjectMapper(), tempDir.toString());
        String first = IdentityStore.deviceIdForOpenId("openid-a");
        String second = IdentityStore.deviceIdForOpenId("openid-b");
        store.getOrCreate(first);
        BusinessException mismatch = assertThrows(BusinessException.class, () -> store.getOrCreate(second));
        assertEquals(40300, mismatch.getCode());
        Files.writeString(tempDir.resolve("identity.json"), "{\"schema_version\":2}");
        BusinessException corrupted = assertThrows(BusinessException.class, store::readDeviceId);
        assertEquals(50000, corrupted.getCode(), "损坏身份文件应回可诊断的 50000，而不是身份不匹配或静默重建");
        assertEquals("{\"schema_version\":2}", Files.readString(tempDir.resolve("identity.json")),
                "读取失败不得改写身份文件");
    }

    @Test
    @DisplayName("身份 schema 白名单拒绝未知字段、缺失字段与错误类型，且不静默重建")
    void identitySchemaWhitelistFailClosed() throws Exception {
        IdentityStore store = new IdentityStore(new ObjectMapper(), tempDir.toString());
        String device = IdentityStore.deviceIdForOpenId("openid-a");

        for (String invalid : List.of(
                "{\"schema_version\":1,\"device_id\":\"" + device + "\",\"extra\":1}",
                "{\"schema_version\":1}",
                "{\"device_id\":\"" + device + "\"}",
                "{\"schema_version\":1,\"device_id\":42}",
                "{\"schema_version\":2,\"device_id\":\"" + device + "\"}",
                "{\"schema_version\":1,\"device_id\":\"\"}")) {
            Files.writeString(tempDir.resolve("identity.json"), invalid);
            BusinessException ex = assertThrows(BusinessException.class, store::readDeviceId,
                    "非法身份文件必须被拒绝: " + invalid);
            assertEquals(50000, ex.getCode(), "非法身份文件应回 50000: " + invalid);
            assertEquals(invalid, Files.readString(tempDir.resolve("identity.json")),
                    "读取失败既不改写也不重建身份文件: " + invalid);
        }
    }

    @Test
    @DisplayName("数据目录不可写时 fail closed 回 50000，且不残留临时文件")
    void unwritableDataDirectoryFailsClosed() throws Exception {
        Path blocked = tempDir.resolve("not-a-directory");
        Files.writeString(blocked, "blocking");
        IdentityStore store = new IdentityStore(new ObjectMapper(), blocked.toString());

        BusinessException ex = assertThrows(BusinessException.class,
                () -> store.getOrCreate(IdentityStore.deviceIdForOpenId("openid-a")));
        assertEquals(50000, ex.getCode(), "身份写失败必须 fail closed 为 50000，不得静默回退");
        assertEquals("blocking", Files.readString(blocked), "失败路径不得改写阻塞路径");
        try (var files = Files.list(tempDir)) {
            assertEquals(0, files.filter(path -> path.getFileName().toString().endsWith(".tmp")).count(),
                    "失败的原子写不得残留 .tmp 文件");
        }
    }
}
