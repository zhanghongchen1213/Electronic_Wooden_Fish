# Story 2.4: 实现触摸、PWR 导航与设备设置持久化

baseline_commit: 490c46b0719d6bb90484cd14e2fd492e83318e65

Status: done

<!-- Note: 本 Story 闭合设备侧导航意图、熄亮屏策略、device_touch 生产边界与契约 §11.1 设置存储（volume/brightness/timeout/applied_revision）；完整 DEVICE-01 LVGL 页面投影与设置页视觉闭包属 Epic 3（尤其 3.1/3.5）；Air780EGP 实际上报与立即同步网络事务属 Story 2.5；BOOT0 自动模式属 Story 2.6。 -->

## Story

As a 设备使用者,
I want 用屏幕和 PWR 在主页、设置和唤醒状态之间切换，
so that 不打开手机也能完成基本操作。

**覆盖需求：** FR-E-007、AD-10、AD-15

## Acceptance Criteria

### AC 1：三主页循环、设置入口与熄屏唤醒符合契约

**Given** 设备亮屏或熄屏
**When** 用户左右滑动、下滑进入设置、短按 PWR 或触摸屏幕
**Then** 导航意图按契约推进：亮屏 PWR 短按按 `木鱼→经文→统计` 循环；下滑进入设置（返回保持原主页）；熄屏时 PWR 短按或任意首触只唤醒并点亮屏幕
**And** 自动熄屏按时长档位（5/15/30 秒，默认 15）熄灭；无手动熄屏入口；导航状态以 typed 事实发布，消费者只读不可变快照（AD-9）

### AC 2：木鱼触区唯一计数，gate 不被绕过

**Given** 亮屏且当前主页为木鱼页
**When** 用户点击电子木鱼触区（≥96×96）
**Then** 产生 `device_touch` 并经 `tap_input_service_submit_device_touch` 进入统一队列；完成遮罩/故障锁定/积压已满 gate 仍生效，不得旁路
**And** 熄屏首触只唤醒、不计数；经文/统计/设置页普通触摸与左右滑/下滑手势不产生正式敲击；PWR/BOOT0 控制事件不得注入 `device_touch`

### AC 3：设置写入本地并跨重启保留

**Given** 用户修改亮度、熄屏时长、音量或触发立即同步
**When** 设置保存或动作完成
**Then** 值写入统一设备设置存储：亮度 `low|mid|high`（默认 `mid`）、熄屏 `5|15|30`（默认 `15`）、音量 `0–100`（默认 `50`，0 静音）；立即同步进入可观察状态（`busy/ok/pending/fail` 之一，事实可测）
**And** 设备重启后设置保持；`applied_revision` 与设置字段同事务可恢复；PWR 长按关机仍由板级电源负责，固件不驱动 KILL、不模拟软件关机

### AC 4：显示链路与 ui_task 边界不被破坏

**Given** 导航或熄亮屏请求到达
**When** 系统应用显示与页面意图
**Then** LVGL 只由 `ui_task` 调用；BSP 独占 CO5300/CST9217；其他任务只发布 typed intent/事实，不直改对象树或绕过 BSP 操作 QSPI/I²C
**And** 熄亮屏遵循 CO5300 整帧时序（先提交帧再 DISPON；熄屏分阶段可续跑）；本 Story 不重做 Epic 3 的页面文案/字体/七字带视觉闭包

## Tasks / Subtasks

- [x] Task 1：建立导航/熄亮屏纯逻辑与 typed 事实（AC: 1, 4）
  - [x] 新增零 ESP-IDF 依赖的导航策略模块（建议 `device_nav_policy.c`，沿 `power_boot_policy`/`tap_input_policy` 先例）：输入为当前页、是否在设置、屏幕 on/off、手势/PWR 短按/超时事件；输出为下一页意图、进入/退出设置、唤醒/熄屏决策。
  - [x] 主页枚举固定为 `MUYU`/`JINGWEN`/`TONGJI`（对应 EXPERIENCE 三主页循环）；设置是按需浮层/独立屏意图，不是第四个循环页；PWR 短按在设置打开时的行为：先退出设置再参与循环，或保持「设置中短按不换页」——实现前在 Dev Notes 裁决栏选定并写死，主机测试钉住。
  - [x] 经 `state_service` 发布导航/显示 typed update（owner 单调序号）：至少含 `screen_state`、`active_page`、`settings_open`、最近导航原因；消费者只读快照。可扩展既有 `watch_power_update_t`/`screen_state` 通道或新增 `STATE_SERVICE_UPDATE_NAV`，以最小侵入为准。
  - [x] 自动熄屏：以设置 `timeout` 秒为无操作计时；有效敲击、导航手势、PWR 唤醒、设置修改均重置计时；超时只发熄屏意图，不关机。

