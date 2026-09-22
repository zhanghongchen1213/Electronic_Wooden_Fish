# 跨层同步契约

**contract_version**：`SC-1.0.0`

本文件是电子木鱼跨层同步的**唯一协议真源**。统一字段、WebSocket 帧自有字段、事件来源
枚举、状态词表、序号与轮次边界、恢复与冲突路径、命令修订语义、心跳与重连参数、JSON
持久化文件粒度以及同步域错误语义，**都只在本文各定义一次**。
`docs/contracts/sync-contract.schema.json` 是本文的机器可读派生注册表，与本文逐值一致；
门禁脚本 `docs/contracts/tests/run_sync_contract_tests.py` 负责断言这一致性。

## 1. 权威范围与承接关系

| 事项 | 真源 | 本契约的关系 |
| --- | --- | --- |
| 产品行为与 FR-C 需求 | `prd.md` §4 / §6.1 / §7 | 字段与轮次、恢复不变量的上游来源；本契约不新增产品状态、入口、金额或角色 |
| 架构不变量 | `ARCHITECTURE-SPINE.md` AD-1/2/3/5/6/16/17/19/20 | 承接 AD-17/AD-20：帧格式、心跳、重连参数与 WebSocket origin/path 派生由本契约给出 |
| 经文版本与消费语义 | `docs/contracts/canonical/heart-sutra.manifest.json`、`docs/contracts/canonical/README.md` | 只引用 `scripture_version` 与 `counts.consumable_han`，不重定义取值与消费序列 |
| 状态显示词 | `ARCHITECTURE-SPINE.md` §Consistency Conventions、UX 四份规范 | 只登记 wire 名与冻结中文显示词的映射，不重定义视觉与页面闭包 |
| 接口信封与错误码 | `AGENTS.md` §3.6、`cloud/AGENTS.md` §3 | 按同步域列出拒绝条件与业务码；实现层不得重定义码值 |

### 1.1 单一真源规则

- 字段表、帧自有字段、命令载荷字段、事件来源枚举、状态词表、心跳与重连参数只在本文定义；
  PRD、`ARCHITECTURE-SPINE.md`、`docs/embedded/`、`docs/backend/`、`docs/frontend/` 与 UX 规范
  只引用，不复制。
- 本文与 `sync-contract.schema.json` 出现分歧即为契约错误，门禁逐值比对，不静默取其一。
  门禁实际逐值比对的面为：§2 字段表（类型、单位、所有者、权威、取值域、承载通道、出处）、
  §3 帧自有字段表（类型、单位、角色、取值域、承载通道）、§5 三个状态词表的 wire 名、显示词
  与含义、§6.1 上报必填清单、§6.2 响应必填清单、§9.1 命令载荷字段表、§10.1 帧类型表的帧名、
  方向、内容与出处、§10.3 参数表的默认值、测试上界与单位、§11 文件表的文件名、承载事实、
  `schema_version`、字段与迁移说明以及 §11.1 持久化作用域表、§12 拒绝条件表的条件、HTTP 状态
  与业务码；承载通道的四个取值（HTTPS 上报、HTTPS 响应、WS 帧、持久化文件）各自与本文对应
  清单双向闭合；`samples` 的字段声明、帧样例的 `contract_version` 与枚举型取值必须落在本文
  声明的取值域内；并对注册表 `endpoint`（含 `scheme_map` 与 `token_transport`）、
  `close_codes`、`ws_heartbeat`、`reconnect`（含 `strategy` 与 `hole_rule`）、
  `offline_backlog`（含达上限后的排空路径）的语义键与正文取值做一致性断言。
- 门禁**不参与正文↔注册表逐值比对**的说明文字面（须人工核对）：§1.2 版本化与演进、§4 事件来源
  与累计语义的散列行、§6 活动窗口中除两份必填清单行外的说明文字、§7 轮次裁决正文、§8 恢复
  路径表的语义列、§10.2 Endpoint 派生规则的说明文字，以及注册表顶层 `authority` 与
  `ws_heartbeat.note` 两个自由叙述键。
