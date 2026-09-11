<!-- 来源: main_control · 源文件: components/BSP/GPS/{gps.c,gps_config.h,gps.h} + docs/architecture/硬件架构与软件框架说明.md(§9.3) + docs/Function/设备功能全景文档.md · main_control HEAD: fb458b9 · 迁移: 2026-09-08 · 判定: 适配(去 combo/SCENIC/景区宏, 引脚按 EWF spine) · EWF 适配点: EWF 用整颗 Air780EGP, 无 MQTT/BLE -->
# Air780EGP AT 联网与 HTTPS 经验

> 状态：EWF 绿地阶段方法学迁移。底层经验全部提炼自 `main_control`（HEAD fb458b9，2026-09-05 实现）的 `components/BSP/GPS/gps.*`。该驱动面向 **M100EG-C2（Air780E 系 AT 固件）**，其中 `GPS_HTTP_SSL_CONTEXT_ID=153` 直接点名 Air780EGP——即 EWF 所用整颗模组。所有数值与流程未在 EWF 自有板闭环前一律 `DEFERRED`，不得从“代码能编译”推导出“通信可靠”。
> 引脚/板级唯一权威：`_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` §板级合同（冲突一律以 spine 为准）。本文件不冻结任何未核验电平/引脚；EWF 具体电源与载板网络见 `docs/hardware/电子木鱼-硬件原理图设计基线.md`。
> 兄弟排障文档（AT 零响应故障树/恢复语义的展开版）：`docs/embedded/4g/troubleshooting/Air780EGP-AT零响应与恢复.md`。
> 最近更新：2026-09-08

---

## ① 适用范围（EWF 单条 4G HTTPS 链路）

EWF 唯一产品联网链路 = **Air780EGP 整颗模组 + AT/HTTPS JSON 单事务链**（spine AD-14 / AD-10 / Stack「4G = 沿用 main_control 已验证设计」）：

- **一条链路**：Air780EGP 4G。ESP32-S3 的 BLE/Wi‑Fi 仅硬件能力保留、固件不配置为产品通道；无 MQTT、无独立模组业务云。整颗 Air780EGP（无 combo/外骨骼封装），STM32/外骨骼/UART 遥控、BLE 业务、景区四态授权 payload **不迁移**，仅在排除语境提及。
- **GPS 保留控制能力但默认关闭**：不发送 `AT+CGNS*`、不执行 GNSS 轮询（详见 ⑧）。
- **串行所有权**：UART（IO43 TX / IO44 RX）是模组唯一命令通道。应用层禁止直读写 UART；HTTPS 上报只能通过「有界单事务上报调用」（EWF 仿 `gps_https_post_json` 语义），队列深度固定 1，后台任务单事务持有 UART。
- **活动窗口模型**：在活动窗口内复用 PDP/HTTPS 上下文完成一次上报后回低功耗，不维持长连接、不做周期遥测（EWF 业务节奏区别于 main_control 景区 10 s/30 s 周期遥测；轮询节奏仅作为链路保活基线，见 ⑥）。

公开 API 形态（照搬语义、改名按 EWF 模块规范）：`start / get_status / https_post_json(url, token, json_body, response, result, timeout_ms) / suspend`。

---

## ② 波特率握手与「不写 NVRAM」

目标：在不知道模组当前会话波特率的前提下恢复 AT 通信，并把会话归一化到 115200，**不把任何参数写入模组 NVRAM**（`AT&W` 禁发），重启后仍从扫描开始，避免把一块板钉死在某个波特率上。

### 候选波特率表（目标 + 7 档回退，共 8 档）

| 顺序 | 波特率 | 角色 |
| ---: | ---: | --- |
| 1 | 115200 | 会话目标（`GPS_UART_BAUD_RATE`），也是候选表首项 |
| 2 | 57600 | 回退 1 |
| 3 | 9600 | 回退 2 |
| 4 | 19200 | 回退 3 |
| 5 | 38400 | 回退 4 |
| 6 | 230400 | 回退 5 |
| 7 | 460800 | 回退 6 |
| 8 | 921600 | 回退 7 |

实现要点（`gps_sync_modem`）：

1. **先试上次握手成功的速率**（`s_current_uart_baud_rate`），再遍历候选表跳过重复项；每档切换 ESP32 侧 UART 波特率（`gps_uart_set_baud_rate`：先等 TX done、flush 输入，再 `uart_set_baudrate`，成功后 `vTaskDelay(50 ms)` settle）。
2. **每档最多发 3 次大写 `AT`**（`GPS_UART_SYNC_ATTEMPTS_PER_BAUD=3`）；单次同步 `AT` 超时 800 ms（`GPS_AT_SYNC_TIMEOUT_MS`），同档两次发送间隔 300 ms（`GPS_AT_SYNC_RETRY_INTERVAL_MS`）。
3. 任一档收到 `OK` 即握手成功；全部档扫完无响应返回超时，交由恢复路径处理（③④）。

