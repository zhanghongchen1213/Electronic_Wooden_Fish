package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceLimits;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecovery;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecoveryReport;
import top.zhcmqtt.ewf.backend.common.persistence.PersistenceRecoveryReport.Conclusion;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DailyStatsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.IdentityStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;

/**
 * 只读启动恢复与「等价重启」读回的直接单测：{@code @TempDir} + 真实文件系统，不启动 Spring。
 *
 * <p>覆盖 Story 4.5 AC3（只读恢复、不读半写文件、不静默回退）与 AC4（正常重启后状态一致恢复、
 * 文件缺失视为空状态、重复恢复幂等）。
 *
 * <p><b>静态校验无法覆盖的边界（不得写成已验证）：</b>
 * <ul>
 *   <li>「重建 Store 实例读取」只是**进程重启的近似**，不构成独立进程级重启验证。</li>
 *   <li>{@code @TempDir} 下的测试不能证明生产数据目录的权限、挂载与磁盘写缓存配置。</li>
 *   <li>本测试不能证明掉电安全，也不能证明多进程/跨 JVM 场景下的恢复语义。</li>
 * </ul>
 */
class PersistenceRecoveryTest {

    private static final Set<String> CONTRACT_FILE_NAMES = Set.of(
            ProgressStore.fileName(), IdentityStore.fileName(), CommandsStore.fileName(),
            DeviceStateStore.fileName(), DailyStatsStore.fileName());

    @TempDir
    Path tempDir;

    private final ObjectMapper objectMapper = new ObjectMapper();

    @Test
    @DisplayName("数据目录不存在时恢复全程只读，目录仍不存在，五份文件均为缺失")
    void 目录不存在时保持只读() {
        Path absent = tempDir.resolve("data");
        assertFalse(Files.exists(absent), "前置条件：数据目录不应存在");

        PersistenceRecovery recovery = new PersistenceRecovery(absent.toString());
        recovery.run(null);
        PersistenceRecoveryReport report = recovery.recover();

        assertFalse(Files.exists(absent), "只读恢复不得创建数据目录（启动入口与恢复调用都不得建目录）");
        assertFalse(report.dataDirectoryExists(), "恢复开始时数据目录不存在");
        assertFalse(report.hasUnrecoverable(), "目录缺失不是「不可恢复」");
        assertEquals(CONTRACT_FILE_NAMES.size(), report.files().size(), "应逐文件给出五份契约文件的结论");
        for (PersistenceRecoveryReport.FileStatus status : report.files()) {
            assertEquals(Conclusion.MISSING, status.conclusion(), status.fileName() + " 应视为缺失");
        }
    }

    @Test
    @DisplayName("孤儿 .tmp 不进入读取路径：读取取真实文件值，恢复后被 best-effort 清理")
    void 孤儿临时文件不进入读取路径() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        ProgressStore store = new ProgressStore(objectMapper, dataDir.toString());
        store.write(progressPayload(41));
        byte[] realBytes = Files.readAllBytes(store.file());

        // 结构合法但数值不同的中间产物：若被当作数据源，读回的 acked_total 会变成 0。
        ObjectNode orphanPayload = progressPayload(0);
        orphanPayload.put(PersistenceLimits.SCHEMA_VERSION_FIELD, PersistenceLimits.SCHEMA_VERSION);
        Path orphan = dataDir.resolve(PersistenceLimits.tempPrefix(ProgressStore.fileName()) + "1234.tmp");
        Files.writeString(orphan, objectMapper.writeValueAsString(orphanPayload), StandardCharsets.UTF_8);

        // 先断言读取面：此刻孤儿仍在目录中（尚未经过恢复清理），读取必须只认精确文件名。
        assertEquals(41, store.read().orElseThrow().path("acked_total").asInt(),
                "中间产物永不进入读取路径：不得把 .progress-*.tmp 当作数据源，也不得回退到默认高水位 0");

        PersistenceRecoveryReport report = new PersistenceRecovery(dataDir.toString()).recover();