- 门禁**不能**机械判定的部分（须人工核对，不得据门禁 PASS 推断已一致）：仓库内其它文档
  （未点名的 `docs/embedded/`、`Embedded/AGENTS.md`、UX 规范等）是否复制了字段表、事件来源枚举
  或 WebSocket 参数——门禁只能检查被点名的指针文档与 spine，不能证明全仓无第二份副本；
  `automatic_tap` 不得新增独立 cloud 字段或接口这一禁止项只能通过「字段名模式」与「本文必须
  保留该禁止语句」间接约束，无法证明实现层不存在额外接口；将来时扫描只能覆盖已登记的表述族，
  不能证明指针文档的每一句话都不是将来时。
- 本文不定义 `scripture_version` 的取值，只要求它等于 canonical manifest 的登记值；
  不一致属配置错误，必须停止推进（FR-C-002）。
- 本文不把视口槽位常量或经文长度写成任何字段的上界；`round_cursor` 的取值域引用
  canonical manifest，不硬编码数字。

### 1.2 版本化与演进

- `contract_version` 格式固定为 `SC-<major>.<minor>.<patch>`；当前登记值见文首，并同步
  登记在 `docs/contracts/README.md`。
- 帧参数、心跳、重连、关闭码与业务码的任何变更都必须递增 `contract_version`，且先更新
  本文与注册表，再修改实现（`cloud/AGENTS.md` §6）。
- 生产反向代理的空闲超时尚未冻结，属部署阶段事项：心跳与退避默认值可在此后经
  `contract_version` 递增调整，本文不把部署参数写为已验证事实。

## 2. 统一字段表

字段集合 = PRD §7 的统一字段 + `action_id` + `applied_revision` + `last_applied_seq`
（`action_id` 与 `applied_revision` 由 PRD §7 正文点名；`last_applied_seq` 是 PRD §7 正文的
序号丢弃基准，PRD 使用它但未定义其所有者与落点，本契约在此定义一次）。承载通道的取值只有
四种：`HTTPS 上报`、`HTTPS 响应`、`WS 帧`、`持久化文件`。承载通道与 §6.1、§6.2、§10.1、
§11、§11.1 的各清单互为约束：通道声明的方向必须有对应清单承载，清单列出的字段也必须声明
对应通道。

