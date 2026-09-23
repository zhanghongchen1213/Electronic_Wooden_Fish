package top.zhcmqtt.ewf.backend.common.persistence;

import java.util.HashSet;
import java.util.Set;

/**
 * JSON 原子持久化基线的集中常量。
 *
 * <p>本类是 schema 版本、文件字节上限与临时文件命名规则的**唯一**出处；各 Store 不得再写字面量
 * 副本，否则会出现第二真源（见 {@code docs/contracts/sync-contract.md} §11 的单一真源口径）。
 *
 * <p><b>字节上限裁决：</b>{@link #MAX_JSON_BYTES} 为 256 KiB，对 {@code app.data-dir} 下全部
 * 契约文件统一适用，并在**解析之前**用 {@link java.nio.file.Files#size} 检查，超限即
 * {@code 50000} fail closed，不解析、不落盘。若 {@code daily_stats.json} 的按日桶长期增长逼近
 * 上限，属 Story 5.3 的议题（可为其单独提高上限或改变分片策略），届时必须在**本类内**显式记录
 * 并更新该裁决，不得在 Store 内另设上限。
 *
 * <p><b>临时文件命名：</b>同目录 {@code .<文件名去扩展名>-<随机串>.tmp}。该形状与 Story 4.4
 * 已上线的 {@code .identity-*.tmp} 完全一致，因此提取基元不改变既有落盘字节与残留命名；恢复流程
 * 也据此识别孤儿临时文件（{@link #isTemporaryFileName(String)}）。
 */
public final class PersistenceLimits {

    /** 契约 §11 冻结的 schema 版本；五份文件的 {@code schema_version} 必须逐值等于它。 */
    public static final int SCHEMA_VERSION = 1;

    /** 单文件字节上限（256 KiB），在解析之前检查。 */
    public static final int MAX_JSON_BYTES = 262_144;

    /** 承载 schema 版本的字段名，属于每份文件的信封。 */
    public static final String SCHEMA_VERSION_FIELD = "schema_version";

    /** 临时文件后缀。 */
    public static final String TEMP_SUFFIX = ".tmp";

    /** 契约文件名扩展名。 */
    private static final String FILE_EXTENSION = ".json";

    private PersistenceLimits() {
    }

    /**
     * 由目标文件名推导同目录临时文件前缀：{@code progress.json} → {@code .progress-}。
     *
     * @throws IllegalArgumentException 文件名不以 {@code .json} 结尾
     */
    public static String tempPrefix(String fileName) {
        if (fileName == null || !fileName.endsWith(FILE_EXTENSION)
                || fileName.length() <= FILE_EXTENSION.length()) {
            throw new IllegalArgumentException("临时文件命名规则只适用于 .json 契约文件");
        }
        return "." + fileName.substring(0, fileName.length() - FILE_EXTENSION.length()) + "-";
    }

    /** 是否是本基线产生的临时文件（孤儿 `.tmp` 的识别规则，仅用于 best-effort 清理与日志）。 */
    public static boolean isTemporaryFileName(String fileName) {
        return fileName != null && fileName.startsWith(".") && fileName.endsWith(TEMP_SUFFIX)
                && fileName.length() > TEMP_SUFFIX.length() + 1;
    }

    /**
     * 把契约 §11 的承载字段列补全为严格读取面的白名单：契约字段列 + {@code schema_version}。
     *
     * <p>契约文件表的「字段」列只列承载字段，`schema_version` 是信封而非承载事实，故不在契约列内；
     * 读取面必须同时接受两者。
     */
    public static Set<String> envelopeFields(Set<String> payloadFields) {
        Set<String> fields = new HashSet<>(payloadFields);
        fields.add(SCHEMA_VERSION_FIELD);
        return Set.copyOf(fields);
    }
}