- [x] Task 2：闭合 PWR 短按 → 唤醒/循环（AC: 1）
  - [x] 消费 `power_service` 已交接的 `EWF_POWER_BOOT_EVENT_PWR_INTERVAL`（含 `duration_ms`）：在固件侧增加短按分类阈值（建议 `EWF_PWR_SHORT_PRESS_MAX_MS`，初值落在 500–1500 ms 区间并标注待样机定标）；超过阈值的区间**不**产生导航动作（长按关机属板级，固件禁止 `esp_deep_sleep_start`/`esp_restart`/KILL）。
  - [x] 熄屏 + 短按 → 只唤醒；亮屏 + 短按 → 三主页循环；持续按住同一低电平区间不得形成唤醒/换页风暴（同一次按压只交接一次区间结论——已由 power_boot_policy 保证，本 Story 不得再按电平轮询）。
  - [x] BOOT0 短按结论仍只交给 Story 2.6；本 Story 不得切换自动模式。

- [x] Task 3：闭合触摸导航与 `device_touch` 生产边界（AC: 1, 2）
  - [x] 在现有 `tap_input_service` CST9217 中断路径上补齐产品语义：熄屏首触 → 发布唤醒意图且 `wake_only=true`（已有字段）并**不得** `accepted` 为正式敲击；亮屏后仅当 `active_page==MUYU` 且命中木鱼触区才 `wood_fish_hit=true`。
  - [x] 左右滑/下滑手势识别放在任务上下文（ISR 只投递点/中断消息）：左右滑切换三主页；下滑打开设置；设置内上滑/返回手势关闭设置。手势阈值用有名常量，主机可测。
  - [x] 木鱼触区几何：以 UI 合同 ≥96×96 为下限；当前 `touch_is_wood_fish_region` 矩形可保留为初值，但必须与「仅木鱼页计数」合取，并在注释标明 Epic 3 对拍后可收紧坐标，不得在非木鱼页放宽。
  - [x] 完成/故障/积压满时：导航手势仍可唤醒或切页（产品需要可操作退出路径时以 EXPERIENCE 为准），但**不得**产生新的 `device_touch` 计数；正式敲击继续走既有 gate。

- [x] Task 4：统一设备设置存储并收敛音量真源（AC: 3）
  - [x] 建立契约 §11.1 device 作用域设置存储 owner（建议服务/模块名 `device_settings_service` 或挂在既有服务下的 `settings_store`）：字段恰为 `volume`、`brightness`（wire `low|mid|high`）、`timeout`（`5|15|30`）、`applied_revision`（integer ≥0，单调不减）。
  - [x] **必须收敛 Story 2.3 音量真源**：扩展同一逻辑设置存储并迁移 `audio` namespace 已有 `volume`，或显式一次性迁移后废弃第二写入入口；禁止并存两套可写音量位。`feedback_service_set_volume` 应改为调用统一设置 API（或设置服务在写入后回调反馈服务），保证播放链与持久化同值。
  - [x] NVS：单一 namespace（建议 `settings`，≤15 字符）+ `schema_ver` + 单次 `nvs_commit` 事务边界；实现前查 ESP-IDF v5.5.4 NVS 文档（`nvs_set_*` 在 commit 前不落盘）。越界/非法枚举拒绝并保持原值 + 中文日志；首启缺字段用默认 `mid`/`15`/`50`/`applied_revision=0`。
  - [x] 亮度应用到 CO5300：将 `low|mid|high` 映射为 `co5300_bsp_set_brightness` 原始档（集中常量，标注 `hardware_pending` 样机定标）；熄屏/亮屏走 `co5300_bsp_set_display`，遵守闪屏时序文档，不臆造寄存器。
  - [x] `applied_revision`：本 Story 只建立本地可持久化高水位与读取 API；云端命令下发/比较 `command_revision` 的完整应用属 Epic 5，本地用户改设置时不得伪造「已与云端对齐」成功态。

