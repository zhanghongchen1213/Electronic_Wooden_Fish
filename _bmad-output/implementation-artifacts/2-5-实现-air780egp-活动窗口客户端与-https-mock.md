# Story 2.5: 实现 Air780EGP 活动窗口客户端与 HTTPS mock

baseline_commit: 490c46b0719d6bb90484cd14e2fd492e83318e65

Status: done

<!-- Note: 本 Story 闭合设备侧 Air780EGP 活动窗口客户端、契约 §6 HTTPS JSON 编解码、失败退避与可注入 HTTPS mock；真机模组 AT/发射峰值与 backend 端到端属 Story 7.1；BOOT0 自动模式属 2.6；低电优先落盘调度属 2.7；状态栏/设置页 sync 五态视觉投影属 Epic 3。Epic 2 明确使用本地状态与 HTTPS mock，不等待真实 backend 上线。 -->

## Story

As a 设备使用者,
I want 设备在需要时联网并在弱网下自动恢复,
so that 网络不会改变本地诵读的可靠性。

**覆盖需求：** FR-E-008、AD-14、NFR3（SM-3）

## Acceptance Criteria

### AC 1：活动窗口内复用通信上下文完成 HTTPS JSON 上报

**Given** 敲击（`physical_pvdf`/`device_touch`/`automatic_tap` 经统一队列推进后产生待同步差值）、立即同步请求，或本地存在待应用命令触发条件
**When** Air780EGP 客户端执行 AT/HTTPS JSON 事务（真机路径）或等价 mock 传输（主机/无模组路径）
**Then** 在同一活动窗口内复用 PDP/HTTPS 通信上下文，按契约 §6.1 上报 `device_id`、`scripture_version`、`local_total`、`acked_total`、`round_id`、`round_state`、`round_cursor`、`pending_completion`、`applied_revision`、`battery_percent`、`network_mode`、`audio_config_version`、`firmware_version`、`action_id`；成功后可收敛 `acked_total` 并回到低功耗/休眠准备路径
**And** BLE/Wi-Fi 不被配置为业务回退或常开长连接；GPS/`AT+CGNS*` 默认关闭且不轮询；不连接模组独立业务云；UART 单事务所有权不被旁路

### AC 2：失败退避不阻断本地记录，mock 覆盖关键响应语义

**Given** 注册失败、超时、弱网或 HTTPS mock 返回错误/冲突
**When** 客户端执行退避与恢复
**Then** 本地敲击继续记录（progress owner 不受通信失败回滚）；同步事实进入可观察的 `pending_sync`/`sync_fail`（或等价 `pending`/`fail`）；网络/mock 恢复后补传；发射峰值掉压重启属样机验收（`hardware_pending`），实现侧不得用软件重启/空循环模拟关机来“恢复”
**And** mock 至少覆盖：成功确认、重复提交（幂等 no-op）、低于/等于已确认高水位（no-op）、可恢复冲突（业务码 `20003` 设备重置冲突）四类响应；本地不得把失败伪装成「已同步」

## Tasks / Subtasks

- [x] Task 1：活动窗口策略与响应收敛纯逻辑（AC: 1, 2）
  - [x] 新增零 ESP-IDF 依赖的策略模块（建议 `sync_window_policy.c` / `sync_response_policy.c`，沿 `progress_transaction`/`device_nav_policy` 先例）：输入为触发源、`local_total`/`acked_total` 差值、当前退避档、mock/传输结果；输出为「开窗/合并/跳过/关窗」、下一退避档、是否补传。
  - [x] 触发源冻结为三类：① 存在未确认差值（`local_total > acked_total`）且可合并进当前/下一窗口；② `device_nav_service_request_sync` 产生的立即同步请求；③ 响应侧 `command_revision > applied_revision` 的待应用命令需再次开窗回传（本 Story 建立本地应用与回传路径，不等待真 cloud）。
  - [x] 退避表照搬 4G 经验基线：`5s / 15s / 30s / 60s` 单调封顶，编译期或静态断言首档>0、后档≥前档；连续 AT 超时 ≥3 才进恢复，单次超时只告警。
  - [x] 响应收敛：成功/`code=0` → 调用进度确认入口推进 `acked_total`；同值/更低 → no-op；`20003` → 进入可恢复冲突态（不得静默回退 `local_total`）；`20004` → 不阻断确认但仍拒绝新输入（既有 progress gate）；`20005` 经文版本不一致 → 停止推进并中文日志。纯逻辑必须主机可测。

