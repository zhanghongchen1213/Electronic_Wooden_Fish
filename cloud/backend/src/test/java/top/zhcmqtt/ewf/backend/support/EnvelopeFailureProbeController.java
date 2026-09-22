package top.zhcmqtt.ewf.backend.support;

import java.util.Map;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.exception.ErrorCode;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;

/**
 * 仅测试作用域注册的失败分支探针。
 * 生产代码不得为了可测而新增错误探针接口，故该控制器只存在于 {@code src/test/java}，不进入生产产物。
 * 本类位于 {@code EwfBackendApplication} 的组件扫描基包之下，测试类路径上的任何 Spring 上下文都会注册它；
 * {@code EnvelopeContractTest} 的 {@code @Import} 只是显式声明，不构成隔离。
 */
@RestController
@RequestMapping("/__probe")
public class EnvelopeFailureProbeController {

    /** 成功但无业务数据：用于验证信封在 data 为 null 时省略该键。 */
    @GetMapping("/envelope-empty")
    public ApiResponse<Void> emptyEnvelope() {
        return ApiResponse.success();
    }

    /** 业务错误：HTTP 200 + 业务码（取契约 §12 预留的同步域码）。 */
    @GetMapping("/business-error")
    public ApiResponse<Void> businessError() {
        throw new BusinessException(ErrorCode.SCRIPTURE_VERSION_MISMATCH, "经文版本不一致");
    }

    /** 业务错误：非 200 业务码（契约 §12 的 401/40101），用于验证 HTTP 状态确由业务码推导。 */
    @GetMapping("/unauthorized")
    public ApiResponse<Void> unauthorized() {
        throw BusinessException.unauthorized("令牌失效");
    }

    /** 未捕获异常：用于验证兜底 500 的固定文案。 */
    @GetMapping("/boom")
    public ApiResponse<Void> boom() {
        throw new IllegalStateException("探针：未捕获异常分支");
    }

    /** 可解析的 JSON 请求体：配合畸形 JSON 验证协议错误分支。 */
    @PostMapping("/echo")
    public ApiResponse<Map<String, Object>> echo(@RequestBody Map<String, Object> payload) {
        return ApiResponse.success(payload);
    }

    /** 缺少必需的查询参数：用于验证请求参数绑定失败不落兜底 500。 */
    @GetMapping("/requires-name")
    public ApiResponse<Map<String, Object>> requiresName(@RequestParam("name") String name) {
        return ApiResponse.success(Map.of("name", name));
    }

    /**
     * 方法参数上的约束注解：类上不加 {@code @Validated}，故走 Spring Framework 6.1 的内建方法校验，
     * 失败抛 {@code HandlerMethodValidationException}（自带 HTTP 400 语义），用于验证它不落兜底 500。
     */
    @GetMapping("/constrained-name")
    public ApiResponse<Map<String, Object>> constrainedName(
            @RequestParam("name") @NotBlank(message = "name 不能为空") String name) {
        return ApiResponse.success(Map.of("name", name));
    }

    /** 带校验注解的请求体：用于验证参数不合法分支。 */
    @PostMapping("/validated")
    public ApiResponse<ProbePayload> validated(@Valid @RequestBody ProbePayload payload) {
        return ApiResponse.success(payload);
    }

    /** 探针请求体。 */
    public record ProbePayload(@NotBlank(message = "name 不能为空") String name) {
    }
}
