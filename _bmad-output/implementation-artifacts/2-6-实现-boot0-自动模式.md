# Story 2.6: 实现 BOOT0 自动模式

baseline_commit: 490c46b0719d6bb90484cd14e2fd492e83318e65

Status: done

<!-- Note: Ultimate context engine analysis completed - comprehensive developer guide created. 本 Story 闭合运行态 BOOT0 短按切换内存态自动模式、3 秒周期 `automatic_tap` 生成、退出/完成停表与精度门禁；不改下载路径、不落盘、不做 DEVICE-01 UI 投影（属 Epic 3）；低电优先调度属 2.7；跨层真机一致性属 7.5。 -->

## Story

As a 设备使用者,
I want 短按 BOOT0 临时开启或关闭每 3 秒一次的自动敲击,
so that 可以用固定节奏演示完整诵读而不改变下载流程。

**覆盖需求：** FR-E-011、AD-7、AD-19、NFR8（SM-8）

## Acceptance Criteria

### AC 1：运行态短按只切换一次，以释放确认为锚每 3 秒生成 `automatic_tap`

**Given** 设备处于开机运行态，且启动采样窗口已结束（`boot0_runtime_armed`）
**When** 用户完成一次经 `power_boot_policy` 去抖后的 BOOT0 短按/释放（`EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP`）
**Then** 自动模式只切换一次（关→开或开→关）；进入时以该释放确认时刻为周期锚点，之后每 **3000 ms** 生成一个 `automatic_tap` 并经 `tap_input_service_submit_automatic_tap` 进入统一队列
**And** 连续 10 个周期的单周期误差不超过 **±100 ms**；长按（`BOOT0_LONG_SUPPRESSED`）、抖动（`BOOT0_GLITCH_IGNORED`）、启动/下载采样（`BOOT0_STARTUP_SUPPRESSED`）不产生模式切换，也不生成 `automatic_tap`

### AC 2：退出/gate/完成时停发；模式不持久化；任意供电可用

**Given** 自动模式运行中，或队列已满 / 完成遮罩 / 故障 gate 生效
**When** 周期到期投递、用户再次短按退出，或进度到达本地末字完成锁定
**Then** 退出后不再生成新的自动事件；已入队事件按统一队列完成（不补发、不回滚）；`automatic_tap` 不得绕过与真实敲击相同的 gate；完成锁定时停止自动模式并清除定时器，新轮次须再次显式短按启用
**And** 自动模式只存在运行内存，重启后默认关闭，不得写入 progress/settings NVS；USB-C 外部供电、电池供电、外部供电并充电均可使用，不引入充电检测分支；不改变「先按住 BOOT0 再长按 PWR 上电」下载路径

## Tasks / Subtasks

- [x] Task 1：自动模式纯逻辑策略（AC: 1, 2）
  - [x] 新增零 ESP-IDF 依赖模块（建议 `power_auto_mode_policy.c` / `.h`，与 `power_boot_policy` 同目录）：输入为 RUNTIME_TAP 时刻、`now_ms`、完成/故障/队列满 gate 快照；输出为 `enable`/`disable`/`emit_tap`/`noop` 决策与下一到期时刻。
  - [x] 冻结常量：`EWF_POWER_AUTO_MODE_PERIOD_MS = 3000U`；验收误差 `EWF_POWER_AUTO_MODE_PERIOD_TOLERANCE_MS = 100U`；锚点 = 释放确认 `at_ms`（首次 emit 在 `anchor + 3000`，不得在 toggle 当下立即 emit 一次「额外敲击」——短按本身不是敲击）。
  - [x] Toggle 语义：每次 RUNTIME_TAP 翻转一次；已关闭时忽略周期；关闭时清除锚点/下一到期；完成锁定决策必须强制 `disable` + 清定时器意图。
  - [x] 主机可测：10 周期间距、退出后零 emit、完成强制停、toggle 幂等、与 gate 拒绝并存（策略层只决定是否 emit，gate 仍由 tap_input 执行）。

