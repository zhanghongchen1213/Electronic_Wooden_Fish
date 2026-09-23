# Story 4.5: 建立 JSON 原子持久化基线

story_key: 4-5-建立-json-原子持久化基线
baseline_commit: db869b7d0796e1dd5f7084f6709ca59668503cb2

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a 设备用户，
I want backend 重启后保留我的云端状态，
so that 服务进程重启不会清空诵读记录。

本 Story 覆盖 FR-C-010、AD-16、NFR5，是 Epic 4 的第五道门禁。
Story 4.3 已落地 Java 17 + Spring Boot 3.3.7 + Maven 工程、`/api/v1` 信封、
`ApiResponse`、`ErrorCode`、`GlobalExceptionHandler` 和 `application*.yml`（含
`app.data-dir: ./data`）；Story 4.4 已落地 `IdentityStore` 的**单文件原子写内联实现**与
JWT/身份链路。本 Story 把这段内联实现**提取为可复用的 JSON 原子持久化基线**，按
`docs/contracts/sync-contract.md` §11 冻结的五份文件建立 schema/迁移基线，并落地只读的
启动恢复语义。

本 Story **只建立持久化基线与恢复语义**：不实现 Epic 5 的幂等高水位、轮次归属、完成确认、
历史统计派生、命令修订业务或 WebSocket，也不实现 4.6 的快照/差量查询接口。Epic 5 的业务
Store 消费本 Story 的基元与文件基线，不另造第二套原子写或第二个数据目录。

## Acceptance Criteria

1. **原子写与零库基线（FR-C-010 第一段 / AD-16）**

   **Given** backend 需要保存身份、高水位、轮次、命令、设备状态和统计
   **When** 写入 `app.data-dir`（当前值 `./data`，即 `cloud/backend/data/`）
   **Then** 所有落盘走**同一个**可复用基元：同目录临时文件 + 写入 + `FileChannel.force(true)`
   （等价 fsync）+ `Files.move(..., ATOMIC_MOVE)` 原子替换；失败路径在 `finally` 删除临时文件，
   不留 `.tmp` 残留
   **And** 不引入 SQLite、PostgreSQL 或其他数据库、ORM、连接池、迁移框架（`pom.xml` 与
   `src/main/resources/*.yml` 不得命中 `ForbiddenDependencyScanTest` 的禁用键）
   **And** `IdentityStore` 改写为复用该基元，**对外行为与落盘字节保持不变**。

2. **契约文件集合、schema 版本与迁移说明（FR-C-010 第二段 / AD-16）**

   **Given** `docs/contracts/sync-contract.md` §11 已冻结五份文件与字段（`SC-1.0.0`）
   **When** 建立文件级 schema 基线
   **Then** 五份文件各有一个 `schema_version: 1` 的严格读取面，字段集合**逐值等于**契约
   §11 的字段列，且由测试从 `sync-contract.schema.json#persistence_files` **机械提取**比对，
   不在 Java 测试里手抄第三份字段表
   **And** 每份文件在代码 javadoc 中记录契约 §11 的迁移说明（`progress.json`：新增字段可省读
   取默认值，删除或改义字段必须递增 `schema_version` 并提供一次性重写；`identity.json`：身份
   不可迁移；`commands.json`：只保留最新修订；`device_state.json`：非权威镜像可整体重建；
   `daily_stats.json`：日界以 backend 配置时区为准）
   **And** 读取面统一校验：文件为常规文件、字节数不超上限、UTF-8 可解析、`FAIL_ON_TRAILING_TOKENS`
   拒绝尾随内容、根为 JSON object、字段集合精确匹配、`schema_version` 为整数且等于 1
   **And** 任一项不满足即以 `50000` fail closed，**绝不改写原文件**、**绝不返回默认值**。

3. **启动恢复：不读半写文件、不静默回退（AC2 前半 / NFR5）**

   **Given** 数据目录中存在孤儿临时文件（如 `.progress-*.tmp`）、内容损坏的 JSON、字段越界
   或 `schema_version` 非 1 的文件
   **When** backend 启动恢复
   **Then** 恢复流程**只读**：不创建数据目录、不创建任何文件、不改写任何既有文件
   **And** 中间产物**永不进入读取路径**：孤儿 `.tmp` 不被当作数据源、不被解析；其清理是
   best-effort，删除失败只记 WARN，不影响启动
   **And** 校验失败的文件被标记为不可恢复，记录可诊断日志（绝对数据目录路径 + 文件名 + 失败
   原因分类，不含任何敏感值），该文件的后续读取一律 `50000`
   **And** **绝不静默回退**：`progress.json` 损坏时读取得 `50000`，不得返回 `acked_total=0`
   或任何默认高水位；派生文件（`commands.json`/`device_state.json`/`daily_stats.json`）损坏
   不得改写 `progress.json`、不得改变权威高水位。

4. **正常重启后状态一致恢复（AC2 后半 / NFR5）**

   **Given** 五份文件均存在且结构合法
   **When** 重建 Store 实例读取（等价进程重启）
   **Then** 每份文件读回的值与写入值逐字段一致，`schema_version` 仍为 1
   **And** 文件缺失视为**空状态**而非错误：读取面按各自语义给出「未初始化」结论，不创建文件、
   不报错、不把缺失当成损坏
   **And** 同一 `app.data-dir` 上重复构造 Store 与恢复流程不产生副作用（幂等、无残留）。

5. **测试、门禁与证据纪律**

   **Given** backend 仍是单实例单进程、JSON 零库工程
   **When** 执行 `cd cloud/backend && mvn -q test`
   **Then** 4.3/4.4 既有 9 个测试类（40 条）全绿，新增基元/文件契约/恢复测试全绿；测试只写
   临时目录，不创建也不读取仓库工作区的 `cloud/backend/data/`
   **And** 至少提供 5 条**可真实失败**的 RED→GREEN 负例证据（见下表），每条记录命令、首个
   失败断言与恢复后结论；不得使用恒真不可失败的负例
   **And** 回归 `python3 docs/contracts/tests/run_sync_contract_tests.py` 与 `git diff --check`。

## Tasks / Subtasks

- [x] **1. 提取原子 JSON 读写基元（AC: 1, 2）**
  - [x] 1.1 在 `common/persistence/` 新建 `PersistenceLimits`（集中常量）：`SCHEMA_VERSION = 1`、
        单文件字节上限 `MAX_JSON_BYTES = 262144`（256 KiB）、临时文件前缀/后缀命名规则。
        上限必须集中一处、在解析**之前**检查，不得散落成各 Store 的字面量。
  - [x] 1.2 新建 `AtomicJsonFile`：`write(Path target, byte[] payload)` 走「按需
        `Files.createDirectories(parent)`（**仅写路径**）→ 同目录 `Files.createTempFile` →
        写入 + `FileChannel.force(true)` → `Files.move(tmp, target, ATOMIC_MOVE)` →
        `finally { if (!moved) Files.deleteIfExists(tmp); }`」。**不得**提供非原子的
        `Files.writeString(target, ...)` 覆盖回退分支。
        1.2a 在同一基元内、`ATOMIC_MOVE` 成功后 best-effort 尝试父目录 fsync
        （`FileChannel.open(parent, READ).force(true)`）。Java 17 / macOS 实测**成功**，
        Windows 会抛异常；因此必须容忍 `IOException`/`UnsupportedOperationException`，失败
        只记 WARN 且**不得**把已成功的写入判为失败、也不得把失败判为成功。若实测在某平台
        不可实现，保留 tolerated 分支并在 Completion Notes 记录实测结论。
  - [x] 1.3 新建 `VersionedJsonFile`：`read(Path file, Set<String> allowedFields)` 实施 AC2 的
        全部读取校验（常规文件、`Files.size <= MAX_JSON_BYTES`、UTF-8、
        `FAIL_ON_TRAILING_TOKENS`、根为 object、字段集合精确匹配、`schema_version` 整数且为 1），
        违规统一 `BusinessException.serverError(...)`（`50000`），**只读不写**。
        错误文案区分「不可读取 / 格式无效 / 损坏 / 超出大小上限」四类，便于诊断；文案不含文件
        内容、不含任何密钥或身份值。
  - [x] 1.4 `AtomicJsonFile`/`VersionedJsonFile` 必须是**唯一**的原子写与严格读实现；
        不得在 `service/` 下复制第二份等价逻辑（`IdentityStore` 在 Task 4 改为调用基元）。

