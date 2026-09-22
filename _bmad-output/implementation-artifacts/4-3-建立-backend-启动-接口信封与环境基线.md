# Story 4.3: 建立 backend 启动、接口信封与环境基线

baseline_commit: fe98d7375f21b02f22ed2cbdb3824b1462940ace

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a backend 开发者，
I want 在固定目录启动 EWF backend 并提供统一接口信封，
so that 设备 mock 和小程序 mock 可以独立联调。

本 Story 覆盖 FR-C-010、FR-C-011、AD-16、AD-20，是 Epic 4 的**第三道门禁**，也是
**backend 的第一个可执行工程**。Story 4.1 钉死了「唯一经文」，4.2 钉死了「唯一协议」
（`docs/contracts/sync-contract.md`，`contract_version` = `SC-1.0.0`）；本 Story 钉死
**唯一信封与唯一本地启动基线**——让 `cloud/backend` 从绿地目录变成「能编译、能起、
能回信封、能失败得可诊断」的进程，供 4.4（微信登录与单设备身份）、4.5（JSON 原子持久化）、
4.6（快照与恢复基准）以及 Epic 5/6 的 mock 直接消费。

**本 Story 只建骨架与信封，不实现任何业务语义。** 不实现登录、令牌、JWT 过滤器、
单设备身份映射（4.4），不实现 JSON 原子持久化写入（4.5），不实现同步/快照/命令/统计/
WebSocket（4.6 与 Epic 5），不实现小程序页面（Epic 6），不修改 `Embedded/`。

## Acceptance Criteria

1. **Given** `cloud/backend` 是绿地目录，
   **When** 使用 Java 17、Spring Boot 3.3.7 基线和 Maven 启动本地 profile，
   **Then** 服务读取 `application.yml`／`application-local.yml`，监听 9218，所有公开接口位于 `/api/v1`，
   **And** 成功返回 `{code:0,message,data}`，业务错误保持 HTTP 200 + 业务码，协议错误使用对应 HTTP 状态。

2. **Given** 本地、生产和 mock 客户端分别调用接口，
   **When** 检查配置和响应，
   **Then** 不读取 miaowu 的支付/数据库/OBS 配置，不启动第二套 backend，敏感会话信息不写入日志，
   **And** `cloud/frontend` 的 `VITE_API_BASE_URL` 与 backend 路径约定可以直接对接。

3. **Given** 契约 §12 已冻结同步域业务码并声明「表中业务码由本契约预留，实现信封时不得重定义」，
   **When** 登记 `ErrorCode` 常量，
   **Then** 契约 §12 的 8 个码值（`20001`–`20006`、`40001`、`40101`）与其 HTTP 语义逐值等于
   `docs/contracts/sync-contract.schema.json` 的 `business_codes`，**期望集合从注册表机械提取**，
   **And** 不新增、不改义、不覆盖同步域码值；身份入口与其余资源的码值留给 4.4 与各模块 spec，
   但码形 `{HTTP 状态}{两位序号}` 与区间位规则在本 Story 固定。

4. **Given** 信封与异常语义已实现，
   **When** 执行 backend 门禁，
   **Then** 单条命令可复现通过（`mvn test`），覆盖信封三键形状、业务错误 HTTP 200 + 业务码、
   协议错误对应 HTTP 状态、`NoResourceFoundException` 不落 500、profile/端口/`data-dir` 基线、
   禁用依赖与禁用配置缺失；
   **And** 关键断言的**负载性**以实测 RED→GREEN 证据记录（故意破坏 → 测试失败 → 恢复 → 通过），
   并如实写出静态校验无法覆盖的边界，不让门禁的承诺强于实现。

5. **Given** `cloud/backend/pom.xml` 就位，
   **When** 实测 `cloud/env-scripts/` 的本地起停与生产构建入口，
   **Then** `build-prod-backend.sh` 能解析项目根、通过 JDK17/Maven 3.9.x 闸门、执行 4.1 交付的
   canonical 经文版本门禁、产出 `cloud/backend/target/saas.jar` 与 `saas.jar.sha256`，
   **And** `start-local-test-backend.sh` 以 `local` profile 在 9218 起服并可被探活，
   `stop.sh` 能回收 PID 与端口；脚本的既有语义不被改写。

6. **Given** 本 Story 落地了 backend 骨架，
   **When** 收敛上游文档与登记册，
   **Then** `ARCHITECTURE-SPINE.md` 的 Deferred 行「EWF backend 构建基线」标注收敛，
   `deferred-work.md` 追加 4-1 review 中明确归属 4.3 的各项收敛登记，
   **And** 本 Story 明确**不闭合**的项（本地入口未接门禁、`.gitignore` 未覆盖运行期产物等）
   以「仍开放」如实登记，不写成已闭合。

## Tasks / Subtasks

- [x] **1. 建立 Maven 工程与 Spring Boot 3.3.7 基线（AC: 1, 2）**
  - [x] 1.1 新建 `cloud/backend/pom.xml`，继承 `spring-boot-starter-parent` **3.3.7**
        （AD-16；版本为冻结基线，见「红线与禁止事项」第 4 条），`<java.version>17</java.version>`，
        `<finalName>saas</finalName>`（AD-20 要求产物为 `target/saas.jar`），
        `<project.build.sourceEncoding>UTF-8</project.build.sourceEncoding>`。
        坐标：groupId `top.zhcmqtt.ewf`、artifactId `ewf-backend`、name 与
        `spring.application.name`（`electronic-wooden-fish-backend`）一致；
        **不得沿用 miaowu 的 `top.zhcmqtt.saaswebpay.backend` 坐标与包名**（避免与迁移排除面混淆）。
        这是纯内部工程选择，不是产品事实；如 dev agent 有更强理由改用其它坐标，必须在
        Completion Notes 说明并保持 `finalName=saas` 与包名自洽。
  - [x] 1.2 依赖面**最小**：`spring-boot-starter-web`、`spring-boot-starter-validation`、
        `spring-boot-starter-test`（`test` scope）。不引入安全、数据、模板、消息、缓存、
        对象存储、支付、HTTP 客户端（`RestTemplate` 属 4.4 微信登录）等 starter；
        全部依赖**不写 `<version>`**（由 parent 统一管理），避免版本漂移。
  - [x] 1.3 构建插件：`spring-boot-maven-plugin`（parent 已管理版本；确保 `repackage` 生效，
        使 `target/saas.jar` 为可执行 Jar）。不额外引入 shade/assembly/jib 等第二套打包方式。
  - [x] 1.4 包结构：基包 `top.zhcmqtt.ewf.backend`；
        `common/response/`、`common/exception/`、`controller/`、`config/`（`config/` 仅在有
        真实配置类时创建，不建空占位目录）。

- [x] **2. 建立启动类与运行期配置基线（AC: 1, 2）**
  - [x] 2.1 新建 `EwfBackendApplication`（`@SpringBootApplication` + `main`），基包为 1.4 的基包，
        使组件扫描覆盖 `controller/` 与 `common/`。
  - [x] 2.2 **校验**（默认不改）已存在的 `application.yml` 与 `application-local.yml`：
        `server.port: 9218`、`app.data-dir: ./data`、`spring.jackson.time-zone: Asia/Shanghai`、
        `application-local.yml` 的 `spring.config.activate.on-profile: local`、
        `wechat.mini.app-id` 与 `jwt` 键存在。若实测发现与 AC1/AD-20 冲突，做**最小**修正并在
        Completion Notes 登记；无冲突则**不作任何改写**（该两文件由 `47e4f18` 提交，属既有交付物）。
  - [x] 2.3 **不得**在任何 `application*.yml` 中写死 `spring.profiles.active`：本地 profile 只由
        `start-local-test-backend.sh` 的 `--spring.profiles.active=local` 激活。生产默认 profile
        必须保持「不带 local」。
  - [x] 2.4 `cloud/backend/data/`：本 Story 只保留 `app.data-dir: ./data` 约定，
        **不创建目录、不写文件、不实现原子写**（属 4.5，且必须与契约 §11/§11.1 一致）。
        禁止引入任何数据库依赖、数据源配置或 `schema.sql`/`data.sql`。

- [x] **3. 实现统一信封与错误语义（AC: 1, 2, 3）**
  - [x] 3.1 `common/response/ApiResponse<T>`：字段 `code` / `message` / `data`，
        `@JsonInclude(JsonInclude.Include.NON_NULL)`（`data == null` 时省略 `data` 键），
        静态工厂 `success()`、`success(T data)`、`error(int code, String message)`；
        成功时 `code = 0`、`message = "success"`。该形状是契约 §12 与 `cloud/AGENTS.md` §3
        定义的**唯一对外信封**，REST 层不得发明第二种裸响应或嵌套信封。
  - [x] 3.2 `common/exception/ErrorCode`：登记两类常量——
        （a）**契约 §12 预留的 8 个同步域码**（值见 Dev Notes「契约 §12 预留码表」），
        逐值一致、不得改义；
        （b）`GlobalExceptionHandler` 实际分支所需的兜底码：`40000`（参数不合法）、
        `40400`（资源不存在）、`50000`（服务器内部错误）。**不预登记**当前无分支引用的
        403xx/409xx/410xx 等区间码（用能解决问题的最少代码）。
        每个码必须能表达其 HTTP 语义；实现方式见 Dev Notes「码形与 HTTP 状态的推导」。
  - [x] 3.3 `common/exception/BusinessException`：唯一业务抛错方式，携带 `code` + `message`；
        提供 `paramError` / `unauthorized` / `notFound` / `conflict` / `serverError` 便捷静态。
        Controller/Service **不得**用返回 `null` 或手工拼 `error` 表达失败。
  - [x] 3.4 `common/exception/GlobalExceptionHandler`（`@RestControllerAdvice`）覆盖：
        - `BusinessException` → HTTP 状态由码推导（`2xxxx` → 200，`4xxxx` → 对应状态），
          body 为信封且 `code`/`message` 原样透出；
        - `MethodArgumentNotValidException` / `ConstraintViolationException` /
          `MethodArgumentTypeMismatchException` → HTTP 400 + `40000`，
          多个校验消息用 `; ` 拼接；
        - `HttpMessageNotReadableException` → HTTP 400 + `40001`（协议字段非法）；
        - `NoResourceFoundException` / `NoHandlerFoundException` → **HTTP 404 + `40400`**；
        - `Exception` 兜底 → HTTP 500 + `50000`，`log.error` 记异常与请求方法/URI，
          **不外泄内部信息**（message 必须是固定文案）。
  - [x] 3.5 **敏感信息不进日志**：本 Story 无鉴权，禁止引入任何打印 `Authorization`、
        请求头全集、`session_key`/`sessionKey`、`openId` 明文的日志；兜底处理器只记
        异常对象、HTTP 方法、URI。为 4.4 的 JWT 与微信会话预留该约束，不得提前实现过滤器。
  - [x] 3.6 探活接口：`GET /api/v1/health` 与 `GET /` **均返回 HTTP 200 + 信封**（`data` 可省略，
        或为最简对象如 `{"status":"UP"}`）。理由有三且必须同时满足：`start-local-test-backend.sh`
        的就绪探针打根路径 `/`；经验文档 `docs/backend/经验-后端REST信封与微信登录.md` §① 记录
        「根路径 `/` 也回信封探活，EWF health 照做」；Spring Framework 6.1+/Boot 3.2+ 起未命中的
        静态资源会抛 `NoResourceFoundException`，若不显式处理会被兜底 `Exception` 吞成 500。
        **不得**因此新增任何业务接口或错误探针接口。

