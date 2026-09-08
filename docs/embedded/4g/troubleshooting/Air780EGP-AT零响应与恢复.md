<!-- 来源: main_control · 源文件: components/BSP/GPS/gps.c + gps_config.h + docs/Function/景区管理四态授权与异常路径说明.md(仅重试语义) · main_control HEAD: fb458b9 · 迁移: 2026-09-08 · 判定: 适配 · EWF 适配点: 单条 4G 链路的恢复/重试语义 -->
# Air780EGP AT 零响应与恢复

> 状态：EWF 绿地阶段方法学排障知识。恢复路径提炼自 `main_control`（M100EG-C2 4G/GPS 驱动，含 Air780EGP HTTPS 经验，2026-09-05 实现）；EWF 4G 模组同为 **Air780EGP**，需在自有目标板上重新闭环。所有实测项未验证前一律 `DEFERRED`，不得从编译成功推导出通信可靠。
> 引脚唯一权威：`_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` §板级合同（冲突一律以 spine 为准，先改 spine 再调整）。
> 最近更新：2026-09-08

## 1. 结论（先读）

**现象：** 模组 UART 对 AT 无任何响应——不发 `OK`/`ERROR`/`+CME ERROR`，也没有 URC，单条 AT 事务持续读超时。

**影响：** 若把**单次** AT 超时当成“模组失联”并立刻复位/重启模组，会造成反复软复位、已建立会话被误杀、状态抖动与功耗毛刺。尤其在弱网或模组休眠竞争窗口内，单次超时往往是瞬态，不是失联。

**判定原则：** **连续** AT 超时计数 **≥ `GPS_TIMEOUT_RECOVERY_COUNT = 3`** 才判定失联进入恢复；恢复先做 **DTR 拉唤醒 + 100 ms settle + AT 重同步**，不直接复位；重同步仍失败才清掉可能“假 true”的 `network_ready` 等实时状态，并进入**单调退避 5/15/30/60 s**，等待下一轮重试。整个过程**不做整机/模组断电重启**。

**一句话：** 单次超时只计数；≥3 次才恢复；恢复先 DTR 唤醒 + AT 重同步；仍失败清假 `network_ready` 后按 5/15/30/60 s 退避重试，成功后清零计数与退避档位。

## 2. AT 零响应故障树（判定链）

以下顺序即固件应实现的判据链，每步不可跳过、不可提前放大：

1. **每次发 AT 前先清 RX 残留**：`uart_flush_input()` 清空接收缓冲，避免历史 URC/半行残留被误判为本次响应。清空失败直接按发送错误处理，不计数超时。
2. **发送指令并等待响应**：写指令 + `\r\n` → 等 TX done → 按行读响应直到超时（普通 AT 超时 1.5 s，基础同步 AT 超时 800 ms）。
3. **计数只加在“读超时”上**：收到 `OK` 或 `ERROR`/`+CME ERROR` 都属“链路活着”，**超时计数清零**；只有整个事务读超时才 `timeout_count++`。
4. **连续计数 ≥3 才判定失联**：驱动内层循环以 `timeout_count < GPS_TIMEOUT_RECOVERY_COUNT` 为运行条件；某轮轮询发现 `≥3` 即记录「连续 3 次 AT 事务超时」并跳出活动循环，转入恢复路径。单次、两次超时只告警，继续快/慢节奏轮询。
5. **恢复 = DTR 唤醒 + settle + AT 重同步**：DTR 拉到唤醒电平 → 等待 `GPS_MODEM_WAKE_SETTLE_MS = 100 ms` → 执行 `AT` 重同步（在候选波特率序列上逐档发 `AT` 探测，直到某档回 `OK`）。
6. **重同步仍失败 → 清“假”实时状态**：`gps_clear_live_status()` 把 `sim_ready / registered / attached / pdp_active / ipv4_valid / network_ready` 及 IPv4 等实时标志全部复位为 false——目的不是“抹掉事实”，而是**摘掉上一个会话残留的假 `network_ready`**，避免上层按旧状态误上报。
7. **进入单调退避**：`recovery_backoff_ms[] = { 5000, 15000, 30000, 60000 }` ms。档位索引每轮恢复后 `+1`，到 60 s 后**封顶不再增长**（单调递增、有限上界）。退避期间驱动任务睡满再回第 1 步重新 `gps_modem_init()`。
8. **成功后清零**：任一环节重新拿到有效 `AT` 响应即清零 `timeout_count`；外层初始化成功即把退避档位索引**重置为 0**，回到 5 s 起步。