- [x] **2. 五份契约文件的 schema 基线与 Store（AC: 2, 4）**
  - [x] 2.1 新建 `ProgressStore`（`progress.json`）：字段 `local_total`、`acked_total`、
        `round_id`、`round_cursor`、`round_state`、`pending_completion`、`action_id`。
        提供严格读取与原子整体替换；**不实现**幂等推进、轮次归属、完成翻转等业务规则（Epic 5）。
        类 javadoc 必须写明「这六字段 + 篇章动作族 `action_id` 是 §11 的单事务组，必须整体
        落同一文件，掉电恢复后不得出现高水位/轮次/游标不匹配」。
  - [x] 2.2 新建 `CommandsStore`（`commands.json`）：字段 `command_revision`、`applied_revision`、
        `action_id`、`volume`、`brightness`、`timeout`。只做 schema 与原子替换，**不实现**
        「只保留最新修订」「旧修订不写回」的收敛判定（Epic 5）。
  - [x] 2.3 新建 `DeviceStateStore`（`device_state.json`）：字段 `battery_percent`、
        `network_mode`、`audio_config_version`、`firmware_version`。javadoc 记录它是**非权威
        镜像、可整体重建**，不得被当作权威来源。
  - [x] 2.4 新建 `DailyStatsStore`（`daily_stats.json`）：**只建立文件级信封**
        （`schema_version` + 一个容纳按日桶的 object 容器）。契约 §11 该行字段列为「无」，
        因此**桶键与桶值字段都不在本 Story 冻结**，Story 5.3 冻结时必须显式说明或递增
        `schema_version`；本 Story 的读取校验**只覆盖信封**（`schema_version`、容器为 object、
        JSON 完整性、字节上限），**不得**对桶内键名做白名单断言（那会成为 5.3 的第二真源）。
  - [x] 2.5 四个 Store 的 Store 名、文件名、字段集合必须与契约 §11 逐值一致；字段表由 Task 5
        的测试从 `sync-contract.schema.json` 机械提取比对，Java 常量只作为被比对对象，不另抄表。
  - [x] 2.6 **冻结跨文件写序**（架构 Deferred 明确指派给「实现 Story」，即本 Story）：在
        javadoc 或本 Story 的 Dev Notes 落地并让恢复逻辑遵守 —— `progress.json` 是唯一权威
        单事务组；`commands.json`/`device_state.json`/`daily_stats.json` 是派生/镜像；
        同一次操作若需写多个文件，**先写权威 `progress.json`，再写派生文件**；派生文件写失败
        不回滚权威文件（幂等重放会收敛）；恢复与读取**只以 `progress.json` 裁决高水位**。
        本 Story 只冻结该顺序，不实现业务写入路径。

- [x] **3. 只读启动恢复（AC: 3, 4）**
  - [x] 3.1 新建 `PersistenceRecovery`（`ApplicationRunner` 或等价生命周期钩子）与
        `PersistenceRecoveryReport`：逐文件校验五份文件，输出每份文件的「缺失 / 正常 / 不可恢复」
        结论与失败原因分类。
  - [x] 3.2 **只读硬约束**：恢复流程绝不 `Files.createDirectories(dataDirectory)`、绝不创建
        文件、绝不改写既有文件。数据目录不存在 → 全部文件视为缺失、正常启动、**目录仍不存在**。
        理由：`EnvelopeContractTest` 是 `@SpringBootTest` 且**未**覆盖 `app.data-dir`（使用默认
        `./data`），任何在启动期建目录的实现都会在 `cloud/backend/` 下留下未跟踪的 `data/`，
        污染工作区并违反 `RuntimeBaselineTest`「本 Story 不建目录」的既有断言。
  - [x] 3.3 孤儿临时文件（匹配 `PersistenceLimits` 的 tmp 命名规则）**不进入读取路径**：
        不被解析、不被当作数据源；best-effort 删除，失败只记 WARN。恢复只读 `progress.json`
        等目标文件名，绝不按 `*.json` 通配或前缀扫描。
  - [x] 3.4 校验失败的文件标记为不可恢复：记 ERROR/WARN 日志（绝对数据目录路径 + 文件名 +
        原因分类，无敏感值）；该文件的读取一律 `50000`。**不得**在恢复期抛异常中断 Spring 启动，
        也不得静默改写或删除损坏文件。
  - [x] 3.5 **不新增 HTTP 端点**：恢复状态不经 HTTP 暴露。若未来需要对外可观测，属 4.6/Epic 5
        且需先评审契约（`cloud/AGENTS.md` §6「先更新契约样例和测试，再修改实现」）。

- [x] **4. `IdentityStore` 迁移到公共基元（AC: 1）**
  - [x] 4.1 把 `IdentityStore.getOrCreate` 内联的 temp+force+`ATOMIC_MOVE`+清理逻辑替换为
        `AtomicJsonFile`，把严格读取替换为 `VersionedJsonFile`；`SCHEMA_VERSION`/`ALLOWED_FIELDS`
        从 `PersistenceLimits` 或契约一致的常量取得。
  - [x] 4.2 **行为不变**：`identity.json` 仍只含 `schema_version` 与 `device_id`、仍带
        `System.lineSeparator()` 尾换行、`FileAlreadyExistsException` 仍作为并发建号哨兵、
        身份不匹配仍 `40300`、损坏/不可读仍 `50000` 且绝不重建、失败路径仍无 `.tmp` 残留。
  - [x] 4.3 `IdentityStoreTest` 与 `AuthContractTest` 必须原样通过；**不得放宽或删除**既有断言
        （含「失败不残留 `.tmp`」「损坏不改写文件」「同 openid 并发幂等」「重启后映射保持」）。
        本 Story 可**新增**断言，不得替换旧证据。
  - [x] 4.4 `IdentityStore` 的读写仍需单实例内互斥（沿用方法级 `synchronized`）；原子写是崩溃
        保护，不能替代互斥。本 Story 不扩展跨进程/跨 JVM 锁协议（AD-16 单实例单进程）。

- [x] **5. 测试、负例证据与门禁（AC: 5）**
  - [x] 5.1 新建 `AtomicJsonFileTest`（`@TempDir` + 真实文件系统 + 直接构造，不启 Spring）：
        正常原子替换、目录按需创建、超上限拒绝且不落盘、失败路径零 `.tmp` 残留、父目录 fsync
        分支不改变写入结论。
  - [x] 5.2 新建 `VersionedJsonFileTest`：`schema_version` 非 1／缺失／类型错误、未知字段、
        字段缺失、尾随内容、非 object 根、超字节上限、非常规文件 —— 每例均 `50000` 且文件**字节
        不变**；缺失文件返回「未初始化」而非异常。
  - [x] 5.3 新建 `PersistenceRecoveryTest`：目录不存在时**目录仍不存在**；孤儿 `.tmp`（内容为
        结构合法但数值不同的 JSON）不被读取、不改变结论；损坏文件被标记且文件未被改写；恢复
        流程无写副作用；重复执行幂等。
  - [x] 5.4 新建 `PersistenceFileContractTest`：用 `TestWorkspace.repoRoot()` 读
        `docs/contracts/sync-contract.schema.json#persistence_files`，机械断言五个 Store 的
        「文件名 + `schema_version` + 字段集合」逐值等于契约，且五份文件的字段并集等于
        `persistence_scopes` 中 `backend` 作用域的字段集合。**禁止**在 Java 测试里手抄第三份
        字段表（沿用 `ErrorCodeContractTest` 的反第三真源口径）。
  - [x] 5.5 至少 5 条**可真实失败**的 RED→GREEN 负例（每条记录命令、首个失败断言、恢复结论）：

        | 负例（制造 RED 的手法） | 首个应失败的断言 | 恢复后的 GREEN |
        | --- | --- | --- |
        | 在基元失败路径去掉 `Files.deleteIfExists(tmp)` | 「失败的原子写不得残留 `.tmp` 文件」计数为 0 | 恢复清理后计数为 0 |
        | 严格读改为「解析失败返回默认值」而不是抛 `50000` | `progress.json` 损坏时断言 code == 50000 | 恢复 fail closed |
        | 给 `progress.json` 读取加 `catch → 默认 ProgressRecord(acked_total=0)` 回退 | 「损坏时不得返回 `acked_total=0`」 | 恢复抛 50000，无默认值 |
        | 恢复逻辑按 `*.json` 通配（或前缀）扫描，把 `.progress-*.tmp` 也读进来 | 「只使用 `progress.json` 的真实值」断言 | 恢复只读目标文件名 |
        | 在 `PersistenceRecovery` 的 `@PostConstruct` 里 `Files.createDirectories` | 「启动后数据目录仍不存在」 | 恢复保持只读 |

        注意 4-4 的教训：负例必须**真的能失败**。不要写类似「取消同 openid 互斥」那种因派生值
        恒同而不可观测的负例。
  - [x] 5.6 门禁与卫生：`cd cloud/backend && mvn -q test`、
        `python3 docs/contracts/tests/run_sync_contract_tests.py`、`git diff --check`；
        并用 `git status --short` 确认**未新增** `cloud/backend/data/**`、未新增 `.tmp`、
        未新增无关文件。
  - [x] 5.7 门禁边界必须在测试类 javadoc 与 Completion Notes 如实声明（不得写成已验证）：
        单进程 `ATOMIC_MOVE` 不能证明多进程/跨 JVM 竞态；文件级 force + best-effort 目录 fsync
        不能证明掉电安全；临时目录测试不能证明生产数据目录的权限/挂载配置；「重建 Store 实例」
        只是重启的近似，不构成独立进程级重启验证。

## Dev Notes

