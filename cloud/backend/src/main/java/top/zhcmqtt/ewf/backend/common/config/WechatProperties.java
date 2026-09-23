package top.zhcmqtt.ewf.backend.common.config;

import org.springframework.boot.context.properties.ConfigurationProperties;

/** 微信小程序 code2Session 配置。正式值由部署环境注入。 */
@ConfigurationProperties(prefix = "wechat.mini")
public class WechatProperties {

    private String appId;
    private String appSecret;
    private String endpoint = "https://api.weixin.qq.com/sns/jscode2session";

    public String getAppId() {
        return appId;
    }

    public void setAppId(String appId) {
        this.appId = appId;
    }

    public String getAppSecret() {
        return appSecret;
    }

    public void setAppSecret(String appSecret) {
        this.appSecret = appSecret;
    }

    public String getEndpoint() {
        return endpoint;
    }

    public void setEndpoint(String endpoint) {
        this.endpoint = endpoint;
    }
}