> 反直觉点：`gps_recover_modem()` 在“DTR 唤醒 + 重同步仍失败”时**仍返回成功继续**（只是清了状态、留待下一轮退避重试），只有 DTR 拉电平本身失败才中断外层循环。恢复路径的职责是“推进一轮重试”，不是“当场断言成功”。

## 3. 恢复与退避参数基线（迁移自 gps_config.h）

| 参数 | 值 | 语义 |
| --- | ---: | --- |
| `GPS_AT_TIMEOUT_MS` | 1500 | 普通 AT 单事务读超时 |
| `GPS_AT_SYNC_TIMEOUT_MS` | 800 | 基础同步 `AT` 单次超时 |
| `GPS_AT_SYNC_RETRY_INTERVAL_MS` | 300 | 同波特率两次同步 AT 间隔 |
| `GPS_MODEM_WAKE_SETTLE_MS` | 100 | DTR 唤醒后 settle |
| `GPS_TIMEOUT_RECOVERY_COUNT` | 3 | 连续超时阈值，达到才判失联 |
| `GPS_RECOVERY_BACKOFF_1_MS` | 5000 | 第一轮恢复退避 |
| `GPS_RECOVERY_BACKOFF_2_MS` | 15000 | 第二轮 |
| `GPS_RECOVERY_BACKOFF_3_MS` | 30000 | 第三轮 |
| `GPS_RECOVERY_BACKOFF_4_MS` | 60000 | 第四轮起封顶 |

退避表要求**单调递增**（源文件有编译期静态校验：后一档不得小于前一档、首档不得为 0）。EWF 复制此表时保留该校验。

## 4. 「拒绝单次失败即重启」原则

- 单条 AT 超时 **只累计计数**，不触发任何恢复动作；计数在任意一次有效 `OK`/`ERROR` 响应时清零。这样弱网抖动、偶发 URC 抢占、休眠竞争都只会产生一次可观测告警，而不会升级成恢复。
- **恢复路径不整机重启、不复位模组电源**：首选 DTR 唤醒 + AT 重同步；仅当 AT 重同步反复失败时才依赖退避慢慢磨，把“复位模组”留作最后手段并需要明确理由（本基线不默认走到硬复位）。
- 单一失败即重启会把瞬时弱网放大成“反复掉线→重连”的振荡，也破坏第 5 节的“保持上次状态”语义——**能通过重试收敛的问题不升级处置手段**。

## 5. 「失败保持上次合法状态、不随同步失败突跳」业务语义

源语境（外骨骼景区多态授权）的原文语义：**完成授权后，同步失败本身不改变当前状态；可操作不会因断网突停，已锁定也不会因断网自解锁**——即“继续沿用最近一次**合法响应**确认的状态；下一次合法响应到来后才清除或切换”。EWF 去掉景区/支付/围栏/管理员语境后，收缩为**单条 4G 链路的同步状态语义**：

- **单链首连前**：按快/慢节奏重试（源基线：快速阶段约每 10 s、之后约每 30 s；EWF 网络轮询基线为未联网 5 s / 联网后 30 s，见 `低功耗策略与实测验收.md`），重试节奏只影响“是否发起一次尝试”，**不推进本地业务状态**。
- **连上后按需上报**：EWF 唯一联网链路是 Air780EGP 4G，只在活动窗口内发起一次 HTTPS 上报（`local_total`/`acked_total`/`round_id`/`round_cursor`，见 spine 同步字段清单）；上报后回到离线低功耗记录，不维持长连接、不做周期轮询。
- **上报失败保持上次状态**：一次同步失败不把本地镜像标记为“已同步”、也不回退已确认计数；本地持久化与 `acked_total` 等字段以**最近一次云端合法确认**为准，下一轮活动窗口按需重试。**失败只影响“本轮没送到”，不得让本地计数/状态突跳**（不随失败清零，也不随失败加倍）。