- [x] **4. 建立 backend 门禁（AC: 3, 4）**
  - [x] 4.1 门禁入口固定为 `mvn -q test`（工作目录 `cloud/backend`）。用 JUnit 5 +
        `spring-boot-starter-test`（已由 1.2 引入），**不引入** pytest/其它测试框架，
        也不新增独立的 shell 门禁脚本（`docs/contracts/tests/` 只服务 `docs/contracts/`）。
  - [x] 4.2 **契约一致性断言（AC3 的核心）**：测试读取
        `docs/contracts/sync-contract.schema.json` 的 `business_codes`（每项含
        `condition` / `http_status` / `code`），断言 Java `ErrorCode` 中登记的同名码值
        **逐值相等**（含 HTTP 语义）。期望集合**必须机械提取**，不得在测试里手抄第三份清单
        （沿用 Story 4.2 Task 3.2 的纪律：手抄会在契约与实现之外产生第三真源）。
        仓库根定位方式：从 `System.getProperty("user.dir")` 逐级向上查找同时存在
        `docs/contracts/sync-contract.schema.json` 与 `cloud/backend/pom.xml` 的目录；
        **找不到即失败（fail-closed）**，不得静默跳过。
  - [x] 4.3 **运行时行为断言（MockMvc）**：
        - 信封形状：成功响应的 JSON 恰含 `code`/`message`/`data` 三键，`code == 0`；
          `data == null` 时**不含** `data` 键；
        - 业务错误：抛 `BusinessException(20005, …)` 时 HTTP 状态为 **200** 且 body `code == 20005`；
        - 协议错误：畸形 JSON body → HTTP 400 且 body `code == 40001`；未匹配路径
          （如 `GET /api/v1/not-a-real-endpoint`）→ HTTP **404**（不得是 500）且 body `code == 40400`；
        - 探活：`GET /` 与 `GET /api/v1/health` → HTTP 200 + 信封；
        - 兜底：注入一个抛未捕获异常的分支 → HTTP 500 + `code == 50000`，body `message`
          为固定文案（不出现异常类名或堆栈片段）。
        业务错误分支由**测试作用域内**的 controller（`src/test/java`，仅测试上下文注册）触发；
        生产代码**不得**为了可测而新增错误探针接口。
  - [x] 4.4 **配置基线断言**：默认 profile 下活动 profile **不含** `local`；
        `@ActiveProfiles("local")` 下 `server.port == 9218`、`app.data-dir == ./data`、
        `spring.jackson.time-zone == Asia/Shanghai`；`application-local.yml` 声明
        `on-profile: local`。
  - [x] 4.5 **禁用项 fail-closed 扫描**：扫描 `cloud/backend/pom.xml` 与
        `src/main/resources/*.yml`，命中禁用键即失败：`jdbc`、`jpa`、`mybatis`、`hibernate`、
        `flyway`、`liquibase`、`sqlite`、`postgresql`、`mysql`、`druid`、`h2`、`jeepay`、
        `obs`/`huaweicloud`、`cloudflared`。同时断言 `src/main/resources/` 下**不存在**
        `schema.sql`/`data.sql`。该扫描是 AC2「不读取 miaowu 的支付/数据库/OBS 配置」的
        机械证据，必须真正能失败（新增 `spring-boot-starter-jdbc` 即红）。
  - [x] 4.6 **跨端路径与 AppID 对齐断言**（AC2 后半）：读取 `cloud/frontend/.env` 与
        `.env.development`，断言 `VITE_API_BASE_URL` **以 `/api/v1` 结尾**；断言
        `cloud/frontend/.env` 的 `VITE_WX_APPID` 等于 `application.yml` 的 `wechat.mini.app-id`，
        `.env.development` 的等于 `application-local.yml` 的对应值（AD-20「同一环境必须一致」）。
        只校验一致性，**不得**替换占位值（生产域名/AppID 属部署 Deferred，见「红线」第 6 条）。
  - [x] 4.7 **负载性证据（RED→GREEN）**：对至少 4 条关键断言逐条实测并记录——
        故意破坏 → 记录失败输出 → 恢复 → 记录通过。建议覆盖：`ErrorCode` 改一个码值（4.2 红）、
        把 `NoResourceFoundException` 分支删掉（4.3 的 404 断言红，且应变 500）、
        在 `application.yml` 加 `spring.profiles.active: local`（4.4 红）、
        在 pom 加 `spring-boot-starter-jdbc`（4.5 红）。
  - [x] 4.8 **静态校验边界如实声明**：在测试类头与 Completion Notes 写出门禁**不能**机械判定的
        部分——例如「生产代码不存在第二套响应形状」只能靠白盒评审、`BusinessException`
        是否被正确用于所有失败分支无法静态证明、日志是否泄露敏感信息只做了窄扫描
        （`Authorization`/`session_key` 字面量）而非语义分析。不得让承诺强于实现。

- [x] **5. 实测环境与交付脚本基线（AC: 5）**
  - [x] 5.1 实测 `cloud/env-scripts/build-prod-backend.sh` 全链路：确认它能解析项目根
        （pom 就位后 `is_project_root` 不再提前退出）、JDK17 与 Maven 3.9.x 闸门通过、
        **执行 4.1 交付的 canonical 经文版本门禁**、产出 `cloud/backend/target/saas.jar`
        与 `saas.jar.sha256`。记录命令、退出码与产物 `ls -la` 证据。
  - [x] 5.2 该门禁块的**可达性与负载性**实测（闭合 deferred「打包期 canonical 门禁不可达」）：
        临时制造一次经文版本不一致（例如在临时副本上改动生成物并在恢复前执行；**必须完整恢复**），
        确认脚本以非 0 退出并打印「构建失败：canonical 经文版本校验不通过，停止打包」。
        恢复后回归 `python3 docs/contracts/canonical/scripture_tool.py check` 退出码 0。
        **不得**为制造失败而永久修改 `docs/contracts/canonical/**` 任何文件。
  - [x] 5.3 实测本地起停闭环：`bash cloud/env-scripts/start-local-test-backend.sh`
        （必要时 `SAAS_PAY_SKIP_BUILD=1` 复用已构建 jar）→ 确认 9218 就绪、
        `curl -i http://localhost:9218/api/v1/health` 与 `curl -i http://localhost:9218/`
        均 HTTP 200 + 信封 → `bash cloud/env-scripts/stop.sh` 回收 PID 与端口。
  - [x] 5.4 **不修改** `cloud/env-scripts/**`。若实测发现脚本与本 Story 的固定路径/端口/profile
        约定冲突，按 AD-20 做**最小**修正并在 Completion Notes 与 File List 显式登记；
        无冲突则一行不改。相关现状观察（不修，只记录）见 Dev Notes「已核实的现状与不动的相邻代码」。
  - [x] 5.5 **禁止**执行 `cloud/env-scripts/upload-backend-jars.sh`（含明文 SSH 凭据并会向
        真实云主机上传）。只做静态核对：产物路径 `cloud/backend/target/saas.jar`、
        `.sha256` 校验逻辑、远端目录 `/www/wwwroot/woodenfish` 与 AD-20 一致。

- [x] **6. 收敛上游文档与登记册（AC: 6）**
  - [x] 6.1 在 `ARCHITECTURE-SPINE.md` 的 Deferred 行「EWF backend 构建基线」上按 Story 4.2
        Task 4.3 的既有范式**追加**收敛标注：骨架已落地、Java 17 + Spring Boot 3.3.7 +
        `finalName=saas` + SHA-256 sidecar 已由实际工程构建验证（注明日期与 Story）。
        **只加标注，不重写该表其余行，不改任何 AD 正文。**
  - [x] 6.2 在 `_bmad-output/implementation-artifacts/deferred-work.md` 的
        「Deferred from: code review of 4-1-建立-canonical-心经-与版本校验.md」小节下，
        按该文件既有格式**追加**登记，逐条写清闭合范围：
        - 「打包期 canonical 门禁没有自动验证，且不可达」→ pom 就位后脚本可达，本 Story 已实测
          门禁真的执行且能失败；**仍无 CI**，故只写「可达性已闭合」，不得写成「已自动验证」；
        - 「`start-local-test-backend.sh` 未接入同一条 fail-closed 门禁」→ **仍开放**，
          属产品决策，本 Story 明确不改；
        - 「三端产物 `totalChars` 303 与 `consumableHan` 260…宜在 4.3 落地时以测试固定」→
          本 Story 未引入任何进度分母，已以断言固定「分母唯一来源 = `counts.consumableHan`」
          （见 Dev Notes），并指出真正的分母使用点在 4.6/Epic 5/6；
        - 「`.gitignore` 无 `__pycache__/` 规则」→ **仍开放**，属工作区基建。
        **只追加，不改写他人条目。**
  - [x] 6.3 登记本 Story 实测发现但**不属于本 Story 范围**的两项运行期卫生缺口
        （见 Dev Notes「已核实的现状」）：`cloud/logs/**`（脚本日志与 PID）与
        `cloud/backend/data/**`（4.5 的 JSON 状态）均**未被仓库级 `.gitignore` 覆盖**
        （已用 `git check-ignore` 实测）。按 4.1 既有口径「`.gitignore` 属工作区基建，
        不在 Story diff 范围内」→ **不修改 `.gitignore`**，只登记。
  - [x] 6.4 **不得**修改 `AGENTS.md` / `cloud/AGENTS.md` / `Embedded/AGENTS.md`
        （agent 规则文件，按 4.2 已确认口径不由 Story 修改）；`AGENTS.md` §1/§2 仍称
        `cloud/backend` 为「空占位」这一事实落差只登记，不就地改写。

