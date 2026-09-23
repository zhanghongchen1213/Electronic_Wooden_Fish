package top.zhcmqtt.ewf.backend.controller;

import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import jakarta.validation.Valid;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.dto.auth.AuthTokenResponse;
import top.zhcmqtt.ewf.backend.dto.auth.RefreshTokenRequest;
import top.zhcmqtt.ewf.backend.dto.auth.WechatLoginRequest;
import top.zhcmqtt.ewf.backend.service.AuthService;

/** 微信登录与令牌刷新入口；不提供绑定、迁移或多设备 API。 */
@Validated
@RestController
@RequestMapping("/api/v1/auth")
public class AuthController {

    private final AuthService authService;

    public AuthController(AuthService authService) {
        this.authService = authService;
    }

    @PostMapping("/login/wechat-mini")
    public ApiResponse<AuthTokenResponse> login(@Valid @RequestBody WechatLoginRequest request) {
        return ApiResponse.success(authService.login(request.code()));
    }

    @PostMapping("/refresh")
    public ApiResponse<AuthTokenResponse> refresh(@Valid @RequestBody RefreshTokenRequest request) {
        return ApiResponse.success(authService.refresh(request.refreshToken()));
    }
}