| 字段 | 类型 | 单位 | 所有者 | 权威 | 单调性与取值域 | 承载通道 | 出处 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `device_id` | string | 无 | backend | backend | 不透明标识；单身份单设备，不提供绑定、迁移与多设备 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 |
| `scripture_version` | string | 无 | canonical | canonical manifest | 必须等于 canonical manifest 的 `scripture_version`；不一致即配置错误并停止推进 | HTTPS 上报、持久化文件 | PRD §7 |
| `local_total` | integer ≥ 0 | 无 | 设备 | 设备本地事实 | 单调不减；设备重置除外 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 |
| `acked_total` | integer ≥ 0 | 无 | backend | backend | 单调不减；同值或更低的提交为幂等 no-op | HTTPS 上报、HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `round_id` | integer ≥ 1 | 无 | 设备 | backend 确认 | 严格递增；跨轮归属的唯一分段依据 | HTTPS 上报、HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `round_state` | enum(in_progress\|completed) | 无 | 设备 | backend 确认 | 仅两值；完成只由完成确认翻转 | HTTPS 上报、HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `round_cursor` | integer ≥ 0 | 无 | 设备 | backend 确认 | 取值域引用 canonical manifest 的 `counts.consumable_han`；backend 不按经文长度盲推 | HTTPS 上报、HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `pending_completion` | boolean | 无 | 设备 | backend 确认 | 置位期间不得创建新轮次；完成确认后清除 | HTTPS 上报、HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `command_revision` | integer ≥ 0 | 无 | backend | backend | 单调递增；backend 只保留并下发最新修订 | HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `applied_revision` | integer ≥ 0 | 无 | 设备 | 设备上报 | 单调不减；`applied_revision` ≥ `command_revision` 即已生效 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 正文 |
| `snapshot_seq` | integer ≥ 0 | 无 | backend | backend | 单调递增；在查询响应内冻结，是续订与回放的唯一基准 | HTTPS 响应、WS 帧、持久化文件 | PRD §7 |
| `replay_cursor` | integer ≥ 0 | 无 | frontend | frontend 持久化 | 单调不减；旧轮次事件不得写入当前篇章 | 持久化文件 | PRD §7 |
| `last_applied_seq` | integer ≥ 0 | 无 | frontend | frontend 持久化 | 单调不减；初值取自最近一次查询响应的快照水位，此后随被消费的帧推进；丢弃 `seq` 不大于它的帧 | 持久化文件 | PRD §7 正文 |
| `action_id` | string | 无 | 发起端 | 发起端生成 | 不透明幂等去重键；重复提交返回同一结果 | HTTPS 上报、持久化文件 | PRD §7 正文 |
| `battery_percent` | integer 0–100 | 无 | 设备 | 设备上报 | 取值 0 到 100；主控不检测充电状态，不产生充电分支 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 |
| `network_mode` | enum(connected\|no_signal\|disabled) | 无 | 设备 | 设备上报 | 被动联网状态；其存在不启用 BLE 与 Wi-Fi 业务链路，也不要求小程序接口暴露 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 |
| `audio_config_version` | integer ≥ 0 | 无 | 设备 | 设备 | 单调递增；与音量设置相互独立 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 |
| `firmware_version` | string | 无 | 设备 | 设备 | 设备页「木鱼版本」的取值来源 | HTTPS 上报、WS 帧、持久化文件 | PRD §7 |

## 3. WebSocket 帧自有字段

帧自有字段不属于 §2 的统一字段表（PRD §7 未把它们列为设备↔backend 的同步字段），但它们
同样只在本文定义一次。每帧必带 `contract_version`、`type` 与 `seq` 三个字段；`error` 对象
**仅**在 `error` 帧出现，其余帧不得携带 `error`。

| 字段 | 类型 | 单位 | 角色 | 取值域 | 承载通道 |
| --- | --- | --- | --- | --- | --- |
| `contract_version` | string | 无 | 版本化字段 | 必须等于本契约的 `contract_version`；不一致即拒绝该帧 | WS 帧 |
| `type` | string | 无 | 帧类型判别字段 | 取值见契约 §10.1 帧类型表 | WS 帧 |
| `seq` | integer ≥ 0 | 无 | 单调序号 | 单调递增；只消费 `seq` 大于快照水位的帧 | WS 帧 |
| `error` | object | 无 | 错误对象 | 协议与权限错误；仅在 `error` 帧出现，含 `code` 与 `message` | WS 帧 |

## 4. 事件来源与累计语义

事件来源枚举**恰为** `physical_pvdf`、`device_touch`、`automatic_tap`。

- 三类进入**同一个**有效敲击队列，共享本地累计、经文推进、完成锁定、音频、RGB、持久化
  与同步结果；除该队列外，任何代码路径不得推进正式累计。
- `automatic_tap` 由设备定时器生成（固定周期锚点），按真实敲击相同路径处理；**其数量
  并入 `local_total`/`acked_total`，不新增独立 cloud 累计字段或同步接口**；自动模式只
  存在于内存，不跨重启持久化。
- 不计数的边界：熄屏首次触摸只唤醒、不计数；充电中、完成遮罩中、故障锁定中忽略输入。
- 一次有效敲击只推进一个可消费汉字；标点、空格、换行随紧邻前一个汉字在同一步内出现，
  不消耗敲击、不单独推进游标。消费语义由 canonical 目录固定，本契约只引用，不重定义。

## 5. 状态词表

状态词表分三个**互相独立**的枚举：累计同步状态、命令维度、可信时间门禁。命令维度**不得**
并入累计同步状态枚举。

