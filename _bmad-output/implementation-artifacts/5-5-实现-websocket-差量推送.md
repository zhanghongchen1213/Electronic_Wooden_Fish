# Story 5.5: 实现 WebSocket 差量推送

story_key: 5-5-实现-websocket-差量推送
baseline_commit: 23216215b4342da96faf2bd4fc5a63b784e97948

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a 小程序用户，
I want 在线时按确认顺序收到逐字差量和设备状态，
So that 阅读流可以及时更新而不跳字。

**覆盖需求：** FR-C-009、AD-17、NFR2、NFR3

## Acceptance Criteria

1. **确认后推送单调序号差量，且与同水位查询一致（FR-C-009 / AD-17）**
   **Given** backend 经既有写路径确认了新的高水位、完成态或命令/设备状态变化，且存在已鉴权 WebSocket 连接
   **When** 连接正常
   **Then** 按契约 §10.1 推送 JSON 文本帧：至少覆盖 `delta`（已确认差量）、`completion`（完成确认时）、`command_state`（命令修订/已生效变化时）；帧必带 `contract_version=SC-1.0.0`、`type`、`seq`
   **And** `delta` 载荷字段闭包恰为注册表样例 / `ws_frames[delta].payload_fields`：`acked_total`、`local_total`、`round_id`、`round_cursor`（**不**发明 `glyph`/`char` 等「当前字」wire 字段——「当前字」由 frontend 用 `round_cursor` + canonical 派生，属 Epic 6）
   **And** 同一时刻对同一 `device_id`：任意已推送帧所表达的权威事实，与 `GET /api/v1/sync/snapshot` 在相同 `snapshot_seq` 下装配的 17 字段快照**不矛盾**（水位、轮次、命令修订对、设备镜像一致）
   **And** WebSocket **不是** `{code,message,data}` 信封通道；帧形状以契约 §3/§10 为准

2. **断线恢复以查询冻结水位为唯一基准（FR-C-009 / AD-17 / FR-C-006）**
   **Given** 连接断开、重连，或客户端序号落后 / 出现空洞
   **When** 客户端请求恢复
   **Then** backend **不**以客户端自报「已展示计数」或连接记忆为权威；补齐入口仍是既有 `GET /api/v1/sync/snapshot`（冻结 `snapshot_seq`）
   **And** 重连后推送只允许 `seq >` 该次查询冻结的 `snapshot_seq`（契约 §8：从 `snapshot_seq + 1` 续订）
   **And** 客户端规则（本 Story 用 mock 客户端断言）：`seq <= last_applied_seq` 可丢弃；出现空洞**不得**直接播放后续帧（须先查询补齐）——backend 侧不得主动「跳 seq 填洞播放」
   **And** backend **不**实现可回放的历史帧环形缓冲（契约恢复路径 = 查询；WS 只推**确认后的新事件**）

3. **通道、鉴权、心跳与门禁（AD-17 / AD-20 / NFR2 / NFR3）**
   **Given** 本地 `VITE_API_BASE_URL=http://localhost:9218/api/v1`
   **When** 建立 WebSocket
   **Then** origin/path 按契约 §10.2 派生：`ws://localhost:9218/api/v1/ws`；鉴权**只**经 query `token=`（access JWT）；小程序「单一活跃 socket」约束由客户端遵守，backend 对同一 `device_id` **推荐**在新连接 `afterConnectionEstablished` 时关闭该设备旧连接（避免双推）
   **And** 应用层心跳：backend 按默认 `ws_heartbeat.interval_ms=30000` 发 `heartbeat`，客户端回 `heartbeat_ack`；超过 `timeout_ms=10000` 无应答则关闭（关闭码优先标准段；业务拒绝用 `4000`–`4999`）
   **And** 非法/过期 `token`、身份不匹配：握手失败或发送 `error` 帧（`error.code`/`message`）后关闭；**不得**把 JWT/`session_key` 写入日志正文
   **And** 不得改 `docs/contracts/**` 字节；不得新增 `ErrorCode` 常量；不得新增第六个 JSON 状态文件或数据库；不得改 17 字段快照闭包

## Tasks / Subtasks

