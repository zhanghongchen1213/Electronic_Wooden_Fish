package top.zhcmqtt.ewf.backend.common.security;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import org.springframework.http.MediaType;
import org.springframework.stereotype.Component;

import com.fasterxml.jackson.databind.ObjectMapper;

import jakarta.servlet.http.HttpServletResponse;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;

/** 过滤器层认证失败写回统一信封。 */
@Component
public class AuthFailureWriter {

    private final ObjectMapper objectMapper;

    public AuthFailureWriter(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    public void write(HttpServletResponse response, int code, String message) {
        if (response.isCommitted()) {
            return;
        }
        response.setStatus(code / 100);
        response.setCharacterEncoding(StandardCharsets.UTF_8.name());
        response.setContentType(MediaType.APPLICATION_JSON_VALUE);
        try {
            response.getWriter().write(objectMapper.writeValueAsString(ApiResponse.error(code, message)));
        } catch (IOException ignored) {
            // 容器已在写响应阶段，不能再改变统一错误语义。
        }
    }
}