- [x] Task 5：立即同步可观察状态（AC: 3）
  - [x] 「立即同步」动作发布 typed 请求事实（含篇章动作族 `action_id` 占位或本地单调请求号）；同步状态机至少可进入 `pending`/`busy`，并在无网络客户端时落到可观察的 `pending` 或 `fail`（不得假装 HTTPS 成功）。
  - [x] 真实 Air780EGP HTTPS 活动窗口由 Story 2.5 消费该请求；本 Story 提供稳定订阅点/快照字段，不实现 AT/TLS。
  - [x] 设置页 `sync-btn` 五态视觉投影属 Story 3.5；本 Story 只保证状态事实存在且可测。

- [x] Task 6：服务登记、BSP 边界与合同断言（AC: 4）
  - [x] 若新增服务：登记 `legbot_services.h` 枚举、`legbot_services.c` 描述符/启动图、`contract_checks.c` 服务数量断言（当前反馈后为 7 → +1）；遵循中文日志 + ASCII TAG（如 `SVC_NAV`/`SVC_SETTINGS`）。
  - [x] 禁止在 `generated/` 写业务；若需最小 intent 接线，只改 `bindings/` 且不得引入 legbot 外骨骼/BLE/支付文案到用户可见路径。
  - [x] 不修改 Pen/HTML/字体合同；用户可见文案不得出现 TODO/draft/placeholder/Node ID。

- [x] Task 7：主机测试与固件集成验证（AC: 1–4）
  - [x] 纯逻辑测试覆盖：三主页循环顺序、设置进入/返回、熄屏首触只唤醒、非木鱼页不计数、PWR 短按/超短抖/超长忽略、timeout 5/15/30 重置与超时熄屏、设置校验与重启恢复、音量迁移/单一真源、立即同步状态迁移。
  - [x] 编译级接线测试（沿 `test_feedback_runtime`/`test_progress_runtime`）：导航/设置服务与 `state_service`/`tap_input`/`feedback` 的关键 API 可链接；不只测纯 policy。
  - [x] 运行 `python3 Embedded/tests/run_host_tests.py` 全量通过；环境允许时 ESP-IDF v5.5.4 增量构建并记录到 `docs/embedded/build_records/`。
  - [x] 无样机时标注 `hardware_pending`：PWR 短按时长阈值、亮度三档观感、CST9217 手势阈值、熄亮屏闪屏实机、触摸坐标与木鱼热区对拍。

## Dev Notes

### 事实源与架构约束

- FR-E-007：CST9217 左右滑三主页、下滑设置、木鱼点击；仅木鱼页木鱼区产生 `device_touch`；熄屏首触只唤醒；PWR 短按唤醒/循环；亮度/熄屏/音量/立即同步跨重启保留；长按关机不由固件模拟。[Source: `prd.md` §5.7；`epics.md` Story 2.4]
- AD-10：共享 I²C 单一仲裁；UI 仅 `ui_task`；触摸读点在任务上下文经 BSP，ISR 只投递。[Source: `ARCHITECTURE-SPINE.md` AD-10]
- AD-15：音量/亮度/熄屏与高水位等跨重启保留；设置与计数分域但同属设备本地持久化纪律。[Source: `ARCHITECTURE-SPINE.md` AD-15]
- AD-9：导航/设置/显示事实经 typed update；禁止跨服务读私有变量。[Source: `ARCHITECTURE-SPINE.md` AD-9]
- AD-13 / Embedded AGENTS：PWR=`IO8` 只读 `PWR_INT`；禁止软件关机与驱动 KILL；短按只唤醒/循环。[Source: `ARCHITECTURE-SPINE.md` AD-13；`Embedded/AGENTS.md` PWR 段]
- Epic 边界：FR-E-007 在 Epic 2 落「导航与设置持久化」，Epic 3 落「页面与交互投影」。本 Story 交付可运行的意图与存储；3.1/3.5 交付 DEVICE-01 视觉闭包。[Source: `epics.md` Requirements → Epic 映射]

### 同步契约硬约束（`docs/contracts/sync-contract.md` SC-1.0.0）