## 6. 日志防泄露提醒

- 含令牌/请求正文的 AT 事务（如 `AT+HTTPDATA` 正文、URL 内 `token`）**不得明文打印**：源实现以 `sensitive` 标志隐藏命令内容，日志只输出「发送敏感 AT 事务，命令内容已隐藏」；超时错误同样隐藏命令前 64 字符。
- 响应日志同样过滤：敏感事务的响应行不逐行 `ESP_LOGI`。
- EWF 沿用该约定：任何含 `token`/`Authorization`/请求体的日志分支都必须走脱敏路径，违反即视为泄露缺陷。

## 7. 板级引脚与电平复核（按 EWF spine §板级合同）

| 信号 | ESP32-S3 GPIO | 来源 | 约束 |
| --- | ---: | --- | --- |
| Air780EGP UART TX / RX | IO43 / IO44 | spine §板级合同 | 模组独占 UART；单 AT 入口 + 单事务所有权 |
| Air780EGP DTR（休眠/唤醒控制） | IO10 | spine §板级合同 | 与源工程 GPIO7 不同，按 EWF 原理图接线；**开漏驱动** |
| Air780EGP RST / NET_STATUS / GNSS_VCC | IO15 / IO16 / IO8 | spine §板级合同 | RST、NET_STATUS 源实现保持输入高阻；GNSS 默认关闭，仅保留供电开关能力 |

- 源工程 DTR 约定（`gps.h` + `gps_config.h`）：`GPS_DTR_AWAKE_LEVEL = 0`（低电平唤醒）、`GPS_DTR_SLEEP_LEVEL = 1`（高电平休眠），ESP32 侧用**开漏**驱动，预置安全电平后再挂 UART。
- **电平极性必须复核整模组手册**：EWF 换板后 DTR 接线与电平有效极性以 Air780EGP 硬件手册/整机原理图为准，源工程值只作移植默认，不得在样机/原理图核验前冻结。若手册极性相反，仅需对调 `AWAKE`/`SLEEP` 两个宏，故障树与恢复流程不变。
- 与 spine 冲突时一律以 spine 为准：本文件不冻结任何未核验电平/引脚，实施前先读 spine §板级合同与整机原理图。

## 8. 源证据索引（迁移核对用）

- `main_control/components/BSP/GPS/gps.c`：`gps_uart_init`（DTR 开漏预置 + UART 挂接/清残留重挂）、`gps_update_timeout_count`（超时计数/清零）、`gps_send_at_internal`（发 AT 前 `uart_flush_input`、读超时才 `++`）、`gps_sync_modem`（候选波特率逐档 `AT` 重同步）、`gps_recover_modem`（DTR 唤醒 + 100 ms + 重同步，失败清假状态）、`gps_clear_live_status`（复位实时标志）、`gps_prepare_sleep`（退出前 `AT+CSCLK=1` 等）、`gps_driver_task`（退避表与 `recovery_index` 单调/封顶/清零）。
- `main_control/components/BSP/GPS/gps_config.h`：§AT、网络与 GNSS 时序（阈值/退避表/编译期单调校验）。
- `main_control/docs/Function/景区管理四态授权与异常路径说明.md`：仅取「启动后保持原状态，同步失败不改变当前状态」与「快/慢重试」重试语义，已剥离景区/支付/围栏/管理员语境。
- EWF 基线：spine §板级合同引脚（IO43/44、IO10）；`低功耗策略与实测验收.md`（活动窗口单次上报、5 s/30 s 轮询节奏）。
