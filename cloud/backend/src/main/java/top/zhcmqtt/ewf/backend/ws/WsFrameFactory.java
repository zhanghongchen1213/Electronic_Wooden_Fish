package top.zhcmqtt.ewf.backend.ws;

import org.springframework.stereotype.Component;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;

/**
 * WebSocket JSON 文本帧工厂（Story 5.5）。
 *
 * <p><b>裁决 A（传输栈）：</b>原生 WebSocket 文本帧，不用 STOMP/SockJS；帧形状以契约 §3/§10 为准，
 * <b>不是</b> {@code {code,message,data}} 信封。
 *
 * <p><b>裁决 B（序号）：</b>{@code delta.seq :=} 确认后 {@code acked_total}（= 导出 {@code snapshot_seq}）；
 * {@code snapshot} 帧的 {@code seq} 与载荷 {@code snapshot_seq} 均取当前导出值。非进度帧的 {@code seq}
 * 由调用方按会话公式 {@code max(last_sent, snapshot_seq)+1} 传入。
 *
 * <p>本 Story <b>不修改契约</b>：字段闭包只读引用 {@code docs/contracts/sync-contract.schema.json}
 * 的 {@code ws_frames[*].payload_fields}；{@code contract_version} 常量 = {@code SC-1.0.0}。
 */
@Component
public class WsFrameFactory {

    /** 契约 {@code contract_version}；不一致即拒绝该帧（§3）。 */
    public static final String CONTRACT_VERSION = "SC-1.0.0";

    private final ObjectMapper objectMapper;

    public WsFrameFactory(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    public String delta(StateSnapshotResponse snapshot) {
        ObjectNode root = base("delta", snapshot.snapshotSeq());
        root.put("acked_total", snapshot.ackedTotal());
        root.put("local_total", snapshot.localTotal());
        root.put("round_id", snapshot.roundId());
        root.put("round_cursor", snapshot.roundCursor());
        return write(root);
    }

    public String completion(StateSnapshotResponse snapshot, int seq) {
        ObjectNode root = base("completion", seq);
        root.put("round_id", snapshot.roundId());
        root.put("round_state", snapshot.roundState());
        root.put("pending_completion", snapshot.pendingCompletion());
        return write(root);
    }

    public String commandState(StateSnapshotResponse snapshot, int seq) {
        ObjectNode root = base("command_state", seq);
        root.put("command_revision", snapshot.commandRevision());
        root.put("applied_revision", snapshot.appliedRevision());
        root.put("volume", snapshot.volume());
        root.put("brightness", snapshot.brightness());
        root.put("timeout", snapshot.timeout());
        return write(root);
    }

    public String snapshot(StateSnapshotResponse snapshot) {
        int seq = snapshot.snapshotSeq();
        ObjectNode root = base("snapshot", seq);
        root.put("device_id", snapshot.deviceId());
        root.put("acked_total", snapshot.ackedTotal());
        root.put("local_total", snapshot.localTotal());
        root.put("round_id", snapshot.roundId());
        root.put("round_state", snapshot.roundState());
        root.put("round_cursor", snapshot.roundCursor());
        root.put("pending_completion", snapshot.pendingCompletion());
        root.put("command_revision", snapshot.commandRevision());
        root.put("applied_revision", snapshot.appliedRevision());
        root.put("snapshot_seq", seq);
        root.put("battery_percent", snapshot.batteryPercent());
        root.put("network_mode", snapshot.networkMode());
        root.put("audio_config_version", snapshot.audioConfigVersion());
        root.put("firmware_version", snapshot.firmwareVersion());
        return write(root);
    }

    public String heartbeat(int seq) {
        return write(base("heartbeat", seq));
    }

    public String error(int seq, int code, String message) {
        ObjectNode root = base("error", seq);
        ObjectNode error = objectMapper.createObjectNode();
        error.put("code", Integer.toString(code));
        error.put("message", message);
        root.set("error", error);
        return write(root);
    }

    private ObjectNode base(String type, int seq) {
        ObjectNode root = objectMapper.createObjectNode();
        root.put("contract_version", CONTRACT_VERSION);
        root.put("type", type);
        root.put("seq", seq);
        return root;
    }

    private String write(ObjectNode root) {
        try {
            return objectMapper.writeValueAsString(root);
        } catch (Exception ex) {
            throw new IllegalStateException("WebSocket 帧序列化失败", ex);
        }
    }
}
