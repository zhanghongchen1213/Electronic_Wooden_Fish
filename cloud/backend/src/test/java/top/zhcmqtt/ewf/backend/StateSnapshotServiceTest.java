package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeSet;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.service.CommandsStore;
import top.zhcmqtt.ewf.backend.service.DeviceStateStore;
import top.zhcmqtt.ewf.backend.service.ProgressStore;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;

/**
 * 只读装配、{@code snapshot_seq} 导出与恢复基准校验的单元覆盖（不启 Spring 上下文）。
 *
 * <p>测试只写 {@link TempDir}，不创建、读取或修改仓库工作区的 {@code cloud/backend/data/}。
 *
 * <p><b>静态校验无法覆盖的边界：</b>本测试证明单进程内装配与选码行为，不证明多进程/跨 JVM 并发下的
 * 水位一致；「重建 Store 实例」只是重启的**近似**，不构成独立进程级重启验证。
 */
class StateSnapshotServiceTest {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private static final String DEVICE_ID = "ewf-test-device";

    @TempDir
    Path dataDir;

    @Test
    @DisplayName("空状态：未初始化按契约取值域内的缺省表达装配，且查询不创建任何文件")
    void 空状态返回契约缺省表达() {
        Path neverCreated = dataDir.resolve("never-created");
        StateSnapshotService service = serviceOver(neverCreated);

        StateSnapshotResponse snapshot = service.assemble(DEVICE_ID);

        assertEquals(DEVICE_ID, snapshot.deviceId());
        assertEquals(StateSnapshotResponse.NO_COUNT, snapshot.ackedTotal());
        assertEquals(StateSnapshotResponse.NO_COUNT, snapshot.localTotal());
        assertEquals(StateSnapshotResponse.NO_CONFIRMED_ROUND_ID, snapshot.roundId(),
                "round_id 取值域下界为 1，未初始化不得回 0 或伪造更高轮次");
        assertEquals(StateSnapshotResponse.NO_ROUND_STATE, snapshot.roundState());
        assertEquals(StateSnapshotResponse.NO_COUNT, snapshot.roundCursor());
        assertEquals(StateSnapshotResponse.NO_PENDING_COMPLETION, snapshot.pendingCompletion());
        assertEquals(StateSnapshotResponse.NO_COUNT, snapshot.commandRevision());
        assertEquals(StateSnapshotResponse.NO_COUNT, snapshot.appliedRevision());
        assertEquals(StateSnapshotResponse.NO_COUNT, snapshot.snapshotSeq());
        assertEquals(StateSnapshotResponse.NO_BATTERY_PERCENT, snapshot.batteryPercent());
        assertEquals(StateSnapshotResponse.NO_NETWORK_MODE, snapshot.networkMode(),
                "未收到设备上报时不得伪造 connected");
        assertEquals(StateSnapshotResponse.NO_AUDIO_CONFIG_VERSION, snapshot.audioConfigVersion());
        assertEquals(StateSnapshotResponse.NO_FIRMWARE_VERSION, snapshot.firmwareVersion());
        assertEquals(StateSnapshotResponse.DEFAULT_VOLUME, snapshot.volume());
        assertEquals(StateSnapshotResponse.DEFAULT_BRIGHTNESS, snapshot.brightness());
        assertEquals(StateSnapshotResponse.DEFAULT_TIMEOUT, snapshot.timeout());

        assertTrue(Files.notExists(neverCreated), "只读装配不得创建数据目录");
    }

    @Test
    @DisplayName("已初始化：17 字段全部取自对应的权威或镜像文件")
    void 已初始化按来源装配() {
        ProgressStore progressStore = newProgressStore(dataDir);
        progressStore.write(progress(128, 120, 3, 96, "in_progress", false));
        newCommandsStore(dataDir).write(commands(4, 4, 60, "high", 30));
        newDeviceStateStore(dataDir).write(deviceState(76, "connected", 1, "1.0.0"));

        StateSnapshotResponse snapshot = serviceOver(dataDir).assemble(DEVICE_ID);

        assertEquals(120, snapshot.ackedTotal());
        assertEquals(128, snapshot.localTotal());
        assertEquals(3, snapshot.roundId());
        assertEquals("in_progress", snapshot.roundState());
        assertEquals(96, snapshot.roundCursor());
        assertFalse(snapshot.pendingCompletion());
        assertEquals(4, snapshot.commandRevision());
        assertEquals(4, snapshot.appliedRevision());
        assertEquals(120, snapshot.snapshotSeq(), "snapshot_seq 必须等于权威已确认高水位");
        assertEquals(76, snapshot.batteryPercent());
        assertEquals("connected", snapshot.networkMode());
        assertEquals(1, snapshot.audioConfigVersion());
        assertEquals("1.0.0", snapshot.firmwareVersion());
        assertEquals(60, snapshot.volume());
        assertEquals("high", snapshot.brightness());
        assertEquals(30, snapshot.timeout());
    }