- [x] Task 1：冻结本 Story 裁决并落成 javadoc（AC: #1 #2 #3）
  - [x] 1.1 落地 Dev Notes「必须作出的裁决」A–G，每项在对应类 javadoc 写明选择、理由与约束
  - [x] 1.2 明确「本 Story 不修改契约」：`docs/contracts/sync-contract.md` 与 `sync-contract.schema.json` 只读引用
  - [x] 1.3 把「契约缺口登记」写入 `_bmad-output/implementation-artifacts/deferred-work.md`（新增一节 `Deferred from: create-story of 5-5-实现-websocket-差量推送`）
  - [x] 1.4 更新 `ProgressSyncService` / `SyncController` / `StateSnapshotService` 中「不做 WebSocket」类注释，使其与本 Story 实际交付对齐

- [x] Task 2：依赖与 WebSocket 端点骨架（AC: #3）
  - [x] 2.1 `pom.xml` 增加 `spring-boot-starter-websocket`（随 Spring Boot 3.3.7 BOM，**不**引入 STOMP/SockJS 除非测试需要；生产通道为**原生 WebSocket 文本帧**）
  - [x] 2.2 `@EnableWebSocket` + `WebSocketConfigurer`：注册 handler 到 **`/api/v1/ws`**（契约 `path_suffix=/ws` 接在 REST base `/api/v1` 之后）
  - [x] 2.3 `HandshakeInterceptor`：解析 query `token`；`JwtTokenProvider.parseAccess` + `IdentityStore` 校验 subject；成功则把 `device_id` 写入 `WebSocketSession` attributes；失败 `return false`（或升级后立即 `error`+关闭——二选一须在 javadoc 写明，推荐握手期拒绝）
  - [x] 2.4 调整 `JwtAuthenticationFilter`：对 **`GET /api/v1/ws` 握手**放行到 WebSocket 栈（**不**要求 `Authorization: Bearer`），由拦截器验 `token`；**禁止**把该路径加成「任意 HTTP 方法公开白名单」
  - [x] 2.5 禁止 CORS/`setAllowedOrigins("*")` 无脑放开到生产语义；本地测试可用显式 origin 列表或测试专用配置，理由写入 javadoc

- [x] Task 3：帧编解码与序号空间（AC: #1 #2）
  - [x] 3.1 新增帧 DTO / builder：按注册表 `required` 字段输出 snake_case JSON；`contract_version` 常量 = `SC-1.0.0`
  - [x] 3.2 **裁决 B（序号）**：业务帧 `seq` 与 `StateSnapshotService.exportSnapshotSeq` **同一「已确认敲击」空间**——`delta.seq :=` 确认后的 `acked_total`（= 导出 `snapshot_seq`）；连接后可选推送的 `snapshot` 帧：`seq` 与载荷 `snapshot_seq` 均取当前导出值
  - [x] 3.3 **非进度帧**（`command_state` / `completion` / `heartbeat` / `error`）：在**同一连接会话**内使用 `session_seq = max(last_sent_seq, current_snapshot_seq) + 1` 保证帧间单调；**重启/重连后不承诺重放这些帧**——客户端必须先 REST 查询再续订（AC2）
  - [x] 3.4 明确不做：独立持久化 seq 计数器、第六状态文件、把注册表样例 `ws_snapshot(seq=512,acked=128)` 的偏移当作实现义务（登记 deferred）

- [x] Task 4：推送钩子（AC: #1）
  - [x] 4.1 引入窄接口（如 `RealtimePushPort` / `SyncRealtimePublisher`），由 `ProgressSyncService` 在**写锁释放且事务成功之后**调用；禁止在 Store 层推送；禁止在失败/拒绝路径推送
  - [x] 4.2 `report`：若 `acked_total` 相对写前前进 → 对每个新确认步进或合并后的权威水位推 `delta`（见裁决 C）；若本轮刚确认完成 → 再推 `completion`
  - [x] 4.3 `submitSettings` 成功（含同 `action_id` 幂等返回前若状态相对上次推送有变化）→ `command_state`
  - [x] 4.4 `report` 使 `applied_revision` 追上导致「已生效」翻转 → `command_state`
  - [x] 4.5 `roundAction` 导致轮次/完成字段变化 → 按需 `completion` 和/或依赖后续 snapshot 查询；不发明新帧类型
  - [x] 4.6 无订阅者时推送为 no-op（不抛、不写盘）