### 归一化到会话目标波特率（`gps_normalize_modem_baud_rate`）

握手成功的档位可能不是 115200，归一化流程：

1. 当前速率已是 115200 → 直接成功。
2. 否则在**检测到的当前速率**下发 `AT+IPR=115200`。
3. **不等待该指令一定回 OK**：部分 Air780 固件在返回完整 `OK` 前就立即切速，因此即使应答超时/丢失也继续。
4. ESP32 侧切到 115200 → 发一次 `AT` 复验。
5. 复验失败 → 重新执行候选表扫描（`gps_sync_modem`）恢复链路；若重扫后速率已回到 115200 则视为成功，否则留待外层下一轮继续切换。
6. **全程不发 `AT&W`**，模组断电/复位后回到出厂/上次持久化波特率，靠每次扫描适配。

### 握手成功后的一次性初始化（`gps_modem_init`）

UART 参数：8 数据位 / 无校验 / 1 停止位 / 无流控，RX 缓冲 4096。握手 + 归一化成功后依次执行（均为会话级、不持久化）：

| 指令 | 作用 |
| --- | --- |
| `ATE0` | 关回显 |
| `AT+CMEE=2` | 打开 `+CME ERROR:<err>` 详细错误码 |
| `AT+CSCLK=0` | 关自动休眠（会话运行期保持唤醒，休眠由 DTR 路径显式管理） |
| `ATI` / `AT+CGMM` / `AT+CGMR` | 读型号与固件版本，写入状态快照 |

> UART 重挂细节：`gps_uart_init` 在 `uart_driver_install` 后用 `uart_wait_tx_done → uart_flush_input → gpio_reset_pin(TX/RX) → uart_set_pin → uart_set_baudrate` 清除 GPIO 矩阵历史连接并以首选速率重新挂接，防止上一次会话残留。

---

## ③ AT 零响应 / 超时与恢复

> 完整故障树、判据链、恢复步骤与日志见兄弟文档 **`docs/embedded/4g/troubleshooting/Air780EGP-AT零响应与恢复.md`**。本节只保留 EWF 实现必须固化的规则。

- **单条 AT 读超时**：普通 AT 超时 1.5 s（`GPS_AT_TIMEOUT_MS`），基础同步 `AT` 超时 800 ms。收到 `OK`/`ERROR`/`+CME ERROR` 都算“链路活着”，**超时计数清零**；只有整笔事务**读超时**才 `timeout_count++`。
- **连续 ≥3 次才判定失联**（`GPS_TIMEOUT_RECOVERY_COUNT=3`）：驱动内层活动循环以 `timeout_count < 3` 为运行条件；达到 3 记「连续 3 次 AT 事务超时」并跳出活动循环进恢复。**拒绝单次超时即重启/复位模组**——弱网抖动、休眠竞争、URC 抢占都是瞬态，只告警。
- **恢复步骤（`gps_recover_modem`）**：
  1. DTR 拉到唤醒电平（⑤）；
  2. 等 `GPS_MODEM_WAKE_SETTLE_MS=100 ms` settle；
  3. 执行 `AT` 重同步（候选波特率逐档探测，见②）；
  4. 重同步成功 → 清零超时计数，返回；
  5. 重同步仍失败 → `gps_clear_live_status()` 把 `sim_ready / registered / attached / pdp_active / ipv4_valid / network_ready` 等实时标志复位，**摘掉上一会话残留的“假 `network_ready`”**，清零超时计数，仍返回“成功继续”（推进一轮重试，不当场断言成功）；
  6. 整机/模组断电重启**不在默认路径内**；DTR 拉电平本身失败才中断外层循环。
- 恢复成功后退避档位索引重置回 0（回到 5 s 起步）；`gps_modem_init` 失败或活动循环失联跳出后，均按 ④ 退避再进下一轮。

---

## ④ 恢复退避：5 / 15 / 30 / 60 s 单调 + 编译期校验

| 档位 | 宏 | 值 |
| ---: | --- | ---: |
| 1 | `GPS_RECOVERY_BACKOFF_1_MS` | 5000 |
| 2 | `GPS_RECOVERY_BACKOFF_2_MS` | 15000 |
| 3 | `GPS_RECOVERY_BACKOFF_3_MS` | 30000 |
| 4（封顶） | `GPS_RECOVERY_BACKOFF_4_MS` | 60000 |

