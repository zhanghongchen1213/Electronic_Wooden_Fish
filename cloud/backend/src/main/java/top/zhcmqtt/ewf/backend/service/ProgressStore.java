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
 * {@code progress.json} 的 schema 基线与原子整体替换。
 *
 * <p><b>单事务组约束：</b>{@code local_total}、{@code acked_total}、{@code round_id}、
 * {@code round_cursor}、{@code round_state}、{@code pending_completion} 与篇章动作族的
 * {@code action_id} 是契约 §11 冻结的**同一事务组**，必须整体落同一文件（本文件）并一次原子替换；
 * 掉电恢复后不得出现高水位、轮次与游标不匹配，也不得把它们分散到多个文件。
 *
 * <p><b>权威性：</b>本文件是 cloud 进度的唯一权威载体（AD-2）。派生文件
 * （{@code commands.json}/{@code device_state.json}/{@code daily_stats.json}）缺失或损坏
 * **不得**改写本文件、**不得**改变权威高水位。跨文件写序：同一次操作需写多个文件时，
 * **先写本文件（权威），再写派生文件**；派生文件写失败不回滚本文件（幂等重放会收敛）。
 *
 * <p><b>迁移说明（契约 §11）：</b>新增字段时可省略读取并取默认值；删除或改义字段必须递增
 * {@code schema_version} 并提供一次性重写。
 *
 * <p><b>写路径归属：</b>Story 5.1 落地幂等高水位推进；Story 5.2 落地完成确认、完成锁定与
 * 篇章动作（restart/exit）状态机——均经 {@code ProgressSyncService} 调用本类 {@link #write}
 * 落盘权威事务组。Story 5.3 在确认后挂钩 {@code daily_stats.json} 派生（本类仍不内嵌日期算法）。
 * 本类本身仍只提供严格读取与整体替换，不内嵌业务判定。
 *
 * <p><b>并发：</b>原子写是崩溃保护，不替代单实例内互斥；跨进程/跨 JVM 语义不在 AD-16 承诺范围。
 */
@Service
public class ProgressStore {

    /** 契约 §11 冻结的文件名（{@code app.data-dir} 下）。 */
    public static final String FILE_NAME = "progress.json";

    /** 契约 §11 冻结的承载字段列；{@code schema_version} 属信封，不在契约字段列内。 */
    private static final Set<String> CONTRACT_FIELDS = Set.of(
            "local_total", "acked_total", "round_id", "round_cursor",
            "round_state", "pending_completion", "action_id");

    private final ObjectMapper objectMapper;
    private final Path dataDirectory;
    private final Path file;

    public ProgressStore(ObjectMapper objectMapper, @Value("${app.data-dir:./data}") String dataDir) {
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

    /**
     * 只读的严格读取：文件缺失返回 {@link Optional#empty()}（未初始化），损坏或不合法抛
     * {@code 50000} 且不改写文件。恢复编排与实例读取共用同一校验，避免出现第二套读取面。
     */
    public static Optional<ObjectNode> strictRead(Path file) {
        return VersionedJsonFile.read(file, PersistenceLimits.envelopeFields(CONTRACT_FIELDS));
    }

    /** 读取已落盘的进度；缺失即「未初始化」，不创建文件、不返回默认高水位。 */
    public Optional<ObjectNode> read() {
        return strictRead(file);
    }

    /**
     * 原子整体替换：payload 的字段集合必须恰好等于契约字段列，写入后 {@code schema_version} 恒为
     * {@link PersistenceLimits#SCHEMA_VERSION}。
     */
    public void write(ObjectNode payload) {
        Set<String> fields = new HashSet<>();
        payload.fieldNames().forEachRemaining(fields::add);
        if (!fields.equals(CONTRACT_FIELDS)) {
            throw BusinessException.serverError("进度文件字段集合非法，请修复后重试");
        }
        ObjectNode envelope = payload.deepCopy();
        envelope.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        try {
            AtomicJsonFile.write(file, (objectMapper.writeValueAsString(envelope)
                    + System.lineSeparator()).getBytes(StandardCharsets.UTF_8));
        } catch (JsonProcessingException ex) {
            throw BusinessException.serverError("进度文件无法序列化，请稍后重试");
        } catch (IOException ex) {
            throw BusinessException.serverError("进度文件无法保存，请稍后重试");
        }
    }

    /** @return 数据目录（绝对规范路径） */
    public Path dataDirectory() {
        return dataDirectory;
    }

    /** @return 进度文件路径 */
    public Path file() {
        return file;
    }
}