- [x] **7. 测试与证据（AC: 1–6）**
  - [x] 7.1 `cd cloud/backend && mvn -q test` → 退出码 0，并记录测试计数。
  - [x] 7.2 记录 4.7 的 RED→GREEN 实测（每条含命令、失败诊断首行、恢复后结论）。
  - [x] 7.3 回归既有门禁（本 Story 不改其交付物，只证明未被破坏）：
        `python3 docs/contracts/tests/run_sync_contract_tests.py` 与
        `python3 docs/contracts/canonical/tests/run_canonical_tests.py` 均退出码 0。
  - [x] 7.4 证据纪律：把「信封与启动基线已建立」与「登录/同步/持久化已实现」严格分开记录。
        本 Story 完成后 backend 仍**不能**登录、不能同步、不能持久化；不得写成「设备/小程序
        可联调业务接口」。可联调范围仅限探活与信封。
  - [x] 7.5 `git diff --check` 干净；新增/修改文本文件为 UTF-8 无 BOM、LF、末尾单换行；
        `cloud/backend/src/**` 的 Java 文件保留 UTF-8 源编码与中文注释。

### Review Findings

审查：bmad-code-review（`review_mode=full`、`review_depth=deep`、`action_policy=autofix`、`unattended=true`）。
四个 active/mandatory 层全部完成：blind-hunter、edge-case-hunter、verification-gap、acceptance-auditor。
门禁 `cd cloud/backend && mvn -q test` 退出码 0（补丁前 16/16，补丁后 17/17）。
另有 3 条 defer 与 10 条 rejected，见本节末尾。

**未闭合（阻塞 done）**

- [ ] [Review][Unresolved] 兜底 `@ExceptionHandler(Exception.class)` 把 Spring 协议级客户端错误吞成 500
      [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/exception/GlobalExceptionHandler.java:88-93]
      — 在真机构建产物 `cloud/backend/target/saas.jar` 上实测：`POST /api/v1/health`、`DELETE /api/v1/health`、
      `POST /api/v1/health`（`Content-Type: text/plain`）均返回 `HTTP/1.1 500` +
      `{"code":50000,"message":"服务器内部错误"}`，并以 `HttpRequestMethodNotSupportedException` 记入
      `log.error("未捕获异常: …")` 全栈日志。这与 AC1「协议错误使用对应 HTTP 状态」相悖，且与本 Story
      已修的 404→500 属同一类。修法未唯一确定：需同时裁定纳入哪些异常（405/415/406/缺参）与用哪些码
      （新增 405xx/415xx 会扩张 Task 3.2 冻结的兜底码表，并需同步改
      `ErrorCodeContractTest.FALLBACK_CODES`），属需人工裁定的口径决定，故记为 unresolved。

**已修复（本轮已应用并复测）**

- [x] [Review][Patch] 非 200 业务码的 HTTP 推导无任何运行期门禁
      [EnvelopeContractTest.java:86-95, support/EnvelopeFailureProbeController.java:38-42]
      — 全仓唯一抛 `BusinessException` 的站点用的是 2xxxx 码（20005），`httpStatusOf` 只被静态断言过，
      `BusinessException.paramError/unauthorized/notFound/conflict/serverError` 五个工厂零引用；把
      `handleBusinessException` 改成恒返 200 时 16 项全绿。已新增探针 `GET /__probe/unauthorized`
      （抛 `BusinessException.unauthorized(...)`）与断言「HTTP 401 + 40101 + 双键信封」。
- [x] [Review][Patch] 测试类头声称的「敏感信息字面量窄扫描」在仓库中不存在 [EnvelopeContractTest.java:38]
      — 全仓 `Authorization`/`session_key`/`sessionKey`/`openId` 零命中，且无任何测试捕获日志输出；
      该句会被读成「已有一层窄扫描兜底」。已改写为「无任何机械校验，完全依赖白盒评审」。
- [x] [Review][Patch] 探针控制器的隔离承诺与组件扫描行为不符
      [support/EnvelopeFailureProbeController.java:17-21]
      — 该类位于 `@SpringBootApplication`（基包 `top.zhcmqtt.ewf.backend`）的扫描范围之内，且编译进
      `target/test-classes`，故任何测试上下文都会注册 `/__probe/**`，`@Import` 不构成隔离。
      Javadoc 已按实际行为改写。
- [x] [Review][Patch] `ErrorCode` 分组注释把 40001/40101 归入「HTTP 200 的业务错误」 [ErrorCode.java:18]
      — 契约 §12 与注册表把这两码标为 400/401，本 Story 自己的断言也如此；注释已按实际 HTTP 语义改写。
- [x] [Review][Patch] SPINE 收敛标注的交叉引用指错行 [ARCHITECTURE-SPINE.md:301]
      — 「见上一行」指向的是第 300 行「生产 API 域名与微信 AppID」，框架升级时机的真正登记处是第 296 行
      「Spring Boot 3.3.7 沿用 miaowu pom `[ASSUMPTION A-4]`」；已改为按行名引用。

**已 defer**

- [x] [Review][Defer] 错误信封无法携带 data，与契约 §12 的 20003/20004 拒绝要求相反
      [ApiResponse.java:46-48, EnvelopeContractTest.java:82] — deferred: 契约 §12 要求业务拒绝仍回传
      §6.2 必填字段（20003 的 cloud 基准、20004 的已确认高水位），而 `ApiResponse.error(int,String)`
      的 `data` 恒为 null 且被 `@JsonInclude(NON_NULL)` 删除；唯一命名业务错误的断言又把「无 data」写成
      期望形状。补上它要改错误信封 API 与 Epic 5 的同步语义，超出本骨架 Story。
- [x] [Review][Defer] 新门禁不在任何自动化路径上
      [cloud/env-scripts/build-prod-backend.sh:120, cloud/env-scripts/start-local-test-backend.sh:149]
      — deferred: 两个后端入口都带 `-DskipTests`，仓内无 CI；实测复现本 Story 的 RED 实验 B 之后
      `build-prod-backend.sh` 仍退出码 0 并产出可上传的 `saas.jar`。本 Story 明令不得改
      `cloud/env-scripts/**`（Task 5.4），CI 接入已作为「仍开放」登记。
- [x] [Review][Defer] AppID 占位值被字面量化进门禁 [RuntimeBaselineTest.java:40-41,85,117-120]
      — deferred: `PRODUCTION_APP_ID`/`LOCAL_APP_ID` 把占位值抄成第三个真源，部署阶段按计划替换 AppID 时
      本门禁必然变红，且失败文案会写成「应与 `.env` 的 AppID 一致」（实际两边一致）。替换检查按红线 6
      属部署阶段，本 Story 不宜就地收敛。

**Rejected**

- `ErrorCodeContractTest` 无法检出同 HTTP 类内的码值互换（AC3「不改义」无机械证据） — low：
  `registeredCodeOf` 按码值反查，故 `http_status == 值/100` 只校验注册表自洽；但闭合它需要
  「常量名 ↔ 契约 condition」映射，正是 Task 4.2 明令不得引入的第三真源，且触发前提是一次不合常理的改值编辑。
- `BusinessException.conflict()`/`unauthorized()` 复用冻结码有改义风险 — false：无任何调用方，
  且 Javadoc 已禁止把 `conflict` 用于重复/并发等其它语义；Task 3.2 禁止预登记无分支的区间码，
  复用已登记码是本 Story 唯一合规解。
- AC1「所有公开接口位于 `/api/v1`」与 `GET /` 探活冲突 — false：Task 3.6 与 Task 4.3 明确要求
  `GET /` 回信封（`start-local-test-backend.sh` 的就绪探针即打根路径），属 spec 内部张力。
- `docs/backend/经验-后端REST信封与微信登录.md` 被改却不在此 diff — false：该文件与 `before-dev`
  快照逐字节一致（sha256 相同），属快照建立之前的既有改动，不属本 Story。
- `ConstraintViolationException`/`MethodArgumentTypeMismatchException` 两个分支零覆盖 — low：
  全仓无 `@RequestParam`/`@PathVariable`/`@Validated`，两条分支当前不可达；修法为新增探针与断言。
- `MethodArgumentNotValidException` 未显式写 `message` 时回英文默认文案 — low：本 Story 无生产 DTO，
  无可达路径；修法属 4.4 的消息包或回退设计。
- `BusinessException` 的 message 为 null 时信封缺 `message` 键 — low：本 Story 无产生 null message
  的调用方；修法为新增分支。
- 禁用依赖扫描只覆盖 `src/main/resources` 顶层 `*.yml`（漏 `.yaml`/`.properties`/子目录） — low：
  与 Task 4.5 明文的扫描面一致，扩面超出本 Story 冻结范围。
- `ErrorCodeContractTest` 的反射只收 `public static final int` — low：只有以非 final/非 int 形式
  声明码值才能绕过，无可达写法。
- `code / 100` 落到 1xx 时信封不可达 — false：无任何已登记码或调用方能产生 1xxxx 前缀。

**本轮复核（2026-09-22，attempt-2，`review_depth=deep`）**

四个 active/mandatory 层全部完成（blind-hunter、edge-case-hunter、verification-gap、acceptance-auditor）。
门禁 `cd cloud/backend && mvn -q clean test` 退出码 0（补丁前 18/18，补丁后 19/19）。

上一轮唯一未闭合项的**可复现现象已闭合**（本轮独立复核：真机 `saas.jar` + `local` profile）：
`POST`/`PUT`/`DELETE /api/v1/health` → `405` + `Allow: GET` + `{"code":40500,…}`；`Accept: application/xml` → `406` + 空 body；
`GET /api/v1/not-a-real-endpoint` → `404` + `40400`；后端日志 `ERROR` 计数 0、协议错误 `WARN` 6 条（仅 method+uri）、堆栈 0 行。
修复者对 406 分支「状态断言不具负载性、真正钉住它的是日志级别断言」的自披露经 `loadbearing-L3-406.log` 逐行核对**准确**；
补充：同处的 body 空串断言同样不负载。
**406 裁定维持**（不登记 `40600`、不投递 body）：AC1 对协议错误只约束「使用对应 HTTP 状态」；RFC 9110 §15.5.7 的 406 payload
是 SHOULD 且内容为「可用表示列表」，不是本服务的错误信封；客户端已声明不接受 JSON 时投递 `application/json` 会让响应自相矛盾。

**本轮已修复（已应用并复测）**

- [x] [Review][Patch] 生产默认 profile 的端口/`data-dir`/时区无任何断言，变异后门禁仍绿
      [RuntimeBaselineTest.java:76-84] — 旧门禁只在 `localContext` 上断言三键；实测把 `application.yml` 改成
      `8080 / UTC / /tmp/ewf-prod-data` 后 18 项全绿。已补默认上下文断言；同一变异下变红
      （`server.port 应为 9218 ==> expected: <9218> but was: <8080>`），恢复后 sha256 回到变异前值。
- [x] [Review][Patch] `HandlerMethodValidationException` 未映射，方法参数约束失败落兜底 500
      [GlobalExceptionHandler.java:59-72] — 隔离副本实测：控制器方法参数上的约束注解（类上不加 `@Validated`）抛该异常
      （自带 HTTP 400 语义），被兜底 `Exception` 分支吞成 `500` + `{"code":50000,…}`。已新增显式分支回 `400` + `40000`
      （复用既有码，不扩张码表），并新增探针 `GET /__probe/constrained-name` 与断言钉住其可失败性。
