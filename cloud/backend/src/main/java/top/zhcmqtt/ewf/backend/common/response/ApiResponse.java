package top.zhcmqtt.ewf.backend.common.response;

import com.fasterxml.jackson.annotation.JsonInclude;

/**
 * 对外统一响应信封 {@code {code, message, data}}。
 *
 * <p>成功时 {@code code = 0}、{@code message = "success"}；失败走 {@link #error(int, String)}，
 * 由异常层统一产出，控制器不手工拼错误体（唯一例外是契约 §12 要求「拒绝响应仍携带权威基准」的分支，
 * 见 {@link #error(int, String, Object)}）。{@code data} 为 null 时省略该键，
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

    /**
     * 失败：携带业务码与展示文案，无业务数据（{@code data} 键被 {@code NON_NULL} 省略）。
     *
     * <p>这是异常层唯一使用的失败工厂，其语义与 {@code NON_NULL} 行为逐字不变。
     */
    public static <T> ApiResponse<T> error(int code, String message) {
        return new ApiResponse<>(code, message, null);
    }

    /**
     * 失败但**必须携带权威基准**的响应（裁决 C）。
     *
     * <p>契约 §12 明文要求业务拒绝「§6.2 的响应必填字段照常回传……不得只回 {@code {code,message}}」，
     * 否则 §8 的「禁止无限重试」不可实现（客户端没有可据以重建本地基准的水位）。而
     * {@link #error(int, String)} 的 {@code data} 恒为 null、类级 {@code NON_NULL} 又会删键，
     * 异常层（{@code GlobalExceptionHandler} 的 {@code BusinessException} 分支）同样不携带 data。
     *
     * <p>因此这里补一条**显式携带入口**，且不触碰任何既有语义：
     * <ol>
     *   <li>{@link #error(int, String)} 的行为与 {@code NON_NULL} 语义逐字不变；</li>
     *   <li>{@code GlobalExceptionHandler} 的既有映射一条不改——异常层仍只产出两键信封，
     *       {@code EnvelopeContractTest} 对「业务错误 = {@code {code,message}}」的钉扎断言保持成立；</li>
     *   <li>本工厂只由控制器在「§12 要求带基准的拒绝分支」显式调用，此时 {@code data} 非空，
     *       信封自然成为三键且不依赖任何注解改动。</li>
     * </ol>
     *
     * @param data 必须非空，否则会退回两键信封而违反 §12
     */
    public static <T> ApiResponse<T> error(int code, String message, T data) {
        return new ApiResponse<>(code, message, data);
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
