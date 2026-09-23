package top.zhcmqtt.ewf.backend.service;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Base64;
import java.util.Optional;
import java.util.Set;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.persistence.AtomicJsonFile;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;
import top.zhcmqtt.ewf.backend.common.persistence.VersionedJsonFile;

/**
 * 单身份单设备 JSON 映射；只持久化 schema_version 与 device_id。
 *
 * <p><b>契约 §11 迁移说明：</b>身份不可迁移；重建即重新绑定。
 *
 * <p><b>原子写：</b>落盘统一走 {@link AtomicJsonFile}（同目录临时文件 + fsync + 原子 rename +
 * 失败清理），严格读取统一走 {@link VersionedJsonFile}；本类不再内联第二份实现。
 * {@link FileAlreadyExistsException} 仍作为并发建号哨兵。
 *
 * <p><b>并发：</b>本类的读写以方法级 {@code synchronized} 互斥，覆盖单实例内的并发判空-写。
 * 原子写是崩溃保护，不能替代互斥；跨进程/跨 JVM 锁协议不在 AD-16 单实例单进程承诺范围。
 */
@Service
public class IdentityStore {

    /** 契约 §11 冻结的文件名（{@code app.data-dir} 下）。 */
    public static final String FILE_NAME = "identity.json";

    /** 契约 §11 冻结的承载字段列；{@code schema_version} 属信封，不在契约字段列内。 */
    private static final Set<String> CONTRACT_FIELDS = Set.of("device_id");

    private final ObjectMapper objectMapper;
    private final Path dataDirectory;
    private final Path identityFile;

    public IdentityStore(ObjectMapper objectMapper, @Value("${app.data-dir:./data}") String dataDir) {
        this.objectMapper = objectMapper;
        this.dataDirectory = Path.of(dataDir).toAbsolutePath().normalize();
        this.identityFile = this.dataDirectory.resolve(FILE_NAME);
    }

    /** @return 契约 §11 的文件名 */
    public static String fileName() {
        return FILE_NAME;
    }

    /** @return 契约 §11 的承载字段列（不含 {@code schema_version}） */
    public static Set<String> allowedFields() {
        return CONTRACT_FIELDS;
    }

    /** 只读的严格读取：缺失即「未初始化」，损坏或不合法抛 {@code 50000} 且不改写文件。 */
    public static Optional<ObjectNode> strictRead(Path file) {
        return VersionedJsonFile.read(file, PersistenceLimits.envelopeFields(CONTRACT_FIELDS));
    }

    /** 首登创建，已有身份必须匹配；方法级互斥覆盖单实例内并发判空-写。 */
    public synchronized String getOrCreate(String expectedDeviceId) {
        if (expectedDeviceId == null || expectedDeviceId.isBlank()) {
            throw BusinessException.serverError("身份标识不可用，请稍后重试");
        }
        if (Files.exists(identityFile)) {
            String existing = readDeviceId();
            if (!expectedDeviceId.equals(existing)) {
                throw BusinessException.identityMismatch("当前微信身份与设备不匹配，请重新登录");
            }
            return existing;
        }

        try {
            AtomicJsonFile.write(identityFile, identityJson(expectedDeviceId).getBytes(StandardCharsets.UTF_8));
            return expectedDeviceId;
        } catch (FileAlreadyExistsException concurrentCreate) {
            String existing = readDeviceId();
            if (!expectedDeviceId.equals(existing)) {
                throw BusinessException.identityMismatch("当前微信身份与设备不匹配，请重新登录");
            }
            return existing;
        } catch (IOException ex) {
            throw BusinessException.serverError("身份文件无法保存，请稍后重试");
        }
    }

    /** 校验已有身份并返回 device_id；损坏文件绝不静默重建。 */
    public synchronized String readDeviceId() {
        Optional<ObjectNode> root = strictRead(identityFile);
        if (root.isEmpty()) {
            throw BusinessException.serverError("身份文件不可读取，请修复后重试");
        }
        JsonNode deviceId = root.get().path("device_id");
        if (!deviceId.isTextual() || deviceId.asText().isBlank()) {
            throw BusinessException.serverError("身份文件格式无效，请修复后重试");
        }
        return deviceId.asText();
    }

    public Path identityFile() {
        return identityFile;
    }

    public static String deviceIdForOpenId(String openId) {
        if (openId == null || openId.isBlank()) {
            throw BusinessException.upstreamUnavailable("微信身份响应无效，请重试");
        }
        try {
            byte[] digest = MessageDigest.getInstance("SHA-256")
                    .digest(openId.getBytes(StandardCharsets.UTF_8));
            return "ewf-" + Base64.getUrlEncoder().withoutPadding().encodeToString(digest);
        } catch (NoSuchAlgorithmException ex) {
            throw BusinessException.serverError("身份标识计算失败，请稍后重试");
        }
    }

    private String identityJson(String deviceId) throws JsonProcessingException {
        ObjectNode object = objectMapper.createObjectNode();
        object.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        object.put("device_id", deviceId);
        return objectMapper.writeValueAsString(object) + System.lineSeparator();
    }
}
