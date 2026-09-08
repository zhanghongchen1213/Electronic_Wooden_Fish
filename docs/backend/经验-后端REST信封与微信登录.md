<!-- 来源: miaowu · 源文件: backend/{docs/API-接口说明.md, src/main/java/.../common/**, client/WechatMiniClient.java, service/AuthService.java, resources/application*.yml} + env-scripts + README.md · miaowu HEAD: b1e0a660 · 迁移: 2026-09-08 · 判定: 适配(去 DB/SaaS/支付/OBS, EWF=JSON 零库单身份) · EWF 适配点: 沿用信封/错误码/微信登录/JWT/env-scripts 骨架 -->

# 经验：后端 REST 信封与微信登录（miaowu → EWF 适配）

> 用途：EWF `cloud/backend`（Spring Boot 3.3.7 + Java 17 + Maven，JSON 零库、单实例单进程）**软件层后端**开工前的迁移经验。本文只写「从 miaowu 后端代码里实际读到的做法/坑位」及其在 EWF 的用法；**模块级实现规格不在此定义**——EWF 前端面 REST 路径、`{code,message,data}` 信封与错误码表应在 `docs/contracts/sync-contract.md`（Story S0）冻结（见 `epics.md` Epic S0、AD-16/17）。
>
> 阅读对象：后续 backend spec/实现 agent。§①—§⑧ 一律用两列「miaowu 做法 → EWF 怎么用」；§⑨ 是排除表。逐条标注可复制的类/文件，EWF 侧给出**落盘即用或裁剪**的结论。

---

## ① REST 约定骨架（信封 / 前缀 / 分页 / 裸响应）

miaowu 的全部业务接口统一走 `/api/v1` + 信封 `ApiResponse<T>`，定义在 `top.zhcmqtt.saaswebpay.backend.common.response.ApiResponse`：

```java
@JsonInclude(JsonInclude.Include.NON_NULL)   // data 为 null 时省略该键
public class ApiResponse<T> { int code; String message; T data; }
// 成功: ApiResponse.success(data)   → {code:0, message:"success", data:{...}}
// 成功无 data: ApiResponse.success() → {code:0, message:"success"}（NON_NULL 省掉 data）
// 失败: ApiResponse.error(code, message) → {code:<业务码>, message:<文案>}
```

| miaowu 做法 / 坑位 | EWF 怎么用 |
| --- | --- |
| **信封结构**：`code=0` 成功，非 0 为业务错误码；`message` 给前端展示文案；`data` 业务数据。控制器只做 `ApiResponse.success(...)`，失败全交给异常层（§②），控制器不手工拼 `error`。 | 原样复制 `ApiResponse.java`。EWF 前端面（登录→设备、状态/统计查询、命令等）全部走同一信封，`code=0` 语义写入 S0 契约。 |
| **`@JsonInclude(NON_NULL)`**：`data==null` 时 JSON 里直接省略 `data` 键，前端解析需容忍「无 data」的成功体（如 `DELETE`/空操作）。 | 保留该注解 + 让前端 request 封装把「成功但无 data」也当正常分支；这是 miaowu→uni-app 骨架（AD-18）已有的解析习惯，EWF 小程序沿用。 |
| **路径前缀 `/api/v1`**：除健康检查外，业务资源全部挂在 `/api/v1` 之下，路由面与「SaaS 支付网关裸接口 `/api/pay/*`」物理隔离（防混用）。 | EWF 只保留 `/api/v1`（AD-16 已定）；S1.1 首个 story 即「Maven + Spring Boot 3.3.7 + `/api/v1` + 信封 + health + `data/`」。根路径 `/` 在 miaowu 也回信封探活，EWF health 照做。 |
| **分页 `PageData<T>`**：`{list,total,pageNum,pageSize,hasMore}`，`pageNum` 从 1 开始。部分接口（`discover/stars`）返回 `PageResponse` 会**再包一层** `ApiResponse`，造成「分页元数据嵌在 data 内部」，前端需按接口区分。 | EWF 记录页/回放以**已确认差量 + 序号**为主（AD-17，WS 推 + 快照水位补齐），REST 侧多为「单对象/统计」查询，**大概率不需要分页**。等 S0 冻结 API 表面时若出现明细列表再引入 `PageData`；不照搬「信封套分页信封」的嵌套形态。 |
| **非信封裸响应（仅 SaaS 支付网关）**：`POST /api/pay/create`、`POST /api/pay/notify`（回 String）、`GET /api/orders/{bizOrderNo}`（无单即 404）**不包** `ApiResponse`，对接方勿按 `code/message/data` 解析。 | EWF **没有**支付/SaaS 网关，不存在裸响应 REST 路径。唯一「非信封」通道是 **WebSocket 帧**（AD-17，下沉 `sync-contract.md` 定义）；REST 层禁止发明第二种裸响应。 |

---

## ② 错误码 `{HTTP状态}{两位序号}` 与全局异常处理

编码规范见 `common/exception/ErrorCode.java` 类注释：**错误码 = 期望前端感知的 HTTP 语义前缀 + 两位序号**。`BusinessException` 是唯一业务抛错方式（携带 code+message）。

### code 区间表（miaowu 常量，EWF 建议保留同构骨架）

| 区间 | 含义 | miaowu 已有常量示例 |
| --- | --- | --- |
| `400xx` | 参数类 | `40000` PARAM_INVALID、`40001` PARAM_PARSE_ERROR、`40002` PARAM_MISSING、`40003` PARAM_TYPE_MISMATCH |
| `401xx` | 未登录 / Token 问题 | `40100` UNAUTHORIZED、`40101` TOKEN_EXPIRED、`40102` TOKEN_INVALID、`40103` TOKEN_MISSING |
| `403xx` | 权限不足 | `40300` FORBIDDEN、`40301` ROLE_MISMATCH、`40302` PERMISSION_DENIED |
| `404xx` | 资源不存在 | `40400` NOT_FOUND、`40401` USER_NOT_FOUND、… |
| `409xx` | 业务冲突 / 冻结 / 重复 | `40900` BUSINESS_CONFLICT、`40901` LIMIT_EXCEEDED、`40903` DUPLICATE_OPERATION、`40904` STAR_FROZEN |
| `410xx` | 服务已下线 | `41000` SERVICE_DEPRECATED |
| `500xx` | 服务端错误 | `50000` SERVER_ERROR、`50001` DB_ERROR、`50002` THIRD_PARTY_ERROR、`50003` CRYPTO_ERROR |

### GlobalExceptionHandler 的映射（`@RestControllerAdvice`）

| 异常 | HTTP 状态 | body 错误码 | 说明（坑位） |
| --- | --- | --- | --- |
| `BusinessException` | **200** | 业务码原样 | **业务失败也回 HTTP 200**，靠 body 的 code 区分；前端拦截器只对「HTTP 非 2xx」或「code≠0」做统一 toast |
| `RateLimitExceededException` | **429（真实）** | `40901` LIMIT_EXCEEDED | 埋点限流例外：业务码在 409xx 但 HTTP 用真 429 |
| `MethodArgumentNotValidException`（`@Valid` body） | 400 | `40000` | 把多个 `FieldError.getDefaultMessage()` 用 `; ` 拼接给前端 |
| `ConstraintViolationException`（`@Validated` 路径/参数） | 400 | `40000` | 同上，取 `getConstraintViolations()` |
| `MethodArgumentTypeMismatchException` | 400 | `40000` | 参数类型不匹配 → `"参数类型错误"` |
| `HttpMessageNotReadableException` | 400 | `40001` | body 不是合法 JSON |
| `NoHandlerFoundException` / `NoResourceFoundException` | 404 | `40400` | Spring 6 把 `GET /` 落到 ResourceHttpRequestHandler，故健康检查要显式写 controller 才不会进 404 |
| `MaxUploadSizeExceededException` | 400 | `40000` | multipart 超限（见 §⑦） |
| `Exception`（兜底） | 500 | `50000` | `log.error` 全栈后统一 `"服务器内部错误"`，不外泄内部信息 |

| miaowu 做法 / 坑位 | EWF 怎么用 |
| --- | --- |
| 业务抛错统一 `throw new BusinessException(code, msg)`（含 `paramError/unauthorized/notFound/conflict/serverError` 便捷静态），Controller/Service 不返回 null 表示失败。 | 复制 `ErrorCode` + `BusinessException` + `GlobalExceptionHandler`。EWF 的错误码表按自身资源收敛：`40401` 可改指「设备/身份不存在」，去掉 `ROLE_MISMATCH`/`STAR_FROZEN` 等（单身份单设备），但**区间位保留**，避免前后端对「5 位码=HTTP 前缀+序号」的解析规则漂移。最终表在 S0 冻结（`epics.md` S0 story ②）。 |
| 「业务失败回 HTTP 200 + body 业务码」是信封体系的**关键约定**，前端 request 封装据此统一弹错。 | EWF 沿用同一约定，且要在 `sync-contract.md` / 前端骨架里写明「只看 body.code，HTTP 状态仅在 401/404/429/500 等协议级时使用」。 |
| 校验失败文案由后端拼接自 `jakarta.validation` 注解 message；EWF 的 DTO 校验注解（`@NotBlank/@Size` 等）可直接沿用。 | EWF 登录请求（`code`）与各类上报查询 DTO 加 `@Valid`，异常走同一 400 路径。 |

---

## ③ JWT 认证（HS256 / access+refresh / 路径分级 / ThreadLocal / tokenVersion）

miaowu **不引入 Spring Security**，只用一个 `OncePerRequestFilter` 注册到 `/api/*`（`SecurityConfig` 用 `FilterRegistrationBean`，order=1），由各路径分级决定放行/强制认证。JWT 库 JJWT `0.12.6`，工具 `JwtTokenProvider`（`common/security`）。

| 维度 | miaowu 做法 | EWF 怎么用 |
| --- | --- | --- |
| **签名算法** | HS256：`Keys.hmacShaKeyFor(secret.getBytes(UTF_8))`；生成 `Jwts.builder().subject(userId).claim("role",…).claim("type","access"/"refresh").issuedAt().expiration().signWith(key, Jwts.SIG.HS256)`；解析 `Jwts.parser().verifyWith(key).build().parseSignedClaims(token).getPayload()`。 | 整包复制 `JwtProperties`/`JwtTokenProvider`。`secret` 至少 32 字节，由环境变量 `JWT_SECRET` 注入（不落 Git）。 |
| **双 Token** | access 默认 2h（`7200000`ms），refresh 默认 30d（`2592000000`ms）。refresh 签名后还校验 `claim("type")=="refresh"`（`validateRefreshToken`），access 里带 `type=access`。 | EWF MVP 保留双 Token（access 短 + refresh 续签），微信登录返回 `{accessToken, refreshToken, expiresIn, userInfo/device}`。过期用 refresh 换新对；refresh 也轮换，降低单 Token 全量失效的面。 |
| **按路径分级** | Filter 内 `resolveAccessLevel`：`OPTIONS`→PUBLIC；`/api/v1/debug/**`→DEBUG、`/api/v1/admin/**`→ADMIN；公开白名单（登录/刷新/health/公开读接口）→PUBLIC 但仍「带 token 就解析（authenticateIfPresent）」；其余→AUTHENTICATED（无/坏 token 即写 401 信封：`TOKEN_MISSING`/`TOKEN_INVALID`）。公开写接口也在白名单逐个枚举，绝不 `startsWith` 大开。 | EWF 简化为 **PUBLIC / AUTHENTICATED 两档**：PUBLIC 白名单 = `POST /api/v1/auth/login/wechat-mini`、`POST /api/v1/auth/refresh`、`GET /api/v1/health`（+ WS 握手如需）；其余 `/api/*` 一律 AUTHENTICATED，缺/坏 token 返回真 401 + 信封。ADMIN/DEBUG 档不要迁。 |
| **认证结果入 ThreadLocal** | `UserContext`（static ThreadLocal）在 filter 内 `UserContext.set(UserInfo{userId,role,starId})`，`doFilterInternal` 的 **finally 里必 `UserContext.clear()`**，防线程池泄漏。Controller 用 `UserContext.getUserId()` 取当前用户，为 null 再抛 `UNAUTHORIZED`。 | 复制 `UserContext`，去掉 `role/starId` 字段，可加 `deviceId`（登录后由 userId→固定 `device_id` 映射，FR-B-001）。**finally clear 是硬规则**，EWF 照抄。 |
| **角色刷新** | Filter 每请求用 userId 回查 DB `highestRole` 覆盖 JWT 内 role（权限漂移即时生效），DB 查不到保持 JWT 内值。 | 单身份单设备**无角色**，此逻辑删除（省一次读）。若 JWT 里放 `deviceId`，每次请求从 JSON 身份文件读设备号校验一致性即可，缓存后可省。 |
| **tokenVersion（强制下线/换绑能力）** | DB `user.token_version` 整数；签发 JWT 时写入 `claim("tokenVersion", v)`。Filter/refresh 校验 `tokenVersion==null || !user.tokenVersion.equals(tokenVersion)` 则 401「登录状态已更新，请重新登录」。要**强制下线/换绑**只需把 DB 里 token_version +1，旧 JWT 全部作废。 | EWF 单身份单设备、MVP 无解绑/换机（FR-B-001），**可简化**——但建议在 JSON 身份文件里留 `token_version` 字段并沿用上述比较，理由：①登录后小程序可能二次登录，需要「旧 token 让位」语义；②将来加「换绑/解绑」不必改 JWT 结构。开销只有整数比较。 |

> 相关接口形态（miaowu `AuthController`）：`POST /api/v1/auth/login/wechat-mini`（PUBLIC）、`POST /api/v1/auth/refresh`（PUBLIC）、`GET/PUT /api/v1/auth/me|profile`（JWT）等。EWF 只保留 login + refresh + 「登录即得固定 device」映射；profile/me/phone/stars 等不迁。

---

## ④ 微信登录链路（`WechatMiniClient` 与 RestTemplate 坑位）

链路：小程序 `wx.login` 的 `code` → backend `POST /api/v1/auth/login/wechat-mini` → `WechatMiniClient.code2Session(code)` 调 `https://api.weixin.qq.com/sns/jscode2session` 换 `openid` → 按 openid 查/建身份（§⑤）→ 发 JWT 对。

### 从源码注释提取的坑位清单

| miaowu 做法 / 坑位（源码注释原意） | EWF 怎么用 |
| --- | --- |
| **code2Session 返回 `text/plain` → 先取 String body 再 Jackson 解析**：微信 jscode2session 正常回 JSON，但部分链路 `Content-Type` 是 `text/plain`，`restTemplate.getForObject(url, Code2SessionResponse.class)` 会抛 `UnknownContentTypeException`。miaowu 改 `restTemplate.exchange(url, GET, null, String.class)` 先拿 `String body`，再 `objectMapper.readValue(body, Code2SessionResponse.class)`，与 Content-Type 无关；body 为空/解析失败给用户「请稍后重试」。 | 原样保留这层处理（EWF 直接复制该 client）。这是**必踩坑**，解析失败日志要打印 body 前 240 字符便于排障。 |
| **errcode=40013（invalid appid）三处对齐 + 勿与支付网关混用**：code2Session 失败时，若 `errcode==40013`，miaowu 拼出中文提示——「后端当前 AppID 为 …；请与微信公众平台『开发设置』中的 AppID、微信开发者工具『详情』中的 AppID 对齐；若环境变量曾用于支付网关等，请勿与小程序 AppID 混用」。日志同时打印 `appId` 便于核对。 | EWF 保留 40013 专属提示（去掉支付网关半句）。AppID 有**三处**要对齐：后端 `wechat.mini.app-id`（或 `WECHAT_MINI_APP_ID`）、前端 `project.config.json`/manifest `mp-weixin.appid`、微信公众平台。 |
| **`session_key` 清空 + `openid` 脱敏**：`code2Session` 成功后 `response.setSessionKey(null)`（防意外泄露，DTO 上 `@ToString.Exclude`）；日志只记 `maskOpenId(openid)`（保留首 4 + 尾 4）。 | 照抄：session_key 只在换取瞬间存在，不允许落日志/入库；openid 日志一律脱敏。EWF 身份文件用 openid 哈希做键（§⑤）也不直接落明文日志。 |
| **未配置 AppID/Secret 的启动提示**：AppID 含 `your_` 或 AppSecret 为占位/等于 `wx_your_app_secret` 时，抛 `THIRD_PARTY_ERROR` 提示「设置环境变量后重启，勿把密钥写入 Git」。 | 复制该前置校验：本地联调默认值放 yml，但**生产/正式联调只用环境变量注入**（`WECHAT_MINI_APP_ID/SECRET`），占位值必须能挡在登录第一跳。 |
| **access_token 提前 300s 缓存**（`getAccessToken`）：`volatile String cachedAccessToken + volatile long tokenExpireTime`；未过期直接返回缓存，过期调 `cgi-bin/token` 刷新，缓存有效期 = `expires_in - 300` 秒（**提前 5 分钟**刷新，避免临界过期）。 | EWF MVP 的 `code2Session` **不需要** access_token（走 `appid+secret` 直换）。若未来调 `wxa/getwxacodeunlimit`、`getuserphonenumber` 等才需要；届时按需保留此内存缓存即可，勿提前搬。 |
| **绕系统代理 412（`WechatRestTemplateConfig`）**：本地/公司网络常设 `HTTP(S)_PROXY`，微信开放平台域名走代理 POST 会返回 412。miaowu 定制 `RestTemplate`（qualifier `wechatDirectRestTemplate`）：JDK `HttpClient` + **`NoSystemProxySelector`（`select()` 恒返回 `Proxy.NO_PROXY`）**、HTTP/1.1、connect 10s/read 15s，并补 `User-Agent`。 | 若 EWF 后端仍要直连微信（是），**必须**复制该 `RestTemplate` Bean（`NoSystemProxySelector` 是关键），并给 `WechatMiniClient` 用 `@Qualifier("wechatDirectRestTemplate")` 注入；否则本机开了代理就 412/超时，且排障半天查不到原因。 |

**EWF 裁剪**：只迁 `code2Session` 一条登录链路 +（未来可选）`access_token` 缓存。`getPhoneNumberByCode`（绑手机号）、`generateWxacode`（小程序码）、OBS 转存头像等**不迁**。`WechatProperties`（`wechat.mini.*` 前缀，端点 URL 有默认值）照搬，作为单例配置。

---

## ⑤ 并发建号：JSON 零库版（替代 DB 唯一约束）

miaowu 登录时「按 openid 查→不存在则建」的典型竞态在 DB 层用**唯一索引 + `DuplicateKeyException` 回退**兜底：

```java
try {
    User created = userRepository.create(newUser);          // 依赖 open_id 唯一索引
    return created;
} catch (DuplicateKeyException e) {
    // 并发冲突 → 回退到再查一次，把已建成的那条拿回来
    return userRepository.findByOpenId(openId).orElseThrow(...);
}
```

| miaowu 做法 / 坑位 | EWF 怎么用 |
| --- | --- |
| **并发建号依赖 DB 唯一约束**：`findByOpenId` 判空 + `create` 之间不是原子的，两个并发登录同一新 openid 可能各建一条 → 用 `open_id` 唯一索引让数据库拒绝第二条，再 catch `DuplicateKeyException` 回退查询。 | **EWF JSON 零库没有唯一约束**（AD-16），不能照搬 catch-约束。MVP 是「单实例单进程 + 单作者同一次登录」，竞态窗口极小，但**逻辑上仍需防重**。改为三种替代策略（任选，推荐 ①+③）： |
| —（策略 ① 进程内互斥） | 登录/建号写操作包一层**进程内锁**：`private final Map<String,Object> LOCKS`（或 `ConcurrentHashMap.computeIfAbsent(openId, k->new Object())`）对**同一 openid** `synchronized(lock)` 执行「读 JSON→判空→写」；不同 openid 不互斥。单进程内即可保证不重复建身份。 |
| —（策略 ② 写前查 + 原子写） | 「写前查」只缩小窗口，不保证原子。EWF 的**真原子**来自 JSON 文件原子写基元（tmp + fsync + rename，S1.1 story）：把「身份文件是否已存在」当作唯一约束——对 `data/identities/{openIdSha256}.json` 用 **`FileChannel.open(CREATE_NEW)` 原子创建**做哨兵：创建成功 = 我是第一个，失败（已存在）= 并发撞车 → 回退读已存在文件。语义等价于 catch `DuplicateKeyException` 回退查询。 |
| —（策略 ③ 明确并发上限 + 幂等） | 若担心 map 锁内存，可直接 `synchronized` 整个「登录建号」方法（单进程内全局锁）：EWF 登录是低频操作（设备端固定身份，不是高并发注册），全局锁最简单且可证正确；配合响应/请求幂等键，重复 code 也不会建两条。 |
| 事务/回滚 | miaowu `@Transactional` 保证建号+tokenVersion 写入同事务。 | EWF 无事务；顺序写多个 JSON 文件时**先写身份再写会话/状态**，任一失败允许「身份已存在但本次登录失败」——幂等建号保证下次登录直接取既有身份，不产生脏账。 |

> 结论一句：EWF 把「DB 唯一索引 + DuplicateKey 回退」翻译成「**身份文件原子创建哨兵 + 进程内按 openid 互斥**」，回退路径照抄 miaowu 的「并发冲突→再查一次返回既有身份」。JSON schema/文件粒度尚未冻结（ASSUMPTION A-2），此处的文件划分到 `sync-contract.md` 收敛时再定稿，但哨兵文件做法与 schema 无关，可先落地。

---

## ⑥ 时区与日界（Asia/Shanghai，EWF 权威在 backend）

| miaowu 做法 | EWF 怎么用 |
| --- | --- |
| **全链路统一 Asia/Shanghai**：`spring.jackson.time-zone: Asia/Shanghai`；PG 连接串带 `options=-c%20TimeZone=Asia/Shanghai`，Hikari `connection-init-sql: SET TIME ZONE 'Asia/Shanghai'`；代码用 `OffsetDateTime`/`java.time` 存时间戳，不存 `LocalDateTime` 裸值。 | EWF **没有数据库**，日界权威完全在 backend 进程（AD-6、FR-B-008）：在 `application*.yml` 固定 `spring.jackson.time-zone: Asia/Shanghai`；所有「今日/近 7/30 日/连续天数」在 backend 按 `ZoneId.of("Asia/Shanghai")` 计算自然日边界并归档；JSON 文件里存 UTC 时刻（`Instant`/epochMs）+ 展示时转 Asia/Shanghai，避免设备时区漂移。 |
| 设备不持绝对时间（miaowu 无此约束，EWF 有） | EWF 设备端**不保存绝对敲击时间**，离线差量由 backend 按**接收/确认时间**归档（AD-6）；「设备端今日」只是非权威本地展示桶，带「待同步/待校准」标记。此约束由 EWF 架构主干给定，与 miaowu 无关，但时区常量统一放 backend 配置即可满足。 |
| 无跨时区用户（作者在国内） | 不引入多时区配置；若未来作者出游，改一处 `ZoneId` 常量即可，不做动态时区。 |

---

## ⑦ 上传大小上限（EWF 默认不需要）

| miaowu 做法 | EWF 怎么用 |
| --- | --- |
| `spring.servlet.multipart.max-file-size: 10MB`、`max-request-size: 12MB`（注释：默认 1MB 太小，相册图易在解析阶段失败）；另有 `app.upload.max-file-size-bytes: 5242880`；`GlobalExceptionHandler` 把 `MaxUploadSizeExceededException` → `40000`「图片过大…」。 | EWF MVP **无图片上传**（无 OBS、无头像、无账本凭证）。**默认删掉** multipart 相关配置与 `MaxUploadSizeExceededException` handler；将来若小程序要做头像/分享图再按此三件套加回（multipart 上限 + 统一错误文案 + 上传校验）。 |

---

## ⑧ env-scripts 本地起停模式（去隧道版）

miaowu `env-scripts/{start-local-test-backend.sh, stop.sh, build-prod-backend.sh}` 的骨架对 EWF 高度可复用；差异只在 **cloudflared 支付回调隧道**（EWF 无支付回调）。

| miaowu 做法 / 坑位 | EWF 怎么用 |
| --- | --- |
| **profile=local + 端口可配**：`start-local-test-backend.sh` 里 `SPRING_PROFILE=${SPRING_PROFILE:-local}`、`BACKEND_PORT=${BACKEND_PORT:-9219}`，最终 `java -jar target/saas.jar --spring.profiles.active=local ...`；`application-local.yml` 用 `spring.config.activate.on-profile: local` 激活。 | EWF 照抄：脚本内置 `PROFILE=local`、端口可被环境变量覆盖；`cloud/backend` 下建 `application.yml` + `application-local.yml` 两件套。S1.1 story 的「可用 env-scripts 按 profile=local 一键起停」即此。 |
| **杀端口**：启动/停止先读 pid 文件，再 `pkill -f` 精确匹配 jar 路径，再 `kill_listener_by_port`（`lsof -tiTCP:$port -sTCP:LISTEN`，先 TERM 后 KILL）。 | 复制 `kill_listener_by_port`（含 lsof 不存在时 netstat 回退）与 `stop.sh` 的 pid 文件 + pkill + lsof 三连。日志/pid 统一放 `logs/`（`APP_LOG_ROOT` 可覆盖），EWF 沿用。 |
| **可读日志 + 启动就绪等待**：`nohup java -jar … > "$BACKEND_LOG_FILE" 2>&1 &`；`wait_for_backend_ready` 循环 `curl http://localhost:$PORT/`，同时 grep 日志里的 `APPLICATION FAILED TO START`/`BeanCreationException`/`Could not resolve placeholder` 等失败指纹，进程退出立即 `tail -n 120` 报错。 | 复制该就绪函数（含失败指纹表）；对 EWF 把探活 URL 改指 `/api/v1/health` 或根信封即可。 |
| **JDK 探测**：`/usr/libexec/java_home -v 17` → Homebrew openjdk@17 兜底，找不到即报错退出。 | EWF 同（Java 17 基线，锚 miaowu pom）。 |
| **向前端写 `.env.development.local`**：启动脚本把公网地址模板写入 `frontend/.env.development.local`（注释掉默认不启用，供真机手动放开；此文件被 .gitignore 排除）。 | EWF 保留「脚本向前端目录写本地 `.env.development.local`（`VITE_API_BASE_URL=http://localhost:${PORT}/api/v1`）」，小程序开发工具 H5/模拟器即可连本机。 |
| **cloudflared 隧道（支付回调用）**：启动 `cloudflared tunnel --url http://localhost:$PORT`，从日志抓 `https://*.trycloudflare.com`，写 `logs/application-local-tunnel.yml`（`jeepay.notify-url`）再以 `--spring.config.additional-location` 载入。 | **EWF 无支付回调，默认去掉这段**。标注：小程序**真机预览**需要合法 HTTPS 域名时（微信公众平台要配 request 合法域名，本地 localhost 不行），**如需要再引入**——届时只需 cloudflared quick tunnel 到 `/api/v1` + WS，无需写任何「回调 yml」覆盖配置。 |
| **build-prod-backend.sh（CI）**：环境变量级联解析项目根（`MIAOWU_PROJECT_ROOT`→`WORKSPACE`→`CI_PROJECT_DIR`→`GITHUB_WORKSPACE`→`PWD`→向上找 `backend/pom.xml`），`mvn -B -U clean package -DskipTests`，校验 jar 产物。 | EWF MVP 无 CI/CD（AD-16 边界），可**先只保留本地 start/stop 两脚本**；若日后要出 jar，照抄 `build-prod-backend.sh` 的项目根级联解析（把 `backend/pom.xml` 换成 `cloud/backend/pom.xml`）。 |

---

## ⑨ EWF 边界与排除表（miaowu 未迁）

只迁「**controller / service / DTO / 信封 / 错误码 / JWT / WechatMiniClient / env-scripts 骨架**」；一切依赖 DB/SaaS/支付/OBS/管理后台的代码均不迁。

| 不迁主题 | miaowu 中的形态 | EWF 排除理由 / 替代 |
| --- | --- | --- |
| **数据库全家桶** | PostgreSQL + Flyway + `JdbcTemplate`/RowMapper + 触发器 + GIS/Haversine + `@Transactional` | EWF JSON 零库（AD-16），单实例单进程原子落盘 `cloud/backend/data/`；repository/SQL/Flyway 整层丢弃 |
| **角色 / 租户 / starId / 订阅冻结** | `highestRole` 自适应、`@RequireRole` AOP、`X-Star-Id`、membership、`@RequireUnfrozen`、`STAR_FROZEN`/`PARTICIPATION_CONFLICT` | EWF 单身份单设备（FR-B-001），无角色/多喵星/订阅；`UserContext` 只留身份映射 |
| **支付与回调** | Jeepay SDK、`/api/v1/pay/*`、SaaS 网关裸接口 `/api/pay/*`、`cloudflared` 回调隧道、幂等补单 | EWF 无支付；`/api/pay` 裸响应与回调 yml 机制整体不迁 |
| **对象存储 / 图片 / 审核** | 华为 OBS 直传策略、`storeWechatAvatarToObs`、`ModeratedMediaService`、上传三接口 | EWF 无 OBS/无头像/无图片上传；`WechatMiniClient` 内 OBS 依赖方法与 `WechatProperties` 无关部分剔除 |
| **web-admin / 调试后台** | `/api/v1/web-login`、`/api/v1/admin/**`、`/api/v1/debug/**`、`AdminAccessService`、`WebAdminProperties` | EWF 无管理后台；filter 只留 PUBLIC/AUTHENTICATED 两档 |
| **账号扩展功能** | 绑手机号（`getPhoneNumberByCode`）、unionId 关联、昵称/头像归一化、`/me/stars`、pending-total | EWF 登录即固定设备，无需手机号/unionId/昵称/头像/多星列表；仅保留 openid→device 映射 |
| **分页列表大而全的读接口** | `PageData`/`PageResponse` 的 discover、rankings、public-gallery 等 | EWF 以确认差量 + 快照水位为主（AD-17），列表读接口收敛到 S0 再定 |

---

### 附：EWF 落地最小文件映射（miaowu → cloud/backend）

| miaowu 源 | EWF 落点 | 处置 |
| --- | --- | --- |
| `common/response/ApiResponse.java` | `common/response/ApiResponse.java` | 原样复制 |
| `common/exception/{ErrorCode,BusinessException,GlobalExceptionHandler}.java` | 同结构 | 复制；ErrorCode 收敛 EWF 业务码，区间位保留 |
| `common/security/{JwtProperties,JwtTokenProvider,JwtAuthenticationFilter,UserContext,SecurityConfig}.java` | 同结构 | 复制；去 role/starId，留 tokenVersion、finally clear |
| `client/WechatMiniClient.java` + `common/config/WechatRestTemplateConfig.java` | 同结构 | 复制 code2Session + 直连 RestTemplate（NoSystemProxySelector）；去掉 phone/wxacode/OBS |
| `service/AuthService.java` / `controller/AuthController.java` | 精简版 | 保留 login/refresh；建号竞态改 §⑤ 原子哨兵；身份存 `data/` JSON |
| `resources/application*.yml` | `application.yml` + `application-local.yml` | 去 DB/jeepay/OBS/aes/web-admin；留 jackson 时区 + wechat.mini + jwt + multipart(删) |
| `env-scripts/*.sh` | 本地 start/stop | 复制；去 cloudflared 回调段（真机预览需要时再加） |