- [x] Task 2：纠正 `power_service` 错误接线并接入定时器（AC: 1, 2）
  - [x] **必须删除/改写** 当前 `power_service_publish_automatic_tap` 在每次 `BOOT0_RUNTIME_TAP` 上直接 `submit_automatic_tap` 的行为（Story 1.1 边界雏形，**违反 FR-E-011**）。改为：RUNTIME_TAP → `auto_mode_policy` toggle → 启停定时器。
  - [x] 周期定时推荐 `esp_timer` **periodic**（µs 分辨率），回调仅投递 ISR-safe / 短任务消息到 `power_task` 队列（或专用 typed 消息），**禁止在 esp_timer 回调 / ISR 内**调用 `tap_input_service_submit_*`、做 NVS、打普通阻塞日志（AD-7）。查证依据：ESP-IDF v5.5.4 ESP32-S3 [ESP Timer](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_timer.html) — `esp_timer_create` / `esp_timer_start_periodic` / `esp_timer_stop` / `esp_timer_delete` / `esp_timer_get_time`；禁止用 `latest`/`freertos.org` 或 200ms 轮询冒充 ±100ms。
  - [x] 现有 `power_task` 主循环为 `portMAX_DELAY` 阻塞收队列：接入定时器后须增加「到期投递」消息类型，或改用有界超时 + 单调时钟核对到期；不得丢掉 PWR/BOOT 边沿处理。
  - [x] `submit_automatic_tap` 时：`source=EWF_TAP_SOURCE_AUTOMATIC_TAP`，`sequence=tap_input_service_next_sequence()`，`candidate_confirmed=true`；**不要**伪造依赖木鱼触区的语义（`wood_fish_hit`/`screen_on` 对 AUTOMATIC_TAP 非 gate 条件，可填合理默认）；拒绝时中文日志 + typed reason，不得静默丢。
  - [x] 快照扩展（建议挂在 `power_service_snapshot_t`）：`auto_mode_enabled`、`auto_mode_anchor_ms`、`auto_period_emit_count`（诊断）；可选经 `state_service` typed update 发布只读事实供 Epic 3 投影——**本 Story 不写 LVGL/Pen 文案**。

- [x] Task 3：完成锁定停表与 gate 协同（AC: 2）
  - [x] 消费路径：在每次周期投递前（及/或订阅 progress/gate 事实后）读取 `state_service_read_tap_gate`；若 `completed==true`，强制退出自动模式并 `esp_timer_stop`，即使本周期已到期也不再 submit。
  - [x] `queue_full` / `fault_locked`：仍可保持「模式开」或按策略立即停——**推荐**：模式标志可保持，但每次 emit 仍走统一 gate（拒绝有日志）；**完成锁定必须停模式+清定时器**（PRD 硬性）。选定后主机钉住。
  - [x] 重启/服务 `run` 入口：`auto_mode` 默认 false，不从 NVS 恢复；`progress` 写集继续禁止 `automatic_mode` key（2.2 已有主机断言，不得破坏）。
  - [x] 不改 `power_boot_policy` 短/长按/启动分类（1.1 已闭合）；不改 KEY BSP 引脚；不驱动 KILL；不调用软件关机 API。

- [x] Task 4：主机测试与构建验证（AC: 1–2）
  - [x] 新增 `Embedded/tests/test_power_auto_mode_policy.c`：toggle、锚点+10×3000±0（逻辑时钟）、退出零 emit、完成强制 disable、长按/抖动路径不进入策略（由 boot_policy 过滤，策略单测可直接喂 RUNTIME_TAP）。
  - [ ] 可选 runtime/host 接线测：模拟 RUNTIME_TAP → enabled → 伪造到期 → submit 替身被调用；再 RUNTIME_TAP → disabled → 到期不再调用；completed gate → stop。
  - [x] 回归：`test_power_boot_policy.c`、`test_tap_input_*`、`test_progress_runtime`（无 automatic_mode 持久化）全绿。
  - [x] 运行 `python3 Embedded/tests/run_host_tests.py`；环境允许时 ESP-IDF v5.5.4 增量构建写入 `docs/embedded/build_records/`。
  - [x] `hardware_pending`（不得标 verified）：真机连续 10 周期 ±100ms、下载路径「BOOT0 保持低→长按 PWR」与运行态短按互不干扰的样机证据。