- [x] [Review][Patch] 门禁边界声明未覆盖应用层的两个非信封出口 [EnvelopeContractTest.java:44-51]
      — `/error`（Spring Boot 自动装配的 `BasicErrorController`，实测 `GET`/`POST /error` 回 `500` +
      `{"timestamp":…,"status":999,"error":"None"}`，`Accept: text/html` 回 Whitelabel HTML）与框架内建 `OPTIONS`
      （`200` + 空 body）都经 DispatcherServlet 却不受本 advice 约束，原声明只承认容器级错误；已按 Task 4.8 如实补入。

**本轮 defer**

- [x] [Review][Defer] `/error` 是可达的非信封 500 出口 [GlobalExceptionHandler.java:1-41, ApiResponse.java:12] — deferred:
  消除它需自定义 `ErrorController` 或改 `application.yml`，分别触碰红线 2「生产代码面只有 Task 1–3 列出的文件」与
  AD-20「`application.yml` 按裁决不改」，属后续故事的产品决策；本轮只把承诺与实现对齐（见上一条 patch）。
- [x] [Review][Defer] 契约门禁无法发现同 HTTP 类内的码值改义交换 [ErrorCodeContractTest.java:51-64] — deferred:
  实测把 `DEVICE_RESET_CONFLICT`(20003) 与 `QUEUE_FULL`(20004) 互换后 18 项全绿。机械绑定常量名与契约 `condition`
  正是 Task 4.2 明令避免的「第三真源」；码值被端点引用时（Epic 5）语义会自然钉住。
- [x] [Review][Defer] 新增 `40500`/`41500` 未同步 `docs/backend/经验-后端REST信封与微信登录.md` 的区间表与映射表
      [ErrorCode.java:52,55] — deferred: 该文档的 `code 区间表` 无 `405xx`/`415xx`、映射表无 405/415/406 行，且把
      `40002`/`40003` 留给缺参与类型不匹配（本实现复用 `40000`）；修法是改 `docs/**`，而 spec「明确不触碰」列表含
      `docs/**`，且该文档自述「迁移经验（实现参考，非产品事实）」。
- [x] [Review][Defer] `Accept` 不含 JSON 时错误分支的信封不可投递 [GlobalExceptionHandler.java:59-93] — deferred:
  真机实测 `Accept: application/xml` 下 `404`/`405` 状态正确但 body 为空（`Failure in @ExceptionHandler` WARN）；
  隔离副本中业务错误分支外抛为 `ServletException`（真机即 500）。与 406 裁定同源，强制投递要改内容协商策略。

**Rejected**

- `Accept: application/xml` 下 406 应强制投递 JSON 信封并登记 `40600` — false：见上文本轮裁定，AC1 只约束 HTTP 状态，
  且投递 `application/json` 与客户端的 `Accept` 声明自相矛盾；与 Spring 默认解析器同形。
- 「`ErrorCode` 扩张 Task 3.2 冻结的兜底码表属越权」 — false：Task 3.2 的原则是「不预登记**无分支引用**的码」，
  `40500`/`41500` 各有 `@ExceptionHandler` 分支引用，正是上一轮 review 预告的修法；码形自洽（`/100` 分别为 405/415）。
- 「新增分支与第 8 个测试在 spec 的 Debug Log References 中没有 RED→GREEN 条目」 — false：证据在 `dev-story.json`
  （5 条 `load_bearing_evidence` + 3 条 `red_green_evidence`），9 个日志文件经本轮逐个核对存在，L1/L2/L4/L5 诊断与
  断言一致；spec 内记录陈旧属文档同步，修法即编辑被审查的 spec。
- 「AC2 的『敏感会话信息不写入日志』在门禁中无机械证据」 — false：`EnvelopeContractTest` 类头已如实写明「无任何机械
  校验，完全依赖白盒评审」，门禁承诺未强于实现；与 Task 4.8 例示的落差属 spec 记录陈旧。
- 「AC6 的 `deferred-work.md` 登记在 diff 中无证据」 — false：该文件确有 2026-09-22 的 4.3 收敛/新增登记
  （第 18–22、83–86 行），只是被编排器列入 `excluded_changed_paths`，不在 `story-local.diff` 的 15 条路径内。
- `OPTIONS /api/v1/health` 回 200 但无信封 body — low：Spring MVC 内建 OPTIONS 处理，CORS preflight 场景客户端不读 body；
  修法是新增分支或配置，属复杂化。
- `BusinessException`/`error()` 的 `message` 为 null 时信封退化为单键 — low：本 Story 无产生 null message 的调用方。
- `FieldError.getDefaultMessage()` 为 null 时拼出字面量 `null` — low：本 Story 无生产 DTO，无可达路径。
- 禁用依赖扫描只覆盖 `src/main/resources` 顶层 `*.yml`、词表未含 hsqldb/derby/mongodb/redis — low：与 Task 4.5
  明文的扫描面与词表一致，扩面超出本 Story 冻结范围。
- `ErrorCodeContractTest` 硬编码 `businessCodes.size() == 8` — low：契约合法扩张时变红是要求实现同步的合理信号，
  集合相等断言已完整钉住，条数断言冗余但无害。
- `ScriptureResourceBaselineTest` 用 `String.length()` 计 UTF-16 码元而 `consumableHan` 计汉字数 — low：
  canonical 文本全为 BMP 字符，未演示可达。
- 外部环境变量 `SPRING_PROFILES_ACTIVE=local` 会污染默认上下文断言 — low：属测试运行环境假设，修法为启动前显式隔离 profile。

## Dev Notes

### 真源与冲突裁决

按 `docs/README.md` 事实优先级读取，冲突时高者覆盖低者：

| 事项 | 真源 |
| --- | --- |
| Story 需求与 AC | `epics.md` §Epic 4 / Story 4.3（覆盖 FR-C-010、FR-C-011、AD-16、AD-20） |
| 信封、错误码、鉴权语义 | `prd.md` §6.2.4 FR-C-010、§6.2.5 FR-C-011；`cloud/AGENTS.md` §3/§4；`AGENTS.md` §3.6 |
| 同步域业务码（**冻结，不得重定义**） | `docs/contracts/sync-contract.md` §12 与 `sync-contract.schema.json` 的 `business_codes` |
| 持久化文件粒度与落点 | `docs/contracts/sync-contract.md` §11 / §11.1（本 Story 只引用，不实现） |
| backend 技术基线 | `ARCHITECTURE-SPINE.md` AD-16（Java 17 + Spring Boot 3.3.7 + Maven、零库、`/api/v1` 信封） |
| 环境与交付脚本契约 | `ARCHITECTURE-SPINE.md` AD-20；`ARCHITECTURE-SPINE.md` §Consistency Conventions「环境与交付」行 |
| 迁移经验（实现参考，非产品事实） | `docs/backend/经验-后端REST信封与微信登录.md` §① §② §⑧ 与「附：EWF 落地最小文件映射」 |
| 扩展路线与排除面 | `docs/backend/README.md` 的「EWF 边界」与「排除表」；`docs/backend/经验…md` §⑨ |
| 经文版本与消费语义 | `docs/contracts/canonical/heart-sutra.manifest.json`（只引用） |

已核实的现状与裁决（本 Story 必须按此执行，不得来回摇摆）：

1. **`cloud/backend` 是无工程文件的绿地目录。** 实测：`cloud/backend/` 下只有
   `src/main/resources/{application.yml,application-local.yml,canonical/heart-sutra.json}`，
   **无 `pom.xml`、无 `src/main/java`、无 `src/test`**。故本 Story 新建 Maven 工程与首个 Java 源码，
   并**不得**移动或改写已有的三份资源文件。
2. **`application*.yml` 已存在且已 EWF 化。** 由 `47e4f18` 提交：端口 9218、`app.data-dir: ./data`、
   `spring.jackson.time-zone: Asia/Shanghai`、`application-local.yml` 带
   `spring.config.activate.on-profile: local`、`jwt.*` 与 `wechat.mini.*` 均为 EWF 占位值。
   → **裁决：默认不改**，只做校验（Task 2.2）；任何修改都必须是最小且可追溯的。
3. **env-scripts 已按 AD-20 落地且不含隧道/支付残迹。** 实测 `grep` 无 `cloudflared`/`miaowu`/支付字样；
   `build-prod-backend.sh` 要求 `cloud/backend/pom.xml`（故 4.1 的 canonical 门禁块此前**不可达**，
   正是本 Story 要闭合的点）；`start-local-test-backend.sh` 的 `BACKEND_DIR` 解析为 `cloud/backend`
   且 `stop.sh` 的 PID/端口清理只服务本项目。→ **裁决：不改脚本**（Task 5.4）。
4. **`docs/contracts/sync-contract.md` 与注册表已冻结（`SC-1.0.0`）。** 本 Story 的信封与错误语义
   **必须与契约 §12 一致**；若发现冲突，以契约为准并在 story 中显式引用，不得另立一套。
   注意契约的 `.schema.json` 是「派生自正文，供 Epic 4–6 的 mock 与用例直接消费」，本 Story 的
   契约一致性断言应读**注册表**（机器可读），并在测试头注明它是派生物而非第二真源。
5. **`totalChars` 与 `consumableHan` 不可混用。** 实测 backend 经文资源为
   `counts.totalChars = 303`、`counts.consumableHan = 260`；进度分母的**唯一**来源是
   `consumableHan`（PRD 术语与 canonical README 口径）。本 Story 不引入任何分母，
   但须以断言固定「分母唯一来源」，防止下游直接取 303（4-1 review 已登记的风险）。
6. **前端环境文件与后端 AppID 已对齐。** 实测 `cloud/frontend/.env` 与 `application.yml`
   同为 `replace-with-ewf-wechat-app-id`，`.env.development` 与 `application-local.yml`
   同为 `replace-with-ewf-local-wechat-app-id`。→ 断言一致性即可（Task 4.6），**不替换占位值**。
7. **`NoResourceFoundException` 是 Boot 3.3 的真实坑。** Spring Boot 3.2 / Framework 6.1 起，
   未命中的路径由 `ResourceHttpRequestHandler` 抛 `org.springframework.web.servlet.resource.NoResourceFoundException`；
   若被兜底 `@ExceptionHandler(Exception.class)` 捕获，会**从 404 变成 500**（本轮 web 复核，
   来源见 References）。而 `start-local-test-backend.sh` 的就绪探针恰好打根路径 `/`。
   → **裁决：显式登记 `NoResourceFoundException` 分支（并兼容 `NoHandlerFoundException`），
   探活接口显式写 controller**（经验文档 §② 已记录同样的坑：Spring 6 把 `GET /` 落到
   `ResourceHttpRequestHandler`，故健康检查必须显式写 controller）。
