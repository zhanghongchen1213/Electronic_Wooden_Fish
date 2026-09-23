package top.zhcmqtt.ewf.backend.service;

import java.io.IOException;
import java.nio.channels.FileChannel;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Base64;
import java.util.HashSet;
import java.util.Set;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;

/** 单身份单设备 JSON 映射；只持久化 schema_version 与 device_id。 */
@Service
public class IdentityStore {

    private static final int SCHEMA_VERSION = 1;
    private static final Set<String> ALLOWED_FIELDS = Set.of("schema_version", "device_id");

    private final ObjectMapper objectMapper;
    private final Path dataDirectory;
    private final Path identityFile;

    public IdentityStore(ObjectMapper objectMapper, @Value("${app.data-dir:./data}") String dataDir) {
        this.objectMapper = objectMapper;
        this.dataDirectory = Path.of(dataDir).toAbsolutePath().normalize();
        this.identityFile = this.dataDirectory.resolve("identity.json");
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
            Files.createDirectories(dataDirectory);
            Path temporary = Files.createTempFile(dataDirectory, ".identity-", ".tmp");
            boolean moved = false;
            try {
                byte[] bytes = identityJson(expectedDeviceId).getBytes(StandardCharsets.UTF_8);
                try (FileChannel channel = FileChannel.open(temporary, StandardOpenOption.WRITE)) {
                    ByteBuffer buffer = ByteBuffer.wrap(bytes);
                    while (buffer.hasRemaining()) {
                        channel.write(buffer);
                    }
                    channel.force(true);
                }
                try {
                    Files.move(temporary, identityFile, StandardCopyOption.ATOMIC_MOVE);
                    moved = true;
                } catch (FileAlreadyExistsException concurrentCreate) {
                    String existing = readDeviceId();
                    if (!expectedDeviceId.equals(existing)) {
                        throw BusinessException.identityMismatch("当前微信身份与设备不匹配，请重新登录");
                    }
                    return existing;
                }
                return expectedDeviceId;
            } finally {
                if (!moved) {
                    Files.deleteIfExists(temporary);
                }
            }
        } catch (BusinessException ex) {
            throw ex;
        } catch (IOException ex) {
            throw BusinessException.serverError("身份文件无法保存，请稍后重试");
        }
    }

    /** 校验已有身份并返回 device_id；损坏文件绝不静默重建。 */
    public synchronized String readDeviceId() {
        if (!Files.isRegularFile(identityFile)) {
            throw BusinessException.serverError("身份文件不可读取，请修复后重试");
        }
        try {
            JsonNode root = objectMapper.reader()
                    .with(DeserializationFeature.FAIL_ON_TRAILING_TOKENS)
                    .readTree(Files.readString(identityFile, StandardCharsets.UTF_8));
            if (root == null || !root.isObject() || root.size() != ALLOWED_FIELDS.size()
                    || !fieldsAllowed(root) || !root.path("schema_version").isInt()
                    || root.path("schema_version").asInt(-1) != SCHEMA_VERSION
                    || !root.path("device_id").isTextual() || root.path("device_id").asText().isBlank()) {
                throw BusinessException.serverError("身份文件格式无效，请修复后重试");
            }
            return root.path("device_id").asText();
        } catch (BusinessException ex) {
            throw ex;
        } catch (IOException | RuntimeException ex) {
            throw BusinessException.serverError("身份文件损坏，请重新检查后重试");
        }
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

    private boolean fieldsAllowed(JsonNode root) {
        Set<String> fields = new HashSet<>();
        root.fieldNames().forEachRemaining(fields::add);
        return fields.equals(ALLOWED_FIELDS);
    }

    private String identityJson(String deviceId) throws JsonProcessingException {
        ObjectNode object = objectMapper.createObjectNode();
        object.put("schema_version", SCHEMA_VERSION);
        object.put("device_id", deviceId);
        return objectMapper.writeValueAsString(object) + System.lineSeparator();
    }
}