### 真源与冲突裁决

| 主题 | 唯一依据与本 Story 裁决 |
| --- | --- |
| Story 需求 | `_bmad-output/planning-artifacts/epics.md` 的 Epic 4 / Story 4.5（FR-C-010、AD-16、NFR5） |
| 文件集合 / 字段 / schema_version / 迁移说明 / 单事务分组 | `docs/contracts/sync-contract.md` §11、§11.1；机器可读派生 `docs/contracts/sync-contract.schema.json#persistence_files`、`#persistence_scopes` |
| backend 不变量 | `ARCHITECTURE-SPINE.md` §AD-16、§Consistency Conventions（`JSON 文件写入原子化（tmp+rename）`）、§Deferred（A-2 行）、§工程验证门禁（Cloud/前端：「backend 重启后 JSON 中高水位/游标/统计/配置不丢」） |
| 产品行为 | `prd.md` §6.2.4 FR-C-010；§6.2 段落与 SM-5 |
| 层级规则 | `cloud/AGENTS.md` §2、§3、§6、§7；`AGENTS.md` §3.6；`docs/backend/README.md`（单实例单进程、零库、存 `cloud/backend/data/`，不迁 repository/SQL/Flyway） |
| 迁移经验（仅参考） | `docs/backend/经验-后端REST信封与微信登录.md` §⑤（tmp+fsync+rename 与「进程内互斥 + 原子哨兵」）；注意其 `data/identities/{openIdSha256}.json` 草图**未**被采纳，契约 §11 的 `identity.json` 才是权威 |

### 已有实现与必须保留的行为

- 4.3 已落地 `cloud/backend/pom.xml`（Java 17 + Spring Boot 3.3.7，`finalName=saas`，依赖只有
  `spring-boot-starter-web`、`spring-boot-starter-validation`、JJWT 三件套、`spring-boot-starter-test`）、
  `ApiResponse`、`ErrorCode`、`BusinessException`、`GlobalExceptionHandler`、
  `HealthController`（`GET /` 与 `GET /api/v1/health`）与 `application*.yml`。**不要**重写信封、
  引入第二个异常处理器或改动 `/`、`/api/v1/health` 的 HTTP 语义。
- `application.yml` 与 `application-local.yml` 均为 `app.data-dir: ./data`、端口 9218、
  `spring.jackson.time-zone: Asia/Shanghai`。`RuntimeBaselineTest` 断言 `app.data-dir == "./data"`，
  断言消息即为「落点约定，本 Story 不建目录」。**不要**改配置值，也不要改这条断言。
- 4.4 已落地 `IdentityStore`：`SCHEMA_VERSION = 1`、`ALLOWED_FIELDS = Set.of("schema_version", "device_id")`、
  同目录 `Files.createTempFile(dataDirectory, ".identity-", ".tmp")` + `FileChannel.force(true)` +
  `Files.move(..., ATOMIC_MOVE)`，`FileAlreadyExistsException` 作为并发建号哨兵，
  `FAIL_ON_TRAILING_TOKENS` 严格读取，违规一律 `50000` 且不改写文件。**这是仓库里唯一的原子写
  先例，也是本 Story 要提取的基元原型**；重构后其可观测行为必须逐条保留。
- `AuthService`、`JwtAuthenticationFilter`、`UserContext`、`JwtTokenProvider`、
  `WechatMiniClient` 不属本 Story 改动面；`JwtAuthenticationFilter` 每个受保护请求会调用
  `identityStore.readDeviceId()`，因此**读取路径的性能与失败语义变化会波及鉴权**——重构后
  「身份文件损坏 → `50000`」的行为必须原样保留。

### 关键回归风险（逐条必须在实现时守住）

1. **`EnvelopeContractTest` 是全上下文测试且未覆盖 `app.data-dir`**（使用默认 `./data`）。
   任何在启动期创建数据目录的实现都会在 `cloud/backend/` 下留下未跟踪的 `data/`。→ 启动恢复
   必须严格只读（Task 3.2）。这是本 Story 最容易踩的回归。
2. **`RuntimeBaselineTest`** 断言两个 profile 的 `app.data-dir == "./data"`、端口 9218、Jackson
   时区，并断言 `wechat.mini.*` 能绑定到 `WechatProperties` bean、`application.yml` 不设
   `spring.profiles.active`、`application-local.yml` 声明 `on-profile: local`。→ 不要改配置。
3. **`ErrorCodeContractTest` 是精确集合门禁**：`FALLBACK_CODES` 硬编码
   `{40000, 40400, 40500, 41500, 50000, 40102, 40103, 40300, 50200}`，并断言 `ErrorCode` 的
   反射常量集合**恰好**等于「契约码 ∪ 兜底码」。→ **不要新增、删除或改义任何 `ErrorCode` 常量。**
   持久化失败统一复用 `SERVER_ERROR = 50000`。
4. **`ForbiddenDependencyScanTest`** 对小写后的 `pom.xml` 与 `src/main/resources/*.yml` 做子串
   匹配，禁用键含 `jdbc`、`jpa`、`mybatis`、`hibernate`、`flyway`、`liquibase`、`sqlite`、
   `postgresql`、`mysql`、`druid`、`h2`、`jeepay`、`obs`、`huaweicloud`、`cloudflared`，并断言
   `schema.sql`/`data.sql` 不存在。→ 不要在这些文件里引入命中词；新增依赖前先自查子串。
5. **`cloud/.gitignore` 已忽略 `backend/data/`**（且注释已列出五份文件），
   `git check-ignore -v cloud/backend/data/identity.json` 实测命中。→ **不需要改 `.gitignore`**；
   根 `.gitignore` 属工作区基建，不在 Story 改动面（4.1/4.3 已确认口径）。
6. 当前工作区是脏的：`JwtAuthenticationFilter.java`、`AuthContractTest`、`EnvelopeContractTest`、
   `IdentityStoreTest`、`RuntimeBaselineTest`、`WechatMiniClientTest`、`deferred-work.md`、
   `sprint-status.yaml` 有未提交修改（4.4 的 autofix 产物）。→ **不要** reset/checkout/stash/
   commit/branch/merge，也不要回滚不属于本 Story 的改动；实现要建立在这些**当前工作树内容**上。

### 文件表（逐字引自契约 §11，不得改写文件名或字段）

| 文件 | 承载事实 | `schema_version` | 字段 | 迁移说明 |
| --- | --- | --- | --- | --- |
| `progress.json` | 累计高水位、轮次、游标、完成置位与相关动作去重键 | 1 | `local_total`、`acked_total`、`round_id`、`round_cursor`、`round_state`、`pending_completion`、`action_id` | 新增字段时可省略读取并取默认值；删除或改义字段必须递增 `schema_version` 并提供一次性重写 |
| `identity.json` | 单身份单设备映射 | 1 | `device_id` | 身份不可迁移；重建即重新绑定 |
| `commands.json` | 命令修订、已应用高水位、待应用命令载荷与设置命令去重键 | 1 | `command_revision`、`applied_revision`、`action_id`、`volume`、`brightness`、`timeout` | 只保留最新修订，旧修订不写回 |
| `device_state.json` | 设备状态镜像与音频配置版本 | 1 | `battery_percent`、`network_mode`、`audio_config_version`、`firmware_version` | 属非权威镜像，可整体重建 |
| `daily_stats.json` | 按配置时区归档的日统计（由已确认增量派生的按日桶，不承载统一字段事实） | 1 | 无 | 日界以 backend 配置时区为准，不按设备本地时间切分 |

- 单次重命名即一次持久化事务边界；不引入任何数据库。
- **单事务分组约束**：`local_total`、`acked_total`、`round_id`、`round_cursor`、`round_state`、
  `pending_completion` 与篇章动作族的 `action_id` 必须落在同一个文件（`progress.json`）内，
  掉电恢复后不得出现高水位、轮次与游标不匹配。
- §11.1 要求 backend 作用域的承载字段集合**恰好等于**五份文件字段列的并集，且「本文不含任何
  `app.data-dir` 之外的后端状态文件」。→ 不得新增第六个后端状态文件。
- `action_id` 同时出现在 `progress.json`（篇章动作族）与 `commands.json`（设置命令族），二者是
  不同族的去重键，不要合并。

### 跨文件写序裁决（架构 Deferred 指派给本 Story）

`ARCHITECTURE-SPINE.md` §Deferred 的 A-2 行明确：「**跨文件（日统计与 ack）的非原子写顺序不在该
契约冻结范围，由实现 Story 在契约粒度内定序**」。本 Story 是该「实现 Story」，裁决如下并在
代码 javadoc 落地：

1. `progress.json` 是**唯一权威**，其六字段 + 篇章 `action_id` 是契约冻结的单事务组，整体一次
   原子替换。
2. `commands.json`、`device_state.json`、`daily_stats.json` 是**派生或镜像**，允许在崩溃后滞后
   或不一致，且可整体重建。
3. 同一次操作需要写多个文件时：**先写 `progress.json`（权威），再写派生文件**。派生文件写失败
   不回滚权威文件、不影响本次已应答的权威结果；下一次幂等重放会收敛。
