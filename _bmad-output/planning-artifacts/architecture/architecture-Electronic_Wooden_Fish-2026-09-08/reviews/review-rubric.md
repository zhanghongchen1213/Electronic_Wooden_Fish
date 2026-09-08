---
title: "Rubric 评审：ARCHITECTURE-SPINE"
reviewer: rubric-walker（good-spine checklist）
target: "architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
date: 2026-09-08
status: draft
---

# Architecture Spine — Rubric 逐项评审

评审对象：`_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`（18 条 AD）。
证据基线：09-08 简报/附录、09-07 PRD（+reconcile-brief-2026-09-08）、根 architecture.md、legbot_watch 实际仓库（dependencies.lock / components/BSP / managed_components / sdkconfig）、main_control（components/BSP/GPS）、miaowu（backend/pom.xml、ApiResponse、frontend/package.json、env-scripts），以及 web（Espressif ESP-IDF v6.0 / v5.5.5；Spring Boot EOL）。

---

## 1. 是否修对「下一级会真实发散」的点、且无遗漏

**判定：大体修对（18 条 AD 覆盖了绝大多数真发散点），但有 1 个核心契约洞 + 1 个构建期漂移点漏网。**

命中（修对的点）：
- 双正式输入源（`physical_pvdf`/`device_touch`）共享单队列、无第二计数路径 → AD-1。这是旧根文档没有显式固化的真正发散点，修对了。
- 设备本地记录 vs backend 权威进度 → AD-2/AD-3（幂等高水位、无逐事件重放）。修对。
- 命令单调修订 + 离线「待设备应用」→ AD-5。修对。
- 可信时间门禁（硬断电「待校时」）→ AD-6。修对。
- 后端持久化零库 JSON 覆盖旧 SQLite → AD-16（override 语义、header 注明）。修对。
- 前端「不产生正式输入 / 只消费已确认差量」→ AD-17。修对。

遗漏（下一级会真实发散但未被 AD 钉死）：

- **[HIGH-1] 离线轮次复位（完成→「从头开始」）与 backend 权威游标的对账规则缺位。** spine 的同步模型（AD-3「只对高于已确认高水位的差值幂等推进一次」）以 *local_total 单调递增* 为前提，而 `round_cursor`/`round_state` 是**轮次相对量**：设备离线完成《心经》→ 本地「从头开始」→ 新轮次游标归零 → 继续敲击 → 下次上报时 `local_total` 已跨越轮次边界。backend 若按总差值推进游标，会把旧轮次剩余字符与新轮次字符混算，完成锁定/当前字与设备实际显示分裂；同步字段清单（§Structural Seed）也没有轮次身份（round generation/序号）或显式 round-reset 事件可让 backend 判定差值归属。FR-B-005/addendum §5.5 都允许设备在离线完成态下「从头开始」，所以这是真实可达场景。位置：AD-2 + AD-3 Rule + §Structural Seed 字段清单（无 round 代际字段）。
- **[HIGH-2] canonical 经文文本没有「单一来源 + 构建期一致性」约束。** AD-4 只规定「三端一致、不一致停止推进并返回配置错误」——这是**运行期**版本门禁，不是**构建期**漂移预防。260 字《心经》+ 标点/占位/是否消费敲击的标注需要三端字节一致；若 Embedded / backend / frontend 各自誊写一份文本资源，几乎必然漂移，且要到运行时同步才暴露（届时产品已停在配置错误）。spine 没有钉死 canonical 文本的单一落盘位置（如 `cloud/backend/src/main/resources/scripture/heart-sutra.json`）+ 生成/复制 + 内容哈希比对。位置：AD-4 Rule、§Structural Seed（无经文文本资源位置）。

- [LOW] 设备端「今日」在**跨午夜且长期无网**时的切分口径未规定。AD-6 只覆盖「硬断电后未校时 → 待校时」；设备不保存绝对时间（AD-6/AD-15），离线跨过自然日时设备无时间源判断“今天”已切换，统计页「今日」会沿用旧日期。位置：AD-6。

## 2. 每条 AD 的 Rule 可执行性、是否防住其声称的 divergence、Binds/Prevents/Rule 齐备性

**判定：18 条 AD 的 Binds/Prevents/Rule 三段全部齐备，Rule 绝大多数可执行；2 条 Rule 的防发散强度不足。**