### 5.1 累计同步状态（5 值，互斥）

| wire 名 | 冻结中文显示词 | 含义 |
| --- | --- | --- |
| `local_recorded` | 本地已记录 | 设备已本地落盘，尚未进入同步窗口 |
| `syncing` | 同步中 | 活动窗口内正在上报 |
| `synced` | 已同步 | backend 已确认，本地与云端一致 |
| `pending_sync` | 待同步 | 存在未确认差值，等待下一次活动窗口 |
| `sync_failed` | 同步失败 | 本次同步被拒绝或失败，保留本地事实并重试 |

### 5.2 命令维度（2 值，独立）

| wire 名 | 冻结中文显示词 | 含义 |
| --- | --- | --- |
| `pending_apply` | 待设备应用 | backend 已保存命令，设备尚未在该次活动窗口取得 |
| `applied` | 已生效 | `applied_revision` ≥ `command_revision` |

### 5.3 可信时间门禁（1 值）

| wire 名 | 冻结中文显示词 | 含义 |
| --- | --- | --- |
| `untrusted_time` | 待校时 | 未取得可信日期时设备今日区显示该状态，cloud 不伪造当天统计 |

## 6. HTTPS 活动窗口

设备在敲击、设置或控制触发的活动窗口内，用一次 HTTPS 请求上报累计高水位，并在同一次
响应中收敛本地镜像。

### 6.1 上报请求字段

必填：`device_id`、`scripture_version`、`local_total`、`acked_total`、`round_id`、
`round_state`、`round_cursor`、`pending_completion`、`applied_revision`、
`battery_percent`、`network_mode`、`audio_config_version`、`firmware_version`、
`action_id`。

### 6.2 响应字段与内容

响应必须回传（AD-3）：

- 最新已确认高水位 `acked_total`；本次确认的差量为响应 `acked_total` 与请求 `acked_total` 之差；
- 确认后的轮次状态 `round_state`、`round_cursor`、`round_id` 与 `pending_completion`；
- backend 当前最新命令修订 `command_revision`，以及**待应用命令的载荷字段** `volume`、
  `brightness`、`timeout`（定义见 §9.1；已生效判定所需的 `applied_revision` 由请求携带）；
- 查询场景下冻结的 `snapshot_seq`。

响应必填：`acked_total`、`round_id`、`round_state`、`round_cursor`、`pending_completion`、
`command_revision`、`snapshot_seq`；待应用命令载荷字段 `volume`、`brightness`、`timeout`
（定义见 §9.1）也随响应下发。已生效判定所需的设备已应用高水位由请求携带，不在响应必填内；
`snapshot_seq` 每次响应都必须回传，其取值在查询场景下为冻结后的水位。

### 6.3 幂等规则

- 只对高于已确认高水位的差值幂等推进一次；同值或更低的提交是**幂等 no-op**，不是错误。
- backend 不以经文长度对差值盲推游标或完成锁定；单轮推进只在设备本地发生，backend 以
  设备上报的 `round_id` 与游标做边界校验并确认。
- 携带 `action_id` 的请求重复提交返回同一结果；新轮次动作重复提交返回同一 `round_id`。

## 7. 轮次、完成与跨轮竞态

- 离线差量按 `round_id` 分段提交；无法归属轮次或缺基准高水位时**拒绝确认**，不把一个
  累计高水位盲推到新轮次。
- 设备本地到达末字而 cloud 未确认完成时置位 `pending_completion`；完成确认前不得创建
  新轮次，也不得提前进入完成态。
- 「从头开始」使用幂等 `action_id`，重复请求返回**同一** `round_id`。
- 跨轮竞态裁决：设备在未获完成确认时不得提交新 `round_id`；若设备先行本地重置并提交
  新轮次，backend 以**该次请求携带的 `round_id` 段内差值**为准，只推进属于已确认段的部分；
  未被确认的完成仍归属旧 `round_id`，新轮次从首字重新开始，旧轮次的历史统计与完成状态
  保留。本地重置与 cloud 基准的收敛顺序固定为：设备先按 cloud 已确认高水位重建本地
  `acked_total` 与游标，再创建新轮次。该裁决与 AD-3「单轮推进只在设备本地发生、backend
  只做边界校验与确认」一致。
