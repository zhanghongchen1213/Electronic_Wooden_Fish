package top.zhcmqtt.ewf.backend.common.persistence;

import java.io.IOException;
import java.nio.charset.MalformedInputException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;

/**
 * 唯一的版本化 JSON 严格读基元（**只读不写**）。
 *
 * <p>读取面统一校验（契约 §11 / Story 4.5 AC2）：文件为常规文件、字节数不超
 * {@link PersistenceLimits#MAX_JSON_BYTES}、UTF-8 可解析、以
 * {@link DeserializationFeature#FAIL_ON_TRAILING_TOKENS} 拒绝尾随内容、根为 JSON object、
 * 顶层字段集合精确匹配、{@code schema_version} 为整数且等于
 * {@link PersistenceLimits#SCHEMA_VERSION}。
 *
 * <p><b>Violation 一律 fail closed：</b>任一项不满足即抛
 * {@link BusinessException#serverError(String)}（{@code 50000}），绝不返回默认值、
 * 绝不改写原文件。错误文案按「不可读取 / 已损坏 / 格式无效 / 超出大小上限」四类区分以便诊断，
 * 且只带文件名，不含文件内容、绝对路径、密钥或任何身份值（该文案会经
 * {@code GlobalExceptionHandler} 透出到响应，故不得包含路径）。
 *
 * <p><b>「缺失」与「损坏」不同：</b>文件不存在返回 {@link Optional#empty()}（按各自语义即
 * 「未初始化 / 空状态」），损坏则抛错。把两者混为一谈会造成「静默回退到默认高水位」。
 *
 * <p>本基元不修改文件系统：不做目录创建、不做清理、不做孤儿临时文件扫描（那些属写路径与恢复编排）。
 */
public final class VersionedJsonFile {

    private static final ObjectMapper READER = new ObjectMapper();

    private VersionedJsonFile() {
    }

    /**
     * 读取字段集合被冻结的契约文件。
     *
     * @param file          目标文件；不存在返回 {@link Optional#empty()}
     * @param allowedFields 允许的**全部**顶层字段（含 {@code schema_version}；用
     *                      {@link PersistenceLimits#envelopeFields(Set)} 由契约字段列补全）
     * @throws BusinessException 校验失败，码 {@code 50000}
     */
    public static Optional<ObjectNode> read(Path file, Set<String> allowedFields) {
        Optional<ObjectNode> root = readObject(file);
        if (root.isEmpty()) {
            return Optional.empty();
        }
        ObjectNode object = root.get();
        if (!fieldNames(object).equals(allowedFields)) {
            throw invalidFormat(file);
        }
        requireSchemaVersion(file, object);
        return Optional.of(object);
    }

    /**
     * 读取只有信封、不冻结承载字段的文件（当前唯一用例是 {@code daily_stats.json}：其按日桶的键与桶值
     * 字段属 Story 5.3，本 Story 只建立 {@code schema_version} + 一个 object 容器）。
     *
     * <p>校验范围**只到信封**：{@code schema_version} 合法、容器为 object、JSON 完整、字节不超限。
     * 刻意不对桶内键名做白名单断言——那会成为 Story 5.3 之外的第二真源。
     *
     * @param containerField 容纳按日桶的容器字段名
     */
    public static Optional<ObjectNode> readEnvelope(Path file, String containerField) {
        Optional<ObjectNode> root = readObject(file);
        if (root.isEmpty()) {
            return Optional.empty();
        }
        ObjectNode object = root.get();
        Set<String> envelope = Set.of(PersistenceLimits.SCHEMA_VERSION_FIELD, containerField);
        if (!fieldNames(object).equals(envelope)) {
            throw invalidFormat(file);
        }
        requireSchemaVersion(file, object);
        if (!object.path(containerField).isObject()) {
            throw invalidFormat(file);
        }
        return Optional.of(object);
    }

    /** 结构层读取：存在性、常规文件、字节上限、UTF-8、尾随内容、根为 object。 */
    private static Optional<ObjectNode> readObject(Path file) {
        // 「确定缺失」才是空状态。{@link Files#exists} 在无法判定存在性时（权限不足无法 stat）同样
        // 返回 false，把它当作「缺失」会让权威文件退化成「未初始化」——正是本类与 AC3 要禁止的静默
        // 回退。因此只用 {@link Files#notExists} 判定缺失，其余不可判定情形一律 fail closed。
        if (Files.notExists(file)) {
            return Optional.empty();
        }
        if (!Files.isRegularFile(file)) {
            throw unreadable(file);
        }
        final long size;
        try {
            size = Files.size(file);
        } catch (IOException ex) {
            throw unreadable(file);
        }
        if (size > PersistenceLimits.MAX_JSON_BYTES) {
            throw tooLarge(file);
        }

        final String content;
        try {
            content = Files.readString(file, StandardCharsets.UTF_8);
        } catch (MalformedInputException ex) {
            throw corrupted(file);
        } catch (IOException ex) {
            throw unreadable(file);
        }

        final JsonNode root;
        try {
            root = READER.reader()
                    .with(DeserializationFeature.FAIL_ON_TRAILING_TOKENS)
                    .readTree(content);
        } catch (JsonProcessingException ex) {
            // 解析失败与尾随内容同属「字节不可信」，与「结构可解析但 schema 不合」区分开
            throw corrupted(file);
        } catch (IOException ex) {
            throw corrupted(file);
        }
        if (root == null || !root.isObject()) {
            throw invalidFormat(file);
        }
        return Optional.of((ObjectNode) root);
    }

    private static void requireSchemaVersion(Path file, ObjectNode object) {
        JsonNode version = object.path(PersistenceLimits.SCHEMA_VERSION_FIELD);
        if (!version.isInt() || version.asInt() != PersistenceLimits.SCHEMA_VERSION) {
            throw invalidFormat(file);
        }
    }

    private static Set<String> fieldNames(ObjectNode object) {
        Set<String> names = new HashSet<>();
        object.fieldNames().forEachRemaining(names::add);
        return names;
    }

    private static String label(Path file) {
        Path fileName = file.getFileName();
        return "「" + (fileName != null ? fileName : file) + "」";
    }

    private static BusinessException unreadable(Path file) {
        return BusinessException.serverError(label(file) + "不可读取，请修复后重试");
    }

    private static BusinessException corrupted(Path file) {
        return BusinessException.serverError(label(file) + "已损坏，请修复后重试");
    }

    private static BusinessException invalidFormat(Path file) {
        return BusinessException.serverError(label(file) + "格式无效，请修复后重试");
    }

    private static BusinessException tooLarge(Path file) {
        return BusinessException.serverError(label(file) + "超出文件大小上限，请修复后重试");
    }
}