- [x] Task 5：会话、心跳与入站（AC: #2 #3）
  - [x] 5.1 会话登记表：`device_id → WebSocketSession`（单活跃）；新连接替换旧连接
  - [x] 5.2 调度心跳（默认 30s）；收 `heartbeat_ack` 刷新截止；超时关闭
  - [x] 5.3 入站只接受 `heartbeat_ack`（及可忽略的未知 type 策略须 fail-closed：未知 type → `error`+关闭或静默丢弃，**推荐** `error` 帧 code=`40001` 后关闭）；不接受客户端「自报水位」控制帧
  - [x] 5.4 `contract_version` 不一致 → 拒绝该帧 / 关闭（契约 §3）

- [x] Task 6：测试与门禁（AC: #1–#3）
  - [x] 6.1 新增 `WebSocketPushContractTest`（或等价）：StandardWebSocketClient / Spring 测试客户端 + mock JWT；覆盖连接鉴权、report 后收到 `delta`、`seq` 与 snapshot 水位一致、设置下发收到 `command_state`
  - [x] 6.2 负例（RED→GREEN）至少 5 条：① 无 token / 坏 token 不能升级；② `seq` 与同刻 `snapshot_seq` 矛盾（delta.seq 落后或跳过已确认水位）；③ 拒绝路径（如 `20003`）不推送；④ 重连后仍推 `seq <=` 新查询冻结水位；⑤ 把 Bearer 头当作唯一鉴权而忽略 query `token`（应失败）
  - [x] 6.3 回归：既有 `mvn test` 全绿；`ProgressSync*` / `SettingsCommand*` / `StateSnapshot*` 行为不变
  - [x] 6.4 门禁：`cd cloud/backend && mvn -q test`、`python3 docs/contracts/tests/run_sync_contract_tests.py`、`git diff --check`
  - [x] 6.5 测试只写临时目录；不得创建/修改仓库工作区 `cloud/backend/data/`；不得改 `docs/contracts/**`；不得改 `cloud/frontend/**` / `Embedded/**`

- [x] Task 7：Story 文档与 receipt（AC: 全）
  - [x] 7.1 填写 Dev Agent Record
  - [x] 7.2 如实记录门禁边界与未证明面（真机 1s 端到端、小程序回放 UI、WSS 反代 idle 超时属部署/Epic 7）
  - [x] 7.3 写入 `.autopilot/5/5-5-实现-websocket-差量推送/.../dev-story.json`（由 dev-story 阶段）

## Dev Notes

### 真源与冲突裁决

| 主题 | 唯一依据与本 Story 裁决 |
| --- | --- |
| Story 需求 | `epics.md` Epic 5 / Story 5.5（FR-C-009、AD-17、NFR2、NFR3） |
| 通道与帧 | `ARCHITECTURE-SPINE.md` §AD-17/AD-20；`docs/contracts/sync-contract.md` §3、§8、§10、§10.1–10.3；`sync-contract.schema.json` `ws_frames` / `endpoint` / `ws_heartbeat` / `samples` |
| 快照水位 | Story 4.6 裁决 A：`snapshot_seq := progress.acked_total`（导出，backend 不落盘） |
| 产品行为 | `prd.md` §6.2.3 FR-C-009；§6.1.6 FR-C-006；SM-2/SM-3 |
| UX | `EXPERIENCE.md`：WebSocket 断开自动切查询/回放；本 Story **不**实现小程序 UI |
| 已有实现 | 4.6 快照；5.1 report；5.2 完成/跨轮；5.3 统计；5.4 设置命令 |

**关键真源结论**

