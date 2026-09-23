package top.zhcmqtt.ewf.backend.common.security;

import java.io.IOException;

import org.springframework.web.filter.OncePerRequestFilter;

import io.jsonwebtoken.Claims;
import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.service.IdentityStore;

/**
 * 精确 PUBLIC 白名单 + {@code /api/**} Bearer 认证过滤器。
 *
 * <p><b>Story 5.5 / 裁决 D：</b>对 {@code GET /api/v1/ws} 的 WebSocket <b>升级握手</b>放行到
 * WebSocket 栈（不要求 {@code Authorization: Bearer}），由 {@code WsHandshakeInterceptor}
 * 校验 query {@code token}。判定必须同时满足：路径精确等于 {@code /api/v1/ws}、方法为 GET、
 * 且请求带有 {@code Upgrade: websocket}（或 {@code Connection} 含 upgrade）——
 * <b>禁止</b>把该路径加成「任意 HTTP 方法公开白名单」。
 */
public class JwtAuthenticationFilter extends OncePerRequestFilter {

    private static final String WS_PATH = "/api/v1/ws";

    private final JwtTokenProvider tokenProvider;
    private final AuthFailureWriter failureWriter;
    private final IdentityStore identityStore;

    public JwtAuthenticationFilter(JwtTokenProvider tokenProvider, AuthFailureWriter failureWriter,
            IdentityStore identityStore) {
        this.tokenProvider = tokenProvider;
        this.failureWriter = failureWriter;
        this.identityStore = identityStore;
    }

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain)
            throws ServletException, IOException {
        try {
            String path = request.getRequestURI().substring(request.getContextPath().length());
            if (!path.startsWith("/api/") || isPublic(request, path) || isWebSocketHandshake(request, path)) {
                filterChain.doFilter(request, response);
                return;
            }
            String authorization = request.getHeader("Authorization");
            if (authorization == null || !authorization.startsWith("Bearer ")
                    || authorization.substring("Bearer ".length()).isBlank()) {
                BusinessException missingToken = BusinessException.tokenMissing("缺少登录令牌，请重新登录");
                failureWriter.write(response, missingToken.getCode(), missingToken.getMessage());
                return;
            }
            try {
                Claims claims = tokenProvider.parseAccess(authorization.substring("Bearer ".length()).trim());
                if (!identityStore.readDeviceId().equals(claims.getSubject())) {
                    throw BusinessException.identityMismatch("令牌身份与设备不匹配，请重新登录");
                }
                UserContext.set(claims.getSubject());
            } catch (BusinessException ex) {
                failureWriter.write(response, ex.getCode(), ex.getMessage());
                return;
            }
            filterChain.doFilter(request, response);
        } finally {
            UserContext.clear();
        }
    }

    private static boolean isPublic(HttpServletRequest request, String path) {
        String method = request.getMethod();
        return ("POST".equals(method) && ("/api/v1/auth/login/wechat-mini".equals(path)
                || "/api/v1/auth/refresh".equals(path)))
                || ("GET".equals(method) && ("/api/v1/health".equals(path) || "/".equals(path)));
    }

    /**
     * 仅放行 WebSocket 升级握手到后续栈；普通 GET/POST {@code /api/v1/ws} 仍走 Bearer。
     */
    static boolean isWebSocketHandshake(HttpServletRequest request, String path) {
        if (!"GET".equals(request.getMethod()) || !WS_PATH.equals(path)) {
            return false;
        }
        String upgrade = request.getHeader("Upgrade");
        if (upgrade != null && "websocket".equalsIgnoreCase(upgrade.trim())) {
            return true;
        }
        String connection = request.getHeader("Connection");
        return connection != null && connection.toLowerCase().contains("upgrade");
    }
}
