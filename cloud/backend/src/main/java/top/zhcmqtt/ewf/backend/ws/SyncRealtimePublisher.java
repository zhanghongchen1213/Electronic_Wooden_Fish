package top.zhcmqtt.ewf.backend.ws;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;

/**
 * 确认后实时推送实现（Story 5.5）：组帧 + 按 {@code device_id} 投递。
 *
 * <p><b>裁决 C（delta 粒度）：</b>每个成功确认且 {@code acked_total} 前进的 report，推送 <b>1</b> 帧
 * {@code delta}，载荷为确认后的权威水位/游标（不是逐物理敲击拆帧）。若一次前进 Δ&gt;1：仍 1 帧承载新水位。
 *
 * <p><b>裁决 B：</b>{@code delta.seq := snapshot.snapshotSeq()}（= {@code acked_total}）；
 * {@code completion}/{@code command_state} 使用会话单调序号。
 *
 * <p><b>裁决 E：</b>推送异常只打中文日志；无连接 = no-op。不做历史帧环形缓冲；恢复真源是 REST 快照。
 *
 * <p>本 Story 不修改契约；样例 {@code ws_snapshot} 中 {@code seq/acked} 偏移不作为实现义务（见 deferred）。
 */
@Component
public class SyncRealtimePublisher implements RealtimePushPort {

    private static final Logger log = LoggerFactory.getLogger(SyncRealtimePublisher.class);

    private final SyncWebSocketHandler handler;
    private final WsFrameFactory frameFactory;

    public SyncRealtimePublisher(SyncWebSocketHandler handler, WsFrameFactory frameFactory) {
        this.handler = handler;
        this.frameFactory = frameFactory;
    }

    @Override
    public void dispatch(String deviceId, StateSnapshotResponse snapshot, boolean delta, boolean completion,
            boolean commandState) {
        if (deviceId == null || snapshot == null) {
            return;
        }
        if (!delta && !completion && !commandState) {
            return;
        }
        if (!handler.hasActiveSession(deviceId)) {
            return;
        }
        try {
            if (delta) {
                String json = frameFactory.delta(snapshot);
                handler.sendToDevice(deviceId, json, snapshot.snapshotSeq());
            }
            if (completion) {
                handler.sendWithNextSessionSeq(deviceId, snapshot.snapshotSeq(),
                        seq -> frameFactory.completion(snapshot, seq));
            }
            if (commandState) {
                handler.sendWithNextSessionSeq(deviceId, snapshot.snapshotSeq(),
                        seq -> frameFactory.commandState(snapshot, seq));
            }
        } catch (RuntimeException ex) {
            log.warn("WebSocket 推送失败 device={}：{}", mask(deviceId), ex.getMessage());
        }
    }

    private static String mask(String deviceId) {
        if (deviceId == null || deviceId.length() < 8) {
            return "***";
        }
        return deviceId.substring(0, 4) + "…" + deviceId.substring(deviceId.length() - 2);
    }
}