- 全部 18 条均有 Binds / Prevents / Rule 三段，无缺段（逐条核对：AD-1~18 均完整）。
- 可执行性抽查：
  - AD-1/AD-5/AD-7/AD-9/AD-11/AD-13/AD-15 的 Rule 均为可判定的工程约束（“任何代码路径不得…”“只有 BSP 层可定义 GPIO…”“不上 CH340X”“充电期间暂停…”）。可执行。
  - AD-3 Rule 给出 1000 上限、幂等 no-op、SM-3 95%/1s 端到端口径，可执行。
  - AD-16 Rule 具体到目录 `cloud/backend/data/`、原子写三步（临时文件 + fsync + rename）、信封 `{code,message,data}` code=0。可执行且已被本地基线证实（见 §4/§5）。
- 强度不足的两条：
  - **AD-4**：见 HIGH-2。Prevents 声称防「版本漂移导致游标错位」，但 Rule 实际只防 *运行期不一致继续推进*，不防 *构建期三端文本字节漂移*（漂移会在运行时以“配置错误停推”方式爆发，属晚发现）。
  - **AD-3**：见 HIGH-1。Rule 声称幂等防重/防乱序，但没定义「差值跨轮次边界（离线复位）」时 backend 如何保持游标权威；Prevents 未覆盖轮次归属分裂。

## 3. Deferred 里是否有某项会让两个独立单元发散（本不该延后）

**判定：有且仅有一项真正跨两个独立单元——backend→frontend 实时通道（A-1）。其余各项均不跨单元或可正确下沉。**

- **[HIGH-3] A-1（WebSocket vs 短轮询/SSE）**：backend 推送侧（FR-B-006）与 frontend 消费侧（FR-F-009）是两个可被独立 lane 实现的单元；通道类型、端点、消息信封（增量/心跳/重连补齐游标）若未事先一致，两侧会各写各的。缓解项：AD-17 把增量/去重语义做成通道无关、容器图/序列图默认画 WS、A-1 已声明「finalize 前与用户确认」。但仍建议：在拆分 backend/frontend 故事前，至少在 spine 里钉死 *fallback 默认值（WS 主、短轮询备）与差量消息的最小形状*，或直接标 decided。位置：Deferred 表 A-1 行 + AD-17。
- 其余 Deferred 项核对：
  - A-2（JSON 文件布局/原子写细节）→ 单进程 backend 内部，不跨单元；schema 权威下沉 `docs/contracts/sync-contract.md` 属正确分层（字段清单本身已在 spine 钉死）。
  - A-3/A-4/A-5/A-6/A-7 → 分别是 IDF 档位、Spring Boot 版本、登录映射、端口脚本、域名备案，均为单单元或运行期配置，可延后。
  - 硬件细分/表现参数/非目标行 → 本就不该进 spine 承诺。合理。

## 4. 命名/技术是否已现实核对（本地基线 + web）

**判定：命名与版本均与现实核对一致，仅 1 处 deferral 目标随日期过期。**

本地基线核对（全部命中）：
- legbot_watch：`dependencies.lock` 确认 `idf: 5.5.4`、`lvgl/lvgl 8.4.0`；`components/BSP/` 确有 CO5300/CST9217/CW2015/ES8311/QMI8658C/NS4150；`managed_components` 确有 `esp_lcd_co5300`/`waveshare__esp_lcd_touch_cst9217`/`esp_lvgl_port`。→ spine「沿用 legbot BSP/managed_components、LVGL 8.4、≥5.5.4,<5.6.0」准确。
- main_control：`components/BSP/GPS` 存在，`gps.c` 为 Air780E AT 固件 + `Air780EGP HTTPS SSL 上下文`；`sdkconfig.old` 为 ESP-IDF 5.1.5。→ spine「复用 main_control Air780EGP AT/HTTPS/退避经验；不沿用 v5.1.5」准确。
- miaowu：`backend/pom.xml` = spring-boot-starter-parent 3.3.7 + java 17；`ApiResponse` = `{code,message,data}`，code=0 成功（注释明确）；`WechatMiniClient`/`JwtTokenProvider` 存在；`frontend/package.json` = uni-app `@dcloudio/uni-*` + vue ^3.5 + vite ^5.4 + pinia ^2.3 + TS；`env-scripts/` 有 `start-local-test-backend.sh`、`build-prod-backend.sh`。→ spine AD-16/18 与 Stack 表全部有据。
- legbot `cloud/backend/data/devices-local.json` + cloud README「JSON 存储只支持一个后端实例，不要启动多副本共同写同一个文件」→ AD-16 零库单实例 JSON 原子落盘“参照 legbot cloud”有据。

