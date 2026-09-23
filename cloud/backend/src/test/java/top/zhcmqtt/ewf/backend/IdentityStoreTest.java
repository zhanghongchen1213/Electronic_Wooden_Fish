package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
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
        assertNotEquals(40300, corrupted.getCode());
    }
}
