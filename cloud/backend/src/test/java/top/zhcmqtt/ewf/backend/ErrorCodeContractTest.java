package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * 契约一致性门禁：{@link ErrorCode} 中登记的同步域码必须与
 * {@code docs/contracts/sync-contract.schema.json#business_codes} 逐值一致（含 HTTP 语义）。
 *
 * <p>期望集合**机械提取**自注册表，断言内不手抄第三份清单——手抄会在契约与实现之外再产生一个真源。
 * 注册表本身是契约正文的派生品（{@code docs/contracts/README.md}），不是第二真源。
 *
 * <p><b>静态校验无法覆盖的边界：</b>本测试只能证明「常量值与契约逐值相等」，
 * 不能证明实现层在正确的业务条件下抛出这些码（属 Epic 5 的同步域行为）。
 */
class ErrorCodeContractTest {

    /**
     * 本 Story 的异常层兜底码：除 40000/40400/50000（迁移经验 §② 区间表）外，含协议级客户端错误
     * 40500（方法不支持）与 41500（请求体媒体类型不支持）——两者按码形规则取值，使 HTTP 状态与码形自洽。
     * 该集合不是契约冻结值；4.4 的鉴权/上游码也在这里登记，后续模块可扩但不得改义。
     */
    private static final Set<Integer> FALLBACK_CODES = Set.of(
            40000, 40400, 40500, 41500, 50000, 40102, 40103, 40300, 50200);

    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

    @Test
    @DisplayName("契约 §12 预留的同步域码与注册表逐值一致（含 HTTP 语义）")
    void 同步域码与注册表逐值一致() throws IOException {
        JsonNode businessCodes = businessCodes();
        assertEquals(8, businessCodes.size(), "契约预留的业务码条数应与同步域冻结集合一致");

        Map<String, Integer> registered = registeredCodes();
        for (JsonNode entry : businessCodes) {
            int code = entry.get("code").asInt();
            int httpStatus = entry.get("http_status").asInt();

            int registeredValue = registeredCodeOf(registered, code, entry.get("condition").asText());
            assertEquals(httpStatus, registeredValue / 100,
                    "码 " + code + "（" + entry.get("condition").asText() + "）的 HTTP 语义应与注册表一致");
        }
    }

    @Test
    @DisplayName("ErrorCode 不得登记契约码与兜底码之外的码值")
    void 不得登记契约与兜底码之外的码值() throws IOException {
        Set<Integer> expected = new TreeSet<>(FALLBACK_CODES);
        businessCodes().forEach(entry -> expected.add(entry.get("code").asInt()));

        assertEquals(expected, new TreeSet<>(registeredCodes().values()),
                "登记码集合应恰为契约预留码 + 本 Story 兜底码");
    }

    @Test
    @DisplayName("码形 {HTTP 状态}{两位序号} 是唯一推导规则，无需另立映射表")
    void 码形推导HTTP状态() {
        assertEquals(200, ErrorCode.httpStatusOf(20001));
        assertEquals(400, ErrorCode.httpStatusOf(40001));
        assertEquals(401, ErrorCode.httpStatusOf(40101));
        assertEquals(404, ErrorCode.httpStatusOf(40400));
        assertEquals(500, ErrorCode.httpStatusOf(50000));
    }

    private static JsonNode businessCodes() throws IOException {
        Path schema = TestWorkspace.repoRoot().resolve("docs/contracts/sync-contract.schema.json");
        assertTrue(Files.isRegularFile(schema), "缺少契约注册表：" + schema);

        JsonNode root = OBJECT_MAPPER.readTree(Files.readString(schema, StandardCharsets.UTF_8));
        JsonNode businessCodes = root.get("business_codes");
        assertNotNull(businessCodes, "契约注册表缺少 business_codes");
        assertTrue(businessCodes.isArray() && !businessCodes.isEmpty(), "business_codes 应为非空数组");
        return businessCodes;
    }

    /** 反射提取 {@link ErrorCode} 的公开静态常量（全部为码值）。 */
    private static Map<String, Integer> registeredCodes() {
        Map<String, Integer> codes = new TreeMap<>();
        for (Field field : ErrorCode.class.getDeclaredFields()) {
            int modifiers = field.getModifiers();
            if (field.getType() != int.class || !Modifier.isPublic(modifiers)
                    || !Modifier.isStatic(modifiers) || !Modifier.isFinal(modifiers)) {
                continue;
            }
            try {
                codes.put(field.getName(), field.getInt(null));
            } catch (IllegalAccessException e) {
                fail("无法读取常量 " + field.getName() + "：" + e.getMessage());
            }
        }
        assertTrue(!codes.isEmpty(), "ErrorCode 未登记任何码值");
        return codes;
    }

    private static int registeredCodeOf(Map<String, Integer> registered, int code, String condition) {
        return registered.entrySet().stream()
                .filter(entry -> entry.getValue() == code)
                .map(Map.Entry::getValue)
                .findFirst()
                .orElseGet(() -> fail("ErrorCode 未登记契约码 " + code + "（" + condition + "）"));
    }
}
