package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;

import java.nio.charset.StandardCharsets;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.MvcResult;
import org.springframework.test.web.servlet.request.MockHttpServletRequestBuilder;

import ch.qos.logback.classic.Level;
import ch.qos.logback.classic.Logger;
import ch.qos.logback.classic.spi.ILoggingEvent;
import ch.qos.logback.core.read.ListAppender;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.common.exception.GlobalExceptionHandler;
import top.zhcmqtt.ewf.backend.support.EnvelopeFailureProbeController;

/**
 * 运行时行为门禁：信封形状、业务错误 HTTP 200 + 业务码、协议错误 HTTP 状态、探活与兜底。
 *
 * <p><b>静态校验无法覆盖的边界（不得让承诺强于实现）：</b>
 * <ul>
 *   <li>「生产代码不存在第二套响应形状」只能靠白盒评审，本测试只钉住被显式请求到的路径。</li>
 *   <li>{@code BusinessException} 是否被所有失败分支正确使用无法静态证明，本测试只覆盖探针分支。</li>
 *   <li>信封只承诺经 DispatcherServlet 的请求；容器级错误（畸形请求行、TLS 握手失败、
 *       请求体超限等）由容器直接应答，不在本门禁范围内。</li>
 *   <li>应用层另有两个不经本 advice 的出口：Spring Boot 自动装配的 {@code BasicErrorController}
 *       暴露 {@code /error}（实测 {@code GET /error} 回 500 + {@code {"timestamp":…,"status":999,…}}，
 *       既非信封也不是协议状态），框架内建的 {@code OPTIONS} 回 200 + 空 body。
 *       两者均无门禁覆盖，是否纳入信封属后续故事的产品决策。</li>
 *   <li>敏感信息字面量（{@code Authorization}、{@code session_key}/{@code sessionKey}、
 *       {@code openId}）无任何机械校验：本套件不扫描这些字面量，该面完全依赖白盒评审。
 *       本套件只捕获 {@link GlobalExceptionHandler} 的日志输出，且只用它断言协议级客户端错误
 *       不产生 ERROR 级记录，不构成「日志不含敏感信息」的证明。</li>
 * </ul>
 */
@SpringBootTest
@AutoConfigureMockMvc
@Import(EnvelopeFailureProbeController.class)
class EnvelopeContractTest {

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private ObjectMapper objectMapper;

    @Test
    @DisplayName("探活：根路径与 /api/v1/health 均返回 HTTP 200 与三键信封")
    void 探活接口返回三键信封() throws Exception {
        for (String path : List.of("/", "/api/v1/health")) {
            JsonNode body = jsonOf(get(path), 200);

            assertEquals(Set.of("code", "message", "data"), fieldNames(body),
                    path + " 的信封键集合应为 code/message/data");
            assertEquals(0, body.get("code").asInt(), path + " 的成功 code 应为 0");
            assertEquals("success", body.get("message").asText(), path + " 的成功 message 应为 success");
            assertEquals("UP", body.get("data").get("status").asText(), path + " 的探活 data.status 应为 UP");
        }
    }

    @Test
    @DisplayName("data 为 null 时信封省略 data 键")
    void 无数据成功响应省略data键() throws Exception {
        JsonNode body = jsonOf(get("/__probe/envelope-empty"), 200);

        assertEquals(Set.of("code", "message"), fieldNames(body), "data 为 null 时不应出现 data 键");
        assertEquals(0, body.get("code").asInt());
    }

    @Test
    @DisplayName("业务错误保持 HTTP 200 并通过业务码表达")
    void 业务错误保持HTTP200并携带业务码() throws Exception {
        JsonNode body = jsonOf(get("/__probe/business-error"), 200);

        assertEquals(20005, body.get("code").asInt(), "业务错误应回 HTTP 200 并在 body 透出业务码");
        assertEquals("经文版本不一致", body.get("message").asText());
        assertEquals(Set.of("code", "message"), fieldNames(body));
    }

    @Test
    @DisplayName("非 200 业务码：HTTP 状态由码推导（40101 回 401）")
    void 非200业务码由码推导HTTP状态() throws Exception {
        JsonNode body = jsonOf(get("/__probe/unauthorized"), 401);

        assertEquals(40101, body.get("code").asInt(), "令牌失效应回 40101");
        assertEquals("令牌失效", body.get("message").asText());
        assertEquals(Set.of("code", "message"), fieldNames(body));
    }

    @Test
    @DisplayName("协议错误使用对应 HTTP 状态：畸形 JSON 为 400 + 40001，校验失败为 400 + 40000")
    void 协议错误使用对应HTTP状态() throws Exception {
        JsonNode malformed = jsonOf(post("/__probe/echo")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{\"a\":"), 400);
        assertEquals(40001, malformed.get("code").asInt(), "畸形 JSON 应回 40001");

        JsonNode invalidParam = jsonOf(post("/__probe/validated")
                .contentType(MediaType.APPLICATION_JSON)
                .content("{}"), 400);
        assertEquals(40000, invalidParam.get("code").asInt(), "校验失败应回 40000");

        JsonNode constrainedParam = jsonOf(get("/__probe/constrained-name").param("name", " "), 400);
        assertEquals(40000, constrainedParam.get("code").asInt(),
                "方法参数约束失败应回 40000；落兜底分支会变成 500 + 50000");
    }

