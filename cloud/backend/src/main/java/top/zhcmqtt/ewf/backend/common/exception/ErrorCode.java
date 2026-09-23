package top.zhcmqtt.ewf.backend.common.exception;

/**
 * 统一错误码常量。
 *
 * <p>码形为 {@code {HTTP 状态}{两位序号}}，因此 HTTP 语义由码本身携带：{@code httpStatus = code / 100}。
 * 不要另建一张 {@code code → httpStatus} 映射表，那会产生第二真源。
 *
 * <p>第一组是 {@code docs/contracts/sync-contract.md} §12 预留的同步域业务码，值与
 * {@code sync-contract.schema.json#business_codes} 逐值一致，实现层不得重定义或改义。
 * 第二组是异常层兜底码，不是契约冻结值，后续模块可扩但不得改义；其中协议级客户端错误
 * （{@code 405xx}/{@code 415xx}）按码形规则取值，使 HTTP 状态与码形自洽。
 */
public final class ErrorCode {

    private ErrorCode() {
    }

    // ---- 契约 §12 预留的同步域业务码（20001–20006 为 HTTP 200 的业务错误；40001 走 400、40101 走 401）----

    /** 轮次无法归属。 */
    public static final int ROUND_UNASSIGNED = 20001;

    /** 缺少基准高水位。 */
    public static final int BASELINE_HIGH_WATER_MISSING = 20002;

    /** 设备重置冲突。 */
    public static final int DEVICE_RESET_CONFLICT = 20003;

    /** 队列已满。 */
    public static final int QUEUE_FULL = 20004;

    /** 经文版本不一致。 */
    public static final int SCRIPTURE_VERSION_MISMATCH = 20005;

    /** 命令旧修订。 */
    public static final int COMMAND_STALE_REVISION = 20006;

    /** 协议字段非法（请求体不可解析）——本 Story 有真实分支。 */
    public static final int PROTOCOL_FIELD_INVALID = 40001;

    /** 令牌失效（鉴权实现属 4.4，本 Story 只登记码值）。 */
    public static final int TOKEN_EXPIRED = 40101;

    /** 令牌签名、格式或类型非法。 */
    public static final int TOKEN_INVALID = 40102;

    /** 请求缺少 Bearer 令牌。 */
    public static final int TOKEN_MISSING = 40103;

    /** 当前微信身份与固定设备身份不匹配。 */
    public static final int IDENTITY_MISMATCH = 40300;

    /** 微信上游不可用或身份/JWT 配置仍为占位值。 */
    public static final int WECHAT_UPSTREAM = 50200;

    // ---- 异常层兜底码 ----

    /** 参数不合法（绑定校验失败或参数类型不匹配）。 */
    public static final int PARAM_INVALID = 40000;

    /** 资源不存在。 */
    public static final int NOT_FOUND = 40400;

    /** 请求方法不被支持（对端点使用了未声明的 HTTP 方法）。 */
    public static final int METHOD_NOT_ALLOWED = 40500;

    /** 请求体媒体类型不被支持。 */
    public static final int UNSUPPORTED_MEDIA_TYPE = 41500;

    /** 服务器内部错误。 */
    public static final int SERVER_ERROR = 50000;

    /** 由业务码推导其期望的 HTTP 状态。 */
    public static int httpStatusOf(int code) {
        return code / 100;
    }
}
