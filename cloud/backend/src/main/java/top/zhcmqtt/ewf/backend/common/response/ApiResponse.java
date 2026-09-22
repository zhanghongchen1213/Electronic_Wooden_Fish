package top.zhcmqtt.ewf.backend.common.response;

import com.fasterxml.jackson.annotation.JsonInclude;

/**
 * 对外统一响应信封 {@code {code, message, data}}。
 *
 * <p>成功时 {@code code = 0}、{@code message = "success"}；失败走 {@link #error(int, String)}，
 * 由异常层统一产出，控制器不手工拼错误体。{@code data} 为 null 时省略该键，
 * 前端需容忍「成功但无 data」的响应。
 *
 * <p>这是 REST 层唯一的响应形状，不得再发明第二种裸响应或嵌套信封。
 *
 * @param <T> 业务数据类型
 */
@JsonInclude(JsonInclude.Include.NON_NULL)
public class ApiResponse<T> {

    /** 成功响应的固定业务码。 */
    public static final int SUCCESS_CODE = 0;

    /** 成功响应的固定文案。 */
    public static final String SUCCESS_MESSAGE = "success";

    private final int code;
    private final String message;
    private final T data;

    private ApiResponse(int code, String message, T data) {
        this.code = code;
        this.message = message;
        this.data = data;
    }

    /** 成功且无业务数据。 */
    public static ApiResponse<Void> success() {
        return new ApiResponse<>(SUCCESS_CODE, SUCCESS_MESSAGE, null);
    }

    /** 成功且携带业务数据。 */
    public static <T> ApiResponse<T> success(T data) {
        return new ApiResponse<>(SUCCESS_CODE, SUCCESS_MESSAGE, data);
    }

    /** 失败：携带业务码与展示文案。 */
    public static <T> ApiResponse<T> error(int code, String message) {
        return new ApiResponse<>(code, message, null);
    }

    public int getCode() {
        return code;
    }

    public String getMessage() {
        return message;
    }

    public T getData() {
        return data;
    }
}
