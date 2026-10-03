package top.zhcmqtt.ewf.backend.common.security;

/**
 * 请求身份来源。
 *
 * <p><b>2026-10-02 裁决：</b>小程序完全自用测试，彻底移除鉴权（JWT 过滤器、微信登录、令牌校验均已删除），
 * 单一固定设备身份直接以常量提供，不再经请求解析。
 */
public final class UserContext {

    /** 单一固定设备身份；业务数据仍以此作为 device_id 回显与会话键。 */
    public static final String SOLO_DEVICE_ID = "ewf-solo";

    private UserContext() {
    }

    public static String currentDeviceId() {
        return SOLO_DEVICE_ID;
    }
}