web 复核（2026-09-08 时点）：
- Espressif：v6.0 于 2026-03-27 发布为现行 major；v5.5 自 2026-07 进入 maintenance、最新补丁 5.5.5。→ spine/memlog 的 A-3 与 Stack 表描述一致。
- Spring Boot：3.3.x OSS EOL 2025-06-30（正确，spine 已注）；**但 3.5.x OSS 也已于 2026-06-30 EOL，2026-09 时点全部 3.x 均无 OSS 支持，当前受支持线只有 4.0/4.1。**
- **[LOW] Deferred A-4 行「升级受支持线（3.5.x/4.x）」过期**：3.5.x 不再是受支持线。spine 对 3.3.7 EOL 的判断正确、升级意图正确，但“受支持线”括注应按文档日期（2026-09-08）改为「4.0/4.1」。位置：Deferred 表 A-4 行。
- LVGL 8.4 锁档不升 9：由 legbot 实测 8.4.0 支撑，无需 web。

## 5. 是否「认可而非违背」brownfield（legbot_watch 嵌入式范式、miaowu 软件骨架为本地参照）

**判定：认可而非违背，且“不沿用”清单有本地证据支撑。**

- 认可：AD-7/8/9/10 显式“沿用 legbot_watch 范式”（服务任务事件驱动、BSP 独占、`device_state` 事实源、共享 I²C/UI 单任务独占）；AD-16/18 显式“沿用 miaowu 骨架”（pom 基线、`/api/v1` + `{code,message,data}`、WechatMiniClient/JWT、uni-app 仅 mp-weixin、env-scripts）。§Structural Seed「嵌入式复用与不沿用」把可复用与不可照搬分列。
- 不沿用均有据：legbot 引脚常量（legbot 实为 GPIO43/44 给 ML307R，spine 改为 Air780EGP UART）、ML307R/BLE/外骨骼业务、QMI8658C INT2→GPIO45 合同（spine 明确 GPIO45 不接）、main_control v5.1.5 约束与景区 payload。
- 未发现“嘴上沿用、实际相悖”：信封 code=0（与 miaowu ApiResponse 一致而非臆造）、IDF 档位（与 legbot 5.5.4 锁档一致）、LVGL 8.4（与 legbot 一致）、持久化零库参照 legbot cloud（其确为 JSON 单文件）。

## 6. altitude 拥有维度是否有整条沉默（尤其 operational/environmental 信封）

**判定：无一维整条沉默（都在 decided/deferred/open 三选一内），但部署/运维信封最薄、且“本地 dev 脚本”与“运行期公网信封”被混为一谈。**

逐维核对：
- 数据持久化/infra：decided（零库 JSON 单实例，AD-16）。
- 实时通道/部署形态（BE→FE 推送）：deferred（A-1，见 HIGH-3）。
- 小程序合法域名/备案：deferred（A-7）。
- CI/CD、云运维、宝塔：open/显式不做（Deferred 末行）。
- **backend 运行期公网信封：未明说。** [MEDIUM] Deferred 末行「部署与运维信封：个人原型沿用 miaowu env-scripts 本地/构建脚本即可」只回答了*本地开发/构建*。但本产品自己的核心旅程与 SM-3（4G 设备 + 微信小程序 + 95%/1s）要求一个**公网可达的 HTTPS/wss 端点**（TLS 证书、备案域名、微信后台 request/合法域名 + IP 白名单，外加 PRD 已假设的作者自备 SIM/资费）。spine 未以「decided/deferred/open」任一形式点出“运行期需一台公网主机 + 域名/TLS”，而把它折进 A-7 的域名备案里。位置：Deferred 末行 + A-7 行。

## 7. 覆盖产品输入（09-08 简报/附录、PRD FR-E/B/F）的能力地图是否完整自洽

**判定：FR 群在内容上基本被 AD 覆盖，但「Capability → Architecture Map」表本身不自洽——按 FR 群到 AD 子集的映射会漏掉跨层 AD，追溯会误导下游。**