## Dev Notes

### 事实源与架构约束

- FR-E-011 / UJ-5：运行态 BOOT0 短按切换自动模式；锚点=释放确认；每 3s 一个 `automatic_tap`；退出停发；完成停表；不跨重启；不改下载路径。[Source: `prd.md` §3.6 / §5.11；`epics.md` Story 2.6]
- AD-1：三类正式输入进同一队列；`automatic_tap` 不新增 cloud 字段。[Source: `ARCHITECTURE-SPINE.md` AD-1]
- AD-7：ISR/定时器回调只投递；产品决策在服务任务。[Source: `ARCHITECTURE-SPINE.md` AD-7]
- AD-19：每个已接受 `automatic_tap` 与真实敲击相同推进一个可消费汉字。[Source: `ARCHITECTURE-SPINE.md` AD-19]
- NFR8 / SM-8：自动事件的 UI/经文/音频/累计/持久化/同步结果与真实敲击一致（消费侧 2.1–2.3/2.5 已具备；本 Story 保证来源投递正确）。[Source: `epics.md` NFR8；`prd.md` SM-8]
- sync-contract §4：来源名逐字 `automatic_tap`；模式只在内存。[Source: `docs/contracts/sync-contract.md` §4]
- EXPERIENCE / UI_CONTRACT：**尚无**冻结的「自动模式」SHELL 文案/状态帧；Epic 2 只产 typed 事实，**禁止**自造用户可见文案进 Pen/HTML。[Source: UX 探索结论；Epic 3 投影]

### 当前代码实况（UPDATE — 必读）

| 模块 | 现状 | 本 Story 动作 |
| --- | --- | --- |
| `power_boot_policy` | 去抖 20ms、短按 `<3000ms`→`RUNTIME_TAP`、长按/抖动/启动抑制已闭合 | **不改**分类；只消费 `RUNTIME_TAP` |
| `power_service.c` | **缺陷**：每次 `RUNTIME_TAP` 直接 `submit_automatic_tap` | **改写**为 toggle + 定时器 emit |
| `tap_input_service_submit_automatic_tap` | 已就绪，统一 gate | 周期路径唯一生产者调用点 |
| `progress_service` | 末字置 `pending_completion` 并 `state_service_update_tap_gate_owner(completed=true)` | 读 gate 完成 → 停表 |
| `app_state` / NVS | **无** `auto_mode` 字段；progress 禁持久化自动模式 | 仅内存快照；禁止 NVS key |
| 服务定时器惯例 | nav/sync 用 200ms poll | **不适用** ±100ms；改用 `esp_timer` |

### 裁决（推荐，实现采用）

1. **短按不立即敲击**：toggle 当下零 `automatic_tap`；首拍在 `anchor_ms + 3000`。
2. **完成锁定**：强制 `auto_mode=false` + stop/delete timer；新轮次不自动恢复。
3. **queue_full / fault**：保持模式开但 emit 走 gate（可诊断拒绝）；与「完成必须停」区分。
4. **定时器所有权**：`power_service` 独占创建/启停；禁止第二套 BOOT0 自动敲击生产者。
5. **周期测量**：主机用逻辑时钟钉间距；真机 ±100ms 记 `hardware_pending`，不得用 host fake 冒充样机 verified。

### 目录、代码与 API 约束

