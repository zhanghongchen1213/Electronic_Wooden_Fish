package top.zhcmqtt.ewf.backend.common.exception;

import java.util.Set;
import java.util.stream.Collectors;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.validation.FieldError;
import org.springframework.web.HttpMediaTypeNotAcceptableException;
import org.springframework.web.HttpMediaTypeNotSupportedException;
import org.springframework.web.HttpRequestMethodNotSupportedException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.ServletRequestBindingException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.web.method.annotation.HandlerMethodValidationException;
import org.springframework.web.method.annotation.MethodArgumentTypeMismatchException;
import org.springframework.web.servlet.NoHandlerFoundException;
import org.springframework.web.servlet.resource.NoResourceFoundException;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.ConstraintViolationException;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;

/**
 * 全局异常处理：把异常统一收敛为信封。
 *
 * <p><b>Story 5.6 裁决（A/C/G）：</b>本类只做信封验收硬化与可执行文案，不重造协议；
 * 禁止 RFC 9457 Problem Details（须保持 {@code spring.mvc.problemdetails.enabled=false}）；
 * 禁止给信封加 {@code retryable} 字段——客户端按码表判定动作。业务错误 HTTP 200 + {@code 2xxxx}；
 * 协议错误用准确 HTTP 状态。失败 {@code message} 须含可执行动作关键词（重试/重新登录/重建/收敛/修复）。
 *
 * <p>协议级客户端错误必须显式映射，否则会落进兜底分支从 4xx 变 500：{@code NoResourceFoundException}
 * 必须显式处理——Spring Boot 3.2 / Framework 6.1 起未命中的静态资源由
 * {@code ResourceHttpRequestHandler} 抛该异常；方法不支持（405）、请求体媒体类型不支持（415）、
 * 不可接受的响应类型（406）与请求参数绑定失败（400）同理。
 *
 * <p>敏感信息不进日志：兜底分支只记异常对象、HTTP 方法与 URI，不打印请求头/body/token；
 * 协议级客户端错误记 warn 且不打印堆栈。
 */
@RestControllerAdvice
public class GlobalExceptionHandler {

    private static final Logger log = LoggerFactory.getLogger(GlobalExceptionHandler.class);

    /** 兜底 500 的固定文案：不含异常类名、堆栈或内部提示；含「稍后重试」动作语义。 */
    private static final String SERVER_ERROR_MESSAGE = "服务器内部错误，请稍后重试";

    /** 业务错误：HTTP 状态由业务码推导，body 原样透出 code 与 message。 */
    @ExceptionHandler(BusinessException.class)
    public ResponseEntity<ApiResponse<Void>> handleBusinessException(BusinessException ex) {
        HttpStatus status = HttpStatus.resolve(ErrorCode.httpStatusOf(ex.getCode()));
        if (status == null) {
            status = HttpStatus.OK;
        }
        return ResponseEntity.status(status).body(ApiResponse.error(ex.getCode(), ex.getMessage()));
    }

    /** 请求体绑定校验失败。 */
    @ExceptionHandler(MethodArgumentNotValidException.class)
    public ResponseEntity<ApiResponse<Void>> handleMethodArgumentNotValid(MethodArgumentNotValidException ex) {
        String message = ex.getBindingResult().getFieldErrors().stream()
                .map(FieldError::getDefaultMessage)
                .collect(Collectors.joining("; "));
        return badRequest(ErrorCode.PARAM_INVALID, withRetryHint(message.isBlank() ? "参数不合法" : message));
    }

    /** 控制器方法参数上的约束注解校验失败（Spring 6.1 内建方法校验）：与其它参数类失败同码同状态。 */
    @ExceptionHandler(HandlerMethodValidationException.class)
    public ResponseEntity<ApiResponse<Void>> handleHandlerMethodValidation(HandlerMethodValidationException ex) {
        return badRequest(ErrorCode.PARAM_INVALID, "参数不合法，请修复后重试");
    }

    /** 路径或查询参数校验失败。 */
    @ExceptionHandler(ConstraintViolationException.class)
    public ResponseEntity<ApiResponse<Void>> handleConstraintViolation(ConstraintViolationException ex) {
        String message = ex.getConstraintViolations().stream()
                .map(violation -> violation.getMessage())
                .collect(Collectors.joining("; "));
        return badRequest(ErrorCode.PARAM_INVALID, withRetryHint(message.isBlank() ? "参数不合法" : message));
    }

    /** 参数类型不匹配。 */
    @ExceptionHandler(MethodArgumentTypeMismatchException.class)
    public ResponseEntity<ApiResponse<Void>> handleMethodArgumentTypeMismatch(MethodArgumentTypeMismatchException ex) {
        return badRequest(ErrorCode.PARAM_INVALID, "参数类型错误：" + ex.getName() + "，请修复后重试");
    }