        assertEquals(Conclusion.OK, statusOf(report, ProgressStore.fileName()).conclusion());
        assertFalse(Files.exists(orphan), "孤儿临时文件应被 best-effort 清理（清理失败只记 WARN，不影响启动）");
        assertEquals(41, store.read().orElseThrow().path("acked_total").asInt(),
                "清理后的读取仍必须取 progress.json 的真实值");
        assertArrayEquals(realBytes, Files.readAllBytes(store.file()), "恢复流程不得改写权威文件");
    }

    @Test
    @DisplayName("损坏的 progress.json 被标记为不可恢复，读取一律 50000 且不返回默认高水位")
    void 权威文件损坏时绝不静默回退() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        Path progress = dataDir.resolve(ProgressStore.fileName());
        String corrupt = "{\"schema_version\":2,\"acked_total\":0}";
        Files.writeString(progress, corrupt, StandardCharsets.UTF_8);

        PersistenceRecoveryReport report = new PersistenceRecovery(dataDir.toString()).recover();

        PersistenceRecoveryReport.FileStatus status = statusOf(report, ProgressStore.fileName());
        assertEquals(Conclusion.UNRECOVERABLE, status.conclusion());
        assertTrue(status.failureCategory() != null && !status.failureCategory().isBlank(),
                "不可恢复必须带原因分类");
        assertFalse(status.message().contains(corrupt), "诊断文案不得包含文件内容");
        assertFalse(status.message().contains(dataDir.toString()), "诊断文案不得包含绝对路径");
        assertEquals(corrupt, Files.readString(progress, StandardCharsets.UTF_8), "恢复不得改写损坏文件");

        ProgressStore store = new ProgressStore(objectMapper, dataDir.toString());
        BusinessException ex = assertThrows(BusinessException.class, store::read);
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode(), "损坏权威文件必须 fail closed 为 50000");
        assertNotEquals(0, ex.getCode(), "不得把损坏当作 acked_total=0 之类的默认值");
    }

    @Test
    @DisplayName("派生文件损坏不得改写权威文件、不得改变权威高水位")
    void 派生文件损坏不影响权威() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        ProgressStore progress = new ProgressStore(objectMapper, dataDir.toString());
        progress.write(progressPayload(41));
        byte[] authorityBefore = Files.readAllBytes(progress.file());

        for (String derived : List.of(CommandsStore.fileName(), DeviceStateStore.fileName(),
                DailyStatsStore.fileName())) {
            Files.writeString(dataDir.resolve(derived), "{\"schema_version\":1,\"junk\":true}",
                    StandardCharsets.UTF_8);
        }

        PersistenceRecoveryReport report = new PersistenceRecovery(dataDir.toString()).recover();

        assertEquals(Conclusion.OK, statusOf(report, ProgressStore.fileName()).conclusion(),
                "派生文件损坏不得使权威读取失败");
        for (String derived : List.of(CommandsStore.fileName(), DeviceStateStore.fileName(),
                DailyStatsStore.fileName())) {
            assertEquals(Conclusion.UNRECOVERABLE, statusOf(report, derived).conclusion(),
                    derived + " 应被标记为不可恢复");
        }
        assertEquals(41, progress.read().orElseThrow().path("acked_total").asInt(),
                "恢复与读取只以 progress.json 裁决高水位");
        assertArrayEquals(authorityBefore, Files.readAllBytes(progress.file()), "派生损坏不得改写权威文件");
    }

    @Test
    @DisplayName("超出字节上限的契约文件被标记为不可恢复，且不被解析")
    void 超上限文件被标记为不可恢复() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        Path progress = dataDir.resolve(ProgressStore.fileName());
        writeOversizeFile(progress);

        PersistenceRecoveryReport report = new PersistenceRecovery(dataDir.toString()).recover();

        PersistenceRecoveryReport.FileStatus status = statusOf(report, ProgressStore.fileName());
        assertEquals(Conclusion.UNRECOVERABLE, status.conclusion());
        assertEquals("too_large", status.failureCategory(), "超限必须归到独立的失败原因分类");
    }

    @Test
    @DisplayName("五份文件合法时「等价重启」逐字段读回一致，schema_version 仍为 1")
    void 等价重启后逐字段一致() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        writeAllContractFiles(dataDir);

        PersistenceRecoveryReport report = new PersistenceRecovery(dataDir.toString()).recover();
        assertFalse(report.hasUnrecoverable(), "五份文件均应正常: " + report.files());

        // 重建 Store 实例 = 等价进程重启
        ObjectMapper restarted = new ObjectMapper();
        ObjectNode progress = new ProgressStore(restarted, dataDir.toString()).read().orElseThrow();
        assertPayloadRestored(progressPayload(41), progress, ProgressStore.fileName());

        ObjectNode commands = new CommandsStore(restarted, dataDir.toString()).read().orElseThrow();
        assertPayloadRestored(commandsPayload(), commands, CommandsStore.fileName());

        ObjectNode deviceState = new DeviceStateStore(restarted, dataDir.toString()).read().orElseThrow();
        assertPayloadRestored(deviceStatePayload(), deviceState, DeviceStateStore.fileName());

        ObjectNode buckets = new DailyStatsStore(restarted, dataDir.toString()).read().orElseThrow();
        assertEquals(bucketsPayload().toString(), buckets.toString(),
                DailyStatsStore.fileName() + " 的信封容器必须逐字段一致地恢复");

        String deviceId = new IdentityStore(restarted, dataDir.toString()).readDeviceId();
        assertEquals(IdentityStore.deviceIdForOpenId("openid-a"), deviceId,
                IdentityStore.fileName() + " 必须一致地恢复");
    }

    /**
     * 断言「等价重启」后读回的载荷与写入载荷逐字段一致，且信封 {@code schema_version} 仍为 1。
     *
     * <p>逐字段比较用规范化 JSON 文本，避免 Jackson 数值节点类型差异（{@code LongNode} 与
     * {@code IntNode}）造成的假阴性。
     */
    private static void assertPayloadRestored(ObjectNode expected, ObjectNode actual, String fileName) {
        assertEquals(PersistenceLimits.SCHEMA_VERSION, actual.path(PersistenceLimits.SCHEMA_VERSION_FIELD).asInt(),
                fileName + " 读回后 schema_version 应仍为 " + PersistenceLimits.SCHEMA_VERSION);

        Set<String> actualFields = new TreeSet<>();
        actual.fieldNames().forEachRemaining(actualFields::add);
        Set<String> expectedFields = new TreeSet<>(actualFields);
        expectedFields.remove(PersistenceLimits.SCHEMA_VERSION_FIELD);

        Set<String> payloadFields = new TreeSet<>();
        expected.fieldNames().forEachRemaining(payloadFields::add);
        assertEquals(payloadFields, expectedFields, fileName + " 的承载字段集合应逐字段一致");

        for (String field : payloadFields) {
            assertEquals(expected.get(field).toString(), actual.get(field).toString(),
                    fileName + " 的 " + field + " 应逐字段一致");
        }
    }

    @Test
    @DisplayName("文件缺失视为空状态而非错误：不创建文件、不报错")
    void 文件缺失视为空状态() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));

        assertEquals(Optional.empty(), new ProgressStore(objectMapper, dataDir.toString()).read());
        assertEquals(Optional.empty(), new CommandsStore(objectMapper, dataDir.toString()).read());
        assertEquals(Optional.empty(), new DeviceStateStore(objectMapper, dataDir.toString()).read());
        assertEquals(Optional.empty(), new DailyStatsStore(objectMapper, dataDir.toString()).read());

        assertEquals(0, entryCount(dataDir), "缺失文件不得被读取面创建");
    }

    @Test
    @DisplayName("daily_stats.json 的容器键由外部字面量锚定，不依赖写读同源")
    void 日统计容器键由外部字面量锚定() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        // 手写字面量（不经过 DailyStatsStore.write）锚定落盘形状：容器键名是本 Story 唯一真正落盘
        // 却未被契约冻结的格式元素，一旦漂移，升级后的进程会读不回上一版写下的 daily_stats.json。
        Files.writeString(dataDir.resolve(DailyStatsStore.fileName()),
                "{\"schema_version\":1,\"buckets\":{\"2026-09-23\":{\"acked_total\":128}}}",
                StandardCharsets.UTF_8);

        ObjectNode buckets = new DailyStatsStore(objectMapper, dataDir.toString()).read().orElseThrow();
        assertEquals(128, buckets.path("2026-09-23").path("acked_total").asInt(),
                "外部字面量写下的按日桶必须能被读回");
        assertEquals(Conclusion.OK,
                statusOf(new PersistenceRecovery(dataDir.toString()).recover(), DailyStatsStore.fileName())
                        .conclusion(),
                "外部字面量写下的信封必须被恢复流程判为正常");
    }

    @Test
    @DisplayName("恢复流程无写副作用且重复执行幂等")
    void 恢复无写副作用且幂等() throws Exception {
        Path dataDir = Files.createDirectories(tempDir.resolve("data"));
        ProgressStore progress = new ProgressStore(objectMapper, dataDir.toString());
        progress.write(progressPayload(41));
        Files.writeString(dataDir.resolve(CommandsStore.fileName()), "{\"schema_version\":1,\"junk\":1}",
                StandardCharsets.UTF_8);
        Set<String> before = directoryListing(dataDir);
        byte[] progressBytes = Files.readAllBytes(progress.file());

        PersistenceRecovery recovery = new PersistenceRecovery(dataDir.toString());
        PersistenceRecoveryReport first = recovery.recover();
        PersistenceRecoveryReport second = recovery.recover();

        assertEquals(before, directoryListing(dataDir), "恢复不得创建、删除或改名任何文件");
        assertArrayEquals(progressBytes, Files.readAllBytes(progress.file()));
        assertEquals(first.files(), second.files(), "重复恢复必须得到相同结论");
        assertEquals(Conclusion.UNRECOVERABLE, statusOf(second, CommandsStore.fileName()).conclusion(),
                "损坏文件在重复恢复后仍应被标记");
    }

    private void writeAllContractFiles(Path dataDir) {
        new ProgressStore(objectMapper, dataDir.toString()).write(progressPayload(41));
        new CommandsStore(objectMapper, dataDir.toString()).write(commandsPayload());
        new DeviceStateStore(objectMapper, dataDir.toString()).write(deviceStatePayload());
        new DailyStatsStore(objectMapper, dataDir.toString()).write(bucketsPayload());
        new IdentityStore(objectMapper, dataDir.toString())
                .getOrCreate(IdentityStore.deviceIdForOpenId("openid-a"));
    }

    private ObjectNode progressPayload(long ackedTotal) {
        ObjectNode node = objectMapper.createObjectNode();
        node.put("local_total", 128L);
        node.put("acked_total", ackedTotal);
        node.put("round_id", 3L);
        node.put("round_cursor", 96L);
        node.put("round_state", "in_progress");
        node.put("pending_completion", false);
        node.put("action_id", "sync-20260923-0001");
        return node;
    }

    private ObjectNode commandsPayload() {
        ObjectNode node = objectMapper.createObjectNode();
        node.put("command_revision", 4L);
        node.put("applied_revision", 3L);
        node.put("action_id", "cmd-20260923-0001");
        node.put("volume", 50);
        node.put("brightness", "mid");
        node.put("timeout", 15);
        return node;
    }

    private ObjectNode deviceStatePayload() {
        ObjectNode node = objectMapper.createObjectNode();
        node.put("battery_percent", 76);
        node.put("network_mode", "connected");
        node.put("audio_config_version", 1L);
        node.put("firmware_version", "1.0.0");
        return node;
    }

    private ObjectNode bucketsPayload() {
        ObjectNode node = objectMapper.createObjectNode();
        node.putObject("2026-09-23").put("acked_total", 128L);
        return node;
    }

    private static PersistenceRecoveryReport.FileStatus statusOf(
            PersistenceRecoveryReport report, String fileName) {
        Optional<PersistenceRecoveryReport.FileStatus> status = report.statusOf(fileName);
        assertTrue(status.isPresent(), "报告缺少 " + fileName);
        assertNotNull(status.get().conclusion());
        return status.get();
    }

    private static Set<String> directoryListing(Path directory) throws IOException {
        try (var entries = Files.list(directory)) {
            Set<String> names = new TreeSet<>();
            entries.forEach(path -> names.add(path.getFileName().toString()));
            return names;
        }
    }

    private static long entryCount(Path directory) throws IOException {
        try (var entries = Files.list(directory)) {
            return entries.count();
        }
    }

    private static void writeOversizeFile(Path file) throws IOException {
        StringBuilder filler = new StringBuilder("{\"schema_version\":1,\"acked_total\":0,\"pad\":\"");
        filler.append("a".repeat(PersistenceLimits.MAX_JSON_BYTES));
        filler.append("\"}");
        Files.writeString(file, filler.toString(), StandardCharsets.UTF_8);
    }
}
