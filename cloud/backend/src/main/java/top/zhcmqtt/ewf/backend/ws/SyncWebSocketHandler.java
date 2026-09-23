package top.zhcmqtt.ewf.backend.ws;

import java.io.IOException;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.DisposableBean;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import jakarta.annotation.PostConstruct;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;

/**
 * 同步域 WebSocket 会话、心跳与入站处理（Story 5.5）。
 *
 * <p><b>裁决 F（单设备单连接）：</b>会话登记表 {@code device_id → WebSocketSession}（单活跃）；
 * 新连接在 {@link #afterConnectionEstablished} 时关闭同 {@code device_id} 旧连接（关闭码
 * {@link CloseStatus#NORMAL} = 1000）。理由：契约「单一活跃 socket」；双连接会导致双推与序扰乱。
 *
 * <p><b>裁决 G（心跳）：</b>默认 {@code interval_ms=30000} / {@code timeout_ms=10000}（契约默认，
 * 均低于测试上界 45s/15s）；backend 发 {@code heartbeat}，只认入站 {@code heartbeat_ack}；
 * 超时无应答则关闭。生产反代 idle 超时不在本 Story。
 *
 * <p><b>裁决 B（非进度帧序号）：</b>会话内 {@code session_seq = max(last_sent_seq, current_snapshot_seq) + 1}；
 * 重启/重连后不承诺重放——客户端必须先 REST 查询再续订（AC2）。不做历史帧环形缓冲。
 *
 * <p>入站：仅接受 {@code heartbeat_ack}；未知 {@code type} → {@code error} 帧（{@code code=40001}）后关闭
 * （fail-closed）；{@code contract_version} 不一致同样拒绝后关闭。不接受客户端「自报水位」控制帧。
 */
@Component
public class SyncWebSocketHandler extends TextWebSocketHandler implements DisposableBean {

    private static final Logger log = LoggerFactory.getLogger(SyncWebSocketHandler.class);

    /** 契约默认心跳间隔（ms）。 */
    public static final long HEARTBEAT_INTERVAL_MS = 30_000L;

    /** 契约默认心跳超时（ms）。 */
    public static final long HEARTBEAT_TIMEOUT_MS = 10_000L;

    /** 业务拒绝关闭码段起点（RFC 业务段 4000–4999）；未知帧用 4001。 */
    public static final int CLOSE_PROTOCOL = 4001;

    private final WsFrameFactory frameFactory;
    private final ObjectMapper objectMapper;
    private final StateSnapshotService stateSnapshotService;

    private final ConcurrentHashMap<String, SessionState> sessionsByDevice = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<String, String> deviceBySessionId = new ConcurrentHashMap<>();

    private ScheduledExecutorService scheduler;
    private ScheduledFuture<?> heartbeatTick;

    public SyncWebSocketHandler(WsFrameFactory frameFactory, ObjectMapper objectMapper,
            StateSnapshotService stateSnapshotService) {
        this.frameFactory = frameFactory;
        this.objectMapper = objectMapper;
        this.stateSnapshotService = stateSnapshotService;
    }

    @PostConstruct
    void startHeartbeatScheduler() {
        scheduler = Executors.newSingleThreadScheduledExecutor(r -> {
            Thread t = new Thread(r, "ewf-ws-heartbeat");
            t.setDaemon(true);
            return t;
        });
        heartbeatTick = scheduler.scheduleAtFixedRate(this::heartbeatSweep, HEARTBEAT_INTERVAL_MS,
                HEARTBEAT_INTERVAL_MS, TimeUnit.MILLISECONDS);
    }

    @Override
    public void destroy() {
        if (heartbeatTick != null) {
            heartbeatTick.cancel(false);
        }
        if (scheduler != null) {
            scheduler.shutdownNow();
        }
    }