- 「退出」保留完成状态与历史统计；新轮次不清除旧篇章与累计。

## 8. 恢复与冲突路径

| 场景 | 语义 |
| --- | --- |
| 设备离线后补传 | 差量按 `round_id` 分段；高于 `acked_total` 的差值只推进一次；同值或更低为幂等 no-op |
| 完成未确认 | 置位 `pending_completion`；不得提前进入完成态，不得创建新轮次 |
| 设备重置（`local_total` 低于 cloud 基准） | 返回**可恢复的冲突状态**；禁止静默回退或无限重试；设备必须按 cloud 基准重建本地高水位后再继续 |
| 离线积压（`local_total` 与 `acked_total` 之差）达到上限 1000 | 设备侧拒绝新输入并提示；backend 对差值达到 1000 的请求返回业务码 `20004`；不得静默丢弃。判定输入仅为请求携带的 `local_total` 与 `acked_total`（PRD §4 术语「离线积压」），不新增字段；小程序按同一判定输入呈现该状态 |
| 达到上限后的排空 | 业务码 `20004` 只表示积压已达上限，**不阻断确认**：backend 在同一次响应中仍按 §6.3 幂等推进 `acked_total` 并回传最新确认高水位与差量。设备在拒绝态下必须继续上报（只拒绝新的本地记录，不停活动窗口上报），以响应回传的 `acked_total` 收敛本地镜像；差值回落到低于上限后立即恢复接受输入，故积压是拒绝窗口内的瞬态而非不可恢复的终态。该路径与 AD-3「达到上限拒绝新输入并提示」一致，不新增字段、端点或阈值 |
| WebSocket 序号空洞 | 先查询补齐再续播；不消费空洞之后的序号 |
| 断线重连 | 以查询响应冻结的 `snapshot_seq` 为唯一基准，从 `snapshot_seq + 1` 续订；`seq <= last_applied_seq` 的帧丢弃 |
| 小程序回放 | 持久化 `replay_cursor`；回放期间新事件排队；事件携带 `round_id`，旧轮次事件不得写入当前篇章 |
| 命令重复或乱序 | 旧修订不得覆盖新修订；重复获取与回传幂等；设置命令与篇章动作的 `action_id` 不互相覆盖 |
| `scripture_version` 不一致 | 配置错误：停止推进并报告，不做自动映射、选经或导入 |

## 9. 命令修订与 `action_id`

- `command_revision` = backend 已下发命令的最新修订（单调递增，只保留最新）。
- `applied_revision` = 设备已应用命令的单调高水位（单调不减）。
- 已生效判定：`applied_revision` ≥ `command_revision`；据此把「待设备应用」翻转为
  「已生效」，**不依赖单次 ACK 到达**。
- 设备应用命令时必须原子写入命令字段与 `applied_revision`。
- `action_id` 是**同一个字段名**，按用途分两族，取值互不覆盖（同一个 `action_id` 值不得跨族
  复用）：**篇章动作族**（「从头开始」「立即同步」）的去重键与轮次/水位同属单事务组，落
  `progress.json`；**设置命令族**（`volume`/`brightness`/`timeout` 下发）的去重键随命令修订
  落 `commands.json`。两族的持久化落点见 §11。
- 「待应用命令」不是不透明载荷：其字段集合由 §9.1 冻结，backend 与 Epic 6 设置镜像直接引用，
  不得自行发明 wire 名；新增或改义载荷字段时按 §1.2 递增 `contract_version`，并先更新本文与
  注册表。

### 9.1 待应用命令载荷字段

`command_revision` 修订指向的命令载荷**恰由**下表三个字段构成（PRD §6.2.2：cloud 保存音量、
亮度、熄屏时长、立即同步和篇章命令的最新 `command_revision`；立即同步与篇章动作只携带
`action_id`，无值载荷）。三个字段只在本文定义一次，注册表 `command_payload_fields` 为其派生。

