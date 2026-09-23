package top.zhcmqtt.ewf.backend.common.security;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.time.Instant;
import java.util.Date;

import javax.crypto.SecretKey;

import org.springframework.stereotype.Component;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.ExpiredJwtException;
import io.jsonwebtoken.JwtException;
import io.jsonwebtoken.Jws;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;
import top.zhcmqtt.ewf.backend.common.config.JwtProperties;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;

/** JJWT 0.12.6 provider；access/refresh 通过 type claim 严格分离。 */
@Component
public class JwtTokenProvider {

    private final JwtProperties properties;
    private final SecretKey key;

    public JwtTokenProvider(JwtProperties properties) {
        this.properties = properties;
        this.key = validKey(properties.getSecret())
                ? Keys.hmacShaKeyFor(properties.getSecret().getBytes(StandardCharsets.UTF_8))
                : null;
    }

    public TokenPair issueTokenPair(String deviceId) {
        if (key == null || deviceId == null || deviceId.isBlank()) {
            throw BusinessException.upstreamUnavailable("令牌配置未就绪，请稍后重试");
        }
        long now = System.currentTimeMillis();
        String access = issue(deviceId, "access", now, properties.getAccessTokenExpiration());
        String refresh = issue(deviceId, "refresh", now, properties.getRefreshTokenExpiration());
        return new TokenPair(access, refresh, Math.max(1, properties.getAccessTokenExpiration() / 1000));
    }

    public Claims parseAccess(String token) {
        return parse(token, "access");
    }

    public Claims parseRefresh(String token) {
        return parse(token, "refresh");
    }

    private String issue(String deviceId, String type, long now, long expirationMillis) {
        if (expirationMillis <= 0) {
            throw BusinessException.upstreamUnavailable("令牌过期时间配置无效，请稍后重试");
        }
        Instant issuedAt = Instant.ofEpochMilli(now);
        return Jwts.builder()
                .subject(deviceId)
                .claim("type", type)
                .issuedAt(Date.from(issuedAt))
                .expiration(new Date(now + expirationMillis))
                .signWith(key, Jwts.SIG.HS256)
                .compact();
    }

    private Claims parse(String token, String expectedType) {
        if (key == null || token == null || token.isBlank()) {
            throw BusinessException.tokenInvalid("令牌无效，请重新登录");
        }
        try {
            Jws<Claims> signed = Jwts.parser()
                    .verifyWith(key)
                    .build()
                    .parseSignedClaims(token);
            if (!Jwts.SIG.HS256.getId().equals(signed.getHeader().getAlgorithm())) {
                throw BusinessException.tokenInvalid("令牌签名算法无效，请重新登录");
            }
            Claims claims = signed.getPayload();
            if (!expectedType.equals(claims.get("type", String.class))
                    || claims.getSubject() == null || claims.getSubject().isBlank()
                    || claims.getIssuedAt() == null || claims.getExpiration() == null) {
                throw BusinessException.tokenInvalid("令牌类型或内容无效，请重新登录");
            }
            return claims;
        } catch (BusinessException ex) {
            throw ex;
        } catch (ExpiredJwtException ex) {
            throw BusinessException.unauthorized("令牌已过期，请重新登录");
        } catch (JwtException | IllegalArgumentException ex) {
            throw BusinessException.tokenInvalid("令牌无效，请重新登录");
        }
    }

    private static boolean validKey(String secret) {
        if (secret == null || secret.isBlank()) {
            return false;
        }
        String lower = secret.toLowerCase(java.util.Locale.ROOT);
        if (lower.contains("replace-with") || lower.contains("changeme") || lower.contains("your")) {
            return false;
        }
        return secret.getBytes(StandardCharsets.UTF_8).length >= 32;
    }

    public record TokenPair(String accessToken, String refreshToken, long expiresIn) {
    }
}
