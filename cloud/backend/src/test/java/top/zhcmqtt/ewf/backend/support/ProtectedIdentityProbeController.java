package top.zhcmqtt.ewf.backend.support;

import java.util.Map;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.common.security.UserContext;

/** 仅测试作用域的受保护身份探针；生产代码不暴露调试路由。 */
@RestController
public class ProtectedIdentityProbeController {

    @GetMapping("/api/v1/__probe/identity")
    public ApiResponse<Map<String, String>> identity() {
        return ApiResponse.success(Map.of("deviceId", UserContext.currentDeviceId()));
    }
}