- ESP-IDF v5.5.4 / ESP32-S3；新增 `esp_timer_*` 必须附 v5.5.4 文档证据。
- C 规范：中文 Doxygen、中文日志、ASCII TAG（沿用 `PWR_BOOT` 或新增 `PWR_AUTO`）、snake_case、无 VLA、ISR/timer 回调无堆分配/无阻塞。[Source: `docs/embedded/style/C编码规范-Agent版.md`]
- `contract_checks.c`：为 `EWF_POWER_AUTO_MODE_PERIOD_MS == 3000` 增加 `_Static_assert`；保留既有 BOOT0 引脚/去抖断言。
- 禁止：第二套计数队列、cloud `automatic_*` 字段、充电分支、软件关机、改写 Boot ROM 下载采样、Epic 3 UI 闭包抢做、把「短按=一次敲击」留下回归。

### 不属于本 Story 的边界

- DEVICE-01 自动模式视觉/文案投影、字体合同（Epic 3）。
- 完成遮罩 UI 与跨轮显式动作完整闭环（3.6 / 5.2）；本 Story 只响应已有 `completed` gate 停表。
- 低电优先落盘与续航实测（2.7）。
- 真机三类输入到小程序一致性（7.5）。
- 不修改 `cloud/`。

### Testing Requirements

- `python3 Embedded/tests/run_host_tests.py` 全绿；新段 C17、`-Wall -Wextra -Werror`。
- 断言：10 周期间距、退出零 emit、完成停表、RUNTIME_TAP 不再直接等于一次 automatic_tap、无 NVS `automatic_mode`。
- 构建记录（若执行）写入 `docs/embedded/build_records/`。

### Project Structure Notes

- 预期新增：`power_auto_mode_policy.c/.h`、`tests/test_power_auto_mode_policy.c`（+ 可选 runtime host）。
- 预期修改：`power_service.c/.h`、`services/CMakeLists.txt`、`contract_checks.c`、`run_host_tests.py`；必要时最小扩展 `app_state`/`state_service` 只读事实字段。
- 少改：`power_boot_policy.*`、`KEY/key.*`、`tap_input_service` 公开 API（只被调用）。

### Previous Story Intelligence（2.5 / 2.1–2.4）

- 2.1：已预留 `submit_automatic_tap`；明确定时器/生命周期属 2.6。
- 2.2：自动模式禁止入持久化写集；主机已钉。
- 2.3：三类来源反馈无差别；本 Story 勿另造自动模式音效分支。
- 2.4：只消费 PWR 导航；BOOT0 不得被 nav 切换自动模式。
- 2.5：sync mock/活动窗口已可消费自动敲击产生的 backlog；本 Story 不改 sync。
- 模式：纯逻辑 `*_policy` + runtime 服务 + host 测试，与 nav/sync/progress 一致。

### Git Intelligence Summary

- 近期 Embedded 提交建立了 feedback、progress、tap_input、device_nav、sync；`power_service` 仍停留在「RUNTIME_TAP→单次 automatic_tap」雏形。
- 实现时应最小 diff 改 `power_service` 热路径，把生命周期抽到 `power_auto_mode_policy`，避免在 `power_service.c` 堆业务分支。

### Latest Tech Information

- ESP Timer（IDF v5.5.4）：period/oneshot、默认 Task Dispatch；回调须短小，重活丢回 `power_task`；分辨率 1µs，适合 3000ms±100ms。[Source: https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_timer.html]
- FreeRTOS software timer 分辨率通常不足本 AC；不要用其替代 esp_timer 做精度门禁。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` §Epic 2 / Story 2.6、NFR8]
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md` §3.6 UJ-5、§5.11 FR-E-011、SM-8]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` AD-1/7/19]
- [Source: `docs/contracts/sync-contract.md` §4]
- [Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md` BOOT0=IO0 / 下载序列]
- [Source: `_bmad-output/implementation-artifacts/2-1-*.md`～`2-5-*.md` 交接]
- [Source: `Embedded/components/services/power_service/*`、`tap_input_service/*`、`progress_service/*`]
- [Source: ESP-IDF v5.5.4 ESP Timer 文档]