- §9.1：`volume` 0–100 默认 50；`brightness` enum `low|mid|high` 默认中档；`timeout` enum `5|15|30` 默认 15。Wire 名以契约为准（UX 传说里的 `medium` 不得写入 NVS/API）。
- §9：`applied_revision` 单调不减；设置命令族与篇章动作族 `action_id` 互不覆盖；立即同步属篇章动作族（只带 `action_id`，无值载荷）。
- §11.1 device 作用域：`applied_revision`、`volume`、`brightness`、`timeout` 必须落在设备本地持久化；与 progress 高水位存储分文件/分 namespace，不得写进 `progress` 事务组搞乱累计原子性。

### 亮度 wire 名裁决（必须遵守）

- 契约冻结 `mid`；`UI_CONTRACT-device.md` 分段控件传说曾写 `medium`。实现与测试一律使用 `low|mid|high`。显示文案「中」由 Epic 3 投影，不把 `medium` 写进存储键。

### Story 2.1–2.3 交接与现状（UPDATE 文件必读）

| 模块 | 现状 | 本 Story 动作 |
| --- | --- | --- |
| `tap_input_service` | 已有 `submit_device_touch`、CST9217 ISR→TOUCH 消息、`wake_only`/`wood_fish_hit` 字段与粗矩形热区 | 与导航页状态合取；补手势导航；熄屏唤醒接显示 owner |
| `power_service` | 交接 `PWR_INTERVAL`+时长，不做短按业务 | 短按分类 → 导航意图；不改硬件关机 |
| `feedback_service` + `audio` NVS | 音量 0–100 owner + namespace `audio` | **收敛**到统一设置存储，删/迁第二音量位 |
| `watch_state.screen_state` | 已有 ON/OFF 事实与 apply API | 导航/熄亮屏 owner 写入，禁止多处私写 |
| `co5300_bsp` / `cst9217_bsp` | 亮灭屏、亮度、读点 API 已在 | 只经 BSP；遵守闪屏时序 |
| UI `generated/` + legbot bindings | 仍含外骨骼/BLE 等遗留壳 | 本 Story 不把遗留业务当 EWF 真源；最小 intent 接线或等 3.1 骨架 |

### PWR 短按分类建议裁决（写入实现常量）

- `EWF_PWR_SHORT_PRESS_MAX_MS` 初值建议 `800`（小于板级长按关机约 2.1 s，大于 20 ms 去抖）。
- 区间 `(debounce, max]` → 短按导航；`> max` → 忽略（交给板级关机）。
- 样机定标前不得把该常量宣称为已验证硬件证据。

### 导航页与 PWR 在设置打开时的裁决（推荐）

- **推荐（实现采用）：** 设置打开时，PWR 短按先关闭设置并保持当前主页；下一次短按再进入三主页循环。左右滑在设置打开时忽略或先关闭设置（与上滑返回一致）。主机测试钉死该状态机。

### 立即同步与 2.5 交接

- 本 Story：本地动作 → typed `sync_request` + 状态 `pending/busy/fail/ok` 字段。
- Story 2.5：活动窗口客户端消费请求、执行 HTTPS mock/真机、回写 ok/fail。
- 无 2.5 时，点击立即同步不得留下「已同步」假象；默认停在 `pending` 并中文日志说明等待通信客户端。

### 目录、代码与 API 约束

- ESP-IDF v5.5.4 / ESP32-S3；NVS/GPIO/显示 API 先查官方 v5.5.4 文档（禁止 `latest`/`freertos.org` 作最终依据）。
- NVS：namespace≤15；`nvs_set_*` + `nvs_commit`；fail-closed；参考 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/storage/nvs_flash.html>。
- C 规范：中文 Doxygen 头、中文日志、ASCII TAG、snake_case、无 VLA、ISR 无堆分配。[Source: `docs/embedded/style/C编码规范-Agent版.md`]
- 充电语义：PRD final 三种供电均可用，不实现充电暂停输入/导航分支。

### 不属于本 Story 的边界

- 不实现 DEVICE-01 完整三主页/设置页视觉对拍、七字带、13 槽经文流、统计卡、字体合同闭合（Epic 3）。
- 不实现 Air780EGP AT/HTTPS、证书、活动窗口功耗策略（Story 2.5）。
- 不实现 BOOT0 自动模式定时器（Story 2.6）。
- 不实现 `tap_fault_locked` 生产者与低电优先落盘调度（Story 2.7）。
- 不实现云端命令修订拉取与 `command_revision` 应用闭环（Epic 5）；只预留 `applied_revision` 存储。
- 不把主机测试/编译写成触摸手势、PWR 手感或亮度观感的硬件验收。

