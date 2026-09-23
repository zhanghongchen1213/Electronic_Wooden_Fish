package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import top.zhcmqtt.ewf.backend.common.config.JwtProperties;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.security.JwtTokenProvider;

class JwtAuthenticationTest {

    @Test
    @DisplayName("JWT claims 固定包含 sub/type/iat/exp，access 与 refresh 类型隔离")
    void claimsAndTypeIsolation() {
        JwtProperties properties = properties(60_000, 120_000);
        JwtTokenProvider provider = new JwtTokenProvider(properties);
        JwtTokenProvider.TokenPair pair = provider.issueTokenPair("ewf-device");
        assertTrue(pair.accessToken().startsWith("eyJhbGciOiJIUzI1NiJ9."), "JWT 必须使用 HS256");
        assertEquals("ewf-device", provider.parseAccess(pair.accessToken()).getSubject());
        assertEquals("access", provider.parseAccess(pair.accessToken()).get("type", String.class));
        assertEquals("refresh", provider.parseRefresh(pair.refreshToken()).get("type", String.class));
        assertThrows(BusinessException.class, () -> provider.parseAccess(pair.refreshToken()));
    }

    @Test
    @DisplayName("短 secret fail closed，不签发弱密钥 JWT")
    void weakSecretFailsClosed() {
        JwtProperties properties = properties(60_000, 120_000);
        properties.setSecret("too-short");
        JwtTokenProvider provider = new JwtTokenProvider(properties);
        BusinessException ex = assertThrows(BusinessException.class, () -> provider.issueTokenPair("ewf-device"));
        assertEquals(50200, ex.getCode());
    }

    @Test
    @DisplayName("exp 到期返回冻结 40101，而不是接受过期令牌")
    void expiredTokenRejected() throws Exception {
        JwtTokenProvider provider = new JwtTokenProvider(properties(1, 1));
        String token = provider.issueTokenPair("ewf-device").accessToken();
        Thread.sleep(10);
        BusinessException ex = assertThrows(BusinessException.class, () -> provider.parseAccess(token));
        assertEquals(40101, ex.getCode());
    }

    private static JwtProperties properties(long access, long refresh) {
        JwtProperties properties = new JwtProperties();
        properties.setSecret("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        properties.setAccessTokenExpiration(access);
        properties.setRefreshTokenExpiration(refresh);
        return properties;
    }
}
