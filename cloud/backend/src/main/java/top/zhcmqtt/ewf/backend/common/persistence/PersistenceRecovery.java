package top.zhcmqtt.ewf.backend.common.persistence;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.stereotype.Component;

import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecoveryReport.Conclusion;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecoveryReport.FileStatus;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.IdentityStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;

/**
 * 只读的启动恢复：逐文件校验契约 §11 的五份文件并输出恢复报告（Story 4.5 AC3）。
 *
 * <p><b>只读硬约束（AC3）：</b>本流程**绝不**创建数据目录、**绝不**创建文件、**绝不**改写既有文件。
 * 数据目录不存在时五份文件一律视为缺失、正常启动，且**目录仍不存在**。理由：{@code EnvelopeContractTest}
 * 是全上下文测试且未覆盖 {@code app.data-dir}（使用默认 {@code ./data}），任何在启动期建目录的实现
 * 都会在 {@code cloud/backend/} 下留下未跟踪的 {@code data/}，污染工作区。
 *
 * <p><b>中间产物永不进入读取路径：</b>恢复只读取契约 §11 的**精确文件名**，绝不按 {@code *.json}
 * 通配或前缀扫描；孤儿临时文件（{@link PersistenceLimits#isTemporaryFileName(String)}）不被解析、
 * 不被当作数据源，其清理是 best-effort，删除失败只记 WARN，不影响启动。
 *
 * <p><b>绝不静默回退：</b>校验失败的文件被标记为不可恢复，读取一律 {@code 50000}；本流程不抛异常
 * 中断 Spring 启动，也不改写或删除损坏文件。派生文件（{@code commands.json}/
 * {@code device_state.json}/{@code daily_stats.json}）损坏**不得**改写 {@code progress.json}、
 * **不得**改变权威高水位——恢复对权威只做读取判定，不做任何写入。
 *
 * <p><b>跨文件写序（架构 Deferred A-2 指派本 Story 定序）：</b>{@code progress.json} 是唯一权威
 * 单事务组；其余三份是派生/镜像。同一次操作需写多个文件时先写权威、再写派生；派生写失败不回滚权威
 * （幂等重放会收敛）。恢复与读取只以 {@code progress.json} 裁决高水位。本 Story 只冻结该顺序，
 * 不实现业务写入路径。
 *
 * <p><b>可观测性边界：</b>恢复状态不经 HTTP 暴露（不新增端点）。若未来需要对外可观测，属 4.6/Epic 5
 * 且需先评审契约（{@code cloud/AGENTS.md} §6「先更新契约样例和测试，再修改实现」）。
 */
@Component
public class PersistenceRecovery implements ApplicationRunner {

    private static final Logger LOG = LoggerFactory.getLogger(PersistenceRecovery.class);

    /** 原因分类：文件不可读取（非常规文件，或读取期间发生 IO 失败）。 */
    private static final String CATEGORY_UNREADABLE = "unreadable";

    /** 原因分类：字节数超出 {@link PersistenceLimits#MAX_JSON_BYTES}。 */
    private static final String CATEGORY_TOO_LARGE = "too_large";

    /** 原因分类：可解析面失败（解析失败、尾随内容、根非 object、字段集合或 schema_version 不合法）。 */
    private static final String CATEGORY_INVALID_OR_CORRUPTED = "invalid_or_corrupted";

    /** 非业务异常时的固定诊断文案（不透出内部信息）。 */
    private static final String FALLBACK_MESSAGE = "文件校验失败，请修复后重试";

    /**
     * 契约 §11 的五份文件及其严格读取校验，顺序与契约文件表一致。
     *
     * <p>校验函数是各 Store 的静态严格读取入口，**不**在本类复制第二份字段白名单。
     */
    private static final List<ContractFile> CONTRACT_FILES = List.of(
            new ContractFile(ProgressStore.fileName(), ProgressStore::strictRead),
            new ContractFile(IdentityStore.fileName(), IdentityStore::strictRead),
            new ContractFile(CommandsStore.fileName(), CommandsStore::strictRead),
            new ContractFile(DeviceStateStore.fileName(), DeviceStateStore::strictRead),
            new ContractFile(DailyStatsStore.fileName(), DailyStatsStore::strictRead));

    /**
     * 本基线自己产生的临时文件名前缀（由契约文件名机械推导）。
     *
     * <p>清理只覆盖这些前缀：{@link PersistenceLimits#isTemporaryFileName(String)} 的通用形状判定会
     * 匹配数据目录里任何 {@code .<任意名>.tmp}（例如运维手工备份 {@code .progress.tmp}），而启动期
     * best-effort 删除不应越出本基线自己的中间产物。
     */
    private static final Set<String> TEMPORARY_PREFIXES = CONTRACT_FILES.stream()
            .map(contract -> PersistenceLimits.tempPrefix(contract.fileName()))
            .collect(Collectors.toUnmodifiableSet());

    private final Path dataDirectory;

    public PersistenceRecovery(@Value("${app.data-dir:./data}") String dataDir) {
        this.dataDirectory = Path.of(dataDir).toAbsolutePath().normalize();
    }

    @Override
    public void run(ApplicationArguments args) {
        recover();
    }