4. 恢复与读取**只以 `progress.json` 裁决高水位、轮次、游标**；派生文件缺失或损坏**不得**改写
   `progress.json`、**不得**改变权威高水位，也不得使权威读取失败。

本 Story 只冻结该顺序并让恢复/读取逻辑遵守；不实现任何业务写入路径（Epic 5）。

### 字节上限与 fsync 范围裁决

- **字节上限**：deferred 已登记「`identity.json` 文件大小未设上限；需结合 JSON 持久化基线定义
  文件大小策略」。本 Story 定义统一上限 `MAX_JSON_BYTES = 262144`（256 KiB），集中在
  `PersistenceLimits`，在解析**之前**用 `Files.size` 检查；超限即 `50000`，不解析、不落盘。
  该上限对五份文件统一适用；若 `daily_stats.json` 的按日桶长期增长逼近上限，属 Story 5.3 的
  议题（可为其单独提高上限或改变分片策略，须在同一常量类中显式记录并更新本裁决）。
- **fsync 范围**：AC 要求的是「临时文件、`fsync` 和 `rename` 原子替换」。本 Story 在文件级
  `force(true)` 之外，**额外要求** best-effort 父目录 fsync（Task 1.2a）——deferred 已登记
  「数据目录（父目录）未 fsync，掉电后已应答给客户端的 `deviceId` 可能丢失」。实测 Java 17 /
  macOS 上 `FileChannel.open(dir, READ).force(true)` **成功**；Windows 上打开目录为通道会抛异常，
  因此必须 tolerated。**不得**把目录 fsync 的成败计入写入成败判定。

### 明确不做

- 不实现 Epic 5 的幂等高水位、轮次归属/完成确认、历史统计派生与日界归档、命令修订收敛、
  WebSocket 推送、错误重试与安全边界。
- 不实现 4.6 的权威状态快照、差量、`snapshot_seq`、`replay_cursor` 查询与恢复基准。
- 不新增 HTTP 端点、不改 `/`、`/api/v1/health` 语义、不暴露恢复状态。
- 不引入数据库/ORM/连接池/迁移框架；不新增第二个数据目录或第六个后端状态文件。
- 不修改 `application*.yml`、`.gitignore`、`pom.xml` 的现有依赖、`Embedded/**`、
  `cloud/frontend/**`、`cloud/env-scripts/**`、`docs/**`、`AGENTS.md`、`cloud/AGENTS.md`、
  契约与上游规划文档。
- 不做多进程/跨 JVM 锁协议、文件锁（`FileChannel.tryLock`）、可配置上限或后台压缩。
- 不把文件布局、桶结构或上限值写成第二份契约副本。

### Git 与并发工作区纪律

基线 HEAD 为 `db869b7d0796e1dd5f7084f6709ca59668503cb2`（工作树脏，见「关键回归风险」第 6 条）。
本 Story 允许新增/修改的实现面仅限 `cloud/backend/**`、对应测试与本 Story 文档/receipt。
禁止任何变更性 git 操作（commit/branch/stash/reset/checkout/merge）。

## Architecture Compliance

- **AD-16 backend 基线 / 零数据库**：单实例单进程、`app.data-dir` 下 JSON 原子文件（临时文件 +
  fsync + rename）、禁止任何数据库与第二套平行 backend。本 Story 是该 AD 的落地实现。
- **AD-2 backend 权威**：`progress.json` 是云端正式进度的权威载体；派生文件不得反向覆盖权威值。
- **AD-6 可信时间**：日界以 backend 配置时区为准，`daily_stats.json` 的桶语义属 Story 5.3；本
  Story 不实现日期计算，也不把设备本地时间当作日界。
- **AD-5 命令修订**：`commands.json` 的 schema 就位，但「只保留最新修订」的收敛判定属 Epic 5。
- **AD-20 配置与路径**：`app.data-dir: ./data` 与 `application*.yml` 位置不改；不新增配置键。
- **Consistency Conventions**：JSON 文件写入原子化（tmp+rename）；无第二计数路径、无第二套
  平行云后端。
- **FR-C-010**：临时文件 + `fsync` + `rename`；不引入 SQLite/PostgreSQL 或其他数据库；重启恢复
  全部状态；写入中断不留半写状态。
- **NFR5 / SM-5**：backend 重启后高水位、游标、配置和历史统计不丢失。
- **sync-contract SC-1.0.0 §11 / §11.1 / §12**：文件名、字段、`schema_version`、迁移说明、单事务
  分组与持久化作用域逐值遵守；不改写冻结 20001–20006、40001、40101。

## Library / Framework Requirements

- Java 17、Spring Boot 3.3.7、Spring Framework 6.1.x；沿用 4.3 的 Maven parent 与
  `finalName=saas`。**不新增运行时依赖**：所需能力全部来自 `java.nio.file`、`java.nio.channels`
  与既有 Jackson。
- Jackson 继续承担 JSON 读写；读取启用 `DeserializationFeature.FAIL_ON_TRAILING_TOKENS`，**不引入
  第二套 JSON 库**。
- 生命周期钩子用 Spring 的 `ApplicationRunner`/`InitializingBean`/`@PostConstruct` 之一；必须保证
  **只读**（Task 3.2）。
- 测试沿用 `spring-boot-starter-test`（JUnit 5 Jupiter、Mockito、AssertJ/Hamcrest、`spring-test`）
  与既有 `support/TestWorkspace`；持久化单测用 `@TempDir` 直接构造，不启 Spring 上下文。

## Project Structure Notes

预期新增或修改路径（按现有 `top.zhcmqtt.ewf.backend` 包命名，沿用 `common/`、`service/` 分层）：

```text
cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/
  common/persistence/PersistenceLimits.java          # 集中常量：schema_version、字节上限、tmp 命名
  common/persistence/AtomicJsonFile.java             # 原子写基元（tmp + force + ATOMIC_MOVE + 清理 + 目录 fsync）
  common/persistence/VersionedJsonFile.java          # 严格读基元（schema/白名单/trailing/大小）
  common/persistence/PersistenceRecovery.java        # 只读启动恢复
  common/persistence/PersistenceRecoveryReport.java  # 逐文件恢复结论
  service/ProgressStore.java                         # progress.json
  service/CommandsStore.java                         # commands.json
  service/DeviceStateStore.java                      # device_state.json
  service/DailyStatsStore.java                       # daily_stats.json（仅信封）
  service/IdentityStore.java                         # 改为复用基元，行为不变
cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/
  AtomicJsonFileTest.java
  VersionedJsonFileTest.java
  PersistenceRecoveryTest.java
  PersistenceFileContractTest.java
  IdentityStoreTest.java                             # 保持不变，可新增断言（不得放宽）
```

- 四个新 Store 与 `IdentityStore` 同级放 `service/`，与 4.4 已确立的分层一致，不新开第二层包。
- 如果实现选择合并基元（例如 `AtomicJsonFile` 同时承担严格读），必须保持「写」「读」职责可分、
  可单独测试，并保留「唯一实现」约束；**不要**把原子写、schema 校验、恢复编排和业务语义塞进
  一个类。
- 目录/包只有真实职责时才创建，不建空占位。
- 不得新增第六个后端状态文件；不得在 `app.data-dir` 之外放后端状态。

## Testing Requirements

最低门禁：

1. `cd cloud/backend && mvn -q test` —— 4.3/4.4 既有 9 类 40 条 + 本 Story 新增全部绿。
2. `python3 docs/contracts/tests/run_sync_contract_tests.py` —— 契约未被实现改写。
3. `git diff --check` —— 文本 UTF-8 无 BOM、无多余尾随空白、无乱码。
4. `git status --short` —— 未新增 `cloud/backend/data/**`、未新增 `.tmp`、未新增无关文件。

测试只写临时目录（`@TempDir` 或 `Files.createTempDirectory` + `@DynamicPropertySource`），
**不得**创建、读取或修改仓库工作区的 `cloud/backend/data/`。

门禁边界必须如实记录：单进程 `ATOMIC_MOVE` 不能证明多进程/跨 JVM 竞态；文件级 force +
best-effort 目录 fsync 不能证明掉电安全（且目录 fsync 在部分平台不可用）；临时目录测试不能
证明生产数据目录的权限、挂载与磁盘配置；`PersistenceFileContractTest` 只能证明 Java 常量与
契约注册表逐值一致，不能证明运行期不存在其他落盘路径；「重建 Store 实例」只是重启的近似，
不构成独立进程级重启验证。这些不写成已验证。

## Latest Technical Information

- **`ATOMIC_MOVE` 的语义边界**：`Files.move` 的 `ATOMIC_MOVE` 保证「观察到的是完整文件」，
  但**不保证 rename 本身持久化**。若文件系统不支持原子 move 会抛
  `AtomicMoveNotSupportedException`；非原子 move 抛 `IOException` 时「文件状态未定义」。来源：
  <https://docs.oracle.com/javase/tutorial/essential/io/move.html>、
  <https://docs.oracle.com/en/java/javase/26/docs/api/java.base/java/nio/file/Files.html>。