1. **本 Story 是 cloud backend 第一次实现 backend→frontend 实时通道。** Epic 6 才消费；Epic 7 才做真机/WSS 联调。
2. **恢复真源是 REST 快照，不是 WS 重放缓冲区。** 禁止实现「服务端按客户端 cursor 重放历史 delta」。
3. **「当前字」不是 delta wire 字段。** 注册表 `delta.payload_fields` 只有四计数/游标字段；字面由 frontend+canonical 派生。
4. **序号必须与 4.6 导出的 `snapshot_seq` 对齐。** 不得另起持久化计数器（§11.1 backend 作用域无 `snapshot_seq`；第六文件被门禁拒绝）。
5. **注册表样例 `ws_snapshot` 中 `seq/snapshot_seq=512` 与 `acked_total=128` 的偏移是已知契约张力**（见 `deferred-work.md` 4.6 缺口）。本 Story **实现对齐 4.6 导出口径**，不把样例偏移当成义务；缺口继续 deferred，**不改契约字节**。
6. **鉴权：query `token`，不是 Bearer 头。** 与 REST 过滤器模型不同，必须在握手路径显式处理。
7. **NFR3/SM-3 的 1 秒端到端**含设备→backend→frontend；本 Story 只证明「确认后尽快推送」（同进程内 hook），不声称样机实测达标。

### 已有实现与必须保留的行为

- **`StateSnapshotService.exportSnapshotSeq` / `assemble`：** 只读；WS 一致性断言必须调用同一导出，禁止第二套水位算法。
- **`ProgressSyncService` + `deviceLocks`：** 推送在锁内计算「是否变化」，**发送 I/O 建议在锁外**，避免慢客户端阻塞写路径；不得在推送失败时回滚已确认落盘。
- **`JwtTokenProvider` / `IdentityStore`：** 复用 access 解析与单设备 subject 校验。
- **17 字段 `StateSnapshotResponse`：** 不扩展；WS `snapshot` 帧字段集按注册表 `ws_snapshot.required`（可与 REST 快照字段子集对齐，但**不要**把 REST 信封塞进 WS）。
- **`ErrorCode` 精确集合：** WS `error.code` 使用已有码的十进制字符串（如 `"40101"`），不新增常量。
- **既有门禁：** 契约 Python 套件、Persistence/ErrorCode/Snapshot/Progress/Settings 合同测试——不得放宽。

### 当前状态（UPDATE 文件必须先读）

| 文件 | 今日行为 | 本 Story 改变 | 必须保留 |
| --- | --- | --- | --- |
| `pom.xml` | 无 websocket starter | 增加 `spring-boot-starter-websocket` | Boot 3.3.7 / Java 17 / 零库 |
| `JwtAuthenticationFilter.java` | `/api/**` 要 Bearer | WS 握手特例走 query token | 其余 REST 语义；finally `UserContext.clear()` |
| `ProgressSyncService.java` | 写路径明确「不推 WebSocket」 | 成功后调用 publisher | 5.1–5.4 全部裁决与写序 |
| `SyncController.java` | 注释「仍不做 WS」 | 更新注释；**不**把 WS 塞进该 MVC 控制器 | 既有五条 REST |
| `StateSnapshotService.java` | 导出 snapshot_seq；注释交给 5.5 | 可被 publisher 只读调用；更新注释 | 只读、17 字段、commandApplied |
| （无）WS 配置/Handler | 不存在 | NEW | — |

### 必须作出的裁决（推荐项；实现须写入 javadoc）

**裁决 A：传输栈**
- **推荐：原生 WebSocket（`spring-boot-starter-websocket` + `TextWebSocketHandler`），不用 STOMP/SockJS。**
- 理由：契约帧是自描述 JSON + `type` 判别；小程序 `wx.connectSocket`；STOMP 会引入第二协议表面。
- 参考：https://docs.spring.io/spring-framework/reference/web/websocket/server.html

**裁决 B：`seq` / `snapshot_seq` 空间**
- **推荐：与 4.6 一致——业务进度空间 = `acked_total` 导出值。** `delta.seq` 与确认后 `snapshot_seq` 相等；客户端 `last_applied_seq` 初值取查询冻结水位。
- 会话内非进度帧用 `max(last_sent, snapshot_seq)+1`；重连不重放。
- 禁止：为「严格递增跨命令事件」新增落盘计数器。

**裁决 C：多敲击一次 report 的 delta 粒度**
- **推荐：每个成功确认且 `acked_total` 前进的 report，推送 1 帧 `delta`，载荷为确认后的权威水位/游标（不是逐物理敲击拆帧）。**
- 理由：设备上报的是高水位而非逐事件流；NFR2 的「不丢字」由水位差 + canonical 游标保证；逐事件拆帧无设备事件源。
- 若一次前进 Δ>1：仍 1 帧承载新水位（frontend 按游标/回放策略追平——Epic 6）；**不要**在 backend 伪造中间字事件。

