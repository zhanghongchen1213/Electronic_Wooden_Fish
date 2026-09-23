package top.zhcmqtt.ewf.backend.ws;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.http.server.ServletServerHttpRequest;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.WebSocketHandler;
import org.springframework.web.socket.server.HandshakeInterceptor;

import io.jsonwebtoken.Claims;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.security.JwtTokenProvider;
import top.zhcmqtt.ewf.backend.service.IdentityStore;

/**
 * WebSocket 握手鉴权拦截器（Story 5.5）。
 *
 * <p><b>裁决 D（路径与鉴权）：</b>endpoint = {@code /api/v1/ws}；鉴权<b>只</b>经 query {@code token=}
 * （access JWT），不经 {@code Authorization: Bearer}。握手失败时 {@code return false}，
 * <b>不建立会话</b>（推荐：握手期拒绝，避免半开连接再发 error 帧）。
 *
 * <p>成功后把 {@code device_id} 写入 {@link org.springframework.web.socket.WebSocketSession} attributes
 * （键 {@link #ATTR_DEVICE_ID}）。日志禁止写入完整 JWT / {@code session_key}。
 *
 * <p>本 Story 不修改契约；token 传输口径只读引用契约 §10.2 / 注册表 {@code token_transport=query}。
 *
 * <p><b>Story 5.6 裁决（F/WS）：</b>仅加固握手失败日志脱敏；不把 REST 信封塞进 WS。
 * 失败路径只记中文原因摘要，禁止 {@code Authorization} 值、JWT 三段式明文、AppSecret、未脱敏 openid。
 */
@Component
public class WsHandshakeInterceptor implements HandshakeInterceptor {

    /** Session attribute：已校验的设备身份。 */
    public static final String ATTR_DEVICE_ID = "ewf.device_id";

    private static final Logger log = LoggerFactory.getLogger(WsHandshakeInterceptor.class);

    private final JwtTokenProvider tokenProvider;
    private final IdentityStore identityStore;

    public WsHandshakeInterceptor(JwtTokenProvider tokenProvider, IdentityStore identityStore) {
        this.tokenProvider = tokenProvider;
        this.identityStore = identityStore;
    }

    @Override
    public boolean beforeHandshake(ServerHttpRequest request, ServerHttpResponse response,
            WebSocketHandler wsHandler, Map<String, Object> attributes) {
        if (!(request instanceof ServletServerHttpRequest servletRequest)) {
            log.warn("WebSocket 握手拒绝：非 Servlet 请求");
            return false;
        }
        String token = servletRequest.getServletRequest().getParameter("token");
        if (token == null || token.isBlank()) {
            log.warn("WebSocket 握手拒绝：缺少 query token");
            return false;
        }
        try {
            Claims claims = tokenProvider.parseAccess(token.trim());
            String subject = claims.getSubject();
            if (subject == null || subject.isBlank()
                    || !identityStore.readDeviceId().equals(subject)) {
                log.warn("WebSocket 握手拒绝：令牌身份与设备不匹配");
                return false;
            }
            attributes.put(ATTR_DEVICE_ID, subject);
            return true;
        } catch (BusinessException ex) {
            // 不打印 token 正文
            log.warn("WebSocket 握手拒绝：{}", ex.getMessage());
            return false;
        } catch (RuntimeException ex) {
            log.warn("WebSocket 握手拒绝：令牌校验异常");
            return false;
        }
    }

    @Override
    public void afterHandshake(ServerHttpRequest request, ServerHttpResponse response,
            WebSocketHandler wsHandler, Exception exception) {
        // 无额外动作；会话登记在 SyncWebSocketHandler.afterConnectionEstablished。
    }
}