    @Test
    @DisplayName("snapshot_seq 由权威高水位导出且不后退，未初始化时取 0")
    void snapshot_seq由权威高水位导出() {
        assertEquals(StateSnapshotResponse.NO_COUNT, StateSnapshotService.exportSnapshotSeq(java.util.Optional.empty()),
                "未初始化时导出 0，而不是落盘一个计数器");
        assertEquals(128, StateSnapshotService.exportSnapshotSeq(java.util.Optional.of(progressNode(128))));

        ProgressStore progressStore = newProgressStore(dataDir);
        StateSnapshotService service = serviceOver(dataDir);
        progressStore.write(progress(128, 120, 3, 96, "in_progress", false));
        assertEquals(120, service.assemble(DEVICE_ID).snapshotSeq());
        progressStore.write(progress(140, 131, 4, 8, "in_progress", false));
        assertEquals(131, service.assemble(DEVICE_ID).snapshotSeq(), "高水位推进后 snapshot 水位必须跟随");
    }

    @Test
    @DisplayName("轮次与高水位只来自 progress.json，派生/镜像文件改动不得影响它们")
    void 轮次与高水位只取自权威文件() {
        ProgressStore progressStore = newProgressStore(dataDir);
        progressStore.write(progress(128, 120, 3, 96, "in_progress", false));
        CommandsStore commandsStore = newCommandsStore(dataDir);
        DeviceStateStore deviceStateStore = newDeviceStateStore(dataDir);
        commandsStore.write(commands(4, 4, 60, "high", 30));
        deviceStateStore.write(deviceState(76, "connected", 1, "1.0.0"));

        StateSnapshotService service = serviceOver(dataDir);
        StateSnapshotResponse before = service.assemble(DEVICE_ID);

        // 篡改派生/镜像文件：命令修订与设备状态变，权威轮次与高水位不得变。
        commandsStore.write(commands(9, 9, 10, "low", 5));
        deviceStateStore.write(deviceState(5, "no_signal", 7, "9.9.9"));
        StateSnapshotResponse after = service.assemble(DEVICE_ID);

        assertEquals(before.ackedTotal(), after.ackedTotal(), "acked_total 不得被派生文件改写");
        assertEquals(before.localTotal(), after.localTotal());
        assertEquals(before.roundId(), after.roundId(), "round_id 不得被派生文件改写");
        assertEquals(before.roundCursor(), after.roundCursor(), "round_cursor 不得被派生文件改写");
        assertEquals(before.roundState(), after.roundState());
        assertEquals(before.snapshotSeq(), after.snapshotSeq(), "snapshot 水位不得被派生文件改写");
        assertEquals(9, after.commandRevision(), "命令修订本身就来自 commands.json");
        assertEquals(9, after.appliedRevision());
        assertEquals(5, after.batteryPercent());
    }

    @Test
    @DisplayName("查询路径严格只读：目录内容与文件字节前后完全一致")
    void 查询路径严格只读() throws IOException {
        ProgressStore progressStore = newProgressStore(dataDir);
        progressStore.write(progress(128, 120, 3, 96, "in_progress", false));
        newCommandsStore(dataDir).write(commands(4, 4, 60, "high", 30));
        newDeviceStateStore(dataDir).write(deviceState(76, "connected", 1, "1.0.0"));

        Map<String, String> before = directoryContent(dataDir);
        StateSnapshotService service = serviceOver(dataDir);
        service.assemble(DEVICE_ID);
        service.resolve(DEVICE_ID, null, null);
        service.resolve(DEVICE_ID, 120, null);
        service.resolve(DEVICE_ID, 121, null);
        service.resolve(DEVICE_ID, 119, null);
        Map<String, String> after = directoryContent(dataDir);

        assertEquals(before, after, "装配与校验路径不得创建、删除或改写数据目录下的任何文件");
        assertEquals(new TreeSet<>(java.util.List.of(
                ProgressStore.fileName(), CommandsStore.fileName(), DeviceStateStore.fileName())),
                new TreeSet<>(after.keySet()),
                "查询路径不得新增第六个后端状态文件");
    }