**裁决 D：路径与鉴权**
- **推荐：endpoint = `/api/v1/ws`；query `token`；握手失败不建立会话。**
- Filter：仅放行该 path 的升级握手，不把普通 GET/POST 公开化。

**裁决 E：推送时机与失败策略**
- **推荐：落盘成功后、响应返回前或后均可，但必须在权威状态已可读之后；推送异常只打中文日志，不影响 HTTP 成功确认。**
- 无连接 = no-op。

**裁决 F：单设备单连接**
- **推荐：新连接建立时关闭同 `device_id` 旧连接（1000 或业务段码，javadoc 写明）。**
- 理由：契约「单一活跃 socket」；双连接会导致双推与序扰乱。

**裁决 G：心跳**
- **推荐：默认 interval 30000 / timeout 10000（契约默认，均低于测试上界）；只认 `heartbeat_ack`。**
- 生产反代 idle 超时不在本 Story（architecture Deferred）。

### 明确不做

- Story 5.6：广义 REST 错误语义扩面、401 单飞（属 frontend）——**已由 5.6 交付**；Epic 6 只接壳层与页面。
- Story 5.7：重启原子恢复专项验收
- Epic 6：小程序 WS 客户端、回放排队 UI、`replay_cursor` 持久化消费
- Epic 7 / 7.3：真机 WSS、端到端 1s、反代
- 改 `docs/contracts/**`、新增 `ErrorCode`、第六状态文件、数据库、STOMP
- 服务端历史帧回放缓冲；客户端自报展示计数权威化
- 在 delta 中下发经文字符面值；充电/BLE/Wi-Fi 业务链路

### Git 与并发工作区纪律

基线 HEAD：`23216215b4342da96faf2bd4fc5a63b784e97948`。工作树可能含 5.1–5.4 已交付未提交文件与 Embedded 并行改动。**禁止** reset/checkout/stash/commit/branch/merge；只改本 Story 文件列表内路径，建立在当前工作树 5.4 之上。

## Architecture Compliance

- **AD-2**：只推 backend 已确认事实；不推本地未确认差。
- **AD-17**：WebSocket + 查询冻结水位补齐；序号空洞先查询；完成仅确认后呈现（completion 帧）。
- **AD-20**：WS origin/path 由契约从 `VITE_API_BASE_URL` 派生，不在 env 再拼一套。
- **AD-16**：零库；不新增状态文件承载 seq。
- **NFR2 / SM-2**：不跳字/重字——靠单调 seq + 与 snapshot 一致，而非动画层。
- **NFR3 / SM-3**：本 Story 提供低延迟推送钩子；端到端 1s 留 Epic 7 实测。

## Library / Framework Requirements

- Java 17、Spring Boot **3.3.7**
- **新增：** `org.springframework.boot:spring-boot-starter-websocket`（BOM 管理版本）
- **不新增：** Spring Security（项目无）、STOMP broker、外部 MQ
- API 文档：Spring Framework 6.1 WebSocket —— https://docs.spring.io/spring-framework/reference/web/websocket/server.html
- 握手期 query token 模式与小程序限制一致（契约已定；注意 URL 日志脱敏，勿打印完整 token）

## Project Structure Notes

```text
cloud/backend/
  pom.xml                                              # UPDATE：websocket starter
  src/main/java/top/zhcmqtt/ewf/backend/
    common/security/JwtAuthenticationFilter.java       # UPDATE：WS 握手放行
    common/config/WebSocketConfig.java                 # NEW
    ws/SyncWebSocketHandler.java                       # NEW：入站/心跳/会话
    ws/WsHandshakeInterceptor.java                     # NEW：token 校验
    ws/SyncRealtimePublisher.java                      # NEW：组帧 + 按 device 推送
    ws/WsFrameFactory.java                             # NEW（或内联）：帧 JSON
    service/ProgressSyncService.java                   # UPDATE：成功后 publish
    service/StateSnapshotService.java                  # UPDATE：注释
    controller/SyncController.java                     # UPDATE：注释
  src/test/java/top/zhcmqtt/ewf/backend/
    WebSocketPushContractTest.java                     # NEW
    WebSocketPushServiceTest.java                      # NEW（可选单元）
_bmad-output/implementation-artifacts/deferred-work.md # UPDATE：缺口节
```

