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
 * {@code commands.json} 的 schema 基线与原子整体替换。
 *
 * <p><b>迁移说明（契约 §11）：</b>只保留最新修订，旧修订不写回。
 *
 * <p><b>派生面：</b>本文件是派生文件，不是权威来源。它缺失或损坏时**不得**改写
 * {@code progress.json}、**不得**改变权威高水位，也不得使权威读取失败。跨文件写序为
 * 「先权威 {@code progress.json}，后派生文件」；本文件写失败不回滚已应答的权威结果，
 * 下一次幂等重放会收敛。
 *
 * <p><b>去重键：</b>本文件的 {@code action_id} 是**设置命令族**的幂等去重键，与
 * {@code progress.json} 中**篇章动作族**的 {@code action_id} 是不同族，不要合并。
 *
 * <p><b>写路径归属：</b>Story 5.1 同步成功后由 {@code ProgressSyncService} 更新
 * {@code applied_revision}（单调不减），不递增 {@code command_revision}、不改命令载荷。
 * Story 5.4 设置下发由 {@code ProgressSyncService#submitSettings} 递增 {@code command_revision}
 * 并整体替换最新载荷（只留最新）；「待设备应用 / 已生效」由客户端按
 * {@code applied_revision ≥ command_revision} 派生（见 {@code StateSnapshotService#commandApplied}）。
 * 本类只提供严格读取与整体替换。
 */
@Service
public class CommandsStore {

    /** 契约 §11 冻结的文件名（{@code app.data-dir} 下）。 */
    public static final String FILE_NAME = "commands.json";

    /** 契约 §11 冻结的承载字段列。 */
    private static final Set<String> CONTRACT_FIELDS = Set.of(
            "command_revision", "applied_revision", "action_id",
            "volume", "brightness", "timeout");

    private final ObjectMapper objectMapper;
    private final Path dataDirectory;
    private final Path file;

    public CommandsStore(ObjectMapper objectMapper, @Value("${app.data-dir:./data}") String dataDir) {
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

    /** 读取已落盘的命令状态；缺失即「未初始化」，不创建文件、不返回默认修订。 */
    public Optional<ObjectNode> read() {
        return strictRead(file);
    }

    /** 原子整体替换：payload 字段集合必须恰好等于契约字段列。 */
    public void write(ObjectNode payload) {
        Set<String> fields = new HashSet<>();
        payload.fieldNames().forEachRemaining(fields::add);
        if (!fields.equals(CONTRACT_FIELDS)) {
            throw BusinessException.serverError("命令文件字段集合非法，请修复后重试");
        }
        ObjectNode envelope = payload.deepCopy();
        envelope.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        try {
            AtomicJsonFile.write(file, (objectMapper.writeValueAsString(envelope)
                    + System.lineSeparator()).getBytes(StandardCharsets.UTF_8));
        } catch (JsonProcessingException ex) {
            throw BusinessException.serverError("命令文件无法序列化，请稍后重试");
        } catch (IOException ex) {
            throw BusinessException.serverError("命令文件无法保存，请稍后重试");
        }
    }

    /** @return 数据目录（绝对规范路径） */
    public Path dataDirectory() {
        return dataDirectory;
    }

    /** @return 命令文件路径 */
    public Path file() {
        return file;
    }
}