| 字段 | 类型 | 单位 | 取值域 | 出处 |
| --- | --- | --- | --- | --- |
| `volume` | integer 0–100 | 无 | 0 到 100；默认 50，0 等价于静音 | FR-E-005 / FR-C-008 |
| `brightness` | enum(low\|mid\|high) | 无 | 三档；默认中档 | FR-E-007 / UX `settings-controls` |
| `timeout` | enum(5\|15\|30) | 秒 | 三档自动熄屏时长；默认 15 | FR-E-007 / UX `settings-controls` |

- 三值随 `command_revision` 一并下发，并在同步响应与 `command_state` 帧携带（§6.2、§10.1）。
- 小程序设置页只镜像这三个字段的当前值与命令维度状态（FR-C-017），不复制其类型与取值域。

## 10. WebSocket 契约

### 10.1 帧类型

每帧必带 §3 的 `contract_version`、`type` 与 `seq` 三个帧自有字段；`error` 对象只在 `error`
帧出现。帧类型判别字段为 `type`。

| 帧 | 方向 | 内容 | 出处 |
| --- | --- | --- | --- |
| `snapshot` | backend → frontend | 权威快照、冻结的 `snapshot_seq`、轮次、命令状态与设备状态 | FR-C-006 |
| `delta` | backend → frontend | 已确认差量，带单调 `seq` 与 `round_id` | FR-C-009 |
| `completion` | backend → frontend | 完成确认 | FR-C-016 |
| `command_state` | backend → frontend | 命令下发与已生效状态变化，携带 `volume`、`brightness`、`timeout` 载荷 | FR-C-008 |
| `heartbeat` | backend → frontend | 应用层心跳请求 | AD-17 |
| `heartbeat_ack` | frontend → backend | 应用层心跳应答 | AD-17 |
| `error` | backend → frontend | 协议与权限错误 | FR-C-011 |

### 10.2 Endpoint 派生规则

`VITE_API_BASE_URL` 是含 `/api/v1` 的 REST base；WebSocket 的 origin 与 path 由本契约
派生，不在 env 中重复拼接 REST 前缀（AD-20）：origin 取该 base 的同源（`https` 映射为
`wss`、`http` 映射为 `ws`），path 为该 REST base 加固定后缀 `/ws`。鉴权令牌**只经 query
参数**传递，字段名固定为 `token`（小程序不支持自定义请求头）。小程序同时最多 5 个
WebSocket 连接，故契约约束「单一活跃 socket」：新连接建立前必须关闭旧连接。

### 10.3 心跳与重连参数

心跳由 backend 发起，frontend 以 `heartbeat_ack` 应答。参数均为正整数并带 `_ms` 单位；
测试上界用于门禁断言，默认值必须落在上界之内。

| 参数 | 默认值 | 测试上界 | 单位 |
| --- | --- | --- | --- |
| `ws_heartbeat.interval_ms` | 30000 | 45000 | ms |
| `ws_heartbeat.timeout_ms` | 10000 | 15000 | ms |
| `reconnect.initial_backoff_ms` | 1000 | 无 | ms |
| `reconnect.max_backoff_ms` | 30000 | 30000 | ms |
| `reconnect.max_attempts` | 10 | 无 | 次 |

重连采用指数退避加 full jitter，并在 `max_backoff_ms` 处封顶以抑制惊群。关闭码取值域
依 RFC 6455：`1000` 正常关闭、`1001` 服务端离开、`1006` 异常关闭（只记录不处理）、
`1008` 策略违规；业务错误使用 `4000`–`4999` 私有段。收到 `1000` 或 `1008` 时**不再重连**
（分别为正常关闭与策略违规）；`1001` 与 `1006` 按上表退避重连，其中 `1006` 只记录不处理。

## 11. JSON 持久化文件粒度