## Testing Requirements

最低门禁：

1. `cd cloud/backend && mvn -q test`
2. `python3 docs/contracts/tests/run_sync_contract_tests.py`
3. `git diff --check`
4. `git status --short` —— 未新增 `cloud/backend/data/**`、未改 `docs/contracts/**`、未改 frontend/Embedded

必测行为：

- 合法 `token` 升级成功；非法/缺失拒绝
- report 推进后收到 `delta`，`seq ==` 事后 `assemble().snapshot_seq()`
- 同水位二次 report（no-op）不产生前进 seq 的新 delta（或可观测为无新业务帧）
- 设置下发 → `command_state` 含 volume/brightness/timeout/修订对
- 完成确认 → `completion` 与快照 `round_state/pending_completion` 一致
- 拒绝码路径不推送
- 双连接时仅新连接收后续推送

门禁边界（不得写成已验证）：小程序回放 UI；WSS/反代；样机 SM-3 1s；跨 JVM 会话；历史帧重放。

## Previous Story Intelligence

### 直接前序：Story 5.4（done）

- 交付：`POST /sync/command`、`submitSettings`、`20006` CAS、与 report 的 applied 收敛。
- **明确交接：** WebSocket `command_state` 与推送时机属 5.5。
- **教训：** 契约不改字节；REST/WS 新表面登记 deferred；写路径与查询职责分离；共用 `deviceLocks`；负例必须真红。
- **回归风险：** 在 `submitSettings`/`report` 加 hook 时不要打乱写序或把推送失败升级为业务失败。

### Story 5.1–5.3 / 4.6

- 4.6：`snapshot_seq` 导出口径 + 已登记「WS seq 对齐」缺口——本 Story 必须闭合实现面对齐，契约措辞仍 deferred。
- 5.1：确认差量与 applied 镜像；推送以确认成功为前提。
- 5.2：completion 语义；完成锁定。
- 5.3：统计只读，**不要**在统计路径推 WS。

## Git Intelligence Summary

- HEAD：`2321621`（Embedded 反馈服务）——与 cloud WS 无直接耦合；本 Story 只改 `cloud/backend`（+ deferred-work）。
- 近期 cloud 5.1–5.4 多在工作树未提交交付上；实现基于工作树，禁止 git 清理并行改动。

## Latest Technical Information

- Spring Framework 6.1 / Boot 3.3.7：`WebSocketConfigurer` + `HandshakeInterceptor` 校验 handshake；原生 handler 而非 STOMP，除非产品改契约。
- 微信小程序：自定义头不可用 → 契约已选 query `token`；注意代理/日志中的 query 脱敏。
- 心跳参数以上界门禁为准：实现默认必须 ≤ 测试上界（45s / 15s）。

## Project Context Reference

- 遵守 `AGENTS.md` / `CLAUDE.md` / `cloud/AGENTS.md`：零库、信封（REST）、WS 单调序号、查询水位恢复、UTF-8、无最终页面文案交付。
- 无独立 `project-context.md`；规划产物 + 契约 + 前序 story 即为 persistent facts。
- UX：断线切查询/回放由 Epic 6 实现；本 Story 保证后端通道与帧语义可被 mock 验证。

### 契约缺口登记（写入 deferred-work.md）

1. **注册表样例 `ws_snapshot` 的 `seq/snapshot_seq` 与 `acked_total` 偏移** vs 4.6/5.5 导出对齐口径——需契约层统一样例或改措辞；本 Story 不改契约字节。
2. **`snapshot_seq`「单调递增」 vs `acked_total`「单调不减」措辞**（4.6 已登记）——仍开放。
3. **非进度帧（command_state/completion/heartbeat）的 seq 是否允许偏离敲击空间、重连是否需重放**——本 Story 取「会话单调 + 重连靠 REST」；若产品要强重放需先改契约。
4. **WS 关闭码业务段取值表未逐事件冻结**——实现选用并文档化，正式码表可后续补进契约。
5. **生产 WSS 反代 idle 超时**——architecture Deferred，非本 Story。