- 驱动任务内 `recovery_backoff_ms[]` 档位索引每轮恢复后 `+1`，到 60 s **封顶不再增长**（单调递增、有限上界）；任一环节重新拿到有效 `AT` 响应即清零超时计数，初始化成功即把档位索引重置为 0。
- `gps_config.h` 有**编译期 `#error` 静态校验**：首档不得为 0，后一档不得小于前一档。EWF 复制此表时必须保留该校验，防止有人改成非单调序列。
- 退避只在“失联恢复 / 初始化失败”的外层循环使用；**单个 HTTPS 事务内部**不套退避，用自身 AT 超时（60 s 上限见⑦）。

---

## ⑤ DTR 休眠唤醒

### 电平约定（需 EWF 按整模组手册复核）

main_control 移植默认值（`gps_config.h`）：

| 宏 | 值 | 含义 |
| --- | ---: | --- |
| `GPS_DTR_AWAKE_LEVEL` | 0 | 唤醒有效电平（低） |
| `GPS_DTR_SLEEP_LEVEL` | 1 | 休眠有效电平（高） |

- `gps_config.h` 编译期校验：两宏只能为 0/1 且**必须相反**。
- **EWF 换板后必须复核**：DTR 接线与电平有效极性以 **Air780EGP 硬件手册 / 整机原理图**为准，源值只作移植默认，不得在样机/原理图核验前冻结；若手册极性相反，仅对调两个宏，其余流程不变。

### init 顺序：先预置 DTR 安全电平 → 再配 GPIO → 再拉醒（`gps_uart_init`）

1. 在配置引脚前先把 DTR `gpio_set_level(GPS_DTR_SLEEP_LEVEL)`——让模组先处于休眠安全态，避免配置过程中电平毛刺；
2. 配置 DTR 为**开漏输出**（`GPIO_MODE_OUTPUT_OD`，禁用上下拉）——由 ESP32 侧开漏驱动，配合模组侧上拉；
3. `RST / NET_STATUS` 按载板能力配置为**输入 + 禁用上下拉（输入高阻）**，只读不驱动（RST 保持高阻、不做硬复位，NET_STATUS 只读观测）；M100 `GNSS_VCC` 在 EWF 载板上留 `NC/TP`，ESP32 `IO8` 不进入本初始化路径（IO8 是 `BQ_OTG_EN`，见⑧）；
4. 最后把 DTR 置为唤醒电平进入工作态。

### 运行期唤醒（`gps_modem_init` / `gps_recover_modem`）

DTR → 唤醒电平 → 等 100 ms（`GPS_MODEM_WAKE_SETTLE_MS`）→ 发 AT 同步。**任何休眠/唤醒切换都要给足 settle 时间**，不能在电平跳变后立刻发 AT。

### 进入休眠（`gps_prepare_sleep`，退出路径）

先 `AT+CSCLK=1`（允许模组在 DTR 有效时休眠），再把 DTR 置为休眠电平；随后把状态快照 `suspended=true / gnss_running=false / fix_valid=false / network_ready=false`。`AT+CSCLK=1` 超时容忍 1 s（`GPS_SLEEP_COMMAND_TIMEOUT_MS`），即使失败也继续拉 DTR 电平。

---

## ⑥ PDP / 网络注册链

单次网络轮询 = 依次查询并按需激活，任何一步失败只影响对应状态位，不中断整链（`gps_poll_network`）：

| 步骤 | AT 指令 | 期望前缀 | 解析结果 → 状态位 |
| ---: | --- | --- | --- |
| 1 | `AT+CPIN?` | `+CPIN:` | payload == `READY` → `sim_ready` |
| 2 | `AT+CSQ` | `+CSQ:` | 值 ∈ 0..31 或 99 → `csq`（99=未知） |
| 3 | `AT+CEREG?`（失败依次退 `AT+CGREG?`、`AT+CREG?`） | `+CEREG:` / `+CGREG:` / `+CREG:` | state ∈ {1,5} → `registered` |
| 4 | `AT+CGATT?`（仅 sim_ready 时） | `+CGATT:` | 值 == 1 → `attached` |
| 5 | `AT+CGACT?`（仅 attached 时） | `+CGACT: <cid>,` | cid 匹配且 state == 1 → `pdp_active` |
| 6 | `AT+CGACT=1,<cid>`（attached 且未激活时） | — | 激活，超时 15 s（`GPS_PDP_ACTIVE_TIMEOUT_MS`），随后重查 `AT+CGACT?` |
| 7 | `AT+CGPADDR=<cid>`（仅 pdp_active 时） | `+CGPADDR: <cid>` | 严格校验 4 段 0..255 且非全 0 → `ipv4_valid` + `ipv4` |

