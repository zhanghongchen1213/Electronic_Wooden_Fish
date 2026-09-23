package top.zhcmqtt.ewf.backend.service;

import org.springframework.stereotype.Service;

import io.jsonwebtoken.Claims;
import top.zhcmqtt.ewf.backend.client.WechatMiniClient;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.common.security.JwtTokenProvider;
import top.zhcmqtt.ewf.backend.dto.auth.AuthTokenResponse;

/** 登录、刷新与固定设备身份编排。 */
@Service
public class AuthService {

    private final WechatMiniClient wechatMiniClient;
    private final IdentityStore identityStore;
    private final JwtTokenProvider tokenProvider;

    public AuthService(WechatMiniClient wechatMiniClient, IdentityStore identityStore,
            JwtTokenProvider tokenProvider) {
        this.wechatMiniClient = wechatMiniClient;
        this.identityStore = identityStore;
        this.tokenProvider = tokenProvider;
    }

    public AuthTokenResponse login(String code) {
        String openId = wechatMiniClient.exchangeCode(code);
        String deviceId = IdentityStore.deviceIdForOpenId(openId);
        String fixedDeviceId = identityStore.getOrCreate(deviceId);
        return tokenResponse(tokenProvider.issueTokenPair(fixedDeviceId), fixedDeviceId);
    }

    public AuthTokenResponse refresh(String refreshToken) {
        Claims claims = tokenProvider.parseRefresh(refreshToken);
        String deviceId = claims.getSubject();
        String fixedDeviceId = identityStore.readDeviceId();
        if (!fixedDeviceId.equals(deviceId)) {
            throw BusinessException.identityMismatch("令牌身份与设备不匹配，请重新登录");
        }
        return tokenResponse(tokenProvider.issueTokenPair(fixedDeviceId), fixedDeviceId);
    }

    private static AuthTokenResponse tokenResponse(JwtTokenProvider.TokenPair pair, String deviceId) {
        return new AuthTokenResponse(pair.accessToken(), pair.refreshToken(), pair.expiresIn(), deviceId);
    }
}