## Dev Agent Record

### Agent Model Used

Composer (Cursor Agent) — Epic Autopilot unattended best-effort

### Debug Log References

- `/tmp/ewf-ws-full-test.log` — `mvn test`：Tests run: 157, Failures: 0, Errors: 0
- `/tmp/ewf-ws-contract.log` — `python3 docs/contracts/tests/run_sync_contract_tests.py`：全部门禁通过
- `git diff --check`：通过（exit 0）

### Completion Notes List

- 交付原生 WebSocket（`/api/v1/ws`）：query `token` 握手、单设备单连接、心跳 30s/10s、确认后 `delta`/`completion`/`command_state` 推送。
- 裁决 A–G 已落入 `WebSocketConfig` / `WsHandshakeInterceptor` / `SyncWebSocketHandler` / `WsFrameFactory` / `SyncRealtimePublisher` / `ProgressSyncService` javadoc。
- `delta.seq := snapshot_seq := acked_total`（4.6 导出口径）；非进度帧会话单调；无历史帧缓冲；拒绝路径不推送。
- 契约缺口已写入 `deferred-work.md`（create-story of 5-5）。
- 门禁边界（未证明）：真机 1s 端到端、小程序回放 UI、WSS 反代 idle——属 Epic 6/7。
- 未改 `docs/contracts/**`、`Embedded/**`、`cloud/frontend/**`；未新增 ErrorCode / 第六状态文件。
- **交接更正（2026-09-23，Story 5.6）：** 「5.6：广义 REST 错误语义扩面、401 单飞」已交付；Epic 6 只接壳层与页面，消费统一 `api/request`。

### File List

- cloud/backend/pom.xml
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/config/WebSocketConfig.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/security/JwtAuthenticationFilter.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/controller/SyncController.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/ProgressSyncService.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/service/StateSnapshotService.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/ws/RealtimePushPort.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/ws/SyncRealtimePublisher.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/ws/SyncWebSocketHandler.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/ws/WsFrameFactory.java
- cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/ws/WsHandshakeInterceptor.java
- cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ProgressSyncServiceTest.java
- cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/WebSocketPushContractTest.java
- _bmad-output/implementation-artifacts/deferred-work.md

### Change Log

- 2026-09-23：实现 Story 5.5 WebSocket 差量推送（原生帧、推送钩子、契约测试）；状态 → review。
- 2026-09-23：code-review autofix — 会话序号分配与发送原子化；补双连接契约测试；状态 → done。
- 2026-09-23：文档交接更正——401 单飞与 request 层由 Story 5.6 交付。

### Review Findings

- [x] [Review][Patch] 会话序号分配与发送非原子，心跳与推送可交错重复 seq [SyncWebSocketHandler.java / SyncRealtimePublisher.java] — 已用 `sendWithNextSessionSeq` 在同一 `sendLock` 下分配并发送。
- [x] [Review][Patch] 缺少「双连接仅新连接收后续推送」契约测试 [WebSocketPushContractTest.java] — 已补 `双连接仅新连接收推送`。
- [x] [Review][Defer] 心跳 interval/timeout 路径无自动化覆盖 [SyncWebSocketHandler.java:heartbeatSweep] — deferred: 默认 30s/10s 与契约一致；真机时序与反代 idle 属 Epic 6/7，本 Story 不引入可注入时钟。
- [x] [Review][Defer] `report` 导致 `applied_revision` 追上时的 `command_state` 推送缺专门 WS 断言 [ProgressSyncService.java / WebSocketPushContractTest.java] — deferred: 设置下发 `command_state` 与拒绝路径已覆盖；applied 翻转路径实现存在，补测属增强。

#### Rejected

- `false`：Origin 仅 localhost 会导致小程序无法连 — Spring 对缺失 Origin 放行；生产 WSS/反代属 Epic 7 deferred，非本缺陷。
- `false`：delta/snapshot seq 与样例 512/128 偏移不符 — 故事裁决对齐 4.6 导出口径，缺口已在 create-story deferred，不改契约字节。

---