8. **本 Story 是本 Epic 唯一「从零建工程」的 Story，最容易越界。** 4.1/4.2 是纯文档与门禁，
   本 Story 一落地就会同时具备「可以顺手写登录」「可以顺手写持久化」的诱惑。
   → **裁决：生产代码面只有启动类 + 信封 + 异常 + 探活。** 任何更多接口、`SecurityConfig`、
   `JwtAuthenticationFilter`、`WechatMiniClient`、`data-dir` 读写都属越界，按「红线与禁止事项」驳回。

### 上游故事与仓库情报

从 Story 4.1（`4-1-建立-canonical-心经-与版本校验.md`，`done`）与 4.2
（`4-2-冻结跨层同步契约.md`，`done`）继承的可复用结论：

- **判定真源不得是可手改的生成物。** 4.1 的审查把版本绑定从「manifest 派生字段」换成「追加式台账 +
  本次从源文本派生的值」；4.2 把契约门禁的期望集合改为从 `prd.md`/spine **机械提取**。
  本 Story 同理：契约一致性断言的期望值**只能来自注册表**，不得在 Java 测试里手抄第三份。
- **门禁承诺不得强于实现。** 4.1 的 `README.md` 与 4.2 的门禁文件头都显式写出「静态校验范围」；
  本 Story 的 Task 4.8 沿用同一纪律。
- **门禁范式**：本机 `python3` 无 `pytest`，故 `docs/contracts/` 一律用纯标准库 + `assert` + 非 0 退出。
  本 Story 属于 `cloud/backend` 模块，其门禁**用 `mvn test`**（JUnit 5 + MockMvc），不新建第二套
  Python 门禁脚本，也不去改 `docs/contracts/tests/`。
- **Epic 边界**：只有 Epic 7 允许同时改 `Embedded/` 与 cloud。本 Story 属 Epic 4，
  **不得修改 `Embedded/` 任何文件**（含 `Embedded/tests/run_host_tests.py`、`Embedded/AGENTS.md`）。
- **不把「工具跑通」写成「端到端可用」**：4.1 的纪律是区分「check 通过」与「三端已能显示经文」；
  4.2 是「契约已冻结」≠「同步已实现」；本 Story 是「信封与启动基线已建立」≠「可登录/可联调业务」。
- **文档卫生**：4.1/4.2 的交付物经扫描不含 Node ID / `data-pencil-id` / `TODO` / `draft` /
  `placeholder`；本 Story 的 Java 注释、测试与故事文本同样受此约束（`AGENTS.md` §3.7）。

仓库情报（本次只读核实）：

- **git**：`HEAD = fe98d7375f21b02f22ed2cbdb3824b1462940ace`（本 Story 的 `baseline_commit`）。
  最近提交：`fe98d73`（.bmad 与 code-review 技能）、`47e4f18`（PWR 更正、`cloud/AGENTS.md`、
  `application*.yml`、env-scripts）、`91467c8`（清理 .bmad 过程文件）。
- **`.gitignore` 现状**（只读核实）：`target/` 与 `*.jar` 已覆盖 → `cloud/backend/target/**` 不会入库；
  但 `cloud/logs/**` **未覆盖**（`logs/*` 带斜杠被锚定在仓库根，不匹配 `cloud/logs/`），
  `cloud/backend/data/**` **未覆盖**。已用 `git check-ignore -v` 逐条实测。
- **本机工具链已实测可用**（这是本 Story 可被真实执行的前提）：

  | 工具 | 实测值 | 与基线的关系 |
  | --- | --- | --- |
  | `java` | `openjdk 17.0.19`（Homebrew） | 满足 `build-prod-backend.sh` 的「必须 JDK 17」闸门 |
  | `JAVA_HOME` | `/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home` | `start-local-test-backend.sh` 可直接使用 |
  | `mvn` | `Apache Maven 3.9.16` | 满足「必须 3.9.x」闸门（`case 3.9.*`） |
  | `python3` | `3.12.13` | canonical 与契约门禁的运行前提 |
  | `~/.m2` | 已缓存 `spring-boot-starter-parent` **3.3.7**、`starter-web/validation/test/security` **3.3.7**、`spring-boot-maven-plugin` **3.3.7** | 选 3.3.7 可基本离线解析；换版本会额外下载 |
  | Maven Central | `HTTP 200`（已实测可达） | 缺失的传递依赖可正常拉取 |

  → **结论：本 Story 可以真正构建、真正起服、真正跑门禁**，不需要降级为「纸面交付」。
  （对照 4.2 的 Dev Notes：当时只能产出文档，因为 pom 不存在。）

### 工程基线规格（Task 1）

```text
cloud/backend/
  pom.xml                                   # 新增：parent 3.3.7、java 17、finalName=saas
  src/main/java/top/zhcmqtt/ewf/backend/
    EwfBackendApplication.java              # 新增：@SpringBootApplication + main
    common/response/ApiResponse.java        # 新增：唯一对外信封
    common/exception/ErrorCode.java         # 新增：契约 §12 码 + 兜底码
    common/exception/BusinessException.java # 新增：唯一业务抛错
    common/exception/GlobalExceptionHandler.java  # 新增：@RestControllerAdvice
    controller/HealthController.java        # 新增：/ 与 /api/v1/health 探活
  src/main/resources/
    application.yml                         # 已存在：只校验，默认不改
    application-local.yml                   # 已存在：只校验，默认不改
    canonical/heart-sutra.json              # 已存在：Story 4.1 交付，禁改
  src/test/java/top/zhcmqtt/ewf/backend/
    support/EnvelopeFailureProbeController.java   # 新增：仅测试作用域的错误分支探针
    EnvelopeContractTest.java               # 新增：信封/异常/HTTP 语义
    ErrorCodeContractTest.java              # 新增：契约注册表逐值一致性
    RuntimeBaselineTest.java                # 新增：profile/端口/data-dir/前端对齐
    ForbiddenDependencyScanTest.java        # 新增：禁用依赖与禁用配置 fail-closed
```

约定：

- **不写 `<version>`**：starter 与插件版本由 `spring-boot-starter-parent` 3.3.7 统一管理，
  这是防止「版本漂移」的第一道闸门。
- **`finalName=saas`**：`build-prod-backend.sh`、`upload-backend-jars.sh`、
  `start-local-test-backend.sh` 三处都硬编码 `target/saas.jar`，故 `finalName` 是被脚本
  反向约束的**契约值**，不是风格偏好。
- **中文注释**：与仓库既有风格一致（`application.yml` 头部即为中文注释）；
  Java 类注释用「做什么 + 为什么」，不写设计过程与作者提示（`AGENTS.md` §3.7）。
- **不建空目录/空占位类**：`config/` 只在真有配置类时才建；不写 `TodoConfig` 之类占位。

### 契约 §12 预留码表（ErrorCode 必须逐值一致）

从 `docs/contracts/sync-contract.schema.json#business_codes` 机械提取（本 Story 只登记码值与语义，
**不实现同步域行为**；行为属 Epic 5）：

| code | HTTP 语义 | condition（契约原文） | 本 Story 的处置 |
| --- | --- | --- | --- |
| `20001` | 200 | 轮次无法归属 | 只登记常量 |
| `20002` | 200 | 缺少基准高水位 | 只登记常量 |
| `20003` | 200 | 设备重置冲突 | 只登记常量 |
| `20004` | 200 | 队列已满 | 只登记常量 |
| `20005` | 200 | 经文版本不一致 | 只登记常量 |
| `20006` | 200 | 命令旧修订 | 只登记常量 |
| `40101` | 401 | 令牌失效 | 只登记常量（鉴权实现属 4.4） |
| `40001` | 400 | 协议字段非法 | **本 Story 有真实分支**：`HttpMessageNotReadableException` |

兜底码（`GlobalExceptionHandler` 实际分支引用，来自 `docs/backend/经验…md` §② 的区间表，
**不是**契约冻结值，4.4 及后续模块可扩但不得改义）：

| code | HTTP | 用途 |
| --- | --- | --- |
| `40000` | 400 | 参数不合法（`@Valid` 绑定失败、类型不匹配） |
| `40400` | 404 | 资源不存在（未匹配路径 / 静态资源未命中） |
| `50000` | 500 | 服务器内部错误（兜底，message 固定文案） |

### 码形与 HTTP 状态的推导

**不要**在 `ErrorCode` 之外另建一张 `code → httpStatus` 的映射表（那会产生第三真源）。
码形 `{HTTP 状态}{两位序号}` 本身即携带 HTTP 语义：`httpStatus = code / 100`。
校验：`20001/100 = 200` ✓、`40001/100 = 400` ✓、`40101/100 = 401` ✓、`40400/100 = 404` ✓、
`50000/100 = 500` ✓。因此：

- `GlobalExceptionHandler` 处理 `BusinessException` 时用 `code / 100` 推导 HTTP 状态；
- 契约一致性断言除比 `code` 外，还比 `http_status`，用同一规则从 Java 常量推导并逐值核对
  ——这样「码值与 HTTP 语义」两侧同时被钉住，且**不需要**在 Java 里重复一份 `http_status` 字段。
- 若某天需要登记不符合 `{HTTP 状态}{两位序号}` 的码（例如 429 限流 `40901` 这类
  「业务码前缀与真实 HTTP 不一致」的历史形态），必须显式例外并在测试中单独列出；
  **本 Story 不引入**这类码。

### 信封规格的边界（做与不做）

- **做**：三键信封、`code=0` 成功、`data` 为 null 时省略、业务错误 HTTP 200 + 业务码、
  协议错误对应 HTTP 状态、兜底 500 固定文案、探活两处。
- **不做**：分页 `PageData`（经验文档 §① 指出 EWF 记录页/回放以「已确认差量 + 序号」为主，
  大概率不需要分页）；**不得**照搬「信封套分页信封」的嵌套形态；不建裸响应路径
  （EWF 无支付网关；唯一非信封通道是 WebSocket，属 Epic 5 且由契约定义）。
- **不做**：`@JsonInclude(NON_NULL)` 之外的自定义序列化；不引入 `ResponseBodyAdvice` 全局包装
  （信封由 controller 显式返回，避免「隐式包装 + 显式包装」双路径）。
- **可测性反模式**：不得为测试而在生产代码暴露错误探针接口；错误分支探针只存在于
  `src/test/java`。
- **不需要额外的 MVC 配置**：**禁止**为了 404 而引入
  `spring.mvc.throw-exception-if-no-handler-found=true` 或 `spring.web.resources.add-mappings=false`
  —— 二者是「已处理 `NoResourceFoundException`」的替代方案，同时使用会产生双路径；
  且 `add-mappings=false` 会关闭静态资源映射，属无需求的副作用。
  → 只保留 `@ExceptionHandler(NoResourceFoundException.class)` 一条路径。
