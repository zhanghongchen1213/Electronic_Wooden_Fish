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
 * {@code daily_stats.json} 的**文件级信封**基线与原子整体替换。
 *
 * <p><b>本 Story 只建立信封：</b>文件形如 {@code {"schema_version": 1, "buckets": { ... }}}。
 * 契约 §11 该行的字段列为「无」，因此**按日桶的键名与桶值字段都不在本 Story 冻结**；Story 5.3
 * 冻结时必须显式说明或递增 {@code schema_version}。本 Story 的读取校验**只覆盖信封**
 * （{@code schema_version}、容器为 object、JSON 完整性、字节上限），刻意**不**对桶内键名做白名单
 * 断言——那会成为 Story 5.3 之外的第二真源。
 *
 * <p><b>迁移说明（契约 §11）：</b>日界以 backend 配置时区为准，不按设备本地时间切分。
 *
 * <p><b>派生面：</b>本文件由已确认增量派生，属派生文件。它缺失或损坏时**不得**改写
 * {@code progress.json}、**不得**改变权威高水位。
 *
 * <p><b>本 Story 不做：</b>历史统计派生、日界归档与轮次完成确认（Epic 5）；本类只提供信封的
 * 严格读取与整体替换，不实现任何日期计算。
 */
@Service
public class DailyStatsStore {

    /** 契约 §11 冻结的文件名（{@code app.data-dir} 下）。 */
    public static final String FILE_NAME = "daily_stats.json";

    /**
     * 容纳按日桶的容器字段名。契约 §11 只冻结「一个 object 容器」这一事实，未冻结容器键名，
     * 故本常量不是契约真源，仅是本 Story 的实现取定。
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
     * @return 契约 §11 的承载字段列。契约把该文件的字段列记为「无」（按日桶由 Story 5.3 冻结），
     *         因此这里返回空集合；它参与 §11.1「五份文件字段并集 = backend 作用域字段集合」的比对。
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
     * 原子整体替换：payload 是**容器内容**（按日桶），本方法负责补上 {@code schema_version} 信封。
     * 不对桶内键名做白名单断言——桶结构属 Story 5.3。
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
