package top.zhcmqtt.ewf.backend.dto.auth;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

/** refresh 请求。 */
public record RefreshTokenRequest(
        @NotBlank(message = "refresh token 不能为空")
        @Size(max = 4096, message = "refresh token 长度无效") String refreshToken) {
}
