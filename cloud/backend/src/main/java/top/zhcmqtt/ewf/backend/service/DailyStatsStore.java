package top.zhcmqtt.ewf.backend.service;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
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
 * {@code daily_stats.json} 的文件级信封与原子整体替换。
 *
 * <p><b>信封：</b>文件形如 {@code {"schema_version": 1, "buckets": { ... }}}。契约 §11 该行的字段列
 * 为「无」，故 {@link #allowedFields()} 继续返回空集——参与 §11.1「五份文件字段并集」比对。
 *
 * <h2>裁决 B（Story 5.3）：桶形状（实现冻结，非契约字段列）</h2>
 * <p><b>选择：</b>{@code buckets} 键 = {@code Asia/Shanghai} 的 {@code yyyy-MM-dd}；桶值 object 仅含
 * {@code confirmed_taps}（非负 int）。无敲击日期不写 0 桶。顶层信封仍恰为
 * {@code {schema_version, buckets}}——不得增加顶层字段。
 * <b>理由：</b>契约字段列保持「无」以免破坏 §11.1 并集门禁；桶键/桶值由本 Story 实现冻结并登记 deferred。
 * <b>约束：</b>不得把桶字段塞进 {@link #allowedFields()}；不得改 {@code docs/contracts/**} 字节。
 *
 * <p><b>派生面：</b>本文件由已确认增量派生。缺失或损坏时不得改写 {@code progress.json}、不得改变权威高水位。
 * 日界与归档算法见 {@link HistoryStatsService}；本类只提供信封读写，不实现日期计算。
 *
 * <p><b>本 Story（5.3）交付：</b>桶内容首次由 {@link HistoryStatsService} 写入；查询走
 * {@code GET /api/v1/sync/stats}。
 */
@Service
public class DailyStatsStore {

    /** 契约 §11 冻结的文件名（{@code app.data-dir} 下）。 */
    public static final String FILE_NAME = "daily_stats.json";

    /**
     * 容纳按日桶的容器字段名。契约 §11 只冻结「一个 object 容器」这一事实，未冻结容器键名，
     * 故本常量不是契约真源，仅是实现取定。
     */
    private static final String BUCKETS_FIELD = "buckets";

    private final ObjectMapper objectMapper;
    private final Path dataDirectory;
    private final Path file;

    public DailyStatsStore(ObjectMapper objectMapper, @Value("${app.data-dir:./data}") String dataDir) {
        this.objectMapper = objectMapper;
        this.dataDirectory = Path.of(dataDir).toAbsolutePath().normalize();
        this.file = this.dataDirectory.resolve(FILE_NAME);
    }

    /** @return 契约 §11 的文件名 */
    public static String fileName() {
        return FILE_NAME;
    }

    /**
     * @return 契约 §11 的承载字段列。契约把该文件的字段列记为「无」（按日桶由 Story 5.3 实现冻结、
     *         仍非契约字段列），因此这里返回空集合。
     */
    public static Set<String> allowedFields() {
        return Set.of();
    }

    /** 只读的信封严格读取：缺失即「未初始化」，信封不合法抛 {@code 50000} 且不改写文件。 */
    public static Optional<ObjectNode> strictRead(Path file) {
        return VersionedJsonFile.readEnvelope(file, BUCKETS_FIELD);
    }

    /** 读取已落盘的按日桶容器；缺失即「未初始化」，不创建文件、不返回空默认统计。 */
    public Optional<ObjectNode> read() {
        return strictRead(file).map(root -> (ObjectNode) root.get(BUCKETS_FIELD));
    }

    /**
     * 原子整体替换：payload 是容器内容（按日桶），本方法负责补上 {@code schema_version} 信封。
     * 不对桶内键名做白名单断言——桶结构属 Story 5.3 实现冻结，不进入 {@link #allowedFields()}。
     */
    public void write(ObjectNode buckets) {
        ObjectNode envelope = objectMapper.createObjectNode();
        envelope.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        envelope.set(BUCKETS_FIELD, buckets);
        try {
            AtomicJsonFile.write(file, (objectMapper.writeValueAsString(envelope)
                    + System.lineSeparator()).getBytes(StandardCharsets.UTF_8));
        } catch (JsonProcessingException ex) {
            throw BusinessException.serverError("日统计文件无法序列化，请稍后重试");
        } catch (IOException ex) {
            throw BusinessException.serverError("日统计文件无法保存，请稍后重试");
        }
    }

    /** @return 数据目录（绝对规范路径） */
    public Path dataDirectory() {
        return dataDirectory;
    }

    /** @return 日统计文件路径 */
    public Path file() {
        return file;
    }
}