### `network_ready` 四态

```text
network_ready = sim_ready && attached && pdp_active && ipv4_valid
```

`registered` 只作为独立观测位（状态快照暴露），**不参与** `network_ready` 判定。全部为真才认为“4G 联网成功”（`ESP_LOGI` 打一次 IP 与 CSQ）；从真变假时告警「网络连接已断开」。

### 快 / 慢轮询节奏

| 场景 | 周期 | 宏 |
| --- | ---: | --- |
| 未联网 | 5 s | `GPS_NETWORK_FAST_POLL_MS` |
| 已联网 | 30 s | `GPS_NETWORK_STABLE_POLL_MS` |

后台任务按**绝对到期时间**调度（`network_due = now + pdMS_TO_TICKS(...)`），单次休眠最长 100 ms（`GPS_DRIVER_MAX_IDLE_WAIT_MS`），轮询期间同时可消费 HTTPS 队列（见⑦）。EWF 活动窗口模型可在此基线上收窄（连上即上报、上报后休眠），但“失联时快轮询、联网后慢轮询”的判据保留。

---

## ⑦ HTTPS JSON 单事务

一条事务从“确保承载”到“读完正文”由后台任务独占 UART 串行完成；队列深度固定 1，调用方互斥排队。完整序列：

### 1) 确保 SAPBR 承载（`gps_http_ensure_bearer`）

先查再开，避免周期同步重复打开已激活承载：

1. `AT+SAPBR=2,<cid>`（期望 `+SAPBR:`）→ 解析 `+SAPBR: <cid>,<state>`，cid 匹配且 state==1 直接 OK；
2. 否则 `AT+SAPBR=3,<cid>,"CONTYPE","GPRS"`；
3. `AT+SAPBR=3,<cid>,"APN","<apn>"`——APN 默认空串（`GPS_NETWORK_APN=""`，保持运营商默认空 APN）；
4. `AT+SAPBR=1,<cid>` 激活承载，超时 15 s（`GPS_HTTP_BEARER_ACTIVE_TIMEOUT_MS`）；
5. 再次 `AT+SAPBR=2,<cid>` 确认激活，否则失败。

`<cid>` = `GPS_NETWORK_CONTEXT_ID = 1`（PDP/SAPBR/HTTP 共用同一上下文编号）。

### 2) HTTP 会话初始化与参数

1. `AT+HTTPINIT`：最多重试 2 次（`GPS_HTTP_INIT_MAX_ATTEMPTS`）；**若首试失败，重试前先发 `AT+HTTPTERM` 清理 MCU 复位可能遗留的模组侧 HTTP 上下文**（该清理指令无敏感参数、允许停止中执行）。
2. `AT+HTTPSSL=1`：启用 SSL。
3. `AT+SSLCFG="seclevel",<ctx=153>,<level=0>`：
   - `GPS_HTTP_SSL_CONTEXT_ID=153` 直标 **Air780EGP** 的 SSL 上下文编号；
   - `GPS_HTTP_SSL_SECURITY_LEVEL=0` = **联调不校验 CA 证书**（仅依赖受控域名 + 共享令牌）；**上线按需调高**并预置 CA（EWF 自建项，⑪）。`gps_config.h` 编译期限制 seclevel ≤ 3。
4. `AT+HTTPPARA="CID",<cid>`、`AT+HTTPPARA="CONTENT","application/json"`。
5. `AT+HTTPPARA="URL","<url>"` —— 以 **sensitive** 方式发送（日志不落 URL，见⑨）。
6. `AT+HTTPPARA="USERDATA","X-Device-Token: <token>"` —— **sensitive**（令牌不落日志）。

### 3) 发送正文（`gps_http_send_body`）

`AT+HTTPDATA=<body_len>,10000` → 等模组回 `DOWNLOAD` → 裸写 JSON 正文 → 等 `OK`。正文按下载超时 10 s（`GPS_HTTP_DATA_DOWNLOAD_TIMEOUT_MS`）；正文内容**只记长度不落日志**。

### 4) 触发与等待结果（`AT+HTTPACTION=1`）

发 `AT+HTTPACTION=1`（1=POST），随后等异步 `+HTTPACTION:<method>,<status>,<len>`，**超时 60 s**（`GPS_HTTP_ACTION_TIMEOUT_MS`）；method 必须为 1，否则按响应错误处理。`<status>` 即 HTTP 状态码。

### 5) 读正文（`gps_http_read_body`）

