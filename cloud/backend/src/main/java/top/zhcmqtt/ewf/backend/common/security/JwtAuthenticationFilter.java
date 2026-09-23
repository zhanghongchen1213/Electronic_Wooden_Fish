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

/** 精确 PUBLIC 白名单 + /api/** Bearer 认证过滤器。 */
public class JwtAuthenticationFilter extends OncePerRequestFilter {

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
            if (!path.startsWith("/api/") || isPublic(request, path)) {
                filterChain.doFilter(request, response);
                return;
            }
            String authorization = request.getHeader("Authorization");
            if (authorization == null || !authorization.startsWith("Bearer ")
                    || authorization.substring("Bearer ".length()).isBlank()) {
                failureWriter.write(response, 40103, "缺少登录令牌，请重新登录");
                return;
            }
            try {
                Claims claims = tokenProvider.parseAccess(authorization.substring("Bearer ".length()).trim());
                if (!identityStore.readDeviceId().equals(claims.getSubject())) {
                    throw BusinessException.identityMismatch("令牌身份与设备不匹配，请重新登录");
                }
                UserContext.set(claims.getSubject());
                filterChain.doFilter(request, response);
            } catch (BusinessException ex) {
                failureWriter.write(response, ex.getCode(), ex.getMessage());
            }
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
}