根目录为 backend 的 `app.data-dir`（当前值 `./data`，本契约不修改配置）。每个文件独立
原子写（临时文件 + 落盘 + 重命名），单次重命名即一次持久化事务边界；不引入任何数据库。
跨文件的非原子写顺序不在本章冻结（属实现 Story），本章只冻结粒度、单事务分组与字段落点。

| 文件 | 承载事实 | `schema_version` | 字段 | 迁移说明 |
| --- | --- | --- | --- | --- |
| `progress.json` | 累计高水位、轮次、游标、完成置位与相关动作去重键 | 1 | `local_total`、`acked_total`、`round_id`、`round_cursor`、`round_state`、`pending_completion`、`action_id` | 新增字段时可省略读取并取默认值；删除或改义字段必须递增 `schema_version` 并提供一次性重写 |
| `identity.json` | 单身份单设备映射 | 1 | `device_id` | 身份不可迁移；重建即重新绑定 |
| `commands.json` | 命令修订、已应用高水位、待应用命令载荷与设置命令去重键 | 1 | `command_revision`、`applied_revision`、`action_id`、`volume`、`brightness`、`timeout` | 只保留最新修订，旧修订不写回 |
| `device_state.json` | 设备状态镜像与音频配置版本 | 1 | `battery_percent`、`network_mode`、`audio_config_version`、`firmware_version` | 属非权威镜像，可整体重建 |
| `daily_stats.json` | 按配置时区归档的日统计（由已确认增量派生的按日桶，不承载统一字段事实） | 1 | 无 | 日界以 backend 配置时区为准，不按设备本地时间切分 |

**单事务分组约束**：`local_total`、`acked_total`、`round_id`、`round_cursor`、
`round_state`、`pending_completion` 与篇章动作族的 `action_id` **必须落在同一个文件内**
（`progress.json`，即上表的单事务组），不得分散到多个文件，掉电恢复后不得出现高水位、
轮次与游标不匹配。

### 11.1 持久化作用域与字段落点

§2 中承载通道含「持久化文件」的每个字段，都必须在下表给出落点；作用域互不重叠，同一字段
可同时属于多个作用域（设备本地事实与其 backend 镜像）。

| 作用域 | 载体 | 承载字段 |
| --- | --- | --- |
| backend | `app.data-dir` 下的 §11 文件表 | `device_id`、`local_total`、`acked_total`、`round_id`、`round_cursor`、`round_state`、`pending_completion`、`action_id`、`command_revision`、`applied_revision`、`volume`、`brightness`、`timeout`、`battery_percent`、`network_mode`、`audio_config_version`、`firmware_version` |
| device | Embedded 本地持久化（FR-E-004） | `local_total`、`acked_total`、`round_id`、`scripture_version`、`round_state`、`round_cursor`、`pending_completion`、`action_id`、`applied_revision`、`volume`、`brightness`、`timeout` |
| frontend | 小程序本地存储 | `snapshot_seq`、`replay_cursor`、`last_applied_seq` |
| canonical | `docs/contracts/canonical/heart-sutra.manifest.json`（构建期单一落盘来源，AD-4） | `scripture_version` |

- backend 作用域的承载字段集合必须等于 §11 文件表五份文件字段列的并集；本文不含任何
  `app.data-dir` 之外的后端状态文件。

## 12. 错误语义与拒绝条件

信封为 `/api/v1` + `{code,message,data}`，成功 `code` 为 0。业务错误使用 HTTP 200 加
业务码，码形为 `{HTTP 状态}{两位序号}`；协议错误使用对应 HTTP 状态，不把错误包装成成功。

| 拒绝条件 | HTTP 状态 | 业务码 |
| --- | --- | --- |
| 轮次无法归属 | 200 | 20001 |
| 缺少基准高水位 | 200 | 20002 |
| 设备重置冲突 | 200 | 20003 |
| 队列已满 | 200 | 20004 |
| 经文版本不一致 | 200 | 20005 |
| 命令旧修订 | 200 | 20006 |
| 令牌失效 | 401 | 40101 |
| 协议字段非法 | 400 | 40001 |