    @Override
    public void afterConnectionEstablished(WebSocketSession session) {
        String deviceId = (String) session.getAttributes().get(WsHandshakeInterceptor.ATTR_DEVICE_ID);
        if (deviceId == null || deviceId.isBlank()) {
            closeQuietly(session, new CloseStatus(CLOSE_PROTOCOL, "缺少设备身份"));
            return;
        }
        SessionState incoming = new SessionState(session, deviceId);
        SessionState previous = sessionsByDevice.put(deviceId, incoming);
        deviceBySessionId.put(session.getId(), deviceId);
        if (previous != null && previous.session.isOpen() && previous.session != session) {
            deviceBySessionId.remove(previous.session.getId(), deviceId);
            closeQuietly(previous.session, CloseStatus.NORMAL);
            log.info("WebSocket 替换同设备旧连接 device={}", maskDevice(deviceId));
        }
        try {
            StateSnapshotResponse snap = stateSnapshotService.assemble(deviceId);
            sendRaw(incoming, frameFactory.snapshot(snap), snap.snapshotSeq());
        } catch (RuntimeException ex) {
            log.warn("WebSocket 连接后快照推送失败 device={}：{}", maskDevice(deviceId), ex.getMessage());
        }
        log.info("WebSocket 已建立 device={}", maskDevice(deviceId));
    }

    @Override
    public void afterConnectionClosed(WebSocketSession session, CloseStatus status) {
        String deviceId = deviceBySessionId.remove(session.getId());
        if (deviceId == null) {
            return;
        }
        sessionsByDevice.computeIfPresent(deviceId, (id, state) -> state.session.getId().equals(session.getId())
                ? null
                : state);
    }

    @Override
    protected void handleTextMessage(WebSocketSession session, TextMessage message) {
        String deviceId = deviceBySessionId.get(session.getId());
        SessionState state = deviceId == null ? null : sessionsByDevice.get(deviceId);
        if (state == null || state.session != session) {
            return;
        }
        JsonNode root;
        try {
            root = objectMapper.readTree(message.getPayload());
        } catch (IOException ex) {
            rejectAndClose(state, ErrorCode.PROTOCOL_FIELD_INVALID, "帧不是合法 JSON");
            return;
        }
        String version = textOrNull(root, "contract_version");
        if (!WsFrameFactory.CONTRACT_VERSION.equals(version)) {
            rejectAndClose(state, ErrorCode.PROTOCOL_FIELD_INVALID, "contract_version 不一致");
            return;
        }
        String type = textOrNull(root, "type");
        if ("heartbeat_ack".equals(type)) {
            state.lastAckAtMs.set(System.currentTimeMillis());
            return;
        }
        // fail-closed：未知 type → error(40001) + 关闭；不接受自报水位控制帧
        rejectAndClose(state, ErrorCode.PROTOCOL_FIELD_INVALID, "不支持的帧类型");
    }

    @Override
    public void handleTransportError(WebSocketSession session, Throwable exception) {
        log.warn("WebSocket 传输异常 session={}：{}", session.getId(), exception.getMessage());
        closeQuietly(session, CloseStatus.SERVER_ERROR);
    }

    /**
     * 向指定设备的活跃会话发送文本帧；无会话则 no-op。
     *
     * @return 是否实际发出
     */
    public boolean sendToDevice(String deviceId, String json, int seq) {
        SessionState state = sessionsByDevice.get(deviceId);
        if (state == null || !state.session.isOpen()) {
            return false;
        }
        return sendRaw(state, json, seq);
    }

    /**
     * 在同一把 {@code sendLock} 下分配会话单调序号并发送，避免与心跳/并发推送交错产生重复 {@code seq}。
     *
     * @param frameBuilder 接收已分配 {@code seq}，返回完整 JSON 文本
     * @return 实际发出的 {@code seq}；无会话或发送失败返回 -1
     */
    public int sendWithNextSessionSeq(String deviceId, int snapshotSeq,
            java.util.function.IntFunction<String> frameBuilder) {
        SessionState state = sessionsByDevice.get(deviceId);
        if (state == null || !state.session.isOpen()) {
            return -1;
        }
        synchronized (state.sendLock) {
            if (!state.session.isOpen()) {
                return -1;
            }
            int seq = Math.max(state.lastSentSeq.get(), snapshotSeq) + 1;
            String json = frameBuilder.apply(seq);
            return sendRawLocked(state, json, seq) ? seq : -1;
        }
    }

    /** 读取会话 last_sent_seq；无会话返回 -1。 */
    public int lastSentSeq(String deviceId) {
        SessionState state = sessionsByDevice.get(deviceId);
        return state == null ? -1 : state.lastSentSeq.get();
    }

    /** 当前是否存在该设备的活跃连接（测试用）。 */
    public boolean hasActiveSession(String deviceId) {
        SessionState state = sessionsByDevice.get(deviceId);
        return state != null && state.session.isOpen();
    }

