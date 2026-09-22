package top.zhcmqtt.ewf.backend.common.exception;

/**
 * 唯一业务抛错方式：携带业务码与展示文案，HTTP 状态由码推导（见 {@link ErrorCode#httpStatusOf(int)}）。
 *
 * <p>控制器与服务不得用返回 {@code null} 或手工拼错误体表达失败。
 * 便捷静态只覆盖当前已登记的码值，不在本 Story 预登记尚无分支引用的区间码。
 */
public class BusinessException extends RuntimeException {

    private static final long serialVersionUID = 1L;

    private final int code;

    public BusinessException(int code, String message) {
        super(message);
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    /** 参数不合法。 */
    public static BusinessException paramError(String message) {
        return new BusinessException(ErrorCode.PARAM_INVALID, message);
    }

    /** 未授权：EWF 当前的 401 语义为令牌失效。 */
    public static BusinessException unauthorized(String message) {
        return new BusinessException(ErrorCode.TOKEN_EXPIRED, message);
    }

    /** 资源不存在。 */
    public static BusinessException notFound(String message) {
        return new BusinessException(ErrorCode.NOT_FOUND, message);
    }

    /**
     * 业务冲突：EWF 的业务错误按 FR-C-011 保持 HTTP 200 并用业务码表达，
     * 故冲突落在 2xxxx 区间；当前唯一已登记的冲突码是契约 §12 的「设备重置冲突」。
     * 其它冲突语义（重复、并发等）的码值留给引入该分支的模块定义。
     */
    public static BusinessException conflict(String message) {
        return new BusinessException(ErrorCode.DEVICE_RESET_CONFLICT, message);
    }

    /** 服务器内部错误：文案必须为固定值，不外泄内部信息。 */
    public static BusinessException serverError(String message) {
        return new BusinessException(ErrorCode.SERVER_ERROR, message);
    }
}