- `data_length == 0` → 空正文，直接成功。
- 否则先做**容量边界检查**：正文长 ≥ 模组内响应缓冲（`GPS_HTTPS_RESPONSE_MAX_LEN=3356`）或调用方缓冲 → `ESP_ERR_INVALID_SIZE`，不读。
- `AT+HTTPREAD=0,<len>` → 等 `+HTTPREAD: <announced_len>` 且必须与声明长度一致 → 裸读 `<len>` 字节（**不落日志**）→ 等 `OK`。读正文超时 10 s（`GPS_HTTP_READ_TIMEOUT_MS`），单次 UART 轮询上限 50 ms。

### 6) 收尾

无论成败，若 `http_initialized` 则发 `AT+HTTPTERM` 释放模组 HTTP 上下文。

### 串行与整笔 deadline

- `GPS_HTTP_QUEUE_DEPTH=1`：队列只存一条请求，后台任务一次消费一条；调用方先取 `http_mutex`（等待时间计入整笔 deadline），再把请求复制入队、等完成信号。`gps_config.h` 编译期强制队列深度 ==1，确保 UART 单事务所有权。
- **整笔 deadline**：`gps_https_post_json(url, ..., timeout_ms)` 的 `timeout_ms` 是**从等待互斥、排队到取得结果的绝对截止时间**（内部用 `esp_timer` 截止时间换算剩余量，各步骤共享）。main_control 应用侧对整笔 HTTPS 调用的预算为 **65 s**（`SCENIC_HTTP_OPERATION_TIMEOUT_MS=65000`）：HTTPACTION 本身即 60 s，必须给 bearer 确认 / INIT / 参数 / DATA / READ 串行开销留余量。**EWF 上报调用应沿用 ≥65 s 的单笔截止预算**（DEFERRED 真机校准）。
- 调用方缓冲上限：URL 500 / token 256 / body 4096 / 响应 3356（调用方响应缓冲由调用者提供，内部再以 3356 为上限复核）。

### 敏感内容不落正文

URL、`X-Device-Token` 头、请求正文、响应正文均走脱敏分支：日志只出现「发送敏感 AT 事务，命令内容已隐藏」「正文内容已隐藏，长度=N」。`device_token` 与 `json_body` 在源实现中**只复制进私有请求副本，从不写入日志**（见⑨）。

---

## ⑧ GNSS 默认关与载板 NC 处理

- main_control 用产品宏把 GNSS 整段编译隔离（`GPS_SCENIC_ENABLE`）：不启用时**不编译、不发送任何 `AT+CGNS*` 指令**，不执行 GNSS 轮询，`status.gnss_running` 恒为 false；启用时才发 GNSS 指令。EWF MVP 保持默认关闭，不建立 GPS 业务链路。
- **EWF 板级差异（以硬件 spec 为准）**：原 main_control 的 `GPIO8=GNSS_VCC` 只作历史交叉核对；EWF 已将 `IO8` 释放为 `BQ_OTG_EN`，连接 BQ25895 OTG 脚。M100 载板的 `GNSS_VCC` 不接 ESP32，落图为 `M100_GNSS_VCC_NC`（NC/测试点），不得驱动、拉低或接入业务。
- `NET_STATUS` 在 EWF 保留 `IO16=M100_NET_STATUS_COMPAT` 兼容逻辑位；若具体 M100 载板没有该针脚，IO16 只放测试点/NC，不把它当联网必需条件。M100 `RST=IO15` 仍保留载板交叉表，方向/极性待实测。
- 解析参考（EWF 若日后启用 GPS 再迁移）：`AT+CGNSINF` payload 逗号切 24 字段，≥16 字段有效；run_status==1 && fix_status==1 且经纬度在合法范围才判有效定位；纬度/经度/UTC/海拔/速度/航向/定位模式/可见星/使用星按固定下标取值。

---

## ⑨ 日志防泄露

source 实现约定（EWF 必须沿用，违反即视为泄露缺陷）：

1. **敏感 AT 事务**（`gps_send_at_internal(..., sensitive=true)`）：
   - 发送日志替换为「发送敏感 AT 事务，命令内容已隐藏」，**不回显指令原文**；
   - 响应行**不逐行打印**；
   - 超时错误文本同样隐藏命令（「敏感AT事务超时，命令内容已隐藏」），不以任何形式带出前 N 字符。
2. 应用哪些命令走 sensitive：`AT+HTTPPARA="URL",…`、`AT+HTTPPARA="USERDATA","X-Device-Token: …"`、HTTPDATA 正文写入、HTTPREAD 正文读取。正文路径日志只带长度（`长度=%u`）。
3. 普通 AT 响应（CSQ/注册/PDP/型号等）可正常 `ESP_LOGI`，但**任何含 token / Authorization / 请求体 / 完整 URL 的分支都必须走脱敏路径**。
4. 错误状态只在变化时打印一次（`gps_set_error` 先比对再 `ESP_LOGE`），避免错误风暴。

