package top.zhcmqtt.ewf.backend.common.security;

/** 当前请求的固定设备身份；请求结束必须由过滤器清理。 */
public record UserContext(String deviceId) {

    private static final ThreadLocal<UserContext> CURRENT = new ThreadLocal<>();

    public static void set(String deviceId) {
        CURRENT.set(new UserContext(deviceId));
    }

    public static UserContext current() {
        return CURRENT.get();
    }

    public static String currentDeviceId() {
        UserContext context = CURRENT.get();
        return context == null ? null : context.deviceId();
    }

    public static void clear() {
        CURRENT.remove();
    }
}
