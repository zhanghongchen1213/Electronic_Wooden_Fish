package top.zhcmqtt.ewf.backend.dto.auth;

/** 登录/刷新成功数据；不包含 openid 或 session_key。 */
public record AuthTokenResponse(String accessToken, String refreshToken, long expiresIn, String deviceId) {
}
