package top.zhcmqtt.ewf.backend.service;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.persistence.AtomicJsonFile;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;
import top.zhcmqtt.ewf.backend.common.persistence.VersionedJsonFile;

/**
 * {@code device_state.json} 的 schema 基线与原子整体替换。
 *
 * <p><b>非权威镜像：</b>本文件承载设备状态镜像与音频配置版本，属**非权威镜像、可整体重建**——
 * 它不是任何事实的权威来源，不得据此反向覆盖 {@code progress.json} 或设备上报值。它缺失或损坏时
 * 不得使权威读取失败。
 *
 * <p><b>迁移说明（契约 §11）：</b>属非权威镜像，可整体重建。
 *
 * <p><b>本 Story 不做：</b>设备状态上报的接收、校验与镜像更新路径属 Epic 5；本类只提供严格读取
 * 与整体替换。
 */
@Service
public class DeviceStateStore {

    /** 契约 §11 冻结的文件名（{@code app.data-dir} 下）。 */
    public static final String FILE_NAME = "device_state.json";

    /** 契约 §11 冻结的承载字段列。 */
    private static final Set<String> CONTRACT_FIELDS = Set.of(
            "battery_percent", "network_mode", "audio_config_version", "firmware_version");

    private final ObjectMapper objectMapper;
    private final Path dataDirectory;
    private final Path file;

    public DeviceStateStore(ObjectMapper objectMapper, @Value("${app.data-dir:./data}") String dataDir) {
        this.objectMapper = objectMapper;
        this.dataDirectory = Path.of(dataDir).toAbsolutePath().normalize();
        this.file = this.dataDirectory.resolve(FILE_NAME);
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

    /** 读取已落盘的设备状态镜像；缺失即「未初始化」，不创建文件、不返回默认状态。 */
    public Optional<ObjectNode> read() {
        return strictRead(file);
    }

    /** 原子整体替换：payload 字段集合必须恰好等于契约字段列。 */
    public void write(ObjectNode payload) {
        Set<String> fields = new HashSet<>();
        payload.fieldNames().forEachRemaining(fields::add);
        if (!fields.equals(CONTRACT_FIELDS)) {
            throw BusinessException.serverError("设备状态文件字段集合非法，请修复后重试");
        }
        ObjectNode envelope = payload.deepCopy();
        envelope.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        try {
            AtomicJsonFile.write(file, (objectMapper.writeValueAsString(envelope)
                    + System.lineSeparator()).getBytes(StandardCharsets.UTF_8));
        } catch (JsonProcessingException ex) {
            throw BusinessException.serverError("设备状态文件无法序列化，请稍后重试");
        } catch (IOException ex) {
            throw BusinessException.serverError("设备状态文件无法保存，请稍后重试");
        }
    }

    /** @return 数据目录（绝对规范路径） */
    public Path dataDirectory() {
        return dataDirectory;
    }

    /** @return 设备状态文件路径 */
    public Path file() {
        return file;
    }
}