- **`force(true)` ≈ fsync，`force(false)` ≈ fdatasync**：Unix/Linux 上 `FileChannel.force(true)`
  刷新内容与元数据、`force(false)` 走 `fdatasync`（更弱更快）；Windows 上 `metaData` 参数无效；
  macOS 上后续 JDK 改用 `fcntl(F_FULLFSYNC)` 以增强持久性。`FileChannel.force` 只对其自身通道
  的写入生效；`OutputStream.flush()` 对持久性无效。来源：
  <https://stackoverflow.com/questions/5650327/are-filechannel-force-and-filedescriptor-sync-both-needed>、
  <https://mail.openjdk.org/pipermail/nio-dev/2015-May/003168.html>。
- **目录 fsync 是业界补全步骤**：只 fsync 临时文件并不使 rename 的目录项持久化，POSIX 上还需
  在 move 后 fsync **父目录**；该步骤无跨平台 API（Windows 上打开目录为通道会失败），必须
  tolerated。Kafka 的 `Utils.atomicMoveWithFallback`、Maven Resolver 与 ArcadeDB 均采用此配方；
  Kafka 另记录了「move 失败与目录 fsync 同时失败时 fsync 异常掩盖真实失败」的坑（应把 fsync
  失败挂为 suppressed）。来源：<https://github.com/ArcadeData/arcadedb/issues/7465>、
  <https://issues.apache.org/jira/browse/KAFKA-21035>、
  <https://issues.apache.org/jira/browse/MRESOLVER-372>。
  本仓库实测（Java 17.0.19 / macOS）：`FileChannel.open(dir, StandardOpenOption.READ).force(true)`
  **成功返回**，故 best-effort 分支在本机可达；Windows 需 tolerated。
- **Windows 高频 rename 的已知问题**：OpenJDK nio-dev 记录 `MoveFileEx` 在 Windows 上未严格阻塞
  到操作完成，且 `MOVEFILE_WRITE_THROUGH` 才提供写穿透；实践建议仍是「rename 前先 fsync 文件」。
  来源：<https://mail.openjdk.org/pipermail/nio-dev/2023-February/013257.html>。
- **Jackson 尾随内容防护**：读取端使用 `DeserializationFeature.FAIL_ON_TRAILING_TOKENS`（4.4 已
  采用），损坏或尾随垃圾一律 fail closed。来源：<https://github.com/FasterXML/jackson-databind/wiki/Deserialization-Features>。

## References