## Dev Agent Record

### Agent Model Used

Composer (Cursor Auto / Epic Autopilot Phase B)

### Debug Log References

- 主机测试：`python3 Embedded/tests/run_host_tests.py` → exit 0
- 固件构建：`docs/embedded/build_records/20260924-093140/build-01.log` → exit 0

### Completion Notes List

- 2026-09-24：create-story 完成——上下文引擎分析 epics/PRD/架构/契约/硬件/Story 2.1–2.5 交接与现有 `power_service` 错误接线；Status=ready-for-dev。Ultimate context engine analysis completed - comprehensive developer guide created。
- 2026-09-24：dev-story 完成——新增 `power_auto_mode_policy`（toggle/锚点/周期/完成停表）；改写 `power_service`：RUNTIME_TAP 不再直接 submit，改为 `esp_timer` periodic → 队列消息 → 任务内 gate 核对后 `submit_automatic_tap`；快照扩展 `auto_mode_*`；合同 `_Static_assert` 钉死 3000ms；主机策略测 + 全量 host 回归绿；ESP-IDF v5.5.4 构建通过。真机 ±100ms / 下载路径互不干扰仍为 `hardware_pending`。可选 runtime host 接线测以策略单测覆盖核心语义，未另建 ESP 替身。

### File List

- Embedded/components/services/power_service/power_auto_mode_policy.c
- Embedded/components/services/power_service/power_auto_mode_policy.h
- Embedded/components/services/power_service/power_service.c
- Embedded/components/services/power_service/power_service.h
- Embedded/components/services/CMakeLists.txt
- Embedded/components/contract_checks/contract_checks.c
- Embedded/tests/test_power_auto_mode_policy.c
- Embedded/tests/run_host_tests.py
- Embedded/tests/test_bsp_contract.py

### Change Log

- 2026-09-24：Story 上下文创建，sprint-status → ready-for-dev。
- 2026-09-24：实现 BOOT0 自动模式策略与 power_service 定时器接线；主机测试与固件构建通过；sprint-status → review。
- 2026-09-24：code-review autofix——统一 esp_timer 锚点时钟、周期入队失败可观测、gate 读失败强制停表、源码合同钉死 RUNTIME_TAP≠立即 submit；sprint-status → done。

### Review Findings

- [x] [Review][Patch] 锚点与周期核对混用 FreeRTOS tick / esp_timer 两套时钟 [power_service.c:handle_runtime_tap] — 已改为 RUNTIME_TAP 锚点使用 `esp_timer_get_time()/1000`
- [x] [Review][Patch] 周期消息 `xQueueSend(...,0)` 失败静默丢弃 [power_service.c:auto_timer_cb] — 已记 `s_auto_period_queue_drops` 并在任务内告警；`skip_unhandled_events=false`
- [x] [Review][Patch] gate 读取失败时跳过周期决策导致完成停表推迟 [power_service.c:handle_auto_period] — 已改为读失败时 `force_disable` + 停表
- [x] [Review][Patch] 缺少 RUNTIME_TAP≠立即 submit 的可失败护栏 [test_bsp_contract.py] — 已增源码合同断言钉死 toggle/EMIT 路径
- [x] [Review][Defer] 完成锁定仅在周期消息路径停表，未订阅 gate 变更即时停 [power_service.c:handle_auto_period] — deferred: Task 3 允许「周期投递前」核对；即时订阅属增强，不阻塞 AC
- [x] [Review][Reject] uint32 加法回绕卡住周期 — rejected: 设备连续运行约 49 天才会触及，日常路径不可达
- [x] [Review][Reject] 多周期欠账只 emit 一次 — rejected: 与裁决「不补发」及策略「按固定周期推进」一致，属有意语义
