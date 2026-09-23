package top.zhcmqtt.ewf.backend.common.config;

import org.springframework.boot.context.properties.ConfigurationProperties;

/** JWT 签发与校验配置。密钥只从外部配置读取，不在源码中提供可用默认值。 */
@ConfigurationProperties(prefix = "jwt")
public class JwtProperties {

    private String secret;
    private long accessTokenExpiration;
    private long refreshTokenExpiration;

    public String getSecret() {
        return secret;
    }

    public void setSecret(String secret) {
        this.secret = secret;
    }

    public long getAccessTokenExpiration() {
        return accessTokenExpiration;
    }

    public void setAccessTokenExpiration(long accessTokenExpiration) {
        this.accessTokenExpiration = accessTokenExpiration;
    }

    public long getRefreshTokenExpiration() {
        return refreshTokenExpiration;
    }

    public void setRefreshTokenExpiration(long refreshTokenExpiration) {
        this.refreshTokenExpiration = refreshTokenExpiration;
    }
}