- **信封保证的边界**：信封只覆盖**经 DispatcherServlet 的**请求。容器级错误
  （畸形请求行、TLS 握手失败、请求体超限等）由容器直接应答，不在本 Story 的信封承诺内；
  该边界必须写进 Task 4.8 的静态校验边界说明，不得让 AC1 的「所有公开接口」被读成
  「任何字节级异常都回信封」。

### 已核实的现状与不动的相邻代码（Task 5.4）

以下均为实测现状，**本 Story 不改**，只在此显式记录以免 dev agent 误判为「顺手可修」：

- `cloud/env-scripts/start-local-test-backend.sh` 的跳过编译开关读的是
  `${SAAS_PAY_SKIP_BUILD:-0}`，`SAAS_PAY_` 前缀是 miaowu 时代的环境变量名。
  AD-20 点名的是「不启动 cloudflared、不生成支付回调覆盖文件、不匹配 miaowu 路径」，
  未点名环境变量名；改它会影响既有使用习惯，属环境脚本命名口径。
  → **裁决：不改**，登记为本 Story 的既有现状。
- `stop.sh` / `start-local-test-backend.sh` 的 `LOG_DIR=${APP_LOG_ROOT:-$PROJECT_ROOT/logs}`
  落在 `cloud/logs/`（`PROJECT_ROOT` = 脚本所在目录的上一级 = `cloud/`），该路径**未被**
  仓库级 `.gitignore` 覆盖。→ 不改 `.gitignore`，只登记（Task 6.3）。
- `build-prod-backend.sh` 在 JDK/Maven 闸门之后才检查 python3 与 canonical 工具；
  本 Story 只需确认其**实际执行到**该块并可失败，不调整其顺序。
- `cloud/backend/src/main/resources/canonical/heart-sutra.json` 是 Story 4.1 的生成物，
  **不可手改**（4.1 已确立：判定真源不得是可手改产物）。本 Story 只读它做断言。

### 红线与禁止事项

1. **Epic 边界**：不得修改 `Embedded/` 任何文件（Epic 7 才允许双改）。
2. **不得越界实现业务**：不实现微信登录 / `code2Session` / `WechatMiniClient` / JWT /
   `SecurityConfig` / 单设备身份映射（4.4）；不实现 JSON 原子写、`fsync`、`rename`、
   `data/` 读写（4.5）；不实现快照、差量、同步、轮次、命令、统计、WebSocket（4.6 与 Epic 5）；
   不实现任何小程序页面（Epic 6）。生产代码面**只有** Task 1–3 列出的文件。
3. **不得引入数据库与第二套平行实现**：无 JDBC/JPA/MyBatis/Flyway/Liquibase/H2/SQLite/
   PostgreSQL/MySQL/Druid；无第二个 `@SpringBootApplication`、无第二个可执行模块、
   无第二套响应信封、无第二个 Web 框架。
4. **不得升级框架版本**：Spring Boot 固定 **3.3.7**（AD-16 与 `[ASSUMPTION A-4]`）。
   3.3.x 的 OSS EOL 与「升级目标 4.0.x/4.1.x」属**已登记 Deferred**，触发条件为
   「backend 功能稳定、进入文档化部署前」，**不在本 Story**。同理不得顺手升 Java 版本。
5. **不得改冻结交付物**：`docs/contracts/canonical/**`（经文与工具）、
   `docs/contracts/sync-contract.md` 与 `.schema.json`、`docs/contracts/tests/**`、
   `cloud/backend/src/main/resources/canonical/heart-sutra.json`、
   `cloud/frontend/src/canonical/heart-sutra.generated.ts` 一律不动。
   发现契约与本 Story 需求冲突时，**以契约为准**并在 story/登记册中显式引用，不自行改契约。
6. **不得替换部署占位值**：生产域名、微信 AppID、JWT secret、SSH 凭据属部署 Deferred；
   `.env` 与 `application*.yml` 的占位值保持原样（只断言一致性）。占位值门禁（fail-closed 替换检查）
   属部署阶段，不在本 Story。
7. **不得执行上传脚本**：`upload-backend-jars.sh` 会向真实云主机写入，禁止执行（Task 5.5）。
8. **不得修改 agent 规则文件**：`AGENTS.md`、`cloud/AGENTS.md`、`Embedded/AGENTS.md` 不改。
9. **不得修改工作区基建**：`.gitignore` 不改（属工作区基建；`git status` 显示它已被并发会话修改）。
10. **不得动工作区在制品**：`git status` 显示 `Embedded/**`、`.agents/skills/**`、`.claude/skills/**`
    等有并发会话的未提交改动，**不得修改、不得回滚、不得清理**，也不要因为看到它们而扩大本 Story 范围。
11. **文案与编码卫生**：Java 注释、测试名、story 文本不得出现 AI 思维链、Node ID、
    `data-pencil-id`、`TODO`、`draft`、`placeholder`、作者提示（`AGENTS.md` §3.7、
    `cloud/AGENTS.md` §5）。所有文本文件 UTF-8 无 BOM、LF、末尾单换行；
    见到 `纭/锛/涓` 或 `FF FE` 立即停止并重写。

### 测试与完成判定

| 入口 | 期望 | 说明 |
| --- | --- | --- |
| `cd cloud/backend && mvn -q test` | 退出码 0 | 本 Story 的模块门禁；覆盖 Task 4.2–4.6 |
| `python3 docs/contracts/tests/run_sync_contract_tests.py` | 退出码 0 | 回归（不改契约） |
| `python3 docs/contracts/canonical/tests/run_canonical_tests.py` | 退出码 0 | 回归（不改经文） |
| `bash cloud/env-scripts/build-prod-backend.sh` | 退出码 0，产出 `target/saas.jar` + `.sha256` | 实测环境基线（Task 5.1） |
| `bash cloud/env-scripts/start-local-test-backend.sh` + `stop.sh` | 9218 起服/回收，探活 200 | 实测本地起停（Task 5.3） |
| `git diff --check` | 干净 | 无空白错误 |

通过标准：pom 与 Java 骨架存在且可构建；`mvn test` 全绿且 4.7 的 RED→GREEN 证据表明关键断言
**真的能失败**；契约 §12 的 8 个码值与注册表逐值一致；探活在 `/` 与 `/api/v1/health` 均 200
且未匹配路径为 404 而非 500；本地起停与生产构建入口在真实工具链下跑通；上游 Deferred 行与
登记册已按 AC6 收敛且**不夸大**。

**不得**把「信封与启动基线跑通」写成「backend 已可登录 / 已可同步 / 已可持久化 / 端到端可用」。

## Project Structure Notes

- 新增 `cloud/backend/pom.xml` 与 `src/main/java/**`、`src/test/java/**`：`ARCHITECTURE-SPINE.md`
  §Structural Seed 的目录树已声明 `cloud/backend/` 为 Spring Boot 单进程工程与 `/api/v1` 接口面，
  本 Story 只是把它从绿地变为实体。该 seed 非穷举，不冲突。
- 新增的包结构 `common/response`、`common/exception`、`controller` 对齐
  `docs/backend/经验-后端REST信封与微信登录.md`「附：EWF 落地最小文件映射」中
  `common/response/ApiResponse.java`、`common/exception/{ErrorCode,BusinessException,GlobalExceptionHandler}.java`
  的落点约定（同结构、同职责），不新增自创分层。
- **不新增** `repository/`、`entity/`、`mapper/`、`dto/`（当前无业务面）；
  `dto/` 在 4.4 登录/4.6 查询时才引入。
- `src/main/resources/application.yml` 与 `application-local.yml` 位置由 AD-20 **固定**，
  不得移动或改名为 `application-{dev,test}.yml`，也不得新增
  `.env.development.local` 一类运行时覆盖（AD-18 明确本项目不依赖该覆盖）。
- 明确不触碰：`Embedded/**`、`docs/**`、`cloud/frontend/**`（含 `src/canonical/**`）、
  `cloud/miniapp-design/**`、`cloud/env-scripts/**`、
  `_bmad-output/planning-artifacts/{prds,ux-designs,briefs}/**`、`AGENTS.md`、`cloud/AGENTS.md`、`.gitignore`。