---

## ⑩ EWF 参数基线表（抄自 gps_config.h，逐行 DEFERRED）

> 下表数值 = `main_control/components/BSP/GPS/gps_config.h` 原值，供 EWF 复制为基线。**每项均标 `DEFERRED`：待 EWF 真机/原理图核验后才可冻结**，不得从编译成功推导可靠。引脚不在此表（引脚唯一权威 = spine §板级合同，见 ⑪ 表）。GNSS 相关行 EWF 默认关闭、保留待用。

### UART 与任务

| 参数宏 | 值 | 含义 | 状态 |
| --- | ---: | --- | --- |
| `GPS_UART_PORT` | `UART_NUM_1` | 4G 模组独占 UART | DEFERRED |
| `GPS_UART_BAUD_RATE` | 115200 | 会话目标波特率 | DEFERRED |
| `GPS_UART_FALLBACK_BAUD_RATE_1..4` | 57600 / 9600 / 19200 / 38400 | 标准回退 | DEFERRED |
| `GPS_UART_FALLBACK_BAUD_RATE_5..7` | 230400 / 460800 / 921600 | 高速回退 | DEFERRED |
| `GPS_UART_SYNC_ATTEMPTS_PER_BAUD` | 3 | 每档大写 `AT` 次数上限 | DEFERRED |
| `GPS_UART_RX_BUFFER_SIZE` | 4096 | UART RX 缓冲 | DEFERRED |
| `GPS_AT_LINE_MAX_LEN` | 512 | 单行 AT 响应上限 | DEFERRED |
| `GPS_TASK_STACK_SIZE` | 12 KB | 后台驱动任务栈 | DEFERRED |
| `GPS_TASK_USE_STATIC_INTERNAL_STACK` | 1 | 静态内部 DRAM 栈 | DEFERRED |
| `GPS_TASK_PRIORITY` | 2 | 任务优先级 | DEFERRED |
| `GPS_TASK_CORE_ID` | 0 | 任务固定核心 | DEFERRED |
| `GPS_UART_TX_TIMEOUT_MS` | 100 | UART 发送完成等待 | DEFERRED |
| `GPS_API_MUTEX_TIMEOUT_MS` | 100 | 状态互斥等待 | DEFERRED |
| `GPS_UART_RECONFIG_SETTLE_MS` | 50 | UART 重挂稳定等待 | DEFERRED |
| `GPS_UART_READ_POLL_MAX_MS` | 50 | 逐字节读单次轮询上限 | DEFERRED |

### 状态 / 缓冲容量

| 参数宏 | 值 | 含义 | 状态 |
| --- | ---: | --- | --- |
| `GPS_MODULE_MODEL_MAX_LEN` | 32 | 型号串长度 | DEFERRED |
| `GPS_FIRMWARE_VERSION_MAX_LEN` | 64 | 固件版本串长度 | DEFERRED |
| `GPS_LAST_ERROR_MAX_LEN` | 96 | 最近错误串长度 | DEFERRED |
| `GPS_UTC_MAX_LEN` | 24 | GNSS UTC 串长度 | DEFERRED |
| `GPS_IPV4_MAX_LEN` | 16 | IPv4 串长度 | DEFERRED |
| `GPS_HTTPS_RESPONSE_MAX_LEN` | 3356 | HTTPS 响应正文上限 | DEFERRED |

### AT / 网络 / GNSS 时序