    /**
     * 为非进度帧分配会话单调序号：{@code max(last_sent, snapshot_seq) + 1}。
     * <p>推送路径应优先使用 {@link #sendWithNextSessionSeq}，避免分配与发送之间的竞态。
     */
    public int nextSessionSeq(String deviceId, int snapshotSeq) {
        SessionState state = sessionsByDevice.get(deviceId);
        if (state == null) {
            return snapshotSeq + 1;
        }
        synchronized (state.sendLock) {
            return Math.max(state.lastSentSeq.get(), snapshotSeq) + 1;
        }
    }

    private void heartbeatSweep() {
        long now = System.currentTimeMillis();
        Iterator<Map.Entry<String, SessionState>> it = sessionsByDevice.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry<String, SessionState> entry = it.next();
            SessionState state = entry.getValue();
            if (!state.session.isOpen()) {
                it.remove();
                deviceBySessionId.remove(state.session.getId(), entry.getKey());
                continue;
            }
            long sinceAck = now - state.lastAckAtMs.get();
            if (sinceAck > HEARTBEAT_INTERVAL_MS + HEARTBEAT_TIMEOUT_MS) {
                log.warn("WebSocket 心跳超时关闭 device={}", maskDevice(entry.getKey()));
                closeQuietly(state.session, CloseStatus.SESSION_NOT_RELIABLE);
                continue;
            }
            if (now - state.lastHeartbeatSentAtMs.get() >= HEARTBEAT_INTERVAL_MS) {
                try {
                    StateSnapshotResponse snap = stateSnapshotService.assemble(entry.getKey());
                    int sent = sendWithNextSessionSeq(entry.getKey(), snap.snapshotSeq(), frameFactory::heartbeat);
                    if (sent >= 0) {
                        state.lastHeartbeatSentAtMs.set(now);
                    }
                } catch (RuntimeException ex) {
                    log.warn("WebSocket 心跳发送失败 device={}：{}", maskDevice(entry.getKey()),
                            ex.getMessage());
                }
            }
        }
    }

    private boolean sendRaw(SessionState state, String json, int seq) {
        synchronized (state.sendLock) {
            return sendRawLocked(state, json, seq);
        }
    }

    /** 调用方已持有 {@code state.sendLock}。 */
    private boolean sendRawLocked(SessionState state, String json, int seq) {
        if (!state.session.isOpen()) {
            return false;
        }
        try {
            state.session.sendMessage(new TextMessage(json));
            state.lastSentSeq.updateAndGet(prev -> Math.max(prev, seq));
            return true;
        } catch (IOException | IllegalStateException ex) {
            log.warn("WebSocket 发送失败 device={}：{}", maskDevice(state.deviceId), ex.getMessage());
            return false;
        }
    }

    private void rejectAndClose(SessionState state, int code, String message) {
        try {
            sendWithNextSessionSeq(state.deviceId, 0, seq -> frameFactory.error(seq, code, message));
        } catch (RuntimeException ignored) {
            // 关闭优先
        }
        closeQuietly(state.session, new CloseStatus(CLOSE_PROTOCOL, "协议错误"));
    }

    private static void closeQuietly(WebSocketSession session, CloseStatus status) {
        try {
            if (session.isOpen()) {
                session.close(status);
            }
        } catch (IOException ignored) {
            // 关闭失败忽略
        }
    }

    private static String textOrNull(JsonNode root, String field) {
        JsonNode node = root.get(field);
        return node == null || !node.isTextual() ? null : node.asText();
    }

    private static String maskDevice(String deviceId) {
        if (deviceId == null || deviceId.length() < 8) {
            return "***";
        }
        return deviceId.substring(0, 4) + "…" + deviceId.substring(deviceId.length() - 2);
    }

    private static final class SessionState {
        private final WebSocketSession session;
        private final String deviceId;
        private final Object sendLock = new Object();
        private final AtomicInteger lastSentSeq = new AtomicInteger(-1);
        private final AtomicLong lastAckAtMs = new AtomicLong(System.currentTimeMillis());
        private final AtomicLong lastHeartbeatSentAtMs = new AtomicLong(System.currentTimeMillis());

        private SessionState(WebSocketSession session, String deviceId) {
            this.session = session;
            this.deviceId = deviceId;
        }
    }
}
