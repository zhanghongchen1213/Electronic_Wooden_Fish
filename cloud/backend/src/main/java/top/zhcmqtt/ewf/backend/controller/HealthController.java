package top.zhcmqtt.ewf.backend.controller;

import java.util.Map;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import top.zhcmqtt.ewf.backend.common.response.ApiResponse;

/**
 * 探活接口：根路径 {@code /} 与 {@code /api/v1/health} 均回信封。
 *
 * <p>必须显式写成控制器：Spring Framework 6.1 起未命中的 {@code GET /} 会落到
 * {@code ResourceHttpRequestHandler}，而本地启动脚本的就绪探针恰好打根路径，
 * 若不显式映射会被当作未命中资源处理。本接口不做任何业务判断，也不承担错误探针职责。
 */
@RestController
public class HealthController {

    /** 探活响应体：只表达进程可服务。 */
    @GetMapping({"/", "/api/v1/health"})
    public ApiResponse<Map<String, String>> health() {
        return ApiResponse.success(Map.of("status", "UP"));
    }
}