- 业务拒绝（HTTP 200 + `2xxxx` 业务码）仍是一次响应，§6.2 的响应必填字段照常回传：`20003` 的
  cloud 基准与 `20004` 的已确认高水位都由该次响应携带，设备据此重建本地高水位或回落差值，
  不得只回 `{code,message}`（否则 §8 的「禁止无限重试」不可实现）。
- 「同值或更低高水位」是幂等 no-op，不是错误。
- 「队列已满」（业务码 `20004`）的判定输入为请求携带的 `local_total` 与 `acked_total` 之差
  （PRD §4 术语「离线积压」），阈值为 1000：差值**达到** 1000 即拒绝，阈值语义不是「大于
  1000」；判定不依赖任何新增字段。
- `20004` 只表示积压已达上限并提示设备继续拒绝新输入，**不阻断确认**：backend 仍按 §6.3
  幂等推进 `acked_total`，设备据响应回传的高水位回落差值后恢复接受输入（§8「达到上限后的
  排空」）。
- 经文版本不一致属配置错误，必须停止推进（FR-C-002）。
- 表中业务码由本契约预留，实现信封时不得重定义。

## 13. 静态校验边界

`docs/contracts/tests/run_sync_contract_tests.py` 是本契约的门禁，只读执行，不修改任何
文件。它能机械验证的是：两个交付文件的存在性与编码卫生；`contract_version`；§1.1 列出的
全部逐值比对面（§2 字段表、§3 帧自有字段表、§6.1 上报必填与 §6.2 响应必填清单、§9.1 命令
载荷字段表、§5 三个状态词表、§10.1 帧类型表的方向/内容/出处、§10.3 参数表的默认值与测试
上界、§11 文件表四列、§11.1 持久化作用域表、§12 拒绝条件表）；承载通道四个取值与本文各
清单的双向闭合（§6.1 必填集合与「HTTPS 上报」通道字段集合相等，§6.2 响应必填、§10.1 帧
载荷、§11 文件表与 §11.1 作用域承载的字段必须各自声明对应通道）；注册表 `endpoint`
（含 `scheme_map` 与 `token_transport`）、`close_codes`、`ws_heartbeat`、`reconnect`
（含 `strategy` 与 `hole_rule`）、`offline_backlog`（含排空路径与「达上限仍确认」）语义键与
正文取值的一致性；期望字段与事件来源集合从 `prd.md` §7 与 `ARCHITECTURE-SPINE.md`
§Structural Seed 的清单行反引号标识符提取后逐值比对；样例字段的声明完整性、帧样例
`contract_version` 与注册表一致、枚举型样例取值落在本文声明的取值域内；持久化单事务分组与
「承载通道含持久化文件的字段必有落点」；禁用词、经文正文、槽位常量、经文长度、生产域名与
AppID 的扫描；被点名的指针文档（`docs/contracts/README.md`、`docs/README.md`、
`docs/backend/经验-后端REST信封与微信登录.md`）不再把本契约写成将来时（含「尚未/待/后续/
将来 + 冻结/定稿/收敛」一类同义表述）；spine 的正式输入枚举、字段清单、AD-5 分层措辞与三条
Deferred 收敛标注已收敛。

**它无法机械判定的部分**：仓库内其它文档（未点名的 `docs/embedded/`、`Embedded/AGENTS.md`、
UX 规范等）是否复制了字段表、事件来源枚举或 WebSocket 参数——门禁只能检查被点名的指针
文档与 spine，不能证明全仓无第二份副本；`automatic_tap` 不得新增独立 cloud 字段或接口这一
禁止项也只能通过「字段名模式」与「本文必须保留该禁止语句」来间接约束，无法证明实现层不
存在额外接口；将来时扫描只能覆盖已登记的表述族，不能证明指针文档的每一句话都不是将来时。
门禁是构建与评审前的 fail-closed 检查，不是防篡改签名。