| 参数宏 | 值 | 含义 | 状态 |
| --- | ---: | --- | --- |
| `GPS_AT_TIMEOUT_MS` | 1500 | 普通 AT 超时 | DEFERRED |
| `GPS_AT_SYNC_TIMEOUT_MS` | 800 | 基础同步 AT 超时 | DEFERRED |
| `GPS_AT_SYNC_RETRY_INTERVAL_MS` | 300 | 同步 AT 重试间隔 | DEFERRED |
| `GPS_MODEM_WAKE_SETTLE_MS` | 100 | DTR 唤醒后 settle | DEFERRED |
| `GPS_PDP_ACTIVE_TIMEOUT_MS` | 15000 | PDP 激活超时 | DEFERRED |
| `GPS_NETWORK_FAST_POLL_MS` | 5000 | 未联网轮询周期 | DEFERRED |
| `GPS_NETWORK_STABLE_POLL_MS` | 30000 | 联网后轮询周期 | DEFERRED |
| `GPS_FIX_FAST_POLL_MS` | 1000 | GNSS 首定前轮询 | DEFERRED（GNSS 默认关） |
| `GPS_FIX_STABLE_POLL_MS` | 5000 | GNSS 已定位轮询 | DEFERRED（GNSS 默认关） |
| `GPS_GNSS_AID_TIMEOUT_MS` | 5000 | GNSS 辅助指令超时 | DEFERRED（GNSS 默认关） |
| `GPS_SUSPEND_TIMEOUT_MS` | 3000 | 驱动停止等待 | DEFERRED |
| `GPS_SUSPEND_POLL_INTERVAL_MS` | 20 | 停止状态轮询周期 | DEFERRED |
| `GPS_SLEEP_COMMAND_TIMEOUT_MS` | 1000 | 休眠准备 AT 超时 | DEFERRED |
| `GPS_TIMEOUT_RECOVERY_COUNT` | 3 | 连续超时恢复阈值 | DEFERRED |
| `GPS_DRIVER_MAX_IDLE_WAIT_MS` | 100 | 后台任务单次最长休眠 | DEFERRED |

### 恢复退避

| 参数宏 | 值 | 含义 | 状态 |
| --- | ---: | --- | --- |
| `GPS_RECOVERY_BACKOFF_1_MS` | 5000 | 第一轮退避 | DEFERRED |
| `GPS_RECOVERY_BACKOFF_2_MS` | 15000 | 第二轮退避 | DEFERRED |
| `GPS_RECOVERY_BACKOFF_3_MS` | 30000 | 第三轮退避 | DEFERRED |
| `GPS_RECOVERY_BACKOFF_4_MS` | 60000 | 后续退避上限 | DEFERRED |

（编译期 `#error`：首档>0、后档≥前档，单调不减——EWF 复制时保留。）

### PDP 与 HTTPS 事务

| 参数宏 | 值 | 含义 | 状态 |
| --- | ---: | --- | --- |
| `GPS_NETWORK_CONTEXT_ID` | 1 | PDP/SAPBR/HTTP 共用上下文 | DEFERRED |
| `GPS_NETWORK_APN` | `""` | 运营商 APN（空=默认） | DEFERRED |
| `GPS_NETWORK_APN_MAX_LEN` | 64 | APN 最大长度 | DEFERRED |
| `GPS_NETWORK_COMMAND_MAX_LEN` | 128 | 网络 AT 命令缓冲 | DEFERRED |
| `GPS_HTTP_SSL_CONTEXT_ID` | **153** | Air780EGP SSL 上下文编号 | DEFERRED |
| `GPS_HTTP_SSL_SECURITY_LEVEL` | 0 | SSL 等级（联调 0；上线按需） | DEFERRED |
| `GPS_HTTP_COMMAND_TIMEOUT_MS` | 3000 | HTTP 普通 AT 超时 | DEFERRED |
| `GPS_HTTP_BEARER_ACTIVE_TIMEOUT_MS` | 15000 | SAPBR 承载激活超时 | DEFERRED |
| `GPS_HTTP_DATA_DOWNLOAD_TIMEOUT_MS` | 10000 | HTTPDATA DOWNLOAD/正文确认超时 | DEFERRED |
| `GPS_HTTP_ACTION_TIMEOUT_MS` | 60000 | HTTPACTION 异步结果超时 | DEFERRED |
| `GPS_HTTP_INIT_MAX_ATTEMPTS` | 2 | HTTPINIT 最大尝试 | DEFERRED |
| `GPS_HTTP_READ_TIMEOUT_MS` | 10000 | HTTPREAD 读正文超时 | DEFERRED |
| `GPS_HTTP_READ_POLL_MAX_MS` | 50 | HTTPREAD 单次轮询上限 | DEFERRED |
| `GPS_HTTP_QUEUE_DEPTH` | 1 | HTTPS 队列深度（强制 1） | DEFERRED |
| `GPS_HTTP_BODY_MAX_LEN` | 4096 | 请求正文上限 | DEFERRED |
| `GPS_HTTP_URL_MAX_LEN` | 500 | URL 上限 | DEFERRED |
| `GPS_HTTP_TOKEN_MAX_LEN` | 256 | 设备令牌上限 | DEFERRED |
| `GPS_HTTP_AT_COMMAND_MAX_LEN` | 640 | URL/请求头 AT 命令缓冲 | DEFERRED |

（编译期 `#error`：队列深度必须==1；`ACTION_TIMEOUT > COMMAND_TIMEOUT`；缓冲区容量链约束；seclevel ≤ 3。）

### 模组控制电平（DTR）