### Testing Requirements

- `python3 Embedded/tests/run_host_tests.py` 全绿；新段 C17、`-Wall -Wextra -Werror`。
- 替身记录：页序列、settings_open、screen_state、touch 接受/拒绝原因、设置读写恢复、音量单一真源、sync 状态。
- 构建记录（若执行）写入 `docs/embedded/build_records/`；`hardware_pending` 项不得标 verified。

### Project Structure Notes

- 预期新增：`Embedded/components/services/device_nav_service/` 或等价、`device_settings`/`settings_store_*`、`device_nav_policy.*`、`Embedded/tests/test_device_nav_*.c`、`test_device_settings_*.c`、必要时 host stub。
- 预期修改：`tap_input_service`（页状态合取/手势）、`feedback_service`/`audio_volume_store_*`（收敛）、`state_service`/`app_state`（导航/设置快照）、`legbot_services.*`、`contract_checks.c`、`run_host_tests.py`；可选最小 `bindings/` intent。
- 与统一结构无冲突；禁止新建第二套 event bus 或第二套音量 NVS 真源。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` §Epic 2 / Story 2.4、FR-E-007 映射]
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md` §5.1、§5.7、§6.2.2、§7]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` AD-9/10/13/15]
- [Source: `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/EXPERIENCE.md` 设备导航、Interaction Primitives、State Patterns]
- [Source: `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/UI_CONTRACT-device.md` §1/§3/§4 SHEZHI、触区]
- [Source: `docs/contracts/sync-contract.md` §9、§9.1、§11.1]
- [Source: `Embedded/AGENTS.md` PWR/编码/ESP-IDF 校验；`docs/embedded/guides/低功耗策略与实测验收.md`；`docs/embedded/troubleshooting/CO5300-熄亮屏闪屏与整帧时序.md`]
- [Source: `_bmad-output/implementation-artifacts/2-1-*.md`、`2-2-*.md`、`2-3-*.md` 交接与 File List]
- [Source: `Embedded/components/services/tap_input_service/*`、`power_service/*`、`feedback_service/audio_volume_store*`、`BSP/CO5300/*`、`BSP/CST9217/*`]

## Dev Agent Record

### Agent Model Used

Composer (Cursor Agent Router)

### Debug Log References

- ESP-IDF build attempt-01：`docs/embedded/build_records/20260924-083812/build-01.log`（contract_checks 缺 `feedback_service.h`）
- ESP-IDF build attempt-02：`docs/embedded/build_records/20260924-083812/build-02.log`（成功）

### Completion Notes List

- 2026-09-24：create-story 完成——上下文引擎分析 epics/PRD/架构/UX/契约/Embedded 现状/Story 2.1–2.3 交接；Status=ready-for-dev。Ultimate context engine analysis completed - comprehensive developer guide created。
- 2026-09-24：dev-story 完成——新增 `device_nav_service`（policy + settings store + NVS `settings` + 立即同步 pending）；PWR 短按经 `EWF_PWR_SHORT_PRESS_MAX_MS=800`；触摸手势任务上下文识别；音量收敛到统一设置 API；`STATE_SERVICE_UPDATE_NAV`；服务表 7→8；主机测试全绿；ESP-IDF v5.5.4 构建成功。
- 裁决落地：设置打开时 PWR 短按先关设置保持主页；无 2.5 时立即同步停在 `pending`。
- hardware_pending：PWR 短按时长、亮度三档观感、CST9217 手势阈值、熄亮屏闪屏实机、木鱼热区对拍。

### File List

- Embedded/components/app_state/app_state.h
- Embedded/components/app_state/watch_state.c
- Embedded/components/contract_checks/contract_checks.c
- Embedded/components/services/CMakeLists.txt
- Embedded/components/services/device_nav_service/device_nav_policy.c
- Embedded/components/services/device_nav_service/device_nav_policy.h
- Embedded/components/services/device_nav_service/device_nav_service.c
- Embedded/components/services/device_nav_service/device_nav_service.h
- Embedded/components/services/device_nav_service/device_settings_policy.c
- Embedded/components/services/device_nav_service/device_settings_policy.h
- Embedded/components/services/device_nav_service/device_settings_store.h
- Embedded/components/services/device_nav_service/device_settings_store_nvs.c
- Embedded/components/services/feedback_service/audio_volume_store_nvs.c
- Embedded/components/services/feedback_service/feedback_service.c
- Embedded/components/services/legbot_services.c
- Embedded/components/services/legbot_services.h
- Embedded/components/services/power_service/power_service.c
- Embedded/components/services/state_service/state_service.c
- Embedded/components/services/state_service/state_service.h
- Embedded/components/services/tap_input_service/tap_input_service.c
- Embedded/tests/host_stubs/co5300_bsp.h
- Embedded/tests/host_stubs/co5300_bsp_host.c
- Embedded/tests/host_stubs/device_nav_host.c
- Embedded/tests/host_stubs/device_settings_store_host.c
- Embedded/tests/host_stubs/feedback_host.c
- Embedded/tests/run_host_tests.py
- Embedded/tests/test_device_nav_policy.c
- Embedded/tests/test_device_nav_runtime.c
- Embedded/tests/test_device_settings_policy.c
- Embedded/tests/test_feedback_runtime.c

### Review Findings

- [x] [Review][Patch] 抬起用手势终点 (0,0) 误判滑动 — 已修：`cst9217_bsp_read_point` 保留末次坐标；`tap_input` 回退最近按下点 [`tap_input_service.c` / `cst9217_bsp.c`]
- [x] [Review][Patch] 熄屏首触抬起可连带换页 — 已修：`s_wake_only_contact` 抬起跳过手势 [`tap_input_service.c`]
- [x] [Review][Patch] 木鱼按下即计数 + 抬起滑动双生效 — 已修：正式敲击延后到抬起且 gesture==NONE [`tap_input_service.c`]
- [x] [Review][Patch] 导航 getter 互斥失败 fail-open — 已修：screen_on/page/settings fail-closed [`device_nav_service.c`]
- [x] [Review][Patch] `audio_volume_store_save` 仍可写 settings — 已修：返回 `ESP_ERR_NOT_SUPPORTED` [`audio_volume_store_nvs.c`]
- [x] [Review][Patch] 显示 BSP 失败后 typed 状态不同步 — 已修：失败回滚 `screen_on` [`device_nav_service.c`]
- [x] [Review][Patch] `complete_sync` 可写入无飞行请求的 pending/busy — 已修：状态机校验 [`device_nav_service.c`]
- [x] [Review][Patch] idle tick 回绕永不熄屏 — 已修：回绕重置活动锚点 [`device_nav_policy.c`]
- [x] [Review][Patch] PVDF 有效敲击不重置空闲 — 已修：VALID 发布成功后 `note_activity` [`tap_input_service.c`]
- [x] [Review][Patch] 缺非木鱼/设置/熄屏/滑动接线断言与 idle 关设置 — 已补主机测 [`test_tap_input_runtime.c` / `test_device_nav_policy.c`]
- [x] [Review][Defer] CO5300 整帧后再 DISPON / 分阶段 suspend 未接线 — deferred: 需 `ui_task` 首帧协调，属 Epic 3/`hardware_pending`；当前仍走 `co5300_bsp_set_display`
- [x] [Review][Defer] tap 直接读 nav getter 而非 snapshot — deferred: 公开服务 API + 已发布 `STATE_SERVICE_UPDATE_NAV`；是否强制只读快照留给后续边界统一
- [x] [Review][Defer] power_service→nav / run 空闲轮询 / 真 NVS 迁移缺可失败主机断言 — deferred: 验证债务，非运行时灾难路径；记入 quality-debt
- [x] [Review][Defer] 手势 armed 后丢失抬起 IRQ — deferred: maybe-false，需样机丢中断证据再加超时清武装

Rejected:
- （无）Blind/Edge 中互斥 fail-open、音量第二写入、首触连带滑等均已核实为真缺陷并 patch。

### Change Log

- 2026-09-24：创建 story 上下文并推进 sprint-status → ready-for-dev。
- 2026-09-24：实现导航/设置/立即同步与音量收敛；主机测试全绿；ESP-IDF 构建成功；sprint-status → review。
- 2026-09-24：code-review（deep/autofix）修复触摸手势/计数门禁、fail-closed getter、音量第二写入禁用等；主机测试全绿；sprint-status → done。