- **[MEDIUM] 能力地图映射缺漏/错位**（§Capability → Architecture Map）：
  1. FR-E 行只给 AD-7~15，但 FR-E-002/003（统一输入）的治理 AD 是 AD-1，FR-E-004/008 是 AD-3，FR-E-006 是 AD-6——这些跨层 AD 在表里被归到末行（SM-1~5）而末行又不带 FR 标识，导致按 FR 查 AD 会漏掉最关键的 AD-1。
  2. FR-B 行给 AD-2,3,5,6,16，漏了 AD-4——而 AD-4 的 Binds 明写 FR-B-004。
  3. FR-F-009/FR-B-006（推送/断线补齐）实际由 AD-17 + deferred A-1 治理，地图只落到 frontend 行 AD-17，backend 行未反映。
  建议：地图改成 FR→AD 的精确多对多（至少把 AD-1~6 中 bind 到具体 FR 的项补进对应 FR 群），或加一列「跨层 AD-1~6」。位置：§Capability → Architecture Map 整表。
- FR 群内容覆盖核对（不含映射表）：FR-E-001~010、FR-B-001~009、FR-F-001~010 在 §Invariants & Rules + Deferred 中均有归属（FR-B-001/FR-F-001 登录类由 deferred A-5 覆盖，属可接受延后）。09-08 附录 §3.1 GPIO 基线、I²C/电源/USB、同步时序 seed 均已并入 spine（§Structural Seed），信息未丢失。

## 8. spine 内部一致性

**判定：通过。未发现模板残留、占位符、编号断裂或 mermaid 非法。**

- frontmatter：name/type/purpose/altitude/paradigm/scope/status/sources/companions 齐备，与正文一致；`binds` 覆盖三层 FR 前缀。
- AD 编号：AD-1~AD-18 唯一、严格递增、无跳号重号。
- 模板残留/占位符：逐段扫读无 `TODO`/`TBD`/`xxx`/模板注释；`[ASSUMPTION A-n]`（A-1~A-7）在正文引用与 Deferred 表一一对应（A-2/A-3/A-4 另在 AD-16/Stack 引用，均能落表）。
- mermaid：3 个图块（设计范式 flowchart、系统容器 flowchart、活动窗口 sequenceDiagram）结构合法——subgraph 缩进正确、跨 subgraph 边合法、节点 label 引号配对、中文字符与 `+`/`/`/`（）`均在引号内；未发现语法错误。
- 正文一致性：header 的 override 声明（AD-16 覆盖 FR-B-009/旧 SQLite）与正文、memlog 一致；同步字段清单在两处（Consistency + Structural Seed）字符级一致。

---

## 发现汇总

| 级别 | ID | 一条结论 | 位置 |
| --- | --- | --- | --- |
| Critical | CRIT-1 | 离线轮次复位（完成→从头开始）跨越幂等高水位边界，backend 无轮次身份字段可判定差值归属，两端游标/完成锁会分裂 | AD-3 Rule、AD-2、§Structural Seed 字段清单 |
| High | HIGH-1 | canonical 经文文本无单一来源/构建期一致性约束，三端誊写必然漂移且运行期才暴露 | AD-4 Rule、§Structural Seed |
| High | HIGH-2 | A-1 实时通道（WS vs 轮询）是唯一跨 backend/frontend 两独立单元的 Deferred，未钉 fallback 与消息信封 | Deferred A-1、AD-17 |
| Medium | MED-1 | Capability→Architecture Map 按 FR 群映射会漏跨层 AD（FR-E 漏 AD-1/3/6，FR-B 漏 AD-4） | §Capability → Architecture Map |
| Medium | MED-2 | 部署/运维信封最薄，且本地 dev 脚本与运行期公网 HTTPS/wss 信封混为一谈；backend 运行宿主/域名/TLS 未三选一 | Deferred 末行、A-7 |
| Low | LOW-1 | A-4「受支持线（3.5.x/4.x）」过期：2026-09 全部 3.x 已 OSS EOL，受支持线仅 4.0/4.1 | Deferred A-4 |
| Low | LOW-2 | 设备「今日」跨午夜长期无网时无切分口径（无 RTC/绝对时间） | AD-6 |

## 结论

**整体：一条高质量 spine（18 AD 三段齐备、brownfield 现实核对扎实、override 显式、fast-path 假设已标注），作为 build-substrate 可信；但有一个会直接造成 Embedded/backend 两单元分裂的核心契约洞（离线轮次复位对账）必须先补，才能拆 lane。**