| 参数宏 | 值 | 含义 | 状态 |
| --- | ---: | --- | --- |
| `GPS_DTR_AWAKE_LEVEL` | 0 | 唤醒有效电平 | DEFERRED（按整模组手册复核） |
| `GPS_DTR_SLEEP_LEVEL` | 1 | 休眠有效电平 | DEFERRED（按整模组手册复核） |

（编译期 `#error`：只能 0/1 且唤醒/休眠必须相反。）

---

## ⑪ EWF 自建项（main_control 未覆盖，需 EWF 自行立项闭环）

| 自建项 | 说明 | 依据 / 门禁 |
| --- | --- | --- |
| **发射峰值 / 掉压** | Air780EGP 高电流电池轨（持续 >1 A / 瞬时 >2 A 能力）。4G 发射峰值时电池轨不得掉压到触发 ESP32-S3 重启；需实测发射瞬态（弱网满功率发射 + 最大占空比上报场景）。 | spine AD-13 / §板级合同「电源分轨」；工程验证门禁「发射峰值电池轨无掉压重启」 |
| **SIM 实名 / 资费** | HTTPS 需要 SIM 已实名且套餐含数据流量；停机/欠费/定向流量不足的表现要纳入 AT 恢复与上报失败语义验证。 | EWF 运营商选型自建；main_control 无覆盖 |
| **载板控制脚 / 天线** | ① DTR（IO10）有效极性、开漏驱动是否与 Air780EGP 整模组手册一致（对调 AWAKE/SLEEP 两宏即可适配，见⑤）；② M100 RST（IO15）方向/极性、NET_STATUS（IO16）是否有载板针脚；③ GNSS_VCC 保持 NC/TP，IO8 已用于 BQ_OTG_EN；④ 天线选型/布线/天线检测与弱网 CSQ。 | 整模组/载板手册 + 原理图核验后才冻结 |

---

## ⑫ 参考源代码位置清单

### main_control（只读蓝本）

- `components/BSP/GPS/gps.c`：
  - `gps_uart_init` —— DTR 预置/开漏配置、只读引脚输入高阻、UART 安装与重挂；
  - `gps_uart_set_baud_rate` —— 不写 NVRAM 的 ESP32 侧切速；
  - `gps_send_at_internal` / `gps_send_at` / `gps_http_send_body` / `gps_http_read_body` —— sensitive 发送/读正文、超时与错误语义；
  - `gps_sync_modem` / `gps_normalize_modem_baud_rate` —— 波特率握手与归一化；
  - `gps_http_ensure_bearer` / `gps_http_wait_action` / `gps_execute_https_post` / `gps_process_http_queue` —— HTTPS 单事务序列；
  - `gps_poll_network` / `gps_parse_registration` / `gps_parse_pdp_state` / `gps_parse_ipv4` —— 网络注册链；
  - `gps_poll_gnss` / `gps_parse_gnss_info`（`GPS_SCENIC_ENABLE` 门内）—— GNSS（EWF 默认关）；
  - `gps_read_module_info` / `gps_modem_init` / `gps_clear_live_status` / `gps_recover_modem` / `gps_prepare_sleep` / `gps_driver_task` —— 初始化、恢复、休眠、退避调度；
  - `gps_start` / `gps_get_status` / `gps_https_post_json` / `gps_suspend` —— 公开 API。
- `components/BSP/GPS/gps_config.h` —— 全部数值宏与编译期 `#error` 校验（§⑩ 抄录源）。
- `components/BSP/GPS/gps.h` —— 引脚（EWF 不复制）+ `gps_status_t` / `gps_https_result_t` 类型 + API 声明。
- `main/app_scenic_management/app_scenic_config.h` —— `SCENIC_HTTP_OPERATION_TIMEOUT_MS=65000`（整笔 HTTPS 截止预算出处）。
- `docs/architecture/硬件架构与软件框架说明.md` §9.3 —— 波特率扫描/引脚/能力/后台任务/HTTPS 联调配置浓缩。
- `docs/Function/设备功能全景文档.md` §20.1 —— 仅作历史 GNSS 默认关闭/不编译 `AT+CGNS*` 语义交叉核对；EWF IO8/载板 NC 口径以硬件基线为准（景区业务语义不迁）。

### EWF 落地目标

- 本文件：`docs/embedded/4g/Air780EGP-AT联网与HTTPS经验.md`（§①-⑫）。
- 兄弟排障：`docs/embedded/4g/troubleshooting/Air780EGP-AT零响应与恢复.md`。
- 固件实现归 story E4.1/E4.2；引脚/电源权威 = `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` §板级合同；逐网/电源/载板交叉表见 `docs/hardware/电子木鱼-硬件网络清单.json`。
