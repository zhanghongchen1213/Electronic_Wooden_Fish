package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.test.web.client.MockRestServiceServer;
import org.springframework.web.client.RestTemplate;

import ch.qos.logback.classic.Level;
import ch.qos.logback.classic.Logger;
import ch.qos.logback.classic.spi.ILoggingEvent;
import ch.qos.logback.core.read.ListAppender;

import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.client.WechatMiniClient;
import top.zhcmqtt.ewf.backend.common.config.WechatProperties;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;

import static org.springframework.test.web.client.match.MockRestRequestMatchers.method;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.header;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.queryParam;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

class WechatMiniClientTest {

    private RestTemplate restTemplate;
    private MockRestServiceServer server;
    private WechatMiniClient client;

    @BeforeEach
    void setUp() {
        restTemplate = new RestTemplate();
        server = MockRestServiceServer.bindTo(restTemplate).build();
        WechatProperties properties = new WechatProperties();
        properties.setAppId("wx-test-app");
        properties.setAppSecret("test-secret");
        properties.setEndpoint("https://api.weixin.qq.com/sns/jscode2session");
        client = new WechatMiniClient(restTemplate, new ObjectMapper(), properties);
    }

    @Test
    @DisplayName("先读 String 再解析，兼容 text/plain JSON 且不返回 session_key")
    void textPlainResponseOnlyReturnsOpenId() {
        server.expect(requestTo(org.hamcrest.Matchers.startsWith("https://api.weixin.qq.com/sns/jscode2session")))
                .andExpect(method(HttpMethod.GET))
                .andExpect(header("User-Agent", "EWF-Backend/1.0 code2Session"))
                .andExpect(queryParam("appid", "wx-test-app"))
                .andExpect(queryParam("secret", "test-secret"))
                .andExpect(queryParam("js_code", "code-1"))
                .andExpect(queryParam("grant_type", "authorization_code"))
                .andRespond(withSuccess("{\"openid\":\"openid-secret\",\"session_key\":\"session-secret\"}",
                        MediaType.TEXT_PLAIN));

        Logger logger = (Logger) LoggerFactory.getLogger(WechatMiniClient.class);
        Level originalLevel = logger.getLevel();
        ListAppender<ILoggingEvent> appender = new ListAppender<>();
        appender.start();
        logger.addAppender(appender);
        // 成功路径只记 DEBUG，若不显式放开级别则 appender 收不到任何记录，下面的断言会恒真。
        logger.setLevel(Level.DEBUG);
        try {
            assertEquals("openid-secret", client.exchangeCode("code-1"));
            server.verify();
            String logs = appender.list.stream().map(ILoggingEvent::getFormattedMessage).reduce("", (a, b) -> a + b);
            assertFalse(logs.isEmpty(), "成功路径必须留下可被观察的日志，否则敏感日志断言无效");
            assertTrue(logs.contains("***"), "成功路径只允许记录脱敏 openid");
            assertFalse(logs.contains("session-secret"), "日志不得出现 session_key");
            assertFalse(logs.contains("openid-secret"), "日志不得出现原始 openid");
        } finally {
            logger.setLevel(originalLevel);
            logger.detachAppender(appender);
        }
    }

    @Test
    @DisplayName("非官方 endpoint 被拒为 50200 且不发出请求，避免 AppSecret 外泄")
    void nonOfficialEndpointRejectedBeforeNetwork() {
        for (String endpoint : List.of(
                "https://evil.example/sns/jscode2session",
                "http://api.weixin.qq.com/sns/jscode2session",
                "https://api.weixin.qq.com/sns/jscode2session?redirect=1",
                "https://api.weixin.qq.com/sns/other")) {
            WechatProperties properties = new WechatProperties();
            properties.setAppId("wx-test-app");
            properties.setAppSecret("test-secret");
            properties.setEndpoint(endpoint);
            WechatMiniClient guarded = new WechatMiniClient(restTemplate, new ObjectMapper(), properties);

            BusinessException ex = assertThrows(BusinessException.class, () -> guarded.exchangeCode("code-1"),
                    endpoint + " 必须被 endpoint 白名单拒绝");
            assertEquals(50200, ex.getCode(), endpoint + " 应映射为 50200");
        }
        server.verify();
    }

    @Test
    @DisplayName("微信 errcode 转换为统一 50200，不透出上游材料")
    void upstreamErrorIsUnified() {
        server.expect(requestTo(org.hamcrest.Matchers.startsWith("https://api.weixin.qq.com/sns/jscode2session")))
                .andRespond(withSuccess("{\"errcode\":40013,\"errmsg\":\"invalid appid\"}",
                        MediaType.APPLICATION_JSON));

        BusinessException ex = assertThrows(BusinessException.class, () -> client.exchangeCode("code-1"));
        assertEquals(50200, ex.getCode());
        assertFalse(ex.getMessage().contains("invalid appid"));
        assertFalse(ex.getMessage().contains("session"));
    }

    @Test
    @DisplayName("空/畸形响应与占位配置 fail closed")
    void malformedAndPlaceholderFailClosed() {
        server.expect(requestTo(org.hamcrest.Matchers.startsWith("https://api.weixin.qq.com/sns/jscode2session")))
                .andRespond(withSuccess("not-json", MediaType.TEXT_PLAIN));
        assertEquals(50200, assertThrows(BusinessException.class, () -> client.exchangeCode("code-1")).getCode());

        WechatProperties placeholder = new WechatProperties();
        placeholder.setAppId("replace-with-ewf-wechat-app-id");
        placeholder.setAppSecret("replace-with-ewf-wechat-app-secret");
        WechatMiniClient noNetwork = new WechatMiniClient(restTemplate, new ObjectMapper(), placeholder);
        BusinessException ex = assertThrows(BusinessException.class, () -> noNetwork.exchangeCode("code-1"));
        assertEquals(50200, ex.getCode());
        assertTrue(ex.getMessage().contains("配置"));
    }

    @Test
    @DisplayName("上游网络失败、空 body 与 JSON null 一律转 50200，不落兜底 500")
    void upstreamFailureAndEmptyBodyUnified() {
        server.expect(requestTo(org.hamcrest.Matchers.startsWith("https://api.weixin.qq.com/sns/jscode2session")))
                .andRespond(org.springframework.test.web.client.response.MockRestResponseCreators
                        .withException(new java.io.IOException("connect timed out")));
        assertEquals(50200, assertThrows(BusinessException.class, () -> client.exchangeCode("code-1")).getCode(),
                "连接/读取超时等 RestClientException 必须映射为 50200");
        server.reset();

        server.expect(requestTo(org.hamcrest.Matchers.startsWith("https://api.weixin.qq.com/sns/jscode2session")))
                .andRespond(withSuccess("", MediaType.TEXT_PLAIN));
        assertEquals(50200, assertThrows(BusinessException.class, () -> client.exchangeCode("code-1")).getCode(),
                "空 body 必须映射为 50200");
        server.reset();

        server.expect(requestTo(org.hamcrest.Matchers.startsWith("https://api.weixin.qq.com/sns/jscode2session")))
                .andRespond(withSuccess("null", MediaType.APPLICATION_JSON));
        assertEquals(50200, assertThrows(BusinessException.class, () -> client.exchangeCode("code-1")).getCode(),
                "JSON null 必须映射为 50200");
        server.verify();
    }

    @Test
    @DisplayName("code 为空或格式非法时不访问微信")
    void invalidCodeRejectedBeforeNetwork() {
        assertEquals(40000, org.junit.jupiter.api.Assertions.assertThrows(
                BusinessException.class, () -> client.exchangeCode("bad code")).getCode());
    }
}
