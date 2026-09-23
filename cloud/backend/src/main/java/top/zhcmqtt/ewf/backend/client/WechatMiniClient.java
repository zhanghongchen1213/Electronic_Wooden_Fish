package top.zhcmqtt.ewf.backend.client;

import java.net.URI;
import java.util.Locale;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Component;
import org.springframework.web.client.RestClientException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.common.config.WechatProperties;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;

/** 只负责微信小程序 code2Session，绝不把会话材料带出 client。 */
@Component
public class WechatMiniClient {

    private static final Logger log = LoggerFactory.getLogger(WechatMiniClient.class);
    private static final String USER_AGENT = "EWF-Backend/1.0 code2Session";

    private final RestTemplate restTemplate;
    private final ObjectMapper objectMapper;
    private final WechatProperties properties;

    public WechatMiniClient(@Qualifier("wechatDirectRestTemplate") RestTemplate restTemplate,
            ObjectMapper objectMapper, WechatProperties properties) {
        this.restTemplate = restTemplate;
        this.objectMapper = objectMapper;
        this.properties = properties;
    }

    public String exchangeCode(String code) {
        if (code == null || code.isBlank() || code.length() > 512 || !code.matches("[A-Za-z0-9_-]+")) {
            throw BusinessException.paramError("微信 code 无效，请重新登录");
        }
        if (isPlaceholder(properties.getAppId()) || isPlaceholder(properties.getAppSecret())) {
            throw BusinessException.upstreamUnavailable("微信登录配置未就绪，请配置 AppID/AppSecret 后重试");
        }

        URI uri = validatedEndpoint(properties.getEndpoint());
        uri = UriComponentsBuilder.fromUri(uri)
                .queryParam("appid", properties.getAppId())
                .queryParam("secret", properties.getAppSecret())
                .queryParam("js_code", code)
                .queryParam("grant_type", "authorization_code")
                .build()
                .encode()
                .toUri();
        try {
            var request = org.springframework.http.RequestEntity.get(uri)
                    .header("User-Agent", USER_AGENT)
                    .build();
            String body = restTemplate.exchange(request, String.class).getBody();
            if (body == null || body.isBlank()) {
                throw BusinessException.upstreamUnavailable("微信登录响应为空，请重试");
            }
            Code2SessionResponse response = objectMapper.readValue(body, Code2SessionResponse.class);
            if (response == null) {
                throw BusinessException.upstreamUnavailable("微信登录响应无效，请重试");
            }
            String openId = response.getOpenid();
            response.setSessionKey(null);
            Integer errcode = response.getErrcode();
            if (errcode != null && errcode != 0) {
                log.warn("微信 code2Session 失败: errcode={}", errcode);
                String message = errcode == 40013
                        ? "微信 AppID 未对齐，请检查后端、前端与公众平台配置后重试"
                        : "微信登录暂不可用，请重试或重新登录";
                throw BusinessException.upstreamUnavailable(message);
            }
            if (openId == null || openId.isBlank()) {
                throw BusinessException.upstreamUnavailable("微信身份响应无效，请重新登录");
            }
            log.debug("微信 code2Session 成功: openid={}", maskOpenId(openId));
            return openId;
        } catch (BusinessException ex) {
            throw ex;
        } catch (JsonProcessingException ex) {
            throw BusinessException.upstreamUnavailable("微信登录响应无法解析，请重试");
        } catch (RestClientException ex) {
            throw BusinessException.upstreamUnavailable("微信服务暂不可用，请稍后重试");
        }
    }

    private static boolean isPlaceholder(String value) {
        if (value == null || value.isBlank()) {
            return true;
        }
        String lower = value.toLowerCase(Locale.ROOT);
        return lower.contains("replace-with") || lower.contains("your") || lower.contains("changeme")
                || lower.startsWith("wx_your");
    }

    static String maskOpenId(String openId) {
        if (openId == null || openId.length() <= 8) {
            return "***";
        }
        return openId.substring(0, 4) + "***" + openId.substring(openId.length() - 4);
    }

    private static URI validatedEndpoint(String endpoint) {
        try {
            URI uri = URI.create(endpoint);
            if (!"https".equalsIgnoreCase(uri.getScheme())
                    || !"api.weixin.qq.com".equalsIgnoreCase(uri.getHost())
                    || !"/sns/jscode2session".equals(uri.getPath())
                    || uri.getQuery() != null || uri.getFragment() != null) {
                throw new IllegalArgumentException("unexpected endpoint");
            }
            return uri;
        } catch (RuntimeException ex) {
            throw BusinessException.upstreamUnavailable("微信登录配置无效，请检查官方 endpoint 后重试");
        }
    }
}