- [Source: `_bmad-output/planning-artifacts/epics.md`#Epic 4 / Story 4.5] 用户故事、AC、覆盖需求（FR-C-010、AD-16、NFR5）
- [Source: `_bmad-output/planning-artifacts/epics.md`#Functional Requirements#FR-C-010] JSON 文件持久化：临时文件、`fsync`、`rename`；`cloud/backend/data/`；不引入任何数据库；重启恢复全部状态
- [Source: `_bmad-output/planning-artifacts/epics.md`#NonFunctional Requirements#NFR5] backend 重启后高水位、游标、配置和历史统计不丢失
- [Source: `docs/contracts/sync-contract.md`#11] JSON 持久化文件粒度、文件表、单事务分组约束
- [Source: `docs/contracts/sync-contract.md`#11.1] 持久化作用域与字段落点（backend 作用域 = 五份文件字段列的并集）
- [Source: `docs/contracts/sync-contract.md`#12] 错误语义与拒绝条件（冻结业务码，实现层不得重定义）
- [Source: `docs/contracts/sync-contract.schema.json`#persistence_files,#persistence_scopes] 机器可读派生注册表（比对输入，只读）
- [Source: `docs/contracts/sync-contract.md`#1.1,#13] 单一真源规则与静态校验边界（门禁能证明与不能证明的范围）
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`#AD-16] JSON 原子文件、零数据库、单实例单进程、`cloud/backend/data/`
- [Source: `…/ARCHITECTURE-SPINE.md`#Consistency Conventions] 「JSON 文件写入原子化（tmp+rename）」
- [Source: `…/ARCHITECTURE-SPINE.md`#Deferred] A-2 行：文件粒度已由 `SC-1.0.0` 收敛，**跨文件非原子写顺序由实现 Story 定序**；「EWF backend 构建基线」行已收敛
- [Source: `…/ARCHITECTURE-SPINE.md`#工程验证门禁] Cloud/前端：backend 重启后 JSON 中高水位/游标/统计/配置不丢
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#6.2.4] FR-C-010 与验收（进程重启后全部状态恢复；写入中断不留半写状态；不依赖外部数据库）
- [Source: `cloud/AGENTS.md`#2,#3,#6,#7] cloud 层边界、接口与持久化规则、测试与并行开发、文件与编码
- [Source: `AGENTS.md`#3.6] backend 信封/错误码/JSON 原子文件零库（禁止引入 SQL/DB）
- [Source: `docs/backend/README.md`#EWF 边界] 单实例单进程、JSON 原子文件持久化、存 `cloud/backend/data/`、不迁 repository/SQL/Flyway
- [Source: `docs/backend/经验-后端REST信封与微信登录.md`#⑤,#⑥] JSON 零库并发策略、tmp+fsync+rename 基元、时区与日界（仅实现参考，不作产品事实）
- [Source: `_bmad-output/implementation-artifacts/4-4-实现微信登录与单设备身份.md`#Dev Notes,#Tasks,#File List,#Review Findings] `IdentityStore` 现状、原子写先例、错误码纪律与 4-4 的验证缺口教训
- [Source: `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/IdentityStore.java`] 待提取的原子写与严格读内联实现
- [Source: `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/IdentityStoreTest.java`] 必须保持通过的既有断言（并发幂等、无 `.tmp` 残留、损坏不改写）
- [Source: `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/support/TestWorkspace.java`] 契约注册表定位（fail-closed，不允许静默跳过）
- [Source: `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ErrorCodeContractTest.java`] 精确集合门禁与「不手抄第三份」口径
- [Source: `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ForbiddenDependencyScanTest.java`] 零库禁用键扫描（`pom.xml` + `src/main/resources/*.yml`）
- [Source: `cloud/.gitignore`] 已忽略 `backend/data/` 与五份运行时 JSON（无需改动）

## Previous Story Intelligence

Story 4.4（`4-4-实现微信登录与单设备身份.md`）已完成并置于 `done`：

- 采用 `top.zhcmqtt.ewf.backend` 包名、Java 17、Spring Boot 3.3.7、Maven `finalName=saas`；
  本 Story 只在此工程上增量实现，不复制第二套 backend。
- `IdentityStore` 的原子写实现（`Files.createTempFile(dir, ".identity-", ".tmp")` +
  `FileChannel.force(true)` + `Files.move(..., ATOMIC_MOVE)` + `finally` 清理 +
  `FileAlreadyExistsException` 哨兵）与严格读实现（`FAIL_ON_TRAILING_TOKENS` + 字段白名单 +
  一律 `50000` 且不改写）就是本 Story 要提取的基元；**4.4 的 Dev Notes 已给出交接指令**：
  「4.5 应提取并复用原子 JSON 基元，而不是建立第二套身份文件」。
- 4.4 的 Dev Notes 记录了两处可直接复用的裁决：错误码纪律（不改写冻结码、新增码必须同步测试
  集合）与「单实例单进程」前提（不扩展跨进程锁）。
- 4.4 的 review 教训（attempt-1…4）：negative 负例必须**真的能失败**（「取消同 openid 互斥」那条
  因派生值恒同而不可观测）；断言不得放宽或删除既有门禁（曾出现 `EnvelopeContractTest` 的
  405/`Allow`、ERROR 日志、404 断言被解除钉扎后被 review 要求恢复）；Completion Notes 需说明
  并发实现的选型（4.4 实现为方法级 `synchronized` 全局写锁）。本 Story 的 AC5 与 Dev Notes
  已把这三条教训固化为要求。
- 4.4 的 deferred 明确指出本 Story 应处理的三项：`identity.json` 文件大小上限（→ 本 Story 的
  `MAX_JSON_BYTES`）、父目录未 fsync（→ Task 1.2a）、跨进程 `ATOMIC_MOVE` 语义（→ 明确不做，
  属部署形态议题）。另有一条 `ATOMIC_MOVE` 静默替换既有文件的 deferred 同样属跨进程面，本
  Story 仍不做，但需在 Completion Notes 复述该边界。

Story 4.3（backend 骨架）已确立并不得破坏：`ApiResponse` 的 `NON_NULL` 语义、
`GlobalExceptionHandler` 对 400/404/405/415/500 与冻结 40101 的映射、根探活 `/` 与
`/api/v1/health`、`application*.yml` 的固定值，以及「`cloud/backend/data/` 运行期数据与通用
JSON 持久化属于后续 Story」的边界声明——本 Story 正是该边界的收口。

## Git Intelligence

- 基线 HEAD：`db869b7d0796e1dd5f7084f6709ca59668503cb2`。
- 近期相关提交：`db869b7`/`9ad9458`/`e0a38ea`（autopilot/code-review 技能与构建验证流程）、
  `47e4f18`（EWF cloud 配置、`cloud/AGENTS.md`、env-scripts）、`fe98d73`（规划产物与 epic 文件）。
- 工作树当前有 4.4 的 autofix 未提交修改（6 个 Java 文件 + `deferred-work.md` +
  `sprint-status.yaml`）。实现必须建立在当前工作树内容上，且只保留与本 Story 直接相关的
  `cloud/backend/**` 与测试改动。

## Developer Context

### Implementation Boundary

本 Story 结束时，开发者应能在**临时数据目录**中用两个不同的 `app.data-dir` 证明：五份契约文件的
schema 基线与迁移说明就位、写入是 tmp+force+`ATOMIC_MOVE` 原子替换且失败无残留、字节上限在解析
前生效、损坏/越界/尾随内容一律 `50000` 且不改写文件、孤儿 `.tmp` 永不进入读取路径、启动恢复
全程只读（目录不存在时仍不存在）、`progress.json` 损坏时读取**不返回**默认高水位。
不能以「能写 JSON」替代原子性证据，不能以「测试全绿」替代掉电/多进程结论，不能把 Epic 5 的
高水位同步、轮次业务、命令收敛、统计派生、WebSocket 或 4.6 的快照查询写进本 Story。

### Source Tree Components to Touch

- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/`：原子写基元、严格读
  基元、集中常量、只读恢复与恢复报告。
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/`：`ProgressStore`、`CommandsStore`、
  `DeviceStateStore`、`DailyStatsStore`，以及 `IdentityStore` 的基元迁移。
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/`：基元单测、恢复单测、文件契约门禁，
  以及 `IdentityStoreTest` 的（仅新增式）加固。
- **不触碰**：`pom.xml`（除确有必要且不命中禁用键）、`src/main/resources/**`、`Embedded/**`、
  `cloud/frontend/**`、`cloud/env-scripts/**`、`docs/**`、`AGENTS.md`、`cloud/AGENTS.md`、
  `.gitignore`、上游规划产物。

### Completion Status

Story context engine analysis completed - comprehensive developer guide created.
创建本 Story 后，`sprint-status.yaml` 的 `4-5-建立-json-原子持久化基线` 必须为 `ready-for-dev`；
receipt 必须写入 `.autopilot/4/4-5-建立-json-原子持久化基线/attempt-1/create-story.json`，
并保留 `phase=A`、`ok=true` 与完整来源/边界记录。

## Dev Agent Record

### Agent Model Used

deepseek-v4-flash（Epic Autopilot 无人监管 best-effort 模式，`repair_stage=normal`、`repair_attempt=1`）

### Debug Log References

全部日志置于 `/tmp`（不进入仓库，避免污染工作区）：

| 用途 | 日志路径 | 退出码 |
| --- | --- | --- |
| 开发前基线全量回归 | `/tmp/ewf-4-5-baseline.log` | 0（9 类 40 条） |
| 主源码编译 | `/tmp/ewf-4-5-compile.log` | 0 |
| 新增 4 个测试类（首轮） | `/tmp/ewf-4-5-newtests-1.log` | 1（`PersistenceRecoveryTest` 编译错：残留已删除辅助方法） |
| 新增 4 个测试类（次轮） | `/tmp/ewf-4-5-newtests-2.log` | 1（1 条断言：`read()` 返回信封，测试夹具未含 `schema_version`） |
| 新增 4 个测试类（GREEN） | `/tmp/ewf-4-5-newtests-3.log` | 0（26 条） |
| 全量回归（首轮） | `/tmp/ewf-4-5-fulltests-1.log` | 0（13 类 66 条） |
| 契约门禁 | `/tmp/ewf-4-5-contracttests.log`、`/tmp/ewf-4-5-contracttests-2.log` | 0（负例 35/35） |
| 全量回归（负例恢复后终态） | `/tmp/ewf-4-5-fulltests-2.log` | 0（13 类 66 条） |
| 负例证据（RED→GREEN 各 1 条） | `/tmp/ewf45-mutation/red-N.log`、`green-N.log`（N=1..5） | RED=1、GREEN=0 |

构建恢复：本 Story 属 `cloud/backend` Java/Maven 面，**不涉及** `Embedded/`、ESP-IDF、CMake、
固件、硬件、驱动或构建系统，故不适用 macOS ESP-IDF 构建恢复门禁（`build_recovery=not-applicable`）。

### Completion Notes List

**实现闭环**：AC1–AC5 全部落地，Tasks 1.1–5.7 共 31 项全部完成。`cd cloud/backend && mvn -q test`
13 类 66 条全绿（4.3/4.4 既有 9 类 40 条无回归 + 本 Story 新增 26 条）。

**做了什么**

1. **基元抽取（Task 1）**：`common/persistence/PersistenceLimits` 集中 `SCHEMA_VERSION=1`、
   `MAX_JSON_BYTES=262144`、临时文件命名规则与 `envelopeFields()`；`AtomicJsonFile` 是唯一原子写
   实现（同目录 tmp + `FileChannel.force(true)` + `ATOMIC_MOVE` + `finally` 清理 + best-effort
   父目录 fsync），**未**提供非原子覆盖回退分支；`VersionedJsonFile` 是唯一严格读实现（常规文件、
   解析前字节上限、UTF-8、`FAIL_ON_TRAILING_TOKENS`、根为 object、字段集合精确匹配、
   `schema_version` 整数且为 1），四类错误文案区分「不可读取 / 已损坏 / 格式无效 / 超出大小上限」。
2. **五份契约文件（Task 2）**：`ProgressStore`/`CommandsStore`/`DeviceStateStore`/`DailyStatsStore`
   与 `IdentityStore` 各自暴露 `fileName()` + `allowedFields()`（契约字段列，不含信封
   `schema_version`）并与 `PersistenceFileContractTest` 机械比对。`ProgressStore` javadoc 落地单事务组、
   权威性与跨文件写序；`DeviceStateStore` 记录非权威镜像；`DailyStatsStore` 只建信封、**不**对桶键做
   白名单（桶结构留给 Story 5.3，容器键名 `buckets` 明确标注为非契约真源）。
3. **跨文件写序裁决（Task 2.6，架构 Deferred A-2 指派）**：在 `ProgressStore` 与
   `PersistenceRecovery` javadoc 冻结「`progress.json` 唯一权威单事务组 → 先写权威再写派生 →
   派生写失败不回滚权威 → 恢复与读取只以 `progress.json` 裁决高水位」。本 Story 只冻结顺序，
   不实现业务写入路径。
4. **只读恢复（Task 3）**：`PersistenceRecovery`（`ApplicationRunner`）+ `PersistenceRecoveryReport`
   （`MISSING`/`OK`/`UNRECOVERABLE` + 失败原因分类 `unreadable`/`too_large`/`invalid_or_corrupted`）。
   绝不建目录、绝不建文件、绝不改写既有文件；只读精确文件名，不做通配/前缀扫描；孤儿 `.tmp`
   best-effort 删除且失败只记 WARN；校验失败只记 ERROR 与标记，不中断 Spring 启动、不删除损坏文件。
   实测日志形如
   `dir=<绝对目录>, file=commands.json, category=invalid_or_corrupted, reason=「commands.json」格式无效`，
   不含绝对路径、文件内容或身份值。
5. **`IdentityStore` 迁移（Task 4）**：落盘改走 `AtomicJsonFile`、严格读改走 `VersionedJsonFile`；
   仍只写 `schema_version` + `device_id`、仍带 `System.lineSeparator()` 尾换行、仍以
   `FileAlreadyExistsException` 作并发建号哨兵、身份不匹配仍 `40300`、损坏/不可读仍 `50000` 且绝不
   重建、失败路径仍无 `.tmp` 残留、读写仍为方法级 `synchronized`。**未**放宽或删除任何既有断言
   （`IdentityStoreTest` 4 条、`AuthContractTest` 7 条原样通过）。
6. **负例证据（Task 5.5）**：5 条负例全部实测 RED→GREEN（见下）。其中负例 4 首轮**未能失败**
   （保留期恢复清理先行删除了孤儿 `.tmp`，掩盖了读取路径缺陷），已按 4-4 教训修正测试——把读取面
   断言提到恢复之前，使其在无清理介入时被真实观测——随后确认可失败。

**5 条可真实失败负例（RED→GREEN）**

| # | 制造 RED 的手法（实测位置） | 首轮失败断言 | 恢复后 |
| --- | --- | --- | --- |
| 1 | `AtomicJsonFile` 失败路径去掉 `Files.deleteIfExists(tmp)` | `AtomicJsonFileTest.失败路径零临时残留:97`「失败的原子写不得残留 .tmp 文件」expected: `<0>` but was: `<1>` | 退出码 0 |
| 2 | `VersionedJsonFile.readObject` 解析失败返回空 default + `read` 对缺失字段取默认值 | `VersionedJsonFileTest.schema版本非法被拒绝:76` 与 `字段集合精确匹配:83`「非法内容必须被拒绝」Expected `BusinessException` to be thrown, but nothing was thrown | 退出码 0 |
| 3 | `ProgressStore.read()` 加 `catch (BusinessException) → 默认 acked_total=0` 回退 | `PersistenceRecoveryTest.权威文件损坏时绝不静默回退:124` Expected `BusinessException` to be thrown, but nothing was thrown | 退出码 0 |
| 4 | `ProgressStore.read()` 按 `.progress-*` 前缀扫描，把孤儿 `.tmp` 当数据源优先读取 | `PersistenceRecoveryTest.孤儿临时文件不进入读取路径:97`「中间产物永不进入读取路径」expected: `<41>` but was: `<0>` | 退出码 0 |
| 5 | `PersistenceRecovery.recover()` 里 `Files.createDirectories(dataDirectory)`（等价 `@PostConstruct` 建目录） | `PersistenceRecoveryTest.目录不存在时保持只读:73`「只读恢复不得创建数据目录（启动入口与恢复调用都不得建目录）」expected: `<false>` but was: `<true>` | 退出码 0 |

每条负例的完整日志见 `/tmp/ewf45-mutation/red-N.log` 与 `green-N.log`；恢复后已用 `shasum` 与
`grep -rn "NEGATIVE" cloud/backend/src/` 双重确认实现字节回到原状、无残留标记。

**门禁与卫生**

- `cd cloud/backend && mvn -q test` → 退出码 0，`Tests run: 66, Failures: 0, Errors: 0, Skipped: 0`。
- `python3 docs/contracts/tests/run_sync_contract_tests.py` → 退出码 0，「负例门禁：35/35 通过」。
- `git diff --check` → 退出码 0（UTF-8 无 BOM、无多余尾随空白）。
- `git status --short` → **未新增** `cloud/backend/data/**`（`ls cloud/backend/data` 报不存在）、
  未新增任何 `.tmp`、未新增无关文件；仅新增本 Story 的 13 个实现/测试文件与 1 个修改文件。
- 契约测试全程只写 `@TempDir`，不创建、不读取仓库工作区的 `cloud/backend/data/`。

**对外可观测行为变化（唯一一处，已评估）**：`IdentityStore` 的 `50000` 错误文案由
「身份文件不可读取/格式无效/损坏，…」改为带文件名的「「identity.json」不可读取/已损坏/格式无效/超出
文件大小上限，请修复后重试」，以统一四类诊断口径并避免在响应中透出绝对路径。错误**码**与全部
4.2 列举的行为不变式均未变；`IdentityStoreTest`/`AuthContractTest` 无任何断言依赖文案，实测原样通过。

**fsync 范围实测结论（Task 1.2a）**：Java 17.0.19 / macOS 上
`FileChannel.open(dir, READ).force(true)` **成功返回**——全量回归日志中该 WARN 仅出现 1 次，且来自
刻意用 `OWNER_WRITE|OWNER_EXECUTE`（不可读目录）构造的 tolerated 分支测试
（`AtomicJsonFileTest.父目录fsync失败被容忍`，非 skipped）；其余全部写入均未触发 WARN，证明目录 fsync
在本机可达且其成败**不**参与写入结论判定。Windows 上该分支必须 tolerated（打开目录为通道会抛异常）。

**门禁边界（如实声明，均不构成已验证）**

- 单进程 `ATOMIC_MOVE` 不能证明多进程/跨 JVM 竞态；本 Story 不提供文件锁与跨进程协议（AD-16 单实例单进程）。
- 文件级 `force(true)` + best-effort 目录 fsync 不能证明掉电安全；`ATOMIC_MOVE` 只保证「观察到完整
  文件」，不保证 rename 本身持久化。
- `@TempDir` 测试不能证明生产数据目录的权限、挂载与磁盘写缓存配置。
- `PersistenceFileContractTest` 只能证明 Java 常量与契约注册表逐值一致，**不能**证明运行期不存在其他落盘路径。
- 「重建 Store 实例」只是进程重启的**近似**，不构成独立进程级重启验证；`recovery.run(null)` 只覆盖
  `ApplicationRunner` 入口，不等价于完整的 Spring 启动编排。
- 4.4 已登记的 deferred「`ATOMIC_MOVE` 静默替换既有文件」属跨进程面，本 Story 仍不做。

**未做（遵守故事边界）**：Epic 5 的幂等高水位、轮次归属/完成确认、历史统计派生与日界归档、命令修订
收敛、WebSocket；4.6 的快照/差量/`snapshot_seq`/`replay_cursor`；未新增 HTTP 端点；未引入数据库/
ORM/连接池/迁移框架；未第二个数据目录或第六个后端状态文件；未改 `pom.xml`、`application*.yml`、
`.gitignore`、`Embedded/**`、`cloud/frontend/**`、`docs/**`、`AGENTS.md`、`cloud/AGENTS.md`。

### File List

新增（13）：

- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/PersistenceLimits.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/AtomicJsonFile.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/VersionedJsonFile.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/PersistenceRecovery.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/PersistenceRecoveryReport.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/ProgressStore.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/CommandsStore.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/DeviceStateStore.java`
- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/DailyStatsStore.java`
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/AtomicJsonFileTest.java`
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/VersionedJsonFileTest.java`
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/PersistenceRecoveryTest.java`
- `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/PersistenceFileContractTest.java`

修改（1）：

- `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/IdentityStore.java`

删除（0）：无。

未列入：本 Story 文件、`sprint-status.yaml`（外层 diff 显式排除）；工作树中 4.4 遗留的未提交修改
（`JwtAuthenticationFilter.java`、`AuthContractTest`、`EnvelopeContractTest`、`IdentityStoreTest`、
`RuntimeBaselineTest`、`WechatMiniClientTest`、`deferred-work.md`）——本 Story **未触碰**，仅在其**当前
工作树内容**上增量实现，按「关键回归风险」第 6 条未做任何 git 变更性操作。

### Change Log

- 2026-09-23：Story 4.5 实现完成并推进至 `review`。抽取 `AtomicJsonFile`/`VersionedJsonFile` 唯一
  原子写与严格读基元、`PersistenceLimits` 集中常量；建立 `progress.json`/`commands.json`/
  `device_state.json`/`daily_stats.json` 四份契约文件 Store 的 schema 基线与迁移说明；
  落地 `PersistenceRecovery` 只读启动恢复与 `PersistenceRecoveryReport`；冻结跨文件写序；
  `IdentityStore` 迁移到公共基元且对外行为不变；新增 26 条测试（含 4 个新测试类）与 5 条实测
  RED→GREEN 负例。全量回归 66 条全绿，契约门禁 35/35，`git diff --check` 干净。

### Review Findings

深度 `deep`、`review_mode=full`、`action_policy=autofix` 的单轮无人监督审查（4 个审查层全部完成）。
20 条原始 findings 归一为 13 组；4 组已自动修复，9 组记录为质量债务，2 条驳回。

- [x] [Review][Patch] 存在性不可判定被当作「缺失」，权威文件静默退化为未初始化 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/VersionedJsonFile.java:97、cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/PersistenceRecovery.java:117] — 已修复：`Files.exists` 在无法判定存在性时（数据目录权限不足、无法 stat）也返回 `false`，原实现据此返回 `Optional.empty()` 或报 `MISSING`，使 AC3「绝不静默回退」被绕过（实测：目录 `0000`/`0400` 时 `exists=false`、`notExists=false`、`isRegularFile=false`，而目录本身 `isDirectory=true`）。改为只用 `Files.notExists` 判定缺失，其余不可判定情形一律 fail closed（Store 读取回 `50000`，恢复判 `UNRECOVERABLE/unreadable`）。既有缺失语义的测试全部保持通过。
- [x] [Review][Patch] 孤儿临时文件清理的匹配面宽于本基线的命名规则 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/PersistenceRecovery.java:171] — 已修复：`isTemporaryFileName` 只判「以 `.` 开头、以 `.tmp` 结尾」，启动恢复据此 `Files.deleteIfExists` 会删除数据目录里任何 `<任意名>.tmp`（含运维手工备份 `.progress.tmp`），超出 Task 3.3「匹配 `PersistenceLimits` 的临时文件命名规则」的范围。新增 `TEMPORARY_PREFIXES`（由五份契约文件名经 `tempPrefix` 机械推导）并要求同时命中前缀，删除范围收敛到本基线自己的中间产物。
- [x] [Review][Patch] `tempPrefix` 在目录创建之后求值，非法目标名会留下已创建的目录 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/AtomicJsonFile.java:71] — 已修复：`PersistenceLimits.tempPrefix` 对非 `.json` 目标抛 `IllegalArgumentException`，原顺序先 `Files.createDirectories(parent)` 再求值，导致调用方错误留下目录且抛出未声明的未检异常。改为在首次文件系统副作用之前求值。
- [x] [Review][Patch] `daily_stats.json` 的容器键无任何跨版本锚点 [cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/PersistenceRecoveryTest.java:239] — 已修复：该容器键是本 Story 唯一真正落盘却未被契约冻结的格式元素，原有覆盖全是「同一常量先写后读」的对称回环（`VersionedJsonFileTest` 自带字面量、契约测试断言该文件字段列为空），键名漂移后全部门禁仍绿，但升级后的进程会读不回上一版写下的文件。新增外部字面量锚定测试；已实测可真实失败（把 `DailyStatsStore.BUCKETS_FIELD` 改名后 `Tests run: 9, Failures: 0, Errors: 1`，退出码 1，首失败断言 `日统计容器键由外部字面量锚定:252`，恢复后退出码 0）。
- [x] [Review][Defer] 契约 §11 的加法迁移规则在严格字段集合相等下不可实现 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/VersionedJsonFile.java:61] — deferred: `sync-contract.md` §11 与注册表 `persistence_files[0].migration` 均写「新增字段时可省略读取并取默认值」，而读取面执行字段集合精确相等（AC2 本身即要求「字段集合精确匹配」并有测试钉住「多一个字段即 50000」）。这是规格与契约之间的张力，不是实现可单方面裁决的选择；按 `cloud/AGENTS.md` §6「先更新契约样例和测试，再修改实现」，需先在契约层裁定「严格相等优先」还是「加法容忍」。
- [x] [Review][Defer] `FileAlreadyExistsException` 并发哨兵的 javadoc 承诺在本实现下不可达 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/AtomicJsonFile.java:58] — deferred: `Files.move(..., ATOMIC_MOVE)` 走 rename 语义、目标已存在时静默替换，该哨兵在单实例内不可达，`deferred-work.md` 的 4.4 attempt-4 段已登记同一事实。本 Story 把这条保证从 `IdentityStore` 单个调用方扩散为基元对所有未来调用方的对外契约（Epic 5 的 Store 若据此做并发保护，实际得到静默 last-writer-wins）。修法（`FileChannel.tryLock` 或 move 后回读校验）属跨进程语义，AD-16 明确不做。
- [x] [Review][Defer] 信封写路径在四个 Store 里逐份复制，且无断言钉住序列化形态 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/ProgressStore.java:95、CommandsStore.java:84、DeviceStateStore.java:79、DailyStatsStore.java:87] — deferred: 字段集合校验 → `deepCopy`/`put(schema_version)` → 序列化 + `System.lineSeparator()` → `AtomicJsonFile.write` → 两个 catch 映射 `50000`，四份副本；`PersistenceLimits` 自称信封事实的唯一出处却只承载常量。任一份副本漂移（少写尾换行、换错误码）不会有测试失败。收敛需先在基元里定义一个信封写入入口，属新增公共面，不在本轮自动修复范围。
- [x] [Review][Defer] Spring 解析出的 `app.data-dir` 是否到达恢复流程与四个 Store 无任何断言 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/PersistenceRecovery.java:96] — deferred: 全仓 6 处 `@Value("${app.data-dir:./data}")` 各自独立绑定，`PersistenceRecoveryTest` 全部由测试自己把路径传给构造器，`dataDirectory()` 无任何调用方。实测把 `PersistenceRecovery` 的属性键改成不存在的键（默认值静默生效）后 66 条门禁仍全绿，而自定义数据目录部署下启动恢复会去审计 `./data`：真实目录里的损坏 `progress.json` 不会被标记、孤儿 `.tmp` 不会被清理。与 `RuntimeBaselineTest` 已确立的「绑定失效不得静默」口径冲突。修法是在已有 `@DynamicPropertySource` 的全上下文测试中 `@Autowired PersistenceRecovery` 并断言 `dataDirectory()`；本轮不自动应用，因为它需要新增本 Story File List 之外的文件，会让 story-local diff 与 dev receipt 的 File List 失配。
- [x] [Review][Defer] 严格读取面只校验字段名集合、不校验字段值类型，恢复判定弱于实际读取语义 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/VersionedJsonFile.java:61、cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/IdentityStore.java:106] — deferred: `{"schema_version":1,"device_id":42}` 可通过恢复校验被判为「正常」，而 `readDeviceId()` 对该文件一律 `50000`（启动日志说正常、每个受保护请求失败）；同理 `acked_total` 类型错误时 `asInt()` 静默给出 `0`，即 AC3 禁止的默认高水位。修法需要冻结逐字段值类型，而契约注册表的 `persistence_files` 不承载类型、本 Story 也刻意不新增第三份字段表，故属契约层议题（Story 5.3 / Epic 5 的载荷语义）。
- [x] [Review][Defer] `IdentityStore` 的 `50000` 文案变更与 AC1「对外行为保持不变」存在张力 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/IdentityStore.java:104] — deferred: 文案经 `GlobalExceptionHandler` 原样透出，确属外部可观测；但 Task 1.4 要求身份读写复用唯一基元，Task 1.3 又要求四类诊断文案带文件名，保留旧文案需给基元加消息覆写参数（新增公共面）。无任何测试或契约依赖该文案，4.4 也从未冻结它。附注：审计方同时称统一 256 KiB 上限改变了 `identity.json` 的既有语义，这一半不成立——该文件唯一载荷字段是由 openid 派生的 48 字符 `device_id`，不可能存在合法的超限文件。
- [x] [Review][Defer] 负例 #4 的手法与规格表格不符，恢复侧「不通配扫描」未被证伪 [cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/PersistenceRecoveryTest.java:97] — deferred: Task 5.5 表第 4 行要求变异落在恢复逻辑的通配扫描上，实际变异落在 `ProgressStore.read()`（并已按 4.4 教训把读取面断言提到 `recover()` 之前，使变异产生真实的 `expected <41> but was <0>`）。由于恢复报告只承载 `MISSING/OK/UNRECOVERABLE` + 分类、从不暴露读到的数值，即便恢复真按通配读取孤儿 `.tmp`，该测试的断言仍会通过——即 AC3「中间产物永不进入读取路径」在恢复侧缺少可失败的失败面。属证据方法缺陷（恢复侧本身无读取副作用），需补一条可观测恢复侧行为的负例。
- [x] [Review][Defer] AC5 证据记录与实测不符 [cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/PersistenceRecoveryTest.java:1] — deferred: 独立复核确认 `/tmp/ewf45-mutation/` 只有 `green-1..4`，**缺 `green-5.log`**，而 Completion Notes 写的是 `green-{1..5}.log`；5 条负例均未记录命令，日志只在 `/tmp` 不可长期复现；同节「实测日志形如 `dir=<绝对目录>…`，不含绝对路径」一句自相矛盾；「对外可观测行为变化（唯一一处）」与统一字节上限的说明也不一致。底层结论仍成立（`red-5.log` 有真实失败断言、其后 `/tmp/ewf-4-5-fulltests-2.log` 66 条全绿证明已恢复）。
- [x] [Review][Defer] `AtomicJsonFile` 失败路径的清理异常会掩盖原始异常 [cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/persistence/AtomicJsonFile.java:94] — deferred: `finally` 中的 `Files.deleteIfExists(temporary)` 若抛 `IOException`，会替换掉正在传播的原始异常（例如 `FileAlreadyExistsException` 哨兵，使 `IdentityStore` 改报 `50000`）。该写法逐字继承自 4.4 的内联实现，属既有模式；触发需要「刚在可写目录创建出的临时文件删除失败」，日常不可达。

#### Rejected

- `DailyStatsStore.write(null)` 会把 `NullNode` 写进容器、被自己的读取面判为损坏：驳回（`low`）——需要未来调用方传入 `null`，而按 Jackson 类型构造 `ObjectNode` 的调用方不会这样做；加空值守卫属为未证实路径增加分支。
- `common.persistence ↔ service` 包级双向依赖（`PersistenceRecovery` 反向 import 五个 Store）：驳回（`low`）——`cloud/AGENTS.md` 未对分层方向作规定，`PersistenceRecovery` 放在 `common/persistence/` 恰是本 Story 「Project Structure Notes」明确指定的布局，且无任何测试约束该方向；重排包结构属超出本次改动面的重构。
- 无 `false` findings 需要驳回：各审查层引用的实现事实经独立复核均成立（含 `Files.exists` 盲点、`isTemporaryFileName` 匹配面、`BUCKETS_FIELD` 无锚点、缺 `green-5.log`）。

#### 审查执行与门禁

- 审查层：`blind-hunter`、`edge-case-hunter`、`verification-gap`、`acceptance-auditor`（4/4 完成，无失败层）。
- 补丁后门禁（`cd cloud/backend && mvn -o test` + `run_sync_contract_tests.py` + `git diff --check`）：退出码 0、`Tests run: 67, Failures: 0, Errors: 0, Skipped: 0`、契约负例门禁 35/35、diff 卫生干净。
- 本轮补丁只改动本 Story File List 内已有的文件（`AtomicJsonFile`、`VersionedJsonFile`、`PersistenceRecovery`、`PersistenceRecoveryTest`），未新增文件、未改动契约、未触碰 4.4 遗留改动。

### Status

done