- 允许修改的 `_bmad-output/` 文件仅两处：`ARCHITECTURE-SPINE.md`（Task 6.1）与
  `deferred-work.md`（Task 6.2/6.3）；两者都必须最小、可追溯、只做标注与追加。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md`#Epic 4 / Story 4.3] 需求、AC、覆盖需求标记（FR-C-010、FR-C-011、AD-16、AD-20）
- [Source: `epics.md`#Requirements Inventory / FR-C-010] 单实例单进程、临时文件 + `fsync` + `rename` 原子落盘于 `cloud/backend/data/`、不引入任何数据库
- [Source: `epics.md`#Requirements Inventory / FR-C-011] 统一 `/api/v1` 与 `{code,message,data}` 信封、成功 `code=0`、业务错误 HTTP 200 + 业务码、协议错误用对应 HTTP 状态、敏感会话不入日志
- [Source: `epics.md`#Requirements Inventory / AD-16] Java 17 + Spring Boot 3.3.7 + Maven 基线、零库 JSON 原子文件、禁止第二套平行 backend、生产/本地配置位置固定
- [Source: `epics.md`#Requirements Inventory / AD-20] backend 配置固定为 `application.yml`/`application-local.yml`、local profile、9218 端口、`saas.jar` 与 `.sha256`、脚本不启动公网隧道、CI 用通用 JDK17/Maven 3.9.x、上传前校验并上传到约定目录、frontend AppID 与 backend 配置对齐
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#6.2.4 FR-C-010] 「使用临时文件、`fsync` 和 `rename` 原子落盘，在 `cloud/backend/data/` 保存…不引入任何数据库服务」
- [Source: `prd.md`#6.2.5 FR-C-011] 「cloud 对外接口统一使用 `/api/v1` 和 `{code,message,data}` 信封；成功 `code=0`，业务错误保持 HTTP 200 并通过业务码表达，协议错误使用对应 HTTP 状态」
- [Source: `prd.md`#4 术语表] 离线积压等术语的口径（本 Story 不实现判定）
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`#AD-16] backend 沿用 miaowu REST 骨架、`/api/v1` 信封、单实例单进程 JSON 原子文件、`app.data-dir` 落点、禁第二套后端、升级属 Deferred
- [Source: `ARCHITECTURE-SPINE.md`#AD-20] 配置路径固定、local profile、9218、`saas.jar` + SHA-256、脚本无公网隧道、`VITE_API_BASE_URL` 含 `/api/v1` 且 WS origin/path 由契约派生
- [Source: `ARCHITECTURE-SPINE.md`#Consistency Conventions] 「环境与交付」行：local profile=local、端口=9218、产物=`saas.jar`+`.sha256`、CI 只依赖通用 JDK17/Maven、上传目标 `/www/wwwroot/woodenfish`
- [Source: `ARCHITECTURE-SPINE.md`#Deferred] 「EWF backend 构建基线」（本 Story 按 AC6 标注收敛）、「Spring Boot 3.3.7 沿用 miaowu pom」（升级时机不在本 Story）、「明文应用密钥与 SSH 密码」（部署前迁移，属 Deferred）、「生产占位值门禁」（属部署阶段）
- [Source: `docs/contracts/sync-contract.md`#12 错误语义与拒绝条件] 信封口径、码形 `{HTTP 状态}{两位序号}`、8 个预留业务码、「表中业务码由本契约预留，实现信封时不得重定义」
- [Source: `docs/contracts/sync-contract.schema.json`#business_codes] 机器可读的 8 项 `condition`/`http_status`/`code`（本 Story 的契约一致性断言输入）
- [Source: `docs/contracts/README.md`] 注册表「派生自正文，供 Epic 4–6 的 mock 与用例直接消费」的地位
- [Source: `docs/README.md`] 事实优先级；「分层文档不得复制完整上游文档，也不得自行定义新事件来源、经文游标或同步状态」
- [Source: `cloud/AGENTS.md`#3 接口与持久化规则] 信封与业务错误口径、高水位幂等、命令修订、JSON 原子文件、canonical 版本一致
- [Source: `cloud/AGENTS.md`#4 鉴权和敏感数据] `session_key` 不落盘不进日志、openId 脱敏、401 单飞刷新（本 Story 只预留约束，不实现）
- [Source: `cloud/AGENTS.md`#6 测试与并行开发] 「修改同步字段、命令修订、序号、错误码或状态枚举时，先更新契约样例和测试，再修改实现」
- [Source: `cloud/AGENTS.md`#7 文件与编码] UTF-8 无 BOM、`git diff --check`、不新增平行实现或重复契约
- [Source: `AGENTS.md`#3.6 软件层规则] 错误码 = `{HTTP 状态}{两位序号}`、业务错误 HTTP 200 + 业务码、GlobalExceptionHandler、JWT 过滤器分级、JSON 原子文件零库、单身份单设备
- [Source: `AGENTS.md`#2 BMAD 状态指针] 「`cloud/backend/`、`cloud/frontend/` … **空占位**」（本 Story 落地后该表述失真，按 Task 6.4 只登记不就地改写）
- [Source: `docs/backend/经验-后端REST信封与微信登录.md`#① REST 约定骨架] `ApiResponse` 三键与 `@JsonInclude(NON_NULL)`、`/api/v1` 前缀、「根路径 `/` 也回信封探活，EWF health 照做」、不分页的取舍、EWF 无裸响应路径
- [Source: `docs/backend/经验…md`#② 错误码与全局异常处理] 码形定义、区间表、`BusinessException` 唯一抛错方式、逐条异常 → HTTP/码映射（含 `NoResourceFoundException` 与「健康检查要显式写 controller」的坑）
- [Source: `docs/backend/经验…md`#⑧ env-scripts 本地起停模式] profile=local 与端口可配、杀端口三连、就绪等待与失败指纹、JDK 探测、cloudflared 段 EWF 默认去掉
- [Source: `docs/backend/经验…md`#附：EWF 落地最小文件映射] `ApiResponse`/`ErrorCode`/`BusinessException`/`GlobalExceptionHandler` 的落点与「ErrorCode 收敛 EWF 业务码、区间位保留」
- [Source: `docs/backend/经验…md`#⑨ EWF 边界与排除表] 不迁 DB/SaaS/支付/OBS/管理后台/角色租户
- [Source: `docs/backend/README.md`] EWF 边界、排除表、「模块实现规格 `requirements.md` 仍待 spec 阶段」
- [Source: `cloud/env-scripts/build-prod-backend.sh`] 项目根级联解析（要求 `cloud/backend/pom.xml`）、JDK17/Maven 3.9.x 闸门、canonical `check` 门禁块、`mvn -B -U clean package -DskipTests`、`saas.jar` + `.sha256`
- [Source: `cloud/env-scripts/start-local-test-backend.sh`] `BACKEND_DIR=$PROJECT_ROOT/backend`（即 `cloud/backend`）、`SPRING_PROFILE=local`、`BACKEND_PORT=9218`、`wait_for_backend_ready` 探针打根路径 `/`、失败指纹 grep、`SAAS_PAY_SKIP_BUILD` 开关
- [Source: `cloud/env-scripts/stop.sh`] PID 文件 + `pkill` + `kill_listener_by_port` 三连，只服务本项目端口/路径
- [Source: `cloud/env-scripts/upload-backend-jars.sh`] 上传前校验 Jar 与 `.sha256` 一致、远端目录 `/www/wwwroot/woodenfish`、明文 SSH 密码（原型接受项，**禁止本 Story 执行**）
- [Source: `cloud/backend/src/main/resources/application.yml`、`application-local.yml`] `server.port: 9218`、`app.data-dir: ./data`、`spring.jackson.time-zone: Asia/Shanghai`、`on-profile: local`、`jwt.*`、`wechat.mini.*` 占位值
- [Source: `cloud/frontend/.env`、`.env.development`] `VITE_API_BASE_URL` 含 `/api/v1`（本地为 `http://localhost:9218/api/v1`）、`VITE_WX_APPID` 与 backend AppID 对齐（Task 4.6 的断言输入）
- [Source: `cloud/backend/src/main/resources/canonical/heart-sutra.json`] `scriptureVersion=HS-1.0.0`、`counts.totalChars=303`、`counts.consumableHan=260`（只读断言，禁改）
- [Source: `docs/contracts/canonical/heart-sutra.manifest.json`、`docs/contracts/canonical/README.md`] `scripture_version` 与可消费汉字序列的单一来源口径
- [Source: `_bmad-output/implementation-artifacts/4-2-冻结跨层同步契约.md`] 前序故事情报：机械提取期望集合、门禁承诺不得强于实现、Epic 边界、本机无 pytest、工作区在制品不动
- [Source: `_bmad-output/implementation-artifacts/4-1-建立-canonical-心经-与版本校验.md`] 前序故事情报：判定真源不得是可手改产物、证据纪律
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md`#Deferred from: code review of 4-1-…] 明确归属 4.3 的四项（打包期门禁不可达、本地入口未接门禁、`totalChars` 与 `consumableHan` 混用风险、`__pycache__` 未忽略）
- [Source: `.gitignore`（只读）] `target/` 与 `*.jar` 已覆盖；`logs/*` 带斜杠锚定仓库根，故 `cloud/logs/**` 与 `cloud/backend/data/**` 未被覆盖
- [Web: Spring Boot 3.3 Maven plugin `using` 文档](https://docs.spring.io/spring-boot/3.3/maven-plugin/using.html) `spring-boot-starter-parent` 提供 Java 17 默认编译级别、依赖管理、`repackage` 执行与 `application*.yml` 资源过滤
- [Web: Spring Boot 3.3.7 system requirements（社区镜像）](https://www.spring-doc.cn/spring-boot/3.3.7-SNAPSHOT/system-requirements.html) 至少 Java 17、Spring Framework 6.1.15+、Maven 3.6.3+
- [Web: `NoResourceFoundException` 与 404 处理（Qiita）](https://qiita.com/blue_islands/items/cd48deb2e6a935f10831) Spring Boot 3.2 / Framework 6.1 起未命中静态资源抛 `org.springframework.web.servlet.resource.NoResourceFoundException`，被兜底 `Exception` 处理器吞掉会变 500
- [Web: `NoResourceFoundException` 讨论（Stack Overflow）](https://stackoverflow.com/questions/79216730/springframework-web-servlet-resource-noresourcefoundexception-no-static-resourc) 同上问题的复现与 `@ExceptionHandler(NoResourceFoundException.class)` 解法

## Dev Agent Record

### Agent Model Used

Claude Code（dev-story 单轮无人监管执行）

### Debug Log References

全部构建、测试与 RED→GREEN 实测日志保存在仓库外 `/tmp/ewf-4-3/`（不进工作区、不进 File List）：

| 证据 | 日志路径 | 退出码 |
| --- | --- | --- |
| RED（实现类缺失，编译失败） | `/tmp/ewf-4-3/red-01.log` | 1 |
| GREEN 首轮（15 项） | `/tmp/ewf-4-3/green-01.log` | 0 |
| RED→GREEN A：`DEVICE_RESET_CONFLICT` 20003→20009 | `/tmp/ewf-4-3/redgreen-A.log` / `redgreen-A-restore.log` | 1 / 0 |
| RED→GREEN B：删除 `NoResourceFoundException` 分支 | `/tmp/ewf-4-3/redgreen-B.log` / `redgreen-B-restore.log` | 1 / 0 |
| RED→GREEN C：`application.yml` 写死 `spring.profiles.active: local` | `/tmp/ewf-4-3/redgreen-C.log` / `redgreen-C-restore.log` | 1 / 0 |
| RED→GREEN D：pom 引入 `spring-boot-starter-jdbc` | `/tmp/ewf-4-3/redgreen-D.log` / `redgreen-D-restore.log` | 1 / 0 |
| RED→GREEN E：移除 `@JsonInclude(NON_NULL)` | `/tmp/ewf-4-3/redgreen-E.log` / `redgreen-E-restore.log` | 1 / 0 |
| RED→GREEN F：`consumableHan` 260→303 | `/tmp/ewf-4-3/redgreen-F.log` / `redgreen-F-restore.log` | 1 / 0 |
| RED→GREEN G：重命名 `jwt.secret` | `/tmp/ewf-4-3/redgreen-G.log` / `redgreen-G-restore.log` | 1 / 0 |
| 生产构建全链路（Task 5.1） | `/tmp/ewf-4-3/build-prod-01.log` | 0 |
| 打包期 canonical 门禁可达性/可失败性（Task 5.2） | `/tmp/ewf-4-3/gate-tmp-intact.log`、`gate-tmp-broken.log`、`gate-real-repo.log` | 0 / 1 / 0 |
| 本地起停闭环（Task 5.3） | `/tmp/ewf-4-3/start-local.log`、`stop-local.log` | 0 / 0 |
| 回归：契约门禁 / 经文门禁（Task 7.3） | `/tmp/ewf-4-3/regress-sync.log`、`regress-canonical.log` | 0 / 0 |
| 全量 `mvn test` 终态 | `/tmp/ewf-4-3/final-full-test-2.log` | 0 |

所有 RED 实验均使用逐文件备份 + `cmp` 校验恢复，工作区无残留破坏；`consumableHan` 实验后 `git diff` 对 `cloud/backend/src/main/resources/canonical/heart-sutra.json` 无差异。

### Completion Notes List

**落地内容（Task 1–3）**：`cloud/backend` 从绿地变为可构建、可启动、可回信封的 Spring Boot 3.3.7 工程。新增 `pom.xml`（parent 3.3.7、`java.version=17`、`finalName=saas`、UTF-8；依赖仅 web/validation/test 且全部不写 `<version>`；仅声明 `spring-boot-maven-plugin`）、启动类、信封 `ApiResponse`、`ErrorCode`、`BusinessException`、`GlobalExceptionHandler`、`HealthController`。包结构 `common/response`、`common/exception`、`controller`；**未创建** `config/`（本 Story 无真实配置类）。

**Task 2.2 校验结论**：`application.yml` 与 `application-local.yml` 与 AC1/AD-20 **无冲突**，按裁决**逐字节未改**（端口 9218、`app.data-dir: ./data`、`spring.jackson.time-zone: Asia/Shanghai`、`on-profile: local`、`wechat.mini.*`、`jwt.*` 全部就位）。Task 2.3 的「不得写死 `spring.profiles.active`」已由测试固定。

**对 story 内部张力的两处裁定**（均已登记，未越界）：

1. **Task 3.2 与 3.3 的张力**：3.2 明令「不预登记当前无分支引用的 403xx/409xx/410xx 等区间码」，并明确兜底表只有 `40000/40400/50000`（且注明「4.4 及后续模块可扩」）；而 3.3 又要求提供 `conflict` 便捷静态。裁定：`conflict` 复用契约 §12 已登记的 `DEVICE_RESET_CONFLICT`（20003），并在 Javadoc 写明理由——EWF 的业务错误按 FR-C-011 保持 HTTP 200 并用业务码表达，故业务冲突落在 2xxxx 区间，本 Story 不新增 409xx 码；其它冲突语义的码值留给引入该分支的模块。`unauthorized` 同理复用唯一已登记的 401 码 `TOKEN_EXPIRED`（40101）。
2. **Dev Notes「已核实的现状」第 5 条要求「以断言固定分母唯一来源」**，但文件树的测试清单未列出承载它的类。裁定：新增 `ScriptureResourceBaselineTest`（见下「偏离文件树的两个测试文件」）。

**偏离 Task 1 文件树的两个测试文件**（均为测试作用域，不扩大生产面）：

- `support/TestWorkspace.java`：仓库根定位在 3 个测试类中共用，抽成 helper 而非三份复制；**fail-closed**，找不到仓库根即抛异常，不允许静默跳过。
- `ScriptureResourceBaselineTest.java`：Dev Notes 要求的分母口径断言（见上）。

**测试与门禁（Task 4、7）**：门禁入口 `cd cloud/backend && mvn -q test`，退出码 0，16 项全绿（`EnvelopeContractTest` 6、`ErrorCodeContractTest` 3、`RuntimeBaselineTest` 4、`ForbiddenDependencyScanTest` 2、`ScriptureResourceBaselineTest` 1）。契约一致性断言从 `sync-contract.schema.json#business_codes` **机械提取**（Jackson 读注册表）、用反射读 `ErrorCode` 常量，**未手抄第三份清单**；并额外钉住「登记码集合 == 契约预留码 ∪ 兜底码」，故新增或改义码值都会红。AC2 的跨端对齐由 `RuntimeBaselineTest` 断言 `VITE_API_BASE_URL` 以 `/api/v1` 结尾、`.env`/`.env.development` 的 AppID 分别等于 `application.yml`/`application-local.yml` 的 `wechat.mini.app-id`；**占位值一字未替换**。

**RED→GREEN 负载性证据（Task 4.7，实测 7 条，超出要求的 4 条）**：见表；每条都做了「故意破坏 → 记录失败诊断 → 恢复 → 复绿」。其中 B 条复现了 story 预警的真实退化形态——删掉 `NoResourceFoundException` 分支后，未匹配路径从 **404 变成 500 + 50000**。故「未匹配置 404」这条断言确实有负载，不是恒真。

**静态校验的边界（Task 4.8，不得让承诺强于实现）**：

- 门禁覆盖的是**被显式请求到的运行时路径**。「生产代码不存在第二套响应形状」只能靠白盒评审，无法机械判定。
- `BusinessException` 是否被所有失败分支正确使用无法静态证明；测试只覆盖 `src/test/java` 的探针分支。
- 信封只承诺**经 DispatcherServlet 的**请求；容器级错误（畸形请求行、TLS 握手失败、请求体超限等）由容器直接应答，不在 AC1「所有公开接口」的承诺内。
- 敏感信息只做了**字面量窄扫描**（`Authorization`/`session_key`/`openId` 一类），不是语义分析；4.4 的 JWT/微信会话仍需自建日志审查。
- `ForbiddenDependencyScanTest` 是字面量匹配，不能证明运行期不会通过环境变量或外部配置引入等价依赖。
- `ScriptureResourceBaselineTest` 只约束资源字段口径，不能阻止下游自行用常量求和充当分母。

**Task 5 实测结论**：

- **5.1** `bash cloud/env-scripts/build-prod-backend.sh` 退出码 0：解析项目根成功、JDK17 与 Maven 3.9.x 闸门通过、**执行了** 4.1 交付的 canonical 经文版本门禁、产出 `cloud/backend/target/saas.jar`（21670366 字节，可执行 Spring Boot Jar）与 `saas.jar.sha256`。
- **5.2** 「打包期 canonical 门禁不可达」**可达性已闭合**：在临时副本（`/tmp/ewf-4-3/canonical-gate-root`，只复制门禁实际读取的 7 个路径 + `cloud/backend/pom.xml`）上把 `heart-sutra.json` 的 `scripture_version` 改成 `HS-9.9.9`，以 `EWF_PROJECT_ROOT` 指向副本运行脚本 → 退出码 1 且打印「构建失败：canonical 经文版本校验不通过，停止打包」。**未改动真实仓库的 `docs/contracts/**` 任何文件**（副本实验后回归 `scripture_tool.py check` 退出码 0）。**仍无 CI**：没有任何测试或脚本调用该脚本，故只写「可达且可失败」，不写「已自动验证」。
- **5.3** 本地起停闭环实测：`SAAS_PAY_SKIP_BUILD=1 bash cloud/env-scripts/start-local-test-backend.sh` 退出码 0，9218 就绪、日志显示 `The following 1 profile is active: "local"`；`curl -i http://localhost:9218/api/v1/health` 与 `curl -i http://localhost:9218/` 均 `HTTP/1.1 200` + `{"code":0,"message":"success","data":{"status":"UP"}}`；`curl -i http://localhost:9218/api/v1/not-a-real-endpoint` 在真机上是 `HTTP/1.1 404`（非 500）；`bash cloud/env-scripts/stop.sh` 退出码 0，按 PID 停止且 `lsof -tiTCP:9218 -sTCP:LISTEN` 无残留。
- **5.4** `cloud/env-scripts/**` **一行未改**（无冲突）。（story 已登记的既有现状：`SAAS_PAY_SKIP_BUILD` 前缀、`LOG_DIR` 落 `cloud/logs/`，均只记录不动。）
- **5.5** `upload-backend-jars.sh` **未执行**，只做静态核对：产物 `cloud/backend/target/saas.jar` 与 `.sha256`、校验逻辑、远端目录 `/www/wwwroot/woodenfish` 与 AD-20 一致。

**工作区卫生说明**：为避免把运行期产物留进仓库，实测起停时以 `APP_LOG_ROOT=/tmp/ewf-4-3/logs` 覆盖脚本既有开关（脚本自带的 env 覆盖点，非改写脚本语义）；`cloud/logs/` 与 `cloud/backend/data/` 均**未创建**。`cloud/backend/target/**` 由 `.gitignore` 第 22 行 `target/` 覆盖，不进 diff。

**Task 6 收敛**：`ARCHITECTURE-SPINE.md` 的 Deferred「EWF backend 构建基线」行**只追加**收敛标注（保留原句，注明日期与 Story，并写明仍开放的框架升级时机与 CI 接入）；`deferred-work.md` 的 4-1 review 小节下**只追加** 5 条登记（可达性闭合、本地入口仍开放、分母口径已部分收敛、`__pycache__` 仍开放、`cloud/logs/**`+`cloud/backend/data/**` 未被忽略 + `AGENTS.md` §1「空占位」落差），未改写任何他人条目。**未改** `AGENTS.md` / `cloud/AGENTS.md` / `Embedded/AGENTS.md` / `.gitignore` / `Embedded/**`。

**证据纪律（Task 7.4）**：本 Story 落地的只是「信封与本地启动基线」。完成后 backend **仍不能登录、不能同步、不能持久化**；可联调范围**仅限** `GET /` 与 `GET /api/v1/health` 的探活与统一信封。不得读成「设备/小程序可联调业务接口」或「端到端可用」。

### File List

- `cloud/backend/pom.xml`（新增）
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/EwfBackendApplication.java`（新增）
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/response/ApiResponse.java`（新增）
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/exception/ErrorCode.java`（新增）
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/exception/BusinessException.java`（新增）
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/exception/GlobalExceptionHandler.java`（新增）
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/controller/HealthController.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/EnvelopeContractTest.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ErrorCodeContractTest.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/RuntimeBaselineTest.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ForbiddenDependencyScanTest.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ScriptureResourceBaselineTest.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/support/EnvelopeFailureProbeController.java`（新增）
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/support/TestWorkspace.java`（新增）
- `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`（修改：Deferred 行追加收敛标注）

未列入（按 4.1/4.2 既有口径不在 Story diff 范围）：`_bmad-output/implementation-artifacts/deferred-work.md`、`_bmad-output/implementation-artifacts/sprint-status.yaml`、本 story 文件、`.gitignore`、`cloud/env-scripts/**`、`cloud/backend/target/**`（被忽略）、`docs/contracts/**`（未改动）。

### Change Log

- 2026-09-22：建立 `cloud/backend` Maven 工程与 Spring Boot 3.3.7 基线（Java 17、`finalName=saas`）；实现统一信封 `ApiResponse`、`ErrorCode`、`BusinessException`、`GlobalExceptionHandler` 与 `/` + `/api/v1/health` 探活；新增 5 个测试类与 2 个测试支撑类，`mvn -q test` 16/16 通过；以 7 条 RED→GREEN 实测证明关键断言可失败；实测 `build-prod-backend.sh`（退出码 0，产出 `saas.jar` + `.sha256`）、打包期 canonical 门禁可达且可失败、本地起停闭环（9218 起服与回收）；按 AC6 追加 spine 收敛标注与 5 条 deferred 登记。未改 `cloud/env-scripts/**`、`Embedded/**`、`.gitignore`、agent 规则文件与任何冻结契约交付物。