    @Test
    @DisplayName("恢复基准校验：无基准 20002、越前 20002、落后 20003、一致为成功，且任一分支都携带完整权威基准")
    void 恢复基准选码且始终携带权威基准() {
        newProgressStore(dataDir).write(progress(128, 120, 3, 96, "in_progress", false));
        newCommandsStore(dataDir).write(commands(4, 4, 60, "high", 30));
        newDeviceStateStore(dataDir).write(deviceState(76, "connected", 1, "1.0.0"));
        StateSnapshotService service = serviceOver(dataDir);

        assertOutcome(service.resolve(DEVICE_ID, null, null), ErrorCode.BASELINE_HIGH_WATER_MISSING);
        assertOutcome(service.resolve(DEVICE_ID, null, 121), ErrorCode.BASELINE_HIGH_WATER_MISSING);
        assertOutcome(service.resolve(DEVICE_ID, 121, null), ErrorCode.BASELINE_HIGH_WATER_MISSING);
        assertOutcome(service.resolve(DEVICE_ID, 119, null), ErrorCode.DEVICE_RESET_CONFLICT);
        assertOutcome(service.resolve(DEVICE_ID, 120, 120), ApiResponse.SUCCESS_CODE);
    }

    @Test
    @DisplayName("客户端自报基准只用于比较：任何分支都不改写权威字段")
    void 自报基准只被当作未核实声明() {
        ProgressStore progressStore = newProgressStore(dataDir);
        progressStore.write(progress(128, 120, 3, 96, "in_progress", false));
        String before = readProgressBytes();

        StateSnapshotService service = serviceOver(dataDir);
        service.resolve(DEVICE_ID, 0, null);
        service.resolve(DEVICE_ID, 120, 120);
        service.resolve(DEVICE_ID, 121, 121);
        service.resolve(DEVICE_ID, 999_999, 999_999);

        assertEquals(before, readProgressBytes(), "自报基准绝不得写入 progress.json");

        StateSnapshotResponse snapshot = service.resolve(DEVICE_ID, 999_999, null).snapshot();
        assertEquals(120, snapshot.ackedTotal(), "越前声明不得被采纳为权威高水位");
        assertEquals(3, snapshot.roundId(), "不得把累计高水位盲推到新轮次");
        assertEquals(96, snapshot.roundCursor());
    }

    @Test
    @DisplayName("权威轮次标识落在契约取值域外时返回 20001，并只表达「暂无已确认轮次」")
    void 轮次无法归属返回20001() {
        // round_id 的契约取值域是 integer ≥ 1；0 在域外，此时无法把请求归入任何已确认轮次。
        newProgressStore(dataDir).write(progress(128, 120, 0, 96, "in_progress", false));
        StateSnapshotService service = serviceOver(dataDir);

        StateSnapshotService.SnapshotOutcome outcome = service.resolve(DEVICE_ID, 120, null);

        assertEquals(ErrorCode.ROUND_UNASSIGNED, outcome.code());
        assertFalse(outcome.accepted());
        assertEquals(StateSnapshotResponse.NO_CONFIRMED_ROUND_ID, outcome.snapshot().roundId(),
                "拒绝时也必须给出域内的 round_id，不得回 0");
        assertEquals(StateSnapshotResponse.NO_ROUND_STATE, outcome.snapshot().roundState());
        assertEquals(StateSnapshotResponse.NO_COUNT, outcome.snapshot().roundCursor());
        assertEquals(120, outcome.snapshot().ackedTotal(),
                "拒绝仍须携带权威累计高水位，客户端据此重建本地基准（契约 §12）");
    }

    @Test
    @DisplayName("轮次状态落在契约枚举外同样判为不可归属")
    void 轮次状态域外同样不可归属() {
        newProgressStore(dataDir).write(progress(128, 120, 3, 96, "syncing", false));

        assertEquals(ErrorCode.ROUND_UNASSIGNED, serviceOver(dataDir).resolve(DEVICE_ID, 120, null).code(),
                "round_state 的契约取值域只有 in_progress|completed");
    }

    @Test
    @DisplayName("字段类型不符 fail closed 抛 50000，不静默回退到默认值")
    void 字段类型不符不静默回退() {
        ObjectNode wrongInt = progress(128, 120, 3, 96, "in_progress", false);
        wrongInt.put("acked_total", "120");
        assertTypeMismatchFailsClosed(wrongInt,
                "acked_total 类型错误必须 fail closed，而不是 asInt() 静默给出 0");

        ObjectNode wrongText = progress(128, 120, 3, 96, "in_progress", false);
        wrongText.put("round_state", 3);
        assertTypeMismatchFailsClosed(wrongText,
                "round_state 类型错误必须 fail closed，而不是 asText() 静默给出 \"3\" 并落成越域的轮次状态");

        ObjectNode wrongBoolean = progress(128, 120, 3, 96, "in_progress", false);
        wrongBoolean.put("pending_completion", "false");
        assertTypeMismatchFailsClosed(wrongBoolean,
                "pending_completion 类型错误必须 fail closed，而不是 asBoolean() 静默改值");
    }

