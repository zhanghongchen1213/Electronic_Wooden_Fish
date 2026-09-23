package top.zhcmqtt.ewf.backend.dto.auth;

import jakarta.validation.constraints.NotBlank;

/** 微信登录请求；只接收一次性 code，不接收或回显微信会话材料。 */
public record WechatLoginRequest(@NotBlank(message = "微信 code 不能为空") String code) {
}
