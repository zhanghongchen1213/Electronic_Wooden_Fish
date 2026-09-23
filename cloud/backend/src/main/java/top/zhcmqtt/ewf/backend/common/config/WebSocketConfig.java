package top.zhcmqtt.ewf.backend.common.config;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;

import top.zhcmqtt.ewf.backend.ws.SyncWebSocketHandler;
import top.zhcmqtt.ewf.backend.ws.WsHandshakeInterceptor;

/**
 * 原生 WebSocket 端点注册（Story 5.5）。
 *
 * <p><b>裁决 A（传输栈）：</b>推荐原生 WebSocket（{@code spring-boot-starter-websocket} +
 * {@link org.springframework.web.socket.handler.TextWebSocketHandler}），不用 STOMP/SockJS。
 * 理由：契约帧是自描述 JSON + {@code type} 判别；小程序 {@code wx.connectSocket}；STOMP 会引入第二协议表面。
 *
 * <p><b>裁决 D：</b>注册路径 = {@code /api/v1/ws}（契约 {@code path_suffix=/ws} 接在 REST base
 * {@code /api/v1} 之后）。
 *
 * <p><b>CORS：</b>禁止 {@code setAllowedOrigins("*")} 无脑放开到生产语义。本地/测试用显式 origin 列表
 *（含空 origin 的非浏览器客户端与 {@code localhost}）；生产反代同源升级不依赖通配。
 */
@Configuration
@EnableWebSocket
public class WebSocketConfig implements WebSocketConfigurer {

    /** 契约 path：REST base + {@code /ws}。 */
    public static final String WS_PATH = "/api/v1/ws";

    private final SyncWebSocketHandler syncWebSocketHandler;
    private final WsHandshakeInterceptor handshakeInterceptor;

    public WebSocketConfig(SyncWebSocketHandler syncWebSocketHandler,
            WsHandshakeInterceptor handshakeInterceptor) {
        this.syncWebSocketHandler = syncWebSocketHandler;
        this.handshakeInterceptor = handshakeInterceptor;
    }

    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
        registry.addHandler(syncWebSocketHandler, WS_PATH)
                .addInterceptors(handshakeInterceptor)
                // 显式 origin 模式：允许本机任意端口（本地联调 / 随机端口测试）；禁止 "*" 通配生产语义
                .setAllowedOriginPatterns(
                        "http://localhost:*",
                        "http://127.0.0.1:*");
    }
}