    @Test
    @DisplayName("协议级客户端错误回对应 HTTP 状态与 405 Allow 头，且不记 ERROR 全栈日志")
    void 协议级客户端错误回对应HTTP状态() throws Exception {
        Logger handlerLogger = (Logger) LoggerFactory.getLogger(GlobalExceptionHandler.class);
        ListAppender<ILoggingEvent> appender = new ListAppender<>();
        appender.start();
        handlerLogger.addAppender(appender);
        try {
            JsonNode methodNotAllowed = jsonOf(post("/api/v1/health"), 401);
            assertEquals(40103, methodNotAllowed.get("code").asInt(), "受保护 API 缺少令牌应回 40103");

            MvcResult wrongMethod = responseOf(get("/__probe/echo"), 405);
            assertEquals(40500, bodyOf(wrongMethod).get("code").asInt(), "方法不支持应回 40500");
            String allow = wrongMethod.getResponse().getHeader("Allow");
            assertNotNull(allow, "405 必须按 RFC 9110 回 Allow 头");
            assertTrue(allow.contains("POST"), "Allow 头应含 POST，实际为 " + allow);

            JsonNode unsupportedMediaType = jsonOf(post("/__probe/echo")
                    .contentType(MediaType.TEXT_PLAIN)
                    .content("plain"), 415);
            assertEquals(41500, unsupportedMediaType.get("code").asInt(), "请求体媒体类型不支持应回 41500");

            MvcResult notAcceptable = responseOf(get("/api/v1/health").accept(MediaType.APPLICATION_XML), 406);
            assertEquals("", notAcceptable.getResponse().getContentAsString(StandardCharsets.UTF_8),
                    "客户端声明不接受 JSON 时信封在协议上不可投递，406 只回状态与空 body");

            JsonNode missingParam = jsonOf(get("/__probe/requires-name"), 400);
            assertEquals(40000, missingParam.get("code").asInt(), "缺少必需的请求参数应回 40000");

            List<ILoggingEvent> errors = appender.list.stream()
                    .filter(event -> event.getLevel() == Level.ERROR)
                    .toList();
            assertEquals(List.of(), errors.stream().map(ILoggingEvent::getFormattedMessage).toList(),
                    "协议级客户端错误不得记 ERROR 全栈日志");
        } finally {
            handlerLogger.detachAppender(appender);
        }
    }

    @Test
    @DisplayName("未匹配路径回 HTTP 404 + 40400，受保护 API 缺令牌优先回 40103")
    void 未匹配路径回404而非500() throws Exception {
        JsonNode protectedMissing = jsonOf(get("/api/v1/not-a-real-endpoint"), 401);
        assertEquals(40103, protectedMissing.get("code").asInt(), "未匹配的受保护 API 应先回 40103");

        JsonNode notFound = jsonOf(get("/not-a-real-endpoint"), 404);
        assertEquals(40400, notFound.get("code").asInt(), "未匹配路径应回 40400，不得落兜底 500");
    }

    @Test
    @DisplayName("兜底异常回 HTTP 500 + 50000，且 message 为固定文案不外泄内部信息")
    void 兜底异常返回固定文案() throws Exception {
        JsonNode body = jsonOf(get("/__probe/boom"), 500);

        assertEquals(50000, body.get("code").asInt());
        assertEquals("服务器内部错误，请稍后重试", body.get("message").asText(), "兜底 message 必须是固定文案");

        String raw = body.toString();
        assertFalse(raw.contains("IllegalStateException"), "兜底响应不得外泄异常类名");
        assertFalse(raw.contains("探针"), "兜底响应不得外泄内部提示");
        assertFalse(raw.contains("\tat "), "兜底响应不得外泄堆栈片段");
    }

    private JsonNode jsonOf(MockHttpServletRequestBuilder request, int expectedStatus) throws Exception {
        return bodyOf(responseOf(request, expectedStatus));
    }

    private MvcResult responseOf(MockHttpServletRequestBuilder request, int expectedStatus) throws Exception {
        MvcResult result = mockMvc.perform(request).andReturn();
        assertEquals(expectedStatus, result.getResponse().getStatus(),
                "请求 " + request + " 的 HTTP 状态不符合预期；响应体="
                        + result.getResponse().getContentAsString(StandardCharsets.UTF_8));
        return result;
    }

    private JsonNode bodyOf(MvcResult result) throws Exception {
        return objectMapper.readTree(result.getResponse().getContentAsString(StandardCharsets.UTF_8));
    }

    private static Set<String> fieldNames(JsonNode body) {
        Set<String> names = new LinkedHashSet<>();
        body.fieldNames().forEachRemaining(names::add);
        return names;
    }
}