    /** 请求体不是合法 JSON。 */
    @ExceptionHandler(HttpMessageNotReadableException.class)
    public ResponseEntity<ApiResponse<Void>> handleHttpMessageNotReadable(HttpMessageNotReadableException ex) {
        return badRequest(ErrorCode.PROTOCOL_FIELD_INVALID, "请求体不是合法 JSON，请修复后重试");
    }

    /** 未匹配的路径与静态资源：必须回 404 而不是落到兜底 500。 */
    @ExceptionHandler({NoResourceFoundException.class, NoHandlerFoundException.class})
    public ResponseEntity<ApiResponse<Void>> handleNotFound(Exception ex) {
        return ResponseEntity.status(HttpStatus.NOT_FOUND)
                .body(ApiResponse.error(ErrorCode.NOT_FOUND, "资源不存在，请修复后重试"));
    }

    /** 请求方法不被端点支持：405，并按 RFC 9110 回 {@code Allow} 头（与 Spring 默认解析器同形）。 */
    @ExceptionHandler(HttpRequestMethodNotSupportedException.class)
    public ResponseEntity<ApiResponse<Void>> handleMethodNotSupported(
            HttpRequestMethodNotSupportedException ex, HttpServletRequest request) {
        logProtocolError(request);
        ResponseEntity.BodyBuilder response = ResponseEntity.status(HttpStatus.METHOD_NOT_ALLOWED);
        Set<HttpMethod> supported = ex.getSupportedHttpMethods();
        if (supported != null) {
            response.allow(supported.toArray(HttpMethod[]::new));
        }
        return response.body(ApiResponse.error(ErrorCode.METHOD_NOT_ALLOWED, "请求方法不被支持，请修复后重试"));
    }

    /** 请求体媒体类型不被端点支持：415。 */
    @ExceptionHandler(HttpMediaTypeNotSupportedException.class)
    public ResponseEntity<ApiResponse<Void>> handleMediaTypeNotSupported(
            HttpMediaTypeNotSupportedException ex, HttpServletRequest request) {
        logProtocolError(request);
        return ResponseEntity.status(HttpStatus.UNSUPPORTED_MEDIA_TYPE)
                .body(ApiResponse.error(ErrorCode.UNSUPPORTED_MEDIA_TYPE, "请求体媒体类型不被支持，请修复后重试"));
    }

    /**
     * 客户端不接受本服务可生成的响应类型：406。
     *
     * <p>此时客户端已声明不接受 JSON，任何信封 body 都属内容协商违规，故只回状态与空 body；
     * 该路径不可能携带信封，这是协议本身的结果而不是遗漏。
     */
    @ExceptionHandler(HttpMediaTypeNotAcceptableException.class)
    public ResponseEntity<Void> handleMediaTypeNotAcceptable(
            HttpMediaTypeNotAcceptableException ex, HttpServletRequest request) {
        logProtocolError(request);
        return ResponseEntity.status(HttpStatus.NOT_ACCEPTABLE).build();
    }

    /** 缺少必需的请求参数或请求头：400。 */
    @ExceptionHandler(ServletRequestBindingException.class)
    public ResponseEntity<ApiResponse<Void>> handleRequestBinding(
            ServletRequestBindingException ex, HttpServletRequest request) {
        logProtocolError(request);
        return badRequest(ErrorCode.PARAM_INVALID, "请求参数缺失或不合法，请修复后重试");
    }

    /** 兜底：记异常与请求方法/URI，对外只回固定文案。 */
    @ExceptionHandler(Exception.class)
    public ResponseEntity<ApiResponse<Void>> handleUnexpected(Exception ex, HttpServletRequest request) {
        log.error("未捕获异常: method={}, uri={}", request.getMethod(), request.getRequestURI(), ex);
        return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR)
                .body(ApiResponse.error(ErrorCode.SERVER_ERROR, SERVER_ERROR_MESSAGE));
    }

    private static ResponseEntity<ApiResponse<Void>> badRequest(int code, String message) {
        return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(ApiResponse.error(code, message));
    }

    /** 协议级客户端错误只记方法与 URI，级别 warn 且不带堆栈：它不是服务端故障。 */
    private static void logProtocolError(HttpServletRequest request) {
        log.warn("协议错误: method={}, uri={}", request.getMethod(), request.getRequestURI());
    }

    /** 校验文案若已含动作关键词则原样返回，否则追加「请修复后重试」。 */
    private static String withRetryHint(String message) {
        if (message.contains("重试") || message.contains("修复") || message.contains("重新登录")
                || message.contains("重建") || message.contains("收敛")) {
            return message;
        }
        return message + "，请修复后重试";
    }
}