    /** 写入一份含类型错误字段的权威文件，断言装配 fail closed 回 50000 且不改写文件。 */
    private void assertTypeMismatchFailsClosed(ObjectNode wrongTyped, String message) {
        newProgressStore(dataDir).write(wrongTyped);

        BusinessException ex = assertThrows(BusinessException.class,
                () -> serviceOver(dataDir).assemble(DEVICE_ID));
        assertEquals(ErrorCode.SERVER_ERROR, ex.getCode(), message);
    }

    @Test
    @DisplayName("已生效判定按契约 §9：applied_revision ≥ command_revision")
    void 已生效判定按契约() {
        assertTrue(StateSnapshotService.commandApplied(4, 4));
        assertTrue(StateSnapshotService.commandApplied(5, 4));
        assertFalse(StateSnapshotService.commandApplied(3, 4));
    }

    private StateSnapshotService serviceOver(Path directory) {
        return new StateSnapshotService(newProgressStore(directory), newCommandsStore(directory),
                newDeviceStateStore(directory));
    }

    private static void assertOutcome(StateSnapshotService.SnapshotOutcome outcome, int expectedCode) {
        assertEquals(expectedCode, outcome.code());
        assertEquals(expectedCode == ApiResponse.SUCCESS_CODE, outcome.accepted());
        assertFalse(outcome.message() == null || outcome.message().isBlank(), "拒绝文案不得为空");
        // 契约 §12：业务拒绝仍携带完整权威基准。
        assertEquals(120, outcome.snapshot().ackedTotal());
        assertEquals(3, outcome.snapshot().roundId());
        assertEquals(120, outcome.snapshot().snapshotSeq());
        assertEquals(DEVICE_ID, outcome.snapshot().deviceId());
    }

    private String readProgressBytes() {
        try {
            return Files.readString(dataDir.resolve(ProgressStore.fileName()), StandardCharsets.UTF_8);
        } catch (IOException ex) {
            throw new IllegalStateException(ex);
        }
    }

    private static Map<String, String> directoryContent(Path directory) throws IOException {
        Map<String, String> content = new LinkedHashMap<>();
        try (var paths = Files.list(directory)) {
            for (Path path : paths.sorted().toList()) {
                content.put(path.getFileName().toString(),
                        Files.readString(path, StandardCharsets.UTF_8));
            }
        }
        return content;
    }

    private static ProgressStore newProgressStore(Path directory) {
        return new ProgressStore(MAPPER, directory.toString());
    }

    private static CommandsStore newCommandsStore(Path directory) {
        return new CommandsStore(MAPPER, directory.toString());
    }

    private static DeviceStateStore newDeviceStateStore(Path directory) {
        return new DeviceStateStore(MAPPER, directory.toString());
    }

    private static ObjectNode progressNode(int ackedTotal) {
        return progress(ackedTotal, ackedTotal, 3, 96, "in_progress", false);
    }

    /** 契约 §11 的 progress.json 单事务组字段列（字段集合必须精确匹配）。 */
    private static ObjectNode progress(int localTotal, int ackedTotal, int roundId, int roundCursor,
            String roundState, boolean pendingCompletion) {
        ObjectNode node = MAPPER.createObjectNode();
        node.put("local_total", localTotal);
        node.put("acked_total", ackedTotal);
        node.put("round_id", roundId);
        node.put("round_cursor", roundCursor);
        node.put("round_state", roundState);
        node.put("pending_completion", pendingCompletion);
        node.put("action_id", "sync-20260923-0001");
        return node;
    }

    private static ObjectNode commands(int commandRevision, int appliedRevision, int volume,
            String brightness, int timeout) {
        ObjectNode node = MAPPER.createObjectNode();
        node.put("command_revision", commandRevision);
        node.put("applied_revision", appliedRevision);
        node.put("action_id", "cmd-20260923-0001");
        node.put("volume", volume);
        node.put("brightness", brightness);
        node.put("timeout", timeout);
        return node;
    }

    private static ObjectNode deviceState(int batteryPercent, String networkMode, int audioConfigVersion,
            String firmwareVersion) {
        ObjectNode node = MAPPER.createObjectNode();
        node.put("battery_percent", batteryPercent);
        node.put("network_mode", networkMode);
        node.put("audio_config_version", audioConfigVersion);
        node.put("firmware_version", firmwareVersion);
        return node;
    }
}