    /**
     * 执行一次只读恢复并返回报告。幂等：同一数据目录重复调用不产生任何写副作用。
     *
     * @return 五份文件的逐文件结论
     */
    public PersistenceRecoveryReport recover() {
        boolean dataDirectoryExists = Files.isDirectory(dataDirectory);
        List<FileStatus> statuses = new ArrayList<>();
        for (ContractFile contract : CONTRACT_FILES) {
            statuses.add(check(contract));
        }
        cleanOrphanTemporaries(dataDirectoryExists);

        PersistenceRecoveryReport report =
                new PersistenceRecoveryReport(dataDirectoryExists, List.copyOf(statuses));
        logReport(report);
        return report;
    }

    /** 只读校验单个契约文件：缺失 / 正常 / 不可恢复。 */
    private FileStatus check(ContractFile contract) {
        Path file = dataDirectory.resolve(contract.fileName());
        // 只有「确定缺失」才是 MISSING；数据目录不可搜索（权限不足）时 exists 与 notExists 同为
        // false，此时必须交给校验函数按「不可读取」fail closed，而不是在启动日志里报成「缺失」。
        if (Files.notExists(file)) {
            return new FileStatus(contract.fileName(), Conclusion.MISSING, "", "");
        }
        try {
            contract.validator().apply(file);
            return new FileStatus(contract.fileName(), Conclusion.OK, "", "");
        } catch (BusinessException ex) {
            return new FileStatus(
                    contract.fileName(), Conclusion.UNRECOVERABLE, categorize(file), ex.getMessage());
        } catch (RuntimeException ex) {
            // 只读恢复不得中断 Spring 启动：非业务异常同样记为不可恢复，并保留堆栈以便诊断。
            LOG.error("契约文件校验出现非业务异常: dir={}, file={}", dataDirectory, contract.fileName(), ex);
            return new FileStatus(
                    contract.fileName(), Conclusion.UNRECOVERABLE, CATEGORY_UNREADABLE, FALLBACK_MESSAGE);
        }
    }

    /** 由文件的可观察状态推导失败原因分类；不复制严格读取的 schema 校验逻辑。 */
    private static String categorize(Path file) {
        if (!Files.isRegularFile(file)) {
            return CATEGORY_UNREADABLE;
        }
        try {
            if (Files.size(file) > PersistenceLimits.MAX_JSON_BYTES) {
                return CATEGORY_TOO_LARGE;
            }
        } catch (IOException ex) {
            return CATEGORY_UNREADABLE;
        }
        return CATEGORY_INVALID_OR_CORRUPTED;
    }

    /**
     * best-effort 清理孤儿临时文件：只删除、不解析、不作为数据源。失败只记 WARN。
     *
     * <p>仅在数据目录已存在时执行；目录不存在时本流程不创建它（只读硬约束）。
     */
    private void cleanOrphanTemporaries(boolean dataDirectoryExists) {
        if (!dataDirectoryExists) {
            return;
        }
        final List<Path> orphans;
        try (Stream<Path> entries = Files.list(dataDirectory)) {
            orphans = entries
                    .filter(path -> isOwnTemporary(path.getFileName().toString()))
                    .toList();
        } catch (IOException ex) {
            LOG.warn("数据目录不可列举，跳过孤儿临时文件清理: dir={}", dataDirectory, ex);
            return;
        }
        for (Path orphan : orphans) {
            try {
                Files.deleteIfExists(orphan);
                LOG.warn("已清理孤儿临时文件（中间产物不进入读取路径）: dir={}, file={}",
                        dataDirectory, orphan.getFileName());
            } catch (IOException ex) {
                LOG.warn("孤儿临时文件清理失败，不影响启动: dir={}, file={}",
                        dataDirectory, orphan.getFileName(), ex);
            }
        }
    }

    /**
     * 是否为本基线产生的孤儿临时文件：通用临时文件形状 + 契约文件的临时前缀。
     *
     * <p>两者都要满足，删除范围才恰好等于 {@link PersistenceLimits#tempPrefix(String)} 定义的本基线
     * 中间产物，不会误删数据目录里其它来源的 {@code .<名字>.tmp}。
     */
    private static boolean isOwnTemporary(String fileName) {
        return PersistenceLimits.isTemporaryFileName(fileName)
                && TEMPORARY_PREFIXES.stream().anyMatch(fileName::startsWith);
    }

    /** 记录可诊断日志：绝对数据目录路径 + 文件名 + 失败原因分类，不含任何敏感值。 */
    private void logReport(PersistenceRecoveryReport report) {
        for (FileStatus status : report.files()) {
            switch (status.conclusion()) {
                case OK -> LOG.info("契约文件正常: dir={}, file={}", dataDirectory, status.fileName());
                case MISSING -> LOG.info("契约文件缺失（视为未初始化，非错误）: dir={}, file={}",
                        dataDirectory, status.fileName());
                case UNRECOVERABLE -> LOG.error(
                        "契约文件不可恢复，该文件后续读取一律 50000，且不改写原文件: dir={}, file={}, category={}, reason={}",
                        dataDirectory, status.fileName(), status.failureCategory(), status.message());
            }
        }
        LOG.info("持久化启动恢复完成: dir={}, dirExists={}, unrecoverable={}",
                dataDirectory, report.dataDirectoryExists(), report.hasUnrecoverable());
    }

    /** 数据目录（绝对规范路径）。 */
    public Path dataDirectory() {
        return dataDirectory;
    }

    /** 契约文件与其严格读取校验的绑定。 */
    private record ContractFile(String fileName, Function<Path, Optional<ObjectNode>> validator) {
    }
}
