package top.zhcmqtt.ewf.backend.common.persistence;

import java.util.List;
import java.util.Optional;

/**
 * 启动恢复的逐文件结论报告（纯数据，不经 HTTP 暴露）。
 *
 * <p>Story 4.5 AC3 要求恢复流程对契约 §11 的五份文件各输出「缺失 / 正常 / 不可恢复」结论与失败原因
 * 分类。本报告是**只读观测**的结果，不承载任何可写语义；恢复状态不经 HTTP 暴露，若未来需要对外
 * 可观测则属 4.6/Epic 5，且须先评审契约（{@code cloud/AGENTS.md} §6）。
 *
 * @param dataDirectoryExists 恢复开始时数据目录是否存在。只读硬约束下它必须保持恢复前的值——
 *                            目录不存在时恢复流程不得创建它。
 * @param files               五份契约文件的结论，顺序与契约 §11 文件表一致
 */
public record PersistenceRecoveryReport(boolean dataDirectoryExists, List<FileStatus> files) {

    /** 单份文件的恢复结论。 */
    public enum Conclusion {
        /** 文件不存在。按各自语义即「未初始化 / 空状态」，不是错误。 */
        MISSING,
        /** 文件存在且通过该文件的严格读取面校验。 */
        OK,
        /** 文件存在但不可恢复：读取一律 {@code 50000}，且绝不改写成默认值。 */
        UNRECOVERABLE
    }

    /**
     * 单份文件的结论明细。
     *
     * @param fileName        契约文件名（不含目录，日志中的绝对目录由调用方拼接）
     * @param conclusion      缺失 / 正常 / 不可恢复
     * @param failureCategory 不可恢复时的原因分类；正常与缺失时为空串
     * @param message         不可恢复时的可诊断文案；**不含**文件内容、绝对路径、密钥或身份值
     */
    public record FileStatus(String fileName, Conclusion conclusion, String failureCategory, String message) {
    }

    /** @return 该文件的结论；文件名不在报告内时返回空 */
    public Optional<FileStatus> statusOf(String fileName) {
        return files.stream().filter(status -> status.fileName().equals(fileName)).findFirst();
    }

    /** @return 是否存在任何不可恢复的文件 */
    public boolean hasUnrecoverable() {
        return files.stream().anyMatch(status -> status.conclusion() == Conclusion.UNRECOVERABLE);
    }
}