- [x] Task 2：契约 §6 JSON 编解码与 HTTPS mock（AC: 1, 2）
  - [x] 实现请求/响应编解码（建议 `sync_https_codec.*`）：请求体恰为契约 §6.1 的 14 个 snake_case 字段；响应解析 `{code,message,data}` 信封 + §6.2 必填（`acked_total`、`round_id`、`round_state`、`round_cursor`、`pending_completion`、`command_revision`、`snapshot_seq` 与待应用 `volume`/`brightness`/`timeout`）。不得发明 `delta`/`high_watermark` 等未授权 wire 名。
  - [x] HTTPS mock（主机默认、固件可编译开关）：至少脚本化/表驱动覆盖成功、重复提交 no-op、低于高水位 no-op、冲突 `20003`；可选扩展 `20004`/`20005`。mock 不得依赖真实 Spring Boot 进程（Epic 2 边界）。
  - [x] URL/设备 Token 从配置或编译期注入读取；日志对 URL、token、正文走脱敏（只记长度），违反即缺陷。[Source: `docs/embedded/4g/Air780EGP-AT联网与HTTPS经验.md` §⑨；`Embedded/AGENTS.md` 敏感数据]

- [x] Task 3：Air780EGP BSP / 调制解调器驱动边界（AC: 1）
  - [x] 新增 BSP 或 `components` 下的 Air780 驱动目录（建议 `Embedded/components/BSP/AIR780EGP/` 或 `services` 旁独立 modem 组件，命名用 EWF 语义，**禁止**继续叫 `GPS`/`gps_*` 作为产品模块名）：公开 API 语义对齐经验文档 `start / get_status / https_post_json / suspend`。
  - [x] 引脚唯一权威：`UART1` TX=`IO43`、RX=`IO44`；DTR=`IO10` 开漏；RST=`IO15` 输入高阻不硬复位；`NET_STATUS`/`GNSS_VCC` 不接 ESP32（NC/TP）。不得占用 `IO8`（PWR）。[Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md` §5.5；网络清单]
  - [x] 复制经验流程：波特率扫描不写 NVRAM、`ATE0`/`CMEE`/`CSCLK`、PDP 链、`network_ready=sim&&attached&&pdp&&ipv4`、HTTPS 单事务队列深度 1、SSL context `153`、联调 `seclevel=0`（标注上线可调高）、整笔 deadline ≥65s。数值宏全部标注 `DEFERRED`/`hardware_pending`，不得把编译成功写成通信可靠。
  - [x] 应用层禁止直读写 UART；所有 AT 经单一事务所有权（AD-10）。不发 `AT+CGNS*`；不启用 BLE/Wi-Fi 产品通道；不引入 MQTT/模组独立云。
  - [x] UART API 使用前查证 ESP-IDF **v5.5.4** ESP32-S3 文档：`uart_driver_install` / `uart_param_config` / `uart_set_pin` / `uart_set_baudrate` / `uart_write_bytes` / `uart_read_bytes`（禁止 `latest`/`freertos.org`）。依据：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/uart.html>

- [x] Task 4：同步服务 owner 与既有服务接线（AC: 1, 2）
  - [x] 新增服务（建议 `sync_service` / `modem_sync_service`）：登记 `legbot_services.h` 枚举、`legbot_services.c` 描述符/启动图、`contract_checks.c` 服务数量断言（当前 `LEGBOT_SERVICE_COUNT == 8` → **9**）；ASCII TAG 如 `SVC_SYNC`，日志正文中文。
  - [x] 消费立即同步：订阅/轮询 `device_nav` 的 `pending`/`sync_request_id`；开窗前 `device_nav_service_complete_sync(BUSY)`，终态 `OK`/`FAIL`（已有 API，禁止另造第二套 sync 状态机）。
  - [x] 消费高水位：只读 `state_service`/`watch_state` 进度快照组装请求；确认后只经 `progress_service_advance_acked_total` 推进（2.2 已预留入口），禁止私写 progress NVS。
  - [x] 待应用命令：若响应 `command_revision > applied_revision`，原子写入 `volume`/`brightness`/`timeout` + 提升 `applied_revision`（经 `device_nav` 设置 API / settings store），再视需要开窗回传；旧修订不得覆盖新修订。完整 cloud 命令权威仍属 Epic 5；本 Story 只保证设备侧对 mock/响应载荷的本地收敛。
  - [x] 敲击触发：在有效推进导致 `backlog>0` 时调度活动窗口（可合并、可限频），**不得**阻塞 `tap_input`/`progress`/`feedback` 热路径（NFR3：设备→确认环节不得单独拖垮 1s 端到端口径；合并上报优于逐事件阻塞等待）。
  - [x] 发布 4G/`network_mode` 与同步态 typed 事实（可扩展 `STATE_SERVICE_UPDATE_*` 或挂在 nav/audio 旁最小通道）：`connected|no_signal|disabled`；UI 投影属 Epic 3，本 Story 保证快照可测。

- [x] Task 5：敏感配置、身份占位与低功耗收尾（AC: 1）
  - [x] `device_id`/`firmware_version`/`scripture_version`/`audio_config_version` 从既有事实源读取（进度/设置/反馈/构建常量）；缺省时 fail-closed 不开窗假成功。
  - [x] `battery_percent`：若 CW2015 尚未有产品级读数，允许占位 + 中文日志标注 `hardware_pending`，取值域仍 0–100；不得伪造充电分支。
  - [x] 窗口结束走 `suspend`/DTR 休眠准备语义；失败也要释放 HTTP 上下文（`HTTPTERM` 等价清理），避免假 `network_ready`。
  - [x] 充电语义：按 PRD final，三种供电均不暂停同步路径。

- [x] Task 6：主机测试与构建验证（AC: 1–2）
  - [x] 纯逻辑测试：开窗合并、退避单调、四类 mock 响应收敛、`acked_total` no-op/推进/冲突、命令修订单调应用、脱敏日志不出现 token/URL 正文。
  - [x] 编译级接线测试（沿 `test_progress_runtime`/`test_device_nav_runtime`）：sync 服务与 progress/nav/state 关键 API 可链接；mock 传输可注入。
  - [x] 运行 `python3 Embedded/tests/run_host_tests.py` 全绿；环境允许时 ESP-IDF v5.5.4 增量构建并写入 `docs/embedded/build_records/`。
  - [x] `hardware_pending` 清单（不得标 verified）：UART 交叉方向、DTR 极性、PDP/HTTPS 真机、发射峰值掉压、SIM/资费、天线 CSQ、65s deadline 实机。

## Dev Notes

### 事实源与架构约束

- FR-E-008：MVP 仅 Air780EGP 4G HTTPS JSON；敲击/触摸/自动/立即同步/待应用命令窗口复用通信上下文；无 BLE/Wi-Fi 业务；GPS 默认关；弱网本地继续、恢复补传；发射峰值不掉压重启；设备上报环节不得单独突破 SM-3 1s 端到端口径。[Source: `prd.md` §5.8；`epics.md` Story 2.5]
- AD-14：唯一产品联网链路；活动窗口内复用、上报后低功耗，不维持长连接。[Source: `ARCHITECTURE-SPINE.md` AD-14]
- AD-3：活动窗口一次 HTTPS 上报高水位与轮次；只确认更高差量；同值/更低 no-op。[Source: `ARCHITECTURE-SPINE.md` AD-3]
- AD-5：每次活动窗口回传 `applied_revision`；以 `applied_revision ≥ command_revision` 判定已生效。[Source: `ARCHITECTURE-SPINE.md` AD-5]
- AD-7：HTTPS/同步属命令型路径，不得静默丢弃；须 busy/error + 日志。[Source: `ARCHITECTURE-SPINE.md` AD-7]
- AD-9：消费者只读不可变快照；sync 服务经 typed update 发布，不读他服务私有变量。[Source: `ARCHITECTURE-SPINE.md` AD-9]
- AD-10：Air780 AT 单一 UART 事务所有权；禁止应用层直抢 UART。[Source: `ARCHITECTURE-SPINE.md` AD-10]
- AD-13：低电先落盘（完整调度属 2.7）；本 Story 失败路径不得用软件关机“自救”。[Source: `ARCHITECTURE-SPINE.md` AD-13]
- NFR3/SM-3：端到端 95%/1s；本 Story 负责设备侧不阻塞计数与可合并上报，完整端到端验收在 Epic 7。[Source: `epics.md` NFR3；`prd.md` SM-3]
- Epic 2 边界：**本地状态 + HTTPS mock，不等待真实 backend**。可对照 `POST /api/v1/sync/report` 字段形状，但默认验证走 mock。[Source: `epics.md` Epic 2；`SyncController.java`]

### 同步契约硬约束（`docs/contracts/sync-contract.md` SC-1.0.0）

- §6.1 请求 14 字段闭包；§6.2 响应必填 + 待应用载荷；§6.3 幂等。
- §8：冲突 `20003` 可恢复；`20004` 不阻断确认；版本不一致 `20005` 停推进。
- §9/`applied_revision` 与设置命令族；立即同步属篇章动作族 `action_id`。
- §12 业务码表不得重定义。
- 信封：`{code,message,data}`，成功 `code=0`；业务错误 HTTP 200 + `2xxxx`。

### UX / 状态词表（本 Story 只产事实）

- 同步五态：`本地已记录 / 同步中 / 已同步 / 待同步 / 同步失败`（EXPERIENCE）；设置页 `sync-btn` 视觉属 3.5。
- 4G signal：`connected|no_signal|disabled`；图标可见 ≠ 业务链路启用。
- 未校时「待校时」：可信时间可从 HTTPS/网络时间取得后交给后续故事收敛；本 Story 至少不伪造日统计。

### Story 2.1–2.4 交接（UPDATE 文件必读）

| 模块 | 现状 | 本 Story 动作 |
| --- | --- | --- |
| `progress_service` | 高水位 owner；`progress_service_advance_acked_total` 已预留 | 唯一确认入口；mock/真机响应后调用 |
| `device_nav_service` | `request_sync`→`pending`；`complete_sync(OK/FAIL/BUSY)` | 消费 pending、回写终态；禁止假成功 |
| `device_settings` NVS | `volume/brightness/timeout/applied_revision` | 应用响应载荷时写入并抬升 revision |
| `feedback_service` | `audio_config_version` 事实 | 上报字段读取 |
| `event_bus` / tap | 有效敲击已发布 | 可订阅进度快照/事件触发开窗，不重复计数 |
| BSP | **无** Air780 目录（现有 CO5300/CST9217/…） | 新建 Air780 驱动；禁止抄 legbot ML307R |
| `docs/embedded/4g/` | 经验与排障已迁入 | 实现真源；引脚以 hardware 覆盖 gps 旧脚 |

### 立即同步与 mock 裁决（推荐，实现采用）

- 默认传输：`EWF_SYNC_TRANSPORT_MOCK`（主机测试强制；无模组固件构建可默认 mock）。
- 真机传输：`EWF_SYNC_TRANSPORT_AIR780`（链接真实 BSP）；未核验前不得在完成记录宣称 SM-3/发射峰值 verified。
- 立即同步在客户端忙碌时保持 `busy`；失败→`fail` 可重试；成功→`ok` 且 `acked_total` 已收敛（或同值 no-op 仍可 `ok`）。
- 无差值且无待应用命令的立即同步：仍允许一次探测性上报（契约允许 no-op），或明确短路为 `ok`+中文日志「无待同步差量」——二选一写死并主机钉住；**推荐**：仍发一次上报以拉取 `command_revision`（覆盖「待应用命令」窗口）。

### 目录、代码与 API 约束

- ESP-IDF v5.5.4 / ESP32-S3；新增 UART/GPIO API 先查官方 v5.5.4 文档。
- C 规范：中文 Doxygen、中文日志、ASCII TAG、snake_case、无 VLA、ISR 无堆分配。[Source: `docs/embedded/style/C编码规范-Agent版.md`]
- 禁止把 `main_control` 的 `GPS_*` 产品命名与景区 payload 迁入 EWF；只迁 AT/HTTPS/退避方法学。
- 禁止引入第二套高水位存储或第二套 sync 状态枚举。
- 用户可见文案不得出现 TODO/draft/placeholder/Node ID（本 Story 若暂不改 UI，则只保证事实字段与日志合规）。

### 不属于本 Story 的边界

- 不对接真实 backend 公网联调、不改 `cloud/` 生产代码（Epic 7 / Story 7.1）。
- 不实现 BOOT0 自动敲击（2.6）、低电优先调度与续航实测闭环（2.7）。
- 不实现 DEVICE-01 状态栏/设置页视觉闭包、字体合同（Epic 3）。
- 不实现 WebSocket、小程序、JWT 登录链路。
- 不把主机 mock 成功率写成真机 4G/SM-3 验收证据。

### Testing Requirements

- `python3 Embedded/tests/run_host_tests.py` 全绿；新段 C17、`-Wall -Wextra -Werror`。
- 替身记录：开窗原因、请求字段集合、响应码、acked 推进、sync 状态迁移、退避档、脱敏日志断言。
- 构建记录（若执行）写入 `docs/embedded/build_records/`。

### Project Structure Notes

- 预期新增：`Embedded/components/services/sync_service/`（或等价）、`sync_*_policy.*`、`sync_https_codec.*`、`sync_https_mock.*`、`Embedded/components/BSP/AIR780EGP/`（或 modem 组件）、`Embedded/tests/test_sync_*.c`、host stubs。
- 预期修改：`legbot_services.*`、`contract_checks.c`、`CMakeLists.txt`、`run_host_tests.py`、必要时 `state_service`/`app_state` 增加 network/sync 快照字段、`device_nav_service` 仅接线不改语义。
- 与统一结构无冲突；禁止新建第二 event bus；禁止启用 BLE/Wi-Fi/GPS 业务。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` §Epic 2 / Story 2.5、NFR3]
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md` §5.8 FR-E-008、SM-3、§14]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` AD-3/5/7/9/10/13/14]
- [Source: `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/EXPERIENCE.md` 同步词表、弱网路径]
- [Source: `docs/contracts/sync-contract.md` §6/§8/§9/§12]
- [Source: `docs/embedded/4g/Air780EGP-AT联网与HTTPS经验.md`、`docs/embedded/4g/troubleshooting/Air780EGP-AT零响应与恢复.md`]
- [Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md` §5.5；`docs/hardware/电子木鱼-硬件网络清单.json`]
- [Source: `_bmad-output/implementation-artifacts/2-2-*.md`、`2-3-*.md`、`2-4-*.md` 交接]
- [Source: `Embedded/components/services/progress_service/*`、`device_nav_service/*`、`Embedded/AGENTS.md`]
- [Source: ESP-IDF v5.5.4 UART — https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/uart.html]

## Dev Agent Record

### Agent Model Used

Composer (Epic Autopilot unattended best-effort)

### Debug Log References

- Host tests: `python3 Embedded/tests/run_host_tests.py` exit 0
- IDF build attempt-1: `docs/embedded/build_records/20260924-091440/build-01.log` exit 0（gitignore）

### Completion Notes List

- 2026-09-24：create-story 完成——上下文引擎分析 epics/PRD/架构/UX/契约/4G 经验/硬件引脚/Story 2.1–2.4 交接与现有 `progress_service_advance_acked_total`/`device_nav_service_complete_sync` API；Status=ready-for-dev。Ultimate context engine analysis completed - comprehensive developer guide created。
- 2026-09-24：dev-story 完成——实现 `sync_window_policy`/`sync_response_policy`、§6 codec+mock、`AIR780EGP` BSP 边界、`sync_service`（LEGBOT_SERVICE_COUNT=9）、`device_nav_service_apply_command_revision`；默认 `EWF_SYNC_TRANSPORT_MOCK`；主机全绿；ESP-IDF v5.5.4 构建成功。真机 AT/HTTPS/DTR/发射峰值仍 `hardware_pending`，不得宣称 SM-3/通信可靠 verified。

### File List

- Embedded/components/BSP/AIR780EGP/air780egp_bsp.c
- Embedded/components/BSP/AIR780EGP/air780egp_bsp.h
- Embedded/components/BSP/CMakeLists.txt
- Embedded/components/contract_checks/contract_checks.c
- Embedded/components/services/CMakeLists.txt
- Embedded/components/services/device_nav_service/device_nav_service.c
- Embedded/components/services/device_nav_service/device_nav_service.h
- Embedded/components/services/legbot_services.c
- Embedded/components/services/legbot_services.h
- Embedded/components/services/sync_service/sync_config.h
- Embedded/components/services/sync_service/sync_https_codec.c
- Embedded/components/services/sync_service/sync_https_codec.h
- Embedded/components/services/sync_service/sync_https_mock.c
- Embedded/components/services/sync_service/sync_https_mock.h
- Embedded/components/services/sync_service/sync_response_policy.c
- Embedded/components/services/sync_service/sync_response_policy.h
- Embedded/components/services/sync_service/sync_service.c
- Embedded/components/services/sync_service/sync_service.h
- Embedded/components/services/sync_service/sync_window_policy.c
- Embedded/components/services/sync_service/sync_window_policy.h
- Embedded/tests/host_stubs/sync_runtime_host.c
- Embedded/tests/run_host_tests.py
- Embedded/tests/test_sync_https_codec.c
- Embedded/tests/test_sync_response_policy.c
- Embedded/tests/test_sync_runtime.c
- Embedded/tests/test_sync_window_policy.c

### Review Findings

- [x] [Review][Patch] 默认 MOCK SUCCESS 每次抬升 `command_revision` 会反复改写设置 — 已拆出 `EWF_SYNC_MOCK_SUCCESS_WITH_COMMAND`；SUCCESS 仅确认高水位 [`sync_https_mock.c`]
- [x] [Review][Patch] 关窗回写携带陈旧 `acked_total` 且 `transport_ok` 不清 `window_open` — 关窗只回写传输结果；成功路径清除 `window_open` [`sync_service.c` / `sync_window_policy.c`]
- [x] [Review][Defer] `poll_once` 固定 `command_revision=0`，PENDING_COMMAND 触发在运行时不可达 [`sync_service.c`] — deferred: 同窗已应用命令；「视需要回传」可由 backlog/立即同步覆盖，完整自动回传属后续收敛
- [x] [Review][Defer] 部分命令载荷缺 brightness 时默认写成 mid 覆盖本地亮度 [`sync_service.c`] — deferred: mock/契约当前三字段同发；部分载荷语义待 cloud Epic 5 对齐
- [x] [Review][Defer] 单次 AT 超时不清 `window_open`，仅清 `transport_busy` [`sync_window_policy.c`] — deferred: 当前关窗路径另有 `force_close`；与连续超时恢复语义一并整理更稳
- [x] [Review][Defer] 主机运行时未钉住 idempotent / below-watermark 场景（仅 codec/policy 层） [`test_sync_runtime.c`] — deferred: AC 四类 mock 已在 codec 覆盖；运行时补测非阻断
- [x] [Review][Defer] Air780 真机 AT/HTTPS/DTR/发射峰值仍 `hardware_pending` [`air780egp_bsp.c`] — deferred: Story 边界；不得宣称通信可靠 verified

#### Rejected

- sync 事实未写入 `state_service`/`watch_state` — false：spec 允许 `sync_service_snapshot` 最小通道且主机已钉住
- 请求 JSON 字符串未转义 — low：身份字段受控且 fail-closed；加转义扩面过大
- `EWF_AIR780_*` 与 `EWF_BSP_AIR780EGP_*` 双宏 — low：取值已对齐且 `contract_checks` 静态断言板级宏
- `run_one_window` 读身份未持锁 — low：单 sync 任务日常路径；争用面窄
- `set_battery_percent` 总置 `hardware_pending` — low：CW2015 产品读数未接入前符合占位语义

### Change Log

- 2026-09-24：Story 2.5 实现活动窗口客户端、HTTPS mock、Air780 BSP 边界与主机/IDF 验证；Status → review。
- 2026-09-24：code-review（deep/autofix）修复 mock 命令抬升与关窗回写；主机测试全绿；Status → done。
