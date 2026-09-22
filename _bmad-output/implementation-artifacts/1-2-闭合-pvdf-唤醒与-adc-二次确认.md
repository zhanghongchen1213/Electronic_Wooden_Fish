# Story 1.2: 闭合 PVDF 唤醒与 ADC 二次确认

baseline_commit: fe98d7375f21b02f22ed2cbdb3824b1462940ace

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a 设备使用者，
I want 实体敲击能可靠唤醒并被确认，
so that 普通取放和环境振动不会伪造诵读输入。

## Acceptance Criteria

**覆盖需求：** FR-E-002、AD-1

### AC 1：比较器只负责唤醒，ADC 二次确认后才产生候选有效输入

**Given** PVDF、比较器和 ADC 已按硬件网络清单连接（`PVDF_ADC`=IO9/ADC1_CH8、`PVDF_CMP_WAKE`=IO11）
**When** 单次敲击、连续敲击和普通携带振动分别触发输入
**Then** 比较器事件只负责唤醒，ADC 二次确认后才产生候选有效输入，ADC 采样不越界
**And** QMI8658A 不参与 MVP 主计数，也不保存或上报原始波形

实现口径（可验证形式，全部可在无样机条件下判定）：

1. **唤醒链路口径**：IO11 走 light sleep 逐脚 GPIO 唤醒（`gpio_wakeup_enable(IO11, GPIO_INTR_HIGH_LEVEL)`）+ 运行态上升沿 ISR（`GPIO_INTR_POSEDGE`）；ISR 只记录单调时间戳并投递 ISR-safe 消息，**不做任何产品决策、不读 ADC**。ADC 读取与二次确认判定全部在 `pvdf_task` 任务上下文完成。
2. **二次确认口径**：每次唤醒事件在固定确认窗口内做有界次数的 ADC 采样，峰值正向偏移必须超过确认阈值（`EWF_PVDF_CONFIRM_THRESHOLD_MV`）才产出**候选**有效输入事件；未超阈值的唤醒事件只记入拒绝计数与中文日志，**不产生任何候选事件**。
3. **不越界口径**：ADC 原始读数必须在配置位宽的全量程内；校准后毫伏值必须落在该衰减档可测上限内。`adc_oneshot_read()` 返回 `ESP_ERR_TIMEOUT` 或读数落在量程边界时，本次唤醒事件判为 `REJECTED_OUT_OF_RANGE`，**不重试、不放大、不硬扛**，并计入拒绝计数。
4. **上电与初始化屏蔽口径**：比较器阈值节点上电建立时间为 5τ ≈ 22.4 ms，且 TLV7042 在上电复位期间输出高阻、被 10 kΩ 上拉为逻辑高。PVDF 初始化完成后的盲窗（`EWF_PVDF_SETTLING_BLIND_MS`，取值必须 ≥ 22.4 ms）内不接受任何候选输入，以屏蔽该伪高电平与未建立阈值。
5. **单一候选来源口径**：本 Story 只产出候选事件与自检证据，**不存在任何推进正式累计的代码路径**（AD-1）。候选事件在事件总线上以固定来源名 `physical_pvdf` 语义发布（`LEGBOT_EVENT_SOURCE_PVDF`），有效敲击队列与输入闸门归 Story 2.1。
6. **QMI8658A 口径**：PVDF 链路不得读取、注册或依赖 QMI8658A（IO41/INT1）任何状态；`LEGBOT_BSP_QMI8658C_INT2_GPIO` 必须保持 `GPIO_NUM_NC`（已被 `_Static_assert` 钉死）。
7. **波形口径**：不保存、不上报原始波形；快照只暴露计数与 typed 拒绝原因，不暴露采样序列。

### AC 2：输入确认模块自测输出稳定性、误触发和阈值结果

**Given** 输入确认模块运行自测
**When** 使用可复现的敲击/振动样本执行测试
**Then** 输出候选信号稳定性、误触发和阈值结果，供目标外壳演示签收
**And** 任何失败只报告候选输入失败，不直接增加正式累计

实现口径：

1. **可复现样本口径**：样本表为编译期固定、确定性、无随机数的合成激励（单次敲击、1 秒 20 次连续敲击、携带/取放振动、超量程样本、上电盲窗内样本），定义在纯逻辑模块中，固件自检与主机测试**共用同一张表与同一个重放函数**，保证"可复现"是字节级可复现。
2. **判定与输出口径**：自检项输出各类样本的候选产出数、误触发（非敲击样本产出候选）数与阈值边界结果，落在 `selftest_item_result_t.detail_code`（定长 48 字节）与中文日志中。
3. **证据口径**：该项证据类别固定为 `SELFTEST_EVIDENCE_SOFTWARE_OBSERVED`（只证明判定逻辑确定性正确），并在 `detail_code` 中显式携带 `HW_OI_008_PENDING` 标记。单次/1 秒 20 次/环境振动/ADC 不越界的**电气**结论属未闭合开放项（硬件基线 `HW-OI-008`），在无样机条件下**不得**升级为 `SELFTEST_EVIDENCE_HARDWARE_VERIFIED`；`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 必须保持 `0U`。
4. **不增累计口径**：自检失败只产生 `SELFTEST_OUTCOME_FAIL` 与 typed 原因，不写任何累计字段；本 Story 不存在可被自检触达的累计写入路径。

## Tasks / Subtasks

- [x] **Task 1：BSP 层闭合 PVDF ADC 与比较器唤醒所有权（AC: 1.1、1.3、1.4、1.6）**
  - [x] 新建 `Embedded/components/BSP/PVDF/pvdf_bsp.h` 与 `pvdf_bsp.c`，遵循 `Embedded/AGENTS.md` 的文件头、TAG、`.c`/`.h` 分段布局。
  - [x] IO11 配置为输入（用 `gpio_config()`，**不用** `gpio_set_direction`，与 `KEY/key.c` 保持一致）：`GPIO_INTR_POSEDGE` 运行态中断 + `gpio_wakeup_enable(EWF_BSP_PVDF_CMP_WAKE_GPIO, GPIO_INTR_HIGH_LEVEL)` light sleep 逐脚唤醒。两者必须同时存在、不得互相替代（与 PWR/BOOT0 的既有合同一致）。
  - [x] IO9 建 ADC oneshot 单元：`adc_oneshot_new_unit()` + `adc_oneshot_config_channel()`，`atten = ADC_ATTEN_DB_12`、`bitwidth = ADC_BITWIDTH_DEFAULT`；单元固定 `ADC_UNIT_1`（ADC2 与 Wi-Fi 共享，MVP 禁用）。
  - [x] ADC 通道**不得硬编码** `ADC_CHANNEL_8`：用 `adc_oneshot_io_to_channel(EWF_BSP_PVDF_ADC_GPIO, &unit, &chan)` 在运行期解析，并把解析结果与"期望 ADC1_CH8"的一致性作为可观测事实暴露给自检；不一致时返回稳定错误码并降级。
  - [x] 建 ADC 校准句柄：先 `adc_cali_check_scheme()`，ESP32-S3 走 `adc_cali_create_scheme_curve_fitting()`；校准不可用时降级为原始读数路径并记中文日志，不得静默通过。
  - [x] 暴露稳定 API：`pvdf_bsp_init()`、`pvdf_bsp_read_mv()`（内部用 `adc_oneshot_get_calibrated_result()` 或 `adc_oneshot_read()` + `adc_cali_raw_to_voltage()`）、`pvdf_bsp_comparator_level()`、`pvdf_bsp_wake_enable()`/`pvdf_bsp_wake_disable()`、`pvdf_bsp_isr_register(...)`、`pvdf_bsp_error_code()`、BSP 自检入口。
  - [x] 新增目录登记进 `Embedded/components/BSP/CMakeLists.txt`：`src_dirs` 与 `include_dirs` 各加 `PVDF`；`requires` 加 `esp_adc`。
  - [x] `Embedded/components/BSP/BSP_INCLUDE/bsp_include.h` 聚合 `pvdf_bsp.h`。
  - [x] `bsp_resources.c` 的 `LEGBOT_BSP_RESOURCE_PVDF` 条目改为真实归属：`reserved = false`、`owner` 指向 `components/BSP/PVDF`、`boundary` 写明只允许 PVDF 服务经 BSP API 访问、`init_stage = LEGBOT_BSP_STAGE_BOARD_PINS`（PVDF 不占用共享 I2C/SPI/UART/I2S，不属 `SHARED_BUSES`）。
  - [x] **同步更新依赖该条目的两处断言与文案**，否则自检会 FAIL：`selftest_service.c` 的 `handle_bsp_resource_table()` 中 `pvdf->reserved` 断言、以及 `bsp_board.c` 中"IO46/IO45/PVDF 保留"的中文日志。

- [x] **Task 2：抽出纯逻辑二次确认策略（AC: 1.2、1.3、2.1）**
  - [x] 新建 `Embedded/components/services/pvdf_input_service/pvdf_confirm_policy.h` 与 `.c`，照 `power_service/power_boot_policy.{h,c}` 的范式：**零 ESP-IDF 依赖**（不得 include `esp_log.h`、`freertos/*`、`driver/*`、`esp_adc/*`），可被主机测试直接编译运行。
  - [x] 定义策略常量（全部带中文注释并标注"设计值，未冻结，必须按 HW-OI-008 样机实测重新冻结"）：
    - `EWF_PVDF_CONFIRM_THRESHOLD_MV`：确认阈值，锚定 PVDF 前端文档的比较器设计阈值 `148.1 mV`（3.3V × 47kΩ ÷ (1MΩ + 47kΩ)），取整为 `148`。语义：比较器已先行触发，ADC 二次确认负责排除无信号背书的唤醒事件。
    - `EWF_PVDF_CONFIRM_WINDOW_MS`：确认窗口，取 `20U`。依据：前端 ADC 隔离滤波 τ = 1 kΩ × 100 nF = 100 µs，远短于窗口；20 次/秒的最小间隔为 50 ms，窗口必须显著小于 50 ms 以免吞并相邻敲击；与 Story 1.1 已冻结的 `EWF_POWER_BOOT_DEBOUNCE_MS = 20U` 同尺度。
    - `EWF_PVDF_CONFIRM_SAMPLE_INTERVAL_MS`：采样间隔，取 `1U`（窗口内最多 20 次采样，有界）。
    - `EWF_PVDF_SETTLING_BLIND_MS`：盲窗，取 `25U`（严格大于 5τ ≈ 22.4 ms）。
  - [x] 定义 typed 结论枚举 `ewf_pvdf_event_kind_t`：`NONE` / `CANDIDATE_TAP` / `REJECTED_BELOW_THRESHOLD` / `REJECTED_OUT_OF_RANGE` / `REJECTED_BLIND_WINDOW` / `REJECTED_SAMPLE_FAILED` / `COUNT`。
  - [x] 定义策略状态与 API：`ewf_pvdf_confirm_policy_reset()`、`_arm_runtime(policy, now_ms)`（结束盲窗）、`_on_wake_event(policy, now_ms, event*)`（发起确认）、`_on_sample(policy, millivolt, sample_ok, now_ms, event*)`（投喂采样并出结论）、`_deadline_reached_ms()` 之类有界推进入口，风格与 `power_boot_policy` 对齐。
  - [x] 在**同一模块**内提供可复现样本表与重放函数：`ewf_pvdf_selfcheck_samples()` 返回编译期固定的样本数组，`ewf_pvdf_selfcheck_replay(policy_scratch, out_counts)` 返回各类样本的候选数、误触发数、阈值边界结果。固件自检与主机测试共用这两个函数。
  - [x] 样本表至少覆盖：单次敲击（1 个正峰超阈）、1 秒 20 次连续敲击（20 个等间隔正峰，间隔 50 ms）、携带/取放振动（幅值持续低于阈值）、超量程样本（读数触边界）、盲窗内样本、采样失败样本。

- [x] **Task 3：建立 PVDF 输入服务（AC: 1.1、1.2、1.5、1.7）**
  - [x] 新建 `Embedded/components/services/pvdf_input_service/pvdf_input_service.h` 与 `.c`，生命周期样板照 `power_service`：`pvdf_input_service_init_contracts` / `prepare_run` / `cancel_prepared_run` / `deinit_contracts` / `run` / `request_stop` / `snapshot` / `queue`。
  - [x] ISR：`gpio_isr_handler_add()` 注册 IO11 处理函数；ISR 内只取单调时间戳 + `xQueueSendFromISR` 投递 ISR-safe 消息（禁止 `vTaskDelay`、禁止中文日志、禁止读 ADC）。
  - [x] `pvdf_task`：消费唤醒消息 → 调用 `pvdf_bsp_read_mv()` 有界采样 → 投喂 `pvdf_confirm_policy` → 只在 `CANDIDATE_TAP` 时用 `event_bus_publish()` 发布候选事件，`source = LEGBOT_EVENT_SOURCE_PVDF`、`type = LEGBOT_EVENT_TYPE_SIGNAL`、`code = EWF_PVDF_EVENT_CANDIDATE`、`value` 携带确认裕量（mV）。
  - [x] 确认进行期间新到的唤醒事件按 AD-7"可合并事件可丢最新"合并为同一在飞确认，不建立第二套等待队列、不排队堆积。
  - [x] `pvdf_input_service_snapshot()` 暴露：候选计数、按 typed 原因分类的拒绝计数、最近一次结论、最近一次确认裕量、ISR 是否已登记、ADC 校准是否可用。**不得**暴露采样序列或原始波形。
  - [x] 新增 `pvdf_input_service_selfcheck(pvdf_selfcheck_result_t *out)`：调用 Task 2 的重放函数，返回各类样本计数，供自检项消费。
  - [x] `pvdf_input_service.h` 中提供 ISR 采样后重武装入口，遵循 `KEY/key.c` 的"电平保持期间先停中断、owner 采样后重武装"既有语义（比较器释放为高期间避免重复中断风暴）。

- [x] **Task 4：接入统一服务框架（AC: 1.1）**
  - [x] `Embedded/components/services/legbot_services.h`：`legbot_service_id_t` 在 `LEGBOT_SERVICE_COUNT` 之前追加 `LEGBOT_SERVICE_PVDF`。
  - [x] `legbot_services.c`：`s_descriptors` 增 `pvdf_task` 条目（`owner_component = "components/services/pvdf_input_service"`、`input_mask` 含 `QUEUE`、`starts_by_default = true`）；`start_service()` 增加栈大小与优先级分支；`stop_order[]` 增加 PVDF（排在 `STATE` 之前）；`legbot_services_init_contracts()` 增加 `pvdf_input_service_init_contracts()` 与失败回滚；`legbot_services_start_all()` 按确定的依赖顺序启动并保持失败回滚。
  - [x] `Embedded/components/services/CMakeLists.txt`：`SRCS` 增 `pvdf_input_service/pvdf_confirm_policy.c`、`pvdf_input_service/pvdf_input_service.c`；`INCLUDE_DIRS` 增 `pvdf_input_service`。**不新增 `esp_adc` 依赖**——ADC 所有权在 BSP，服务只经 BSP API 访问（AD-8）。
  - [x] 启动顺序与 `app_main.c` 的 `release_startup_light_sleep_lock()` 保持时序正确：逐脚 `gpio_wakeup_enable` 必须在全局 `esp_sleep_enable_gpio_wakeup()` 之前完成（v5.5.4 文档规定的调用顺序）。

- [x] **Task 5：新增 PVDF 自检项（AC: 2）**
  - [x] `Embedded/components/app_state/app_state.h`：`selftest_item_id_t` 在 `SELFTEST_ITEM_COUNT` 之前**追加** `SELFTEST_ITEM_PVDF_INPUT`（不得插队改序，既有项数值是稳定 ID）。
  - [x] `selftest_service.c`：新增 `handle_pvdf_input()`，调用 `pvdf_input_service_selfcheck()`，按返回计数判定 `PASS`/`FAIL`，`evidence = SELFTEST_EVIDENCE_SOFTWARE_OBSERVED`，`detail_code` 写入形如 `PVDF_POLICY_OK_HW_OI_008_PENDING`（≤ `SELFTEST_DETAIL_CODE_CAPACITY`）的稳定码；失败时用既有 `selftest_reason_t`（`SELFTEST_REASON_READ_FAILED` / `SELFTEST_REASON_DRIVER_FAILED` / `SELFTEST_REASON_WINDOW_EXPIRED`），不新增 reason。
  - [x] 把该 handler 登记进 `selftest_service_canonical_items()` 的编译期固定元数据表，`required_in_story = true`，超时用有界值（≤ `SELFTEST_MAX_TIMEOUT_MS`）。`SELFTEST_REGISTRY_CAPACITY = 8U` 现余 2 空位，够用。
  - [x] handler 必须在小步轮询中检查 `selftest_service_cancel_requested()`，并在自身 `timeout_ms` 内返回。
  - [x] **不得**把该项证据升级为 `HARDWARE_VERIFIED`，`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 保持 `0U`。

- [x] **Task 6：事件总线与启动文案同步（AC: 1.5）**
  - [x] `Embedded/components/platform/event_bus/event_bus.h`：`legbot_event_source_t` 末尾追加 `LEGBOT_EVENT_SOURCE_PVDF`（可增值，不重排既有来源）。
  - [x] `Embedded/main/app_main.c`：`release_startup_light_sleep_lock()` 的中文日志把唤醒源扩为 `PWR_INT IO8/BOOT0 IO0/PVDF 比较器 IO11`；不改动 `esp_sleep_enable_gpio_wakeup()` 的调用位置与 PM 锁语义。
  - [x] `Embedded/components/BSP/BOARD/bsp_board.c`：把"IO46/IO45/PVDF 保留"的日志修正为 PVDF 已由 Story 1.2 纳管、IO46/IO45 仍保留（AC 要求自检输出可追溯且不把设计输入误报为样机通过）。
  - [x] **不改动** `sdkconfig.defaults` 与 `sdkconfig`：`CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP` 当前未启用（已核验），启用会使 `gpio_wakeup_enable` 路径失效，属红线。

- [x] **Task 7：更新编译期契约断言（AC: 1.1、1.5、2.3）**
  - [x] `Embedded/components/contract_checks/contract_checks.c`：`_Static_assert(LEGBOT_SERVICE_COUNT == 3, ...)` → `4` 并改写说明为"power、state、self-test 与 PVDF 输入服务"；`_Static_assert(SELFTEST_ITEM_COUNT == 6, ...)` → `7` 并改写说明。
  - [x] 追加 PVDF 契约断言：IO9/IO11 的既有两条保留；新增 IO11 唤醒极性为高电平有效、盲窗常量 `≥ 25U`、确认窗口 `≤ 50U`（不得吞并 20 次/秒）等可在编译期表达的约束。
  - [x] 保持既有 PVDF 引脚断言（`EWF_BSP_PVDF_ADC_GPIO == GPIO_NUM_9`、`EWF_BSP_PVDF_CMP_WAKE_GPIO == GPIO_NUM_11`）不变。

- [x] **Task 8：主机测试门禁与构建证据（AC: 1、2）**
  - [x] 新增 `Embedded/tests/test_pvdf_confirm_policy.c`：用 `cc -std=c11 -Wall -Wextra -Werror` 编译 `pvdf_confirm_policy.c` 并运行，断言 Task 2 样本表的全部期望分类（单次→1 候选、20 次/秒→20 候选且不漏不重、振动→0 候选、超量程→判为越界拒绝、盲窗样本→被拒、采样失败→被拒）。构建方式照 `test_power_boot_policy.c`（`tempfile` 临时目录，不落编译产物）。
  - [x] `Embedded/tests/run_host_tests.py`：在 `run_edge_policy_host_test()` 之外增加 PVDF 纯逻辑用例的同构执行分支。
  - [x] `Embedded/tests/test_bsp_contract.py`：追加 PVDF 断言（`bsp_resources.h` 的 IO9/IO11 宏已存在，需新增：IO11 `gpio_wakeup_enable(..., GPIO_INTR_HIGH_LEVEL)` 存在、PVDF BSP 目录已登记进 `BSP/CMakeLists.txt` 的 `src_dirs`/`include_dirs`、`esp_adc` 在 `requires` 中、PVDF 生产代码未出现 `adc_oneshot_read` 的 ISR 调用、未出现 `esp_deep_sleep_start`/`esp_restart`）。注意该文件当前工作区已被 Story 1.1 的复审补丁改过，本次改动必须是**在其当前工作区内容之上的追加**。
  - [x] 运行 `source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` → `idf.py set-target esp32s3 && idf.py reconfigure && idf.py build`，全绿；构建证据写入 `docs/embedded/build_records/<YYYYMMDD-HHMMSS>/{environment.txt,build-01.log,session.md}`（该目录被 `.gitignore` 忽略，不进变更集）。
  - [x] 运行 `python3 Embedded/tests/run_host_tests.py`，退出码 0。

## Dev Notes

### 真源与冲突裁决

1. **充电语义冲突 —— PRD 口径胜出（本 Story 必须遵守）**
   `brief.md`「最终硬件方向」写"充电期间暂停敲击与音频"，`ARCHITECTURE-SPINE.md` AD-13 写"充电期间暂停实体/触摸输入"，UX `EXPERIENCE.md` 有 `充电中(输入暂停、可显示充电/电量/同步)` 状态。**PRD（updated 2026-09-22）一致否定该分支**：FR-E-001"主控不检测充电状态，产品不建立'充电中'分支"、FR-E-006"不展示主控无法确认的充电状态"、FR-E-009 验收"不存在依赖充电检测的反馈或功能分支"、§13.1-OQ4"旧变体在同步前不得实现为产品行为。BQ25895 的充电能力可以留在硬件/BSP 事实中，但不进入本产品状态机、UI 或计数门控"。
   → **本 Story 不得实现任何充电门控**，也不得新增充电状态读取。既有 `Embedded/tests/test_bsp_contract.py` 的 `FORBIDDEN_TOKENS` 已含 `charging_pause`、`charge_state`、`is_charging`，新增代码命中即门禁失败。UX/spine 的旧变体由后续 bmad-ux/architecture 更新对齐，**不是本 Story 的任务**。

2. **PVDF 归属 Story 编号不一致**
   `Embedded/AGENTS.md` §3.5 写"键盘输入含 PVDF（比较器 IO11 唤醒 + ADC IO9 确认，ADC1_CH8），legbot 无此，按固件 story E2.5 实现"。`epics.md` 把该能力定为 **Story 1.2**（Epic 1，覆盖 FR-E-002/AD-1）。
   → 本仓库的 story 需求真源是 `epics.md`，故按 **Story 1.2** 实现。AGENTS.md 的 `E2.5` 是早期编号残留，属文档漂移，记录在 epic retrospective，不在本 Story 改写 `Embedded/AGENTS.md`。

3. **C 编码规范来源冲突（文件头作者与头文件保护宏）**
   `docs/embedded/style/C编码规范-Agent版.md` 要求 `@author 显力科技`、保护宏 `XLKJ_<PRODUCT>_<MODULE>_H`；而 `Embedded/AGENTS.md`（固件层强制层）的文件头模板与既有 EWF 代码全部是 `@author ZHC`，新 EWF 模块保护宏用 `EWF_*_H`（保留的 legbot 资产用 `LEGBOT_*_H`）。
   → **以 `Embedded/AGENTS.md` 与既有代码为准**：新文件用 `@author ZHC`，保护宏 `EWF_PVDF_*_H`。本 Story 不做全局规范统一（属跨 Story 治理）。

4. **PVDF 硬件文档间的端点定义冲突（不影响固件引脚，但影响参数可信度）**
   `docs/hardware/电子木鱼-硬件网络清单.json` 已冻结且其 PVDF 拓扑为旧口径（`PVDF_RAW` 含串联电阻输出、无 `TLV_AND` 节点），与 `docs/hardware/外围电路设计/TLV7042DGKR与TLV2369IDGKR-PVDF前端设计与接线.md` 及 `docs/hardware/定稿/Netlist_Schematic1_2026-09-21.tel` 不一致。按 `prd.md` §9.1 的事实优先级：`定稿/` + 网络清单 + 硬件原理图基线 > 外围电路文档。
   → 固件只依赖**引脚与通道**（IO9/ADC1_CH8、IO11），三处一致，可直接使用；**全部电气参数（阈值、滞回、极性消抖、钳位漏电）未冻结**，本 Story 只能写设计值并显式标注未冻结。

5. **"熄屏首触只唤醒、不计数"的归属**
   UX `EXPERIENCE.md` 三环契约第 1 条与 `ARCHITECTURE-SPINE.md` AD-1 都规定"熄屏首次触摸只唤醒、不计数"，但该规则的落点是**有效敲击队列与输入闸门**（Story 2.1）与触摸/PWR 导航（Story 2.4），**不是 PVDF 二次确认**。UX 文档对该规则的适用对象（PVDF 首触 vs 仅触屏首触）未逐字确认，属真实歧义。
   → **本 Story 不实现该门控**（避免发明未被冻结的产品规则），只保证产出的候选事件是独立 typed 事件、可被后续闸门按来源与时刻抑制。

6. **PVDF 前端文档核验状态为"未通过"**
   该文档文件头写明"最近核验：未通过（拓扑已由最新网表核验，D2/D3 精确型号待补证）"；两个运放（TLV2369IDGKR / TLV7042DGKR）库存未检出、需采购；D2/D3 钳位件 MPN 未证实。
   → 本 Story 的任何产出**不得**声称 PVDF 电气行为已验证；`hardware_verified` 保持禁止。

### 上游故事与仓库情报（Story 1.1 现状）

**Story 1.1 已完成（`done`），为 Story 1.2 预埋了以下接口：**

| 位置 | 已存在的事实 |
|---|---|
| `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h` | `EWF_BSP_PVDF_ADC_GPIO GPIO_NUM_9`（注释"Story 1.2 独占"）、`EWF_BSP_PVDF_CMP_WAKE_GPIO GPIO_NUM_11`（同注）、枚举 `LEGBOT_BSP_RESOURCE_PVDF` |
| `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.c` | `[LEGBOT_BSP_RESOURCE_PVDF]` 条目：`name="pvdf_adc_cmp"`、`owner="Story 1.2"`、`boundary="reserved_for_story_1_2"`、`init_stage=LEGBOT_BSP_STAGE_SHARED_BUSES`、`gpio_primary=IO9`、`gpio_secondary=IO11`、`reserved=true` |
| `Embedded/components/contract_checks/contract_checks.c` | `_Static_assert(EWF_BSP_PVDF_ADC_GPIO == GPIO_NUM_9, "PVDF ADC must be IO9.")`、`_Static_assert(EWF_BSP_PVDF_CMP_WAKE_GPIO == GPIO_NUM_11, ...)` |
| `Embedded/components/services/selftest_service/selftest_service.c` | `handle_bsp_resource_table()` 断言 `pvdf->reserved`、`gpio_primary==IO9`、`gpio_secondary==IO11`，并把 `PVDF=IO%d/IO%d` 写入观测详情 |
| `Embedded/main/app_main.c` | `release_startup_light_sleep_lock()` 已调用 `esp_sleep_enable_gpio_wakeup()`（全局开放 GPIO 唤醒源的唯一入口）；日志目前只列 `PWR_INT IO8/BOOT0 IO0` |
| `Embedded/components/BSP/KEY/key.c` | 逐脚 light sleep 唤醒范式：`gpio_wakeup_enable(pin, GPIO_INTR_LOW_LEVEL)` + 运行态 `gpio_set_intr_type(ANYEDGE)`；ISR"电平保持期间先停中断，owner 采样后按相反电平重武装" |
| `Embedded/components/services/power_service/power_boot_policy.{h,c}` | **纯逻辑可主机测试模块的范式模板**（零 ESP-IDF 依赖）；`EWF_POWER_BOOT_DEBOUNCE_MS = 20U` |
| `Embedded/tests/run_host_tests.py` | 门禁入口：Python 源码合同扫描 + `cc -std=c11 -Wall -Wextra -Werror` 构建并运行纯逻辑 C 用例 |
| `Embedded/components/services/legbot_services.{h,c}` | 统一服务框架；`legbot_service_id_t` 当前 **3** 项（POWER/STATE/SELFTEST） |
| `Embedded/components/app_state/app_state.h` | `selftest_item_id_t` 当前 **6** 项 + `SELFTEST_ITEM_COUNT`；`selftest_evidence_t` 五类 + COUNT |
| `Embedded/components/platform/event_bus/event_bus.h` | `legbot_event_source_t`（枚举末尾为 `SELFTEST`，无 PVDF 来源）；`event_bus_publish()` / `event_bus_publish_from_isr()` |

**结论：PVDF 驱动代码在 `Embedded/` 下完全不存在。** 全文检索 `adc_oneshot|adc_cali|adc_channel|adc_unit|esp_adc|physical_pvdf` 在固件源码中零命中；`Embedded/components/BSP/` 下无 `PVDF/` 目录；无 ADC 相关 Kconfig；无 PVDF 自检项；无 PVDF 主机测试。本 Story 是从零新增。

**Story 1.1 复审明确 deferred 的 5 项（本 Story 不要顺手修）：**
1. `low` — ISR 投递失败时该脚中断永久关闭且无重武装路径与日志（`key.c:327,341`；`power_service.c:235,253`），既有形态非 1.1 引入。
2. `low` — `bsp_resources.h:80` / `bsp_resources.c:133` 仍把 IO17 记为 ES8311 I2S 数据输入，与硬件事实源"IO17 留空"冲突。
3. `maybe-false` — `handle_pwr_boot_input` 的观察预算等于项目 timeout，用满时 `INCONCLUSIVE` 会被引擎改判 `FAIL`/`SELFTEST_ITEM_TIMEOUT`。
4. `low` — 仓库本地 `Embedded/components/esp_pm` 覆写 IDF esp_pm 并引入自定低功耗常量，Story 1.1 未记录该覆写。
5. `low` — `partitions.csv:3` 仍为已删离线语音链路保留 6 MiB model 分区；`main/CMakeLists.txt:20` 的 spiffs 仍打包已删 `audio_service` 资源。

**Story 1.1 保留的未验证缺口（本 Story 不得声称已闭合）：** 无样机、未烧录、未监控；rail 电压、LTC2954 时序、USB/下载采样、三种供电矩阵全部 `hardware_pending`。最近构建证据 `docs/embedded/build_records/20260922-110310/session.md`（`idf.py build` exit 0，session 明写"未验证：烧录、启动、真机功能（当前无样机）"）。

**工作区状态提醒：** Story 1.1 的 98 条实现路径尚未 commit（停在暂存区/工作区），其中 `Embedded/components/BSP/KEY/key.c` 与 `Embedded/tests/test_bsp_contract.py` 带有 1.1 复审的未提交补丁。本 Story 的编辑必须基于**当前工作区内容**，不得回退这些补丁。

### 硬件事实（PVDF 模拟前端与唤醒路径）

**引脚与通道（三处一致，可直接使用）**

- `PVDF_ADC` = ESP32-S3 **IO9** = **ADC1_CH8**，`analog_input`；硬件基线 §2.1 备注"前端 1MΩ 级限流；低漏钳位，不得越过 ADC 安全范围"。
- `PVDF_CMP_WAKE` = ESP32-S3 **IO11**，`wake_input`；备注"比较器输出；仅唤醒，ADC 二次确认后才生成 `physical_pvdf`"。
- 定稿网表 `Netlist_Schematic1_2026-09-21.tel` 行 142–144：`'PVDF_ADC' ; C35.2 D2.2 D3.1 R16.2 TP_PVDF_ADC.1 U1.17`、`'PVDF_CMP_WAKE' ; R17.1 TP_PVDF_CMP.1 U1.19 U18.1`、`'PVDF_RAW' ; R14.1 R15.2 TP_PVDF_RAW.1 U11.3`。

**信号链（硬件基线 §5.4.1）**

```
PVDF+/- → 1 MΩ 级串联限流
       → PVDF_RAW（泄放 10 MΩ、缓冲器同相输入；低漏钳位待装配）
       → 缓冲器（单位增益跟随，TLV2369IDGKR）
       → TLV_AND ─┬→ 1 kΩ 隔离 + 100 nF 滤波 → PVDF_ADC（IO9 / ADC1_CH8）二次确认
                  └→ 比较器同相输入（TLV7042DGKR；反相端 1 MΩ/47 kΩ 阈值分压 + 100 nF）
                     → PVDF_CMP_WAKE（IO11）低功耗唤醒
```

**电气设计值（全部未冻结，必须样机实测）**

| 参数 | 设计值 | 推导 |
|---|---|---|
| 比较器阈值 `VTH` | **148.1 mV** | 3.3 V × 47 kΩ ÷ (1 MΩ + 47 kΩ) |
| 等效源阻抗 | 44.89 kΩ | 1 MΩ ∥ 47 kΩ |
| 阈值节点滤波 | τ = **4.49 ms**，fc = 35.45 Hz | 44.89 kΩ × 100 nF |
| 阈值上电建立 | 5τ ≈ **22.4 ms** | 同上 |
| 内部迟滞 | 最小 3 / 典型 10 / 最大 25 mV | 折算阈值窗口 127.6–168.6 mV |
| 开漏上拉 | 10 kΩ 至 3.3 V | 手册下限 650 Ω 的 15.4 倍 |
| 空闲态灌电流 | ≈ **330 µA** | 空闲输出恒低，上拉持续灌流，构成静态电流主要项 |
| ADC 隔离滤波 | τ = **100 µs**，fc = 1591.5 Hz | 1 kΩ × 100 nF |
| 输入限流 / 泄放 | 1 MΩ / 10 MΩ | 衰减比 0.909，传感器端等效对地阻抗 11 MΩ |

**极性与唤醒判定（本 Story 的关键推论，来源为硬件文档原文 + 拓扑）**

- 比较器 `INA+` 接信号、`INA−` 接阈值分压，输出为**开漏 + 10 kΩ 上拉**：静止时 `PVDF_RAW` 经泄放电阻趋近 0 V，缓冲输出低于 148.1 mV，**开漏输出恒低**给出有效低电平；正向敲击使信号超过阈值后输出**释放为高**。
- 文档原文："比较器输出极性由硬件接法固定（INA+ 接信号、INA− 接阈值），**固件侧的低功耗唤醒极性配置须与之一致**。"
- → **有效敲击对应 IO11 的上升沿/高电平**：light sleep 逐脚唤引用 `GPIO_INTR_HIGH_LEVEL`，运行态用 `GPIO_INTR_POSEDGE`。若实现为低电平/下降沿，与硬件极性相反，属严重错误。
- 文档原文："TLV7042 在上电复位期间输出保持高阻，被上拉电阻拉至逻辑高，**该行为与固件唤醒极性配置相关**。"
- → **上电瞬间 IO11 可能为伪高电平**，必须在盲窗内屏蔽，不得据此产生候选输入。
- 单电源缓冲无法传递负半周，负向压电信号被钳在 0 V：**ADC 与比较器只见正半周**，因此确认判定只看正向峰值。
- D2/D3 钳位并非理想电源轨钳位：D2 上钳位约 `V3V3 + Vf`、D3 下钳位约 `−Vf`，文档要求"必须实测/计算 `PVDF_ADC` 是否仍低于 ESP32 ADC 输入允许范围"。
- → 固件侧**必须**自行判定越界并拒绝，不得假设前端钳位已经足够（这也是 AC"ADC 采样不越界"的固件落点）。

**唤醒机制（已冻结，非 ext0/ext1、非 deep sleep）**

- `docs/embedded/guides/低功耗策略与实测验收.md` §3.2 原文把唤醒源限定为三项：运行态 PWR IO8、触摸 INT IO39（熄屏首触只唤醒）、**PVDF 比较器输出 IO11（实体敲击唤醒 + ADC 二次确认）**；QMI8658A INT1（IO41）为未来 WoW，MVP 不接入唤醒路径。
- 同文 §3.4 原文："IO11/IO39 由各 BSP 先配置有效电平，应用只开放 GPIO 唤醒源" → **IO11 的逐脚唤醒配置属 BSP 职责**，全局 `esp_sleep_enable_gpio_wakeup()` 已在 `app_main.c` 调用。
- 文档 §4 目标状态："熄屏空闲 → 只等 PVDF 比较器（IO11）、触摸首触（IO39）、PWR（IO8）唤醒事件"。
- 文档 §5.3 发布门禁含"触摸/实体敲击唤醒链在拔线（无 USB）状态下工作正常"、"编译与单元测试通过只代表软件路径存在，**不替代**真机功耗验收"。

**硬件开放项（不得在本 Story 冒充已闭合）**

`docs/hardware/电子木鱼-硬件原理图设计基线.md` §6.2：`HW-OI-008` = "PVDF 1M 限流、钳位、比较器料号/阈值/滞回 | 单次、1 秒 20 次及环境振动测试 | ADC 不越界、误触发/漏触发记录 | **[必须样机验证]**"。同基线 §6.2 末段"样机状态声明"：当前**未放置元件、未运行 ERC、未 PCB 打样、无实物插合与示波器/电子负载测试**；任何编译或静态检查通过都不能替代样机证据。

### 本 Story 冻结的接口契约

以下为 Story 1.2 结束时必须成立的、可被后续 Story 依赖的稳定事实（后续 Story 2.1 的有效敲击队列、Story 2.4 的导航、Story 3.x 的 UI 投影都据此接入）：

1. **候选事件入口**：事件总线上出现 `LEGBOT_EVENT_SOURCE_PVDF` 来源、`code = EWF_PVDF_EVENT_CANDIDATE` 的候选事件；`value` 为确认裕量（mV）。事件来源的产品名固定为 `physical_pvdf`（架构约定表：事件来源固定 `physical_pvdf`/`device_touch`）。
2. **BSP 稳定 API**：`pvdf_bsp_init()` / `pvdf_bsp_read_mv()` / `pvdf_bsp_comparator_level()` / `pvdf_bsp_wake_enable()` / `pvdf_bsp_wake_disable()` / `pvdf_bsp_isr_register()` / `pvdf_bsp_error_code()`。除这些入口外，任何模块不得直接定义 IO9/IO11 或直调 ADC/GPIO 驱动（AD-8）。
3. **服务生命周期**：`pvdf_input_service_init_contracts` / `prepare_run` / `cancel_prepared_run` / `deinit_contracts` / `run` / `request_stop` / `snapshot` / `queue`，风格与 `power_service` 一致；`LEGBOT_SERVICE_PVDF` 进入默认启动图。
4. **纯逻辑策略**：`pvdf_confirm_policy.{h,c}` 零 ESP-IDF 依赖，含 typed 结论枚举与确定性样本重放函数；主机测试与固件自检共用同一实现。
5. **自检项**：`SELFTEST_ITEM_PVDF_INPUT` 存在于 `selftest_item_id_t`，`SELFTEST_ITEM_COUNT` 由 6 变为 7，且 `_Static_assert` 同步更新。

### 架构、编码与 API 约束

**架构不变量（`ARCHITECTURE-SPINE.md`）**

- **AD-1**：`physical_pvdf` 与 `device_touch` 是仅有的两类正式输入，进入**同一个**有效敲击队列；除该队列外任何代码路径不得推进正式累计。充电中、完成遮罩中、故障锁定中直接忽略输入；熄屏首次触摸只唤醒、不计数。
  → 本 Story 只产**候选**，不建队列、不写累计。
- **AD-7**：长期能力实现为 FreeRTOS 服务任务，经 typed event/queue 通信；**ISR 只投递事件、不做产品决策**；可合并事件可丢最新；命令型不得静默丢弃且必须 busy/error + 日志。
- **AD-8**：只有 BSP 定义 GPIO、总线、电源使能与初始化顺序；硬件驱动在自有 BSP 下封装稳定 API、自检入口与错误码；**新增芯片目录必须登记 CMake**。
- **AD-9**：服务只拥有链路内部状态并发布 typed update；消费者只读不可变快照，不读其他服务内部变量。
- **AD-11**：原生 USB-Serial-JTAG；**USB 连接时禁用自动 light sleep**，验收必须覆盖"拔掉 USB 用电池运行"。
- **AD-12**：反馈失败不得阻塞核心链路（本 Story 无音频/渲染，但拒绝路径不得阻塞）。
- **AD-13（已被 PRD 部分取代）**：低电先落盘、PWR 长按由板级负责 —— 有效；"充电期间暂停输入"—— **作废**，见真源与冲突裁决 1。
- **AD-14**：唤醒/活动窗口属唯一联网链路范畴（Epic 2），本 Story 不触网。

**ESP-IDF v5.5.4 API 证据（`Embedded/AGENTS.md` 强制先查证后使用；以下为本次已核验结论，实现时仍以官方页面为准）**

ADC（组件名 **`esp_adc`**）：

- `#include "esp_adc/adc_oneshot.h"`、`"esp_adc/adc_cali.h"`、`"esp_adc/adc_cali_scheme.h"`；CMake 用 `REQUIRES esp_adc` 或 `PRIV_REQUIRES esp_adc`。
- 单元：`esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config, adc_oneshot_unit_handle_t *ret_unit);`，配置结构含 `unit_id`（`adc_unit_t`）、`clk_src`（`adc_oneshot_clk_src_t`，置 0 回落默认时钟源）、`ulp_mode`。
- 通道：`esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle, adc_channel_t channel, const adc_oneshot_chan_cfg_t *config);`，配置结构含 `atten`（`adc_atten_t`）与 `bitwidth`（`adc_bitwidth_t`）。
- 读数：`esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int *out_raw);` —— **"should not be used in an ISR context"**，且可能返回 `ESP_ERR_TIMEOUT`（此时原始结果无效）。开启电源管理时该函数会轮询 CPU 以保持时钟频率稳定。
- 一体化：`esp_err_t adc_oneshot_get_calibrated_result(adc_oneshot_unit_handle_t handle, adc_cali_handle_t cali_handle, adc_channel_t chan, int *cali_result);`（读 + 校准 + 转 mV）。
- 引脚映射：`esp_err_t adc_oneshot_io_to_channel(int io_num, adc_unit_t *const unit_id, adc_channel_t *const channel);` 与 `adc_oneshot_channel_to_io(...)`。
- 枚举：`adc_atten_t` = `ADC_ATTEN_DB_0` / `ADC_ATTEN_DB_2_5` / `ADC_ATTEN_DB_6` / `ADC_ATTEN_DB_12`（`ADC_ATTEN_DB_11` 为 deprecated，行为等同 12 dB）；`adc_bitwidth_t` = `ADC_BITWIDTH_DEFAULT` / `_9` / `_10` / `_11` / `_12` / `_13`；`adc_unit_t` = `ADC_UNIT_1` / `ADC_UNIT_2`。
- 校准：`esp_err_t adc_cali_check_scheme(adc_cali_scheme_ver_t *scheme_mask);`、`esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t handle, int raw, int *voltage);`；`ESP32-S3 支持 curve fitting`（`adc_cali_create_scheme_curve_fitting()`，配置 `adc_cali_curve_fitting_config_t{unit_id, chan, atten, bitwidth}`）；`LINE_FITTING` 与 `CURVE_FITTING` 为两种 scheme 版本枚举。
- 约束：单个 ADC 单元同一时刻只能处于 continuous 或 oneshot 之一；**ADC2 与 Wi-Fi 共享**，故只用 ADC1；重复申请已占用实例返回 `ESP_ERR_NOT_FOUND`。
- 原始值换算：`Vout = Dout * Vmax / Dmax`，`Vmax` 取决于衰减档、`Dmax = 2^bitwidth`。文档未列出各衰减档的具体可测电压上限，需查数据手册 ADC 特性章节；因此**越界判定必须用运行期可获得的量程上限常量或经 `adc_cali_raw_to_voltage()` 的返回值与量程比较**，不得凭记忆写死电压数字。

GPIO 唤醒（light sleep）：

- `esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type);` —— 文档原文 **"Only GPIO_INTR_LOW_LEVEL or GPIO_INTR_HIGH_LEVEL can be used."**，返回 `ESP_OK` 或 `ESP_ERR_INVALID_ARG`。对应 `esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num);`。
- `esp_err_t esp_sleep_enable_gpio_wakeup(void);` —— "Enable wakeup from light sleep using GPIOs"，**仅 light sleep**，无 deprecated 标记；本工程已在 `app_main.c:release_startup_light_sleep_lock()` 调用。
- **调用顺序**：先 `gpio_wakeup_enable()` 配好逐脚电平，再 `esp_sleep_enable_gpio_wakeup()` 开放全局源。
- light sleep 的 GPIO 唤醒源 **对任意 IO（RTC 或数字）均可用**；ext0/ext1 才要求 RTC IO（ESP32-S3 掩码有效范围 0–21），且 `esp_sleep_enable_ext1_wakeup` 已标注将在 release/v6.0 deprecated，替代为 `esp_sleep_enable_ext1_wakeup_io()`。**本工程不使用 ext0/ext1。**
- 注意：若启用 `PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP`，本 API 不可用（GPIO 模块被断电），需改用 `esp_deep_sleep_enable_gpio_wakeup` 或 EXT1。**本工程 `sdkconfig` 中该项未启用（已核验），启用即破坏唤醒路径，属红线。**
- 运行态中断相关签名：`gpio_set_intr_type(gpio_num_t, gpio_int_type_t)`、`gpio_isr_handler_add(gpio_num_t, gpio_isr_t, void *args)`、`gpio_install_isr_service(int intr_alloc_flags)`；`gpio_int_type_t` 成员为 `GPIO_INTR_DISABLE` / `GPIO_INTR_POSEDGE` / `GPIO_INTR_NEGEDGE` / `GPIO_INTR_ANYEDGE` / `GPIO_INTR_LOW_LEVEL` / `GPIO_INTR_HIGH_LEVEL` / `GPIO_INTR_MAX`。

**编码规范（`Embedded/AGENTS.md`，固件层强制）**

- 每个 `.c`/`.h` 以固定 Doxygen 文件头开头（`@file` / `@brief` / `@details` / `@author ZHC` / `@date YYYY-MM-DD`），中文注释与日志用无 BOM UTF-8。
- `.c` 布局固定顺序：文件头 → include → **日志 TAG 定义区** → 宏/常量/类型/变量 → `static` 内部函数声明 → 外部函数实现 → `static` 内部函数实现。`.h` 布局：文件头 → 保护宏包裹 → include → 业务 `#define` → 结构体/枚举/类型 → 外部函数声明。
- 每个使用 ESP 日志的 `.c` 定义 `static const char *TAG = "ASCII_NAME";`；**日志正文必须中文**，技术标识符（芯片名、寄存器名、函数名、宏名、`esp_err_to_name()` 结果、稳定错误码）可保留英文。建议 TAG：`BSP_PVDF`、`SVC_PVDF`。
- 头文件中声明的每个公开函数必须有中文 Doxygen（`@brief`/`@param`/`@return`）；每个业务 `#define`、枚举值、结构体字段、新引入的模块级变量都要有中文解释性注释。函数体内只在资源分配/释放、并发边界、错误处理、状态转换、硬件所有权、不明显约束处加简洁中文注释，禁止逐行噪声注释。
- 命名：`snake_case`；公开函数 `module_verb_object()`；ISR-safe 后缀 `_from_isr`/`_isr`；任务入口 `_task`；`typedef` 以 `_t` 结尾；宏与枚举值 `UPPER_SNAKE_CASE` 带模块前缀；模块级 `static` 变量前缀 `s_`；名称必须带单位后缀（`_ms`/`_us`/`_mv`/`_ticks`/`_count`）；新 EWF 模块用 `EWF_` 前缀（`EWF_PVDF_*`），保留的 legbot 资产继续 `LEGBOT_*`。
- `docs/embedded/style/C编码规范-Agent版.md` 的其余条款（ISO C17、4 空格缩进、禁 Tab、Allman 花括号、控制语句强制括号、120 列软上限、禁 VLA、禁止在 ISR/硬实时路径动态分配内存）一律遵守；断言只验证程序员不变量，**不得用于处理外部输入或可恢复的硬件故障**（ADC 越界、采样超时必须走返回值与 typed 状态，不得 `assert`）。

**既有工程锚点（不得漂移，`contract_checks.c` 已用 `#error` 固定）**

`CONFIG_IDF_TARGET_ESP32S3`、`CONFIG_PM_ENABLE`、`CONFIG_FREERTOS_USE_TICKLESS_IDLE`、`CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP == 3`、`CONFIG_RTC_CLK_SRC_INT_RC`、`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`（且 `CONFIG_ESP_CONSOLE_UART` 必须未定义）、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION`。板级无 32.768 kHz 晶振，RTC 保持内部 RC —— 这决定了**不能用绝对时间做唤醒窗口判定**，一切窗口必须基于单调毫秒。

**构建与测试环境**

- 环境激活：`source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` → `idf.py --version` 应为 `ESP-IDF v5.5.4`。
- 构建门禁：`idf.py set-target esp32s3 && idf.py reconfigure && idf.py build`。
- 主机测试门禁：`python3 Embedded/tests/run_host_tests.py`（不依赖 ESP-IDF，不执行烧录或串口动作）。
- 构建证据：`docs/embedded/build_records/<YYYYMMDD-HHMMSS>/{environment.txt,build-01.log,session.md}`；该目录被根 `.gitignore` 忽略，不入变更集。

### 实现边界（不属于本 Story）

**必须留给后续 Story，本 Story 不得顺带实现：**

| 能力 | 归属 |
|---|---|
| 统一有效敲击队列、输入闸门（充电/完成遮罩/故障锁定）、`physical_pvdf` 正式计数、`local_total` 高水位持久化 | Story 2.1、2.2 |
| 木鱼音频、RGB 反馈 | Story 2.3 |
| 触摸 INT（IO39）与 PWR 导航、熄屏首触只唤醒的**具体门控落点** | Story 2.4 |
| 3 秒周期 `automatic_tap` 定时器 | Story 2.6 |
| 低电与故障下的核心链路保护 | Story 2.7 |
| 设备 UI（含三环 `tap-rings` flash 运行时补丁、7 字带、状态栏） | Story 3.1–3.6 |
| USB-Serial-JTAG 诊断路径与 `light sleep` 关断策略 | Story 1.3 |
| 云端同步、Air780EGP 活动窗口 | Epic 2/5/7 |
| 样机电气验收（单次/20 次每秒/环境振动/ADC 越界签收，HW-OI-008） | Story 7.x + 样机阶段 |

**本 Story 也不做：** 改写 `Embedded/AGENTS.md` 的 `E2.5` 编号残留、修复 Story 1.1 的 5 项 deferred、统一 `docs/embedded/style/` 与 `Embedded/AGENTS.md` 的规范分歧、更新 `docs/embedded/guides/低功耗策略与实测验收.md` 中"`Embedded/` 仍为绿地"的过期陈述（属文档漂移，记录在 epic retrospective）。

### 红线与禁止事项

1. **禁止** `esp_deep_sleep_start()` / `esp_restart()` / 仅关屏 / 空循环模拟关机；**禁止**用任何 GPIO 驱动 LTC2954 `KILL`（`EWF_BSP_KILL_LTC2954_GPIO` 已被 `_Static_assert` 钉为 `GPIO_NUM_NC`）。唤醒语义只能是 light sleep 的 GPIO 唤醒。
2. **禁止**第二计数路径：本 Story 不得出现任何推进正式累计的代码；候选事件是唯一出口。
3. **禁止**在 ISR 上下文中：读 ADC（`adc_oneshot_read()` 文档明令禁止）、打印日志、`vTaskDelay`、做产品决策、动态分配内存。
4. **禁止**硬编码 ADC 通道号：必须经 `adc_oneshot_io_to_channel()` 由 IO9 解析并校验。
5. **禁止**把 IO11 配成低电平/下降沿唤醒（与硬件极性相反）。
6. **禁止**保存或上报 PVDF 原始波形；快照只允许计数与 typed 原因。
7. **禁止**引入任何充电状态读取或充电门控（`charging_pause`/`charge_state`/`is_charging` 是合同测试的禁用符号）。
8. **禁止**让 QMI8658A 参与敲击判定或唤醒路径。
9. **禁止**启用 `PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP`（会直接使 GPIO 唤醒失效）。
10. **禁止**在 services 层直接定义 IO9/IO11、直调 ADC/GPIO 驱动或跨服务同步直调；一切经 BSP 稳定 API（AD-7/AD-8）。
11. **禁止**把 PVDF 前端料号、阈值、滞回、消抖、低功耗参数写成承诺性事实；一切电气数值必须标注"设计值、未冻结、需样机实测"。
12. **禁止**声称样机验证：`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 保持 `0U`，自检证据不得升级为 `HARDWARE_VERIFIED`；编译通过不等于硬件通过。
13. **禁止**未查 ESP-IDF v5.5.4 官方文档即调用新 API（不得用 freertos.org、vanilla FreeRTOS、`latest`/`stable` 页面或其它芯片/版本页面作为依据）。
14. **禁止**把 AI 思维链、内部推理、设计过程、调试说明、Node ID、`data-pencil-id`、`TODO`、`draft`、`placeholder` 或作者提示写进任何用户可见文本、设备 UI、Pen 画面、静态 HTML 或前端文案（本 Story 无 UI 产出，但日志与 `detail_code` 属产品可见面，同样受约束：只写中文状态短句与稳定 ASCII 码，不写解释性文字）。
15. **禁止**任何变更性 git 操作；File List 口径沿用 autopilot 的 `story-local.diff` 排除规则（故事文件与 `sprint-status.yaml` 不计入变更集）。

### 测试与完成判定

**无样机条件下可判定的（本 Story 的完成证据）**

1. `python3 Embedded/tests/run_host_tests.py` 退出码 0，且新增 PVDF 纯逻辑用例全绿：单次敲击→1 个候选；1 秒 20 次→20 个候选且不漏不重；携带振动→0 个候选；超量程→`REJECTED_OUT_OF_RANGE`；盲窗样本→`REJECTED_BLIND_WINDOW`；采样失败→`REJECTED_SAMPLE_FAILED`。
2. `idf.py build` 全绿（`CONFIG_IDF_TARGET=esp32s3`），无新增告警；`contract_checks.c` 的新旧 `_Static_assert` 全部通过；`test_bsp_contract.py` 的禁用符号扫描通过（无 `charging_pause`/`charge_state`/`is_charging`/`esp_deep_sleep_start`/`esp_restart`，未占用 `GPIO_NUM_45/46`）。
3. PVDF 自检项在 `selftest_service` 串行执行中产出 `SOFTWARE_OBSERVED` 结论，`detail_code` 含 `HW_OI_008_PENDING`；证据类别不得为 `HARDWARE_VERIFIED`。
4. 事件总线可观测到候选事件（来源 `PVDF`、`code = EWF_PVDF_EVENT_CANDIDATE`）；快照计数与拒绝原因可查。
5. `LEGBOT_SERVICE_COUNT == 4`、`SELFTEST_ITEM_COUNT == 7`，且默认启动图包含 `pvdf_task`。

**无样机条件下不可判定的（必须保持 pending，不得写入完成声明）**

- 比较器输出极性、消抖与误触发率：需目标外壳 + 样机敲击/环境振动测试（HW-OI-008）。
- 实际敲击波形能否超过 148.1 mV 确认阈值、阈值窗口 127.6–168.6 mV 的真实边界。
- `PVDF_ADC` 在目标瞬态下是否越界的**电气**证据（固件只提供越界判定与拒绝路径，钳位职责在前端）。
- 拔掉 USB 用电池运行时的唤醒链行为与整机静态电流（空闲灌流 ≈ 330 µA 的实测占空比影响）。
- 单次与 1 秒 20 次的端到端计数稳定性（需正式队列，Story 2.1 起）。

**建议的验证顺序**：先跑主机测试（纯逻辑）→ 再跑源码合同扫描 → 最后 `idf.py build`。三者全绿才可进入 review；任一失败必须如实报告为 PVDF **候选输入**失败，不得改写为通过。

## Project Structure Notes

**新增（本 Story 拥有的路径）**

```
Embedded/components/BSP/PVDF/                          # 新增 BSP 目录（须登记 CMake）
  pvdf_bsp.h
  pvdf_bsp.c
Embedded/components/services/pvdf_input_service/        # 新增服务目录
  pvdf_confirm_policy.h                                 # 纯逻辑，零 ESP-IDF 依赖
  pvdf_confirm_policy.c
  pvdf_input_service.h
  pvdf_input_service.c
Embedded/tests/test_pvdf_confirm_policy.c               # 新增主机 C 用例
```

**修改（必须触碰的最小集合，每处都有明确理由）**

```
Embedded/components/BSP/CMakeLists.txt                  # src_dirs/include_dirs 加 PVDF；requires 加 esp_adc
Embedded/components/BSP/BSP_INCLUDE/bsp_include.h       # 聚合 pvdf_bsp.h
Embedded/components/BSP/BSP_INCLUDE/bsp_resources.c     # PVDF 条目改为真实归属（reserved=false、owner、boundary、init_stage=BOARD_PINS）
Embedded/components/BSP/BOARD/bsp_board.c               # 日志文案：PVDF 已纳管，IO46/IO45 仍保留
Embedded/components/services/CMakeLists.txt             # SRCS/INCLUDE_DIRS 加 pvdf_input_service
Embedded/components/services/legbot_services.h          # legbot_service_id_t 追加 LEGBOT_SERVICE_PVDF
Embedded/components/services/legbot_services.c          # 描述符、栈大小、启动/停止顺序、契约初始化与回滚
Embedded/components/services/selftest_service/selftest_service.c  # PVDF handler + BSP 资源表断言随 reserved 变化同步
Embedded/components/app_state/app_state.h               # selftest_item_id_t 追加 SELFTEST_ITEM_PVDF_INPUT
Embedded/components/contract_checks/contract_checks.c   # SERVICE_COUNT 3→4、ITEM_COUNT 6→7 + PVDF 新断言
Embedded/components/platform/event_bus/event_bus.h      # legbot_event_source_t 追加 LEGBOT_EVENT_SOURCE_PVDF
Embedded/main/app_main.c                                # 唤醒源中文日志补 PVDF 比较器 IO11
Embedded/tests/run_host_tests.py                        # 注册 PVDF 纯逻辑用例
Embedded/tests/test_bsp_contract.py                     # 追加 PVDF 源码合同断言（基于当前工作区内容追加）
```

**对齐与偏离说明**

- 目录结构符合 `Embedded/components/` 现有布局（`BSP/<芯片或功能>/` 与 `services/<service_name>/`），与 `CO5300`/`KEY`/`power_service` 完全同构。
- `ARCHITECTURE-SPINE.md` 的目录树写作 `components/{BSP, platform, services, app_state, main}`，`Embedded/AGENTS.md` §1 写作 `components/{BSP, ui, services} + main`。**以实际仓库结构为准**：新增目录落 `components/BSP/PVDF/` 与 `components/services/pvdf_input_service/`，与既有组件并列，不新建顶层组件。
- `legbot_bsp_resource_t` **没有** ADC 单元/通道/衰减字段。本 Story 不扩展该结构体（避免无必要地改动被多处消费的共享类型），而是由 BSP 在运行期用 `adc_oneshot_io_to_channel()` 解析并把结果作为自检可观测事实。若实现时判断必须扩展，须在 story 的 Change Log 中说明理由。
- 变更集口径：故事文件本身与 `sprint-status.yaml` 不计入变更集（autopilot 的 `story-local.diff` 用 `--exclude` 排除）；`docs/embedded/build_records/**` 被 `.gitignore` 忽略。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md`#Story 1.2: 闭合 PVDF 唤醒与 ADC 二次确认]（本 Story 的 BDD 验收与覆盖需求 FR-E-002、AD-1 的唯一真源）
- [Source: `_bmad-output/planning-artifacts/epics.md`#Functional Requirements#FR-E-002]（PVDF 是唯一 MVP 实体传感器；比较器负责唤醒，ESP32 ADC 二次确认；QMI8658A 不参与 MVP 计数；普通振动不应误触发；ADC 不越界）
- [Source: `_bmad-output/planning-artifacts/epics.md`#Additional Requirements#AD-1]（单一正式输入链路：除统一有效敲击队列外不得推进正式累计；原始波形不保存/不上报）
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#5.2 FR-E-002]（比较器唤醒 + ADC 二次确认；验收口径"目标外壳演示签收"；ADC 输入不越界）
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#14 工程验证入口]（GPIO9 ADC、GPIO11 比较器；PVDF 模拟前端、比较器阈值、20 次/秒连续输入、误触发验证项）
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#13.1 Open Question 4]（主控不检测充电状态、所有供电条件均可用；旧"充电暂停"变体在同步前不得实现为产品行为）
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`#Invariants#AD-1、AD-7、AD-8、AD-9、AD-11、AD-12、AD-13]（输入归一、服务任务与 ISR 边界、BSP 独占资源、device_state 事实源、USB 休眠边界、故障隔离）
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`#Consistency Conventions]（事件来源固定 `physical_pvdf`/`device_touch`；固件日志中文、模块 TAG 用 ASCII）
- [Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md`#2.1 权威 GPIO 总表]（`PVDF_ADC`=IO9/ADC1_CH8、`PVDF_CMP_WAKE`=IO11 及备注）
- [Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md`#5.4.1 PVDF 模拟前端与比较器]（ASCII 拓扑；"1MΩ、钳位器件、偏置/泄放、比较器料号、阈值、滞回、输出极性与消抖均为[必须样机验证]"；"PVDF 只产生 `physical_pvdf` 事件，不保存或上传原始波形"）
- [Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md`#6.2 开放项与样机状态声明]（HW-OI-008 必须样机验证；未放置元件、未 ERC、未打样、无示波器/电子负载测试）
- [Source: `docs/hardware/外围电路设计/TLV7042DGKR与TLV2369IDGKR-PVDF前端设计与接线.md`#3 参数复算]（VTH=148.1 mV、等效源阻抗 44.89 kΩ、τ=4.49 ms/5τ≈22.4 ms、内部迟滞 3/10/25 mV、开漏上拉 10 kΩ、空闲灌流 ≈330 µA、ADC 隔离 τ=100 µs）
- [Source: `docs/hardware/外围电路设计/TLV7042DGKR与TLV2369IDGKR-PVDF前端设计与接线.md`#3 未闭合事项]（空闲态比较器输出恒低；"唤醒信号消抖与比较器输出极性"必须样机实测；"固件侧的低功耗唤醒极性配置须与之一致"；TLV7042 上电复位期间输出高阻被上拉为逻辑高）
- [Source: `docs/hardware/外围电路设计/TLV7042DGKR与TLV2369IDGKR-PVDF前端设计与接线.md`#2 逐脚接线 + 定稿网表交叉验证]（D2 上钳位约 `V3V3 + Vf`、D3 下钳位约 `−Vf`，必须实测 `PVDF_ADC` 是否仍低于 ESP32 ADC 输入允许范围）
- [Source: `docs/hardware/定稿/Netlist_Schematic1_2026-09-21.tel`#行 142–144]（`PVDF_ADC` / `PVDF_CMP_WAKE` / `PVDF_RAW` 的权威端点定义）
- [Source: `docs/embedded/guides/低功耗策略与实测验收.md`#3.2 唤醒源、#3.4 API 行、#4 目标状态、#5.3 发布门禁]（唤醒源限定三项；"IO11/IO39 由各 BSP 先配置有效电平，应用只开放 GPIO 唤醒源"；熄屏空闲只等 PVDF/触摸/PWR；编译与单测不替代真机功耗验收）
- [Source: `docs/embedded/bsp/外设硬件软件二开.md`#7 已知坑]（比较器与模拟前端料号、输入范围、阈值均未冻结，冻结前不得写入承诺性参数；比较器唤醒后仍须 ADC 确认才计，避免只靠电平跳变计数；IO9 是 ADC1_CH8，配置衰减时保证输入不越界；不保存不上报原始波形）
- [Source: `Embedded/AGENTS.md`#PWR 实体键与板级电源所有权、#ESP-IDF v5.5.4 ESP32-S3 API 校验规则、#ESP-IDF 固件 C/H 编码规则、#EWF 嵌入式与 cloud 边界]（固件层唯一强制规则；禁止 deep sleep 与软件关机；日志中文 + ASCII TAG；先查证后使用 API）
- [Source: `AGENTS.md`#3.5 EWF 差异红线]（"键盘输入含 PVDF（比较器 IO11 唤醒 + ADC IO9 确认，ADC1_CH8）"；充电暂停口径的历史来源）
- [Source: `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h`]（`EWF_BSP_PVDF_ADC_GPIO`、`EWF_BSP_PVDF_CMP_WAKE_GPIO`、`LEGBOT_BSP_RESOURCE_PVDF`、`legbot_bsp_init_stage_t`、BSP 稳定错误码 API）
- [Source: `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.c`]（`LEGBOT_BSP_RESOURCE_PVDF` 现有条目内容）
- [Source: `Embedded/components/BSP/KEY/key.c`、`KEY/key.h`]（逐脚 light sleep 唤醒 + 运行态边沿中断范式；ISR 停中断/重武装语义）
- [Source: `Embedded/components/services/power_service/power_boot_policy.{h,c}`]（纯逻辑可主机测试模块范式；`EWF_POWER_BOOT_DEBOUNCE_MS = 20U`）
- [Source: `Embedded/components/services/power_service/power_service.{h,c}`]（服务生命周期样板、`power_service_snapshot_t`、ISR 只投递 ISR-safe 消息）
- [Source: `Embedded/components/services/legbot_services.{h,c}`]（服务 ID 枚举、描述符表、启动/停止顺序、契约初始化与回滚）
- [Source: `Embedded/components/services/selftest_service/selftest_service.{h,c}`]（`selftest_service_register()`、canonical items、`SELFTEST_REGISTRY_CAPACITY 8U`、`handle_bsp_resource_table()` 的 PVDF 断言、`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE 0U`）
- [Source: `Embedded/components/app_state/app_state.h`]（`selftest_item_id_t`、`selftest_outcome_t`、`selftest_reason_t`、`selftest_evidence_t`、`selftest_item_result_t`、`SELFTEST_DETAIL_CODE_CAPACITY 48U`）
- [Source: `Embedded/components/contract_checks/contract_checks.c`]（现有 `_Static_assert` 清单，含 PVDF IO9/IO11、`LEGBOT_SERVICE_COUNT == 3`、`SELFTEST_ITEM_COUNT == 6`）
- [Source: `Embedded/components/platform/event_bus/event_bus.h`]（`legbot_event_source_t`、`legbot_event_type_t`、`legbot_event_t`、`event_bus_publish()`、`event_bus_publish_from_isr()`）
- [Source: `Embedded/main/app_main.c`]（六阶段串行初始化、`esp_sleep_enable_gpio_wakeup()` 唯一调用点、PM 配置与启动锁、唤醒源日志）
- [Source: `Embedded/tests/run_host_tests.py`、`tests/test_bsp_contract.py`、`tests/test_power_boot_policy.c`]（主机测试门禁与源码合同扫描范式、禁用符号名单）
- [Source: `_bmad-output/implementation-artifacts/1-1-建立定稿电源-启动与-bsp-自检.md`]（上游 Story 情报、边界声明、deferred 清单、未验证缺口）
- [Source: `docs/embedded/style/C编码规范-Agent版.md`]（C 编码规范基线；文件头作者与保护宏口径见"真源与冲突裁决"3）
- ESP-IDF v5.5.4 官方依据（`Embedded/AGENTS.md` 强制）：ADC oneshot 驱动 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/adc_oneshot.html>；ADC 校准 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/adc_calibration.html>；GPIO <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/gpio.html>；睡眠模式 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/sleep_modes.html>

## Dev Agent Record

### Agent Model Used

Claude Code（BMad `bmad-dev-story` 流程，无人监管单轮执行）

### Debug Log References

- 主机门禁：`python3 Embedded/tests/run_host_tests.py`（`/tmp/pvdf_host_tests2.log`），退出码 0；源码合同 26/26 通过。
- 构建记录：`docs/embedded/build_records/20260922-114124/{environment.txt,build-01.log,build-02.log,session.md}`（该目录被 `.gitignore` 忽略，不入变更集）。

### Completion Notes List

**实现要点**

1. **唤醒链路**：IO11 由 `gpio_config()` 配成上拉关闭的输入（`GPIO_INTR_POSEDGE`），同时挂 light sleep 逐脚高电平唤醒（`gpio_wakeup_enable(..., GPIO_INTR_HIGH_LEVEL)`）与运行态上升沿中断。两条链路同时存在、互不替代，且逐脚武装发生在板级引脚阶段（`bsp_board.c` → `pvdf_bsp_init()`），严格早于 `app_main` 的全局 `esp_sleep_enable_gpio_wakeup()`。
2. **ISR 边界**：比较器 ISR 在 BSP 内先停该脚中断再转交服务回调；服务回调只取 `xTaskGetTickCountFromISR()` 并 `xQueueSendFromISR` 投递 typed 消息。ADC 读取、二次确认判定与事件发布全部在 `pvdf_task` 上下文，且几何上不可能在 ISR 内触发（`test_pvdf_service_reads_adc_only_in_task_context` 按函数体边界断言）。
3. **ADC 通道**：用 `adc_oneshot_io_to_channel((int)EWF_PVDF_ADC_GPIO, ...)` 运行期解析，`ADC_CHANNEL_8` 仅作为一致性期望值使用，绝不作为配置通道；解析不一致时返回稳定码 `PVDF-E02` 并降级（不配置通道、`pvdf_bsp_read_mv()` 返回 `ESP_ERR_INVALID_STATE`）。
4. **越界判定**：原始码上限由 `(1U << SOC_ADC_DIGI_MAX_BITWIDTH) - 1U` 运行期推出；毫伏上限由 `adc_cali_raw_to_voltage(cali, raw_full_scale, …)` 从校准曲线推出。超时或触边界一律返回 `ESP_ERR_TIMEOUT` / `ESP_ERR_INVALID_RESPONSE`，由服务映射为 `REJECTED_OUT_OF_RANGE` 且不重试；校准不可用时降级为原始读数换算路径并记中文告警，同时在自检可观测事实中暴露 `calibration_available=false`。
5. **单一候选来源**：`pvdf_task` 只在 `CANDIDATE_TAP` 时 `event_bus_publish()`，`source=LEGBOT_EVENT_SOURCE_PVDF`、`type=LEGBOT_EVENT_TYPE_SIGNAL`、`code=EWF_PVDF_EVENT_CANDIDATE`、`value` 为确认裕量（mV），产品名常量 `EWF_PVDF_EVENT_SOURCE_NAME = "physical_pvdf"`。全 Story 无任何推进正式累计的代码路径，快照只含计数与 typed 原因（无采样序列/波形）。
6. **可复现自检**：`pvdf_confirm_policy.c` 内是编译期固定的 28 条合成激励与同一份重放函数，固件自检（`handle_pvdf_input`）与主机测试共用；两次重放按字节比较完全一致。误触发定义为"非敲击类别产出候选"，全部类别均为 0。
7. **证据类别**：自检项固定 `SELFTEST_EVIDENCE_SOFTWARE_OBSERVED`，`detail_code` 携带 `HW_OI_008_PENDING`；`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 保持 `0U`，无任何升级为 `HARDWARE_VERIFIED` 的路径。

**对 story 规范的两处有意偏离（均已在实现处注释说明）**

1. `pvdf_confirm_policy_on_sample()` 的第三个参数由 story 写的 `sample_ok`（bool）改为 typed 枚举 `ewf_pvdf_sample_status_t`（VALID / OUT_OF_RANGE / FAILED）。理由：AC 1.3 与 AC 2.1 要求把"读数越界/超时"与"其它读取失败"分成两个互不混淆的 typed 拒绝原因（`REJECTED_OUT_OF_RANGE` 与 `REJECTED_SAMPLE_FAILED`），布尔参数会丢失该区分，使样本表无法断言分类正确。
2. `pvdf_bsp_selfcheck()`（BSP 自检入口）输出 `pvdf_bsp_selfcheck_t`，而不是扩展 `legbot_bsp_resource_t`。理由与 story 的 Project Structure Notes 预判一致：该结构体被多处消费，本 Story 不扩展它，ADC 解析结果改由 BSP 运行期暴露为自检可观测事实。

**待样机验证（不得据此声明已闭合）**

比较器输出极性/消抖/误触发率、实际敲击能否超过 148 mV 确认阈值与阈值窗口 127.6–168.6 mV 的真实边界、`PVDF_ADC` 在目标瞬态下是否越界的电气证据、拔 USB 用电池运行时唤醒链行为与整机静态电流——全部属硬件开放项 `HW-OI-008`。本次编译与主机测试通过只证明软件路径存在，不替代真机与电气验收。

**已执行的验证**

| 门禁 | 命令 | 结果 |
| --- | --- | --- |
| 主机门禁（源码合同 + PWR/BOOT 纯逻辑 + PVDF 纯逻辑） | `python3 Embedded/tests/run_host_tests.py` | 退出码 0；contract 26/26、power 9/9、pvdf 全部通过 |
| ESP-IDF 构建 attempt 01 | `source activate_idf_7355072.sh` → `idf.py reconfigure` + `idf.py build` | 退出码 0，`warning:` 0，五个产物齐全 |
| ESP-IDF 构建 attempt 02（修复后重跑） | 同上 | 退出码 0，`error:`/`warning:` 均 0，五个产物齐全 |

**未执行（明确声明）**：未烧录、未启动、未连接串口监控；未执行任何变更性 git 操作。

### File List

**新增**

- `Embedded/components/BSP/PVDF/pvdf_bsp.h`
- `Embedded/components/BSP/PVDF/pvdf_bsp.c`
- `Embedded/components/services/pvdf_input_service/pvdf_confirm_policy.h`
- `Embedded/components/services/pvdf_input_service/pvdf_confirm_policy.c`
- `Embedded/components/services/pvdf_input_service/pvdf_input_service.h`
- `Embedded/components/services/pvdf_input_service/pvdf_input_service.c`
- `Embedded/tests/test_pvdf_confirm_policy.c`

**修改**

- `Embedded/components/BSP/CMakeLists.txt`（`src_dirs`/`include_dirs` 加 `PVDF`；`requires` 加 `esp_adc`）
- `Embedded/components/BSP/BSP_INCLUDE/bsp_include.h`（聚合 `pvdf_bsp.h`）
- `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h`（PVDF 引脚注释改为真实归属）
- `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.c`（`LEGBOT_BSP_RESOURCE_PVDF` 改为真实归属、`reserved = false`、`init_stage = BOARD_PINS`）
- `Embedded/components/BSP/BOARD/bsp_board.c`（板级引脚阶段调用 `pvdf_bsp_init()`；日志文案同步）
- `Embedded/components/services/CMakeLists.txt`（`SRCS`/`INCLUDE_DIRS` 加 `pvdf_input_service`，不新增 `esp_adc`）
- `Embedded/components/services/legbot_services.h`（追加 `LEGBOT_SERVICE_PVDF`）
- `Embedded/components/services/legbot_services.c`（描述符、队列、栈/优先级、停止顺序、契约初始化与回滚、启动顺序）
- `Embedded/components/services/selftest_service/selftest_service.c`（新增 `handle_pvdf_input`、canonical 元数据与登记；BSP 资源表断言随 `reserved` 变化同步）
- `Embedded/components/app_state/app_state.h`（追加 `SELFTEST_ITEM_PVDF_INPUT`）
- `Embedded/components/contract_checks/contract_checks.c`（`LEGBOT_SERVICE_COUNT` 3→4、`SELFTEST_ITEM_COUNT` 6→7；新增 PVDF 极性与窗口断言）
- `Embedded/components/platform/event_bus/event_bus.h`（追加 `LEGBOT_EVENT_SOURCE_PVDF`）
- `Embedded/main/app_main.c`（唤醒源中文日志补 PVDF 比较器 IO11）
- `Embedded/tests/run_host_tests.py`（新增 `run_pvdf_policy_host_test()` 同构分支）
- `Embedded/tests/test_bsp_contract.py`（在当前工作区内容之上追加 14 项 PVDF 合同断言）

> 构建记录 `docs/embedded/build_records/20260922-114124/` 被 `.gitignore` 忽略，按口径不计入变更集；故事文件与 `sprint-status.yaml` 同样不计入。

### Change Log

| 日期 | 变更 | 说明 |
| --- | --- | --- |
| 2026-09-22 | 从零实现 PVDF 候选输入链路（BSP + 纯逻辑策略 + 服务 + 自检项 + 事件来源） | Story 1.2 全部 8 个 Task、52 个子项完成；`LEGBOT_SERVICE_COUNT` 3→4、`SELFTEST_ITEM_COUNT` 6→7 |
| 2026-09-22 | 偏离记录：`_on_sample` 第三参数用 typed `ewf_pvdf_sample_status_t` 取代 story 写的 bool `sample_ok` | 为在样本表层面区分 `REJECTED_OUT_OF_RANGE` 与 `REJECTED_SAMPLE_FAILED` |
| 2026-09-22 | 偏离记录：BSP 自检入口输出独立 `pvdf_bsp_selfcheck_t`，不扩展 `legbot_bsp_resource_t` | 避免改动被多处消费的共享类型，与 story 的 Project Structure Notes 预判一致 |
| 2026-09-22 | 修复实现缺口：在 `handle_wake_message` 单一出口调用 `pvdf_input_service_rearm_isr()` | ISR 停中断后若无重武装，该脚中断会永久关闭；修复后构建 attempt 02 全绿 |

### Review Findings

代码审查（`bmad-code-review`，depth=deep，mode=full，四层：Blind Hunter / Edge Case Hunter / Verification Gap / Acceptance Auditor）。以下为 triage 后的结论；`patch` 项已在本轮应用并通过门禁复验。

**Decision（未决，阻塞 `done`）**

- [ ] [Review][Decision] 低轨 0 V 读数被判为越界，已超阈的真实敲击被整窗拒绝 [Embedded/components/BSP/PVDF/pvdf_bsp.c:176,197] — `raw <= 0` 与 `millivolt <= 0` 把配置位宽量程**内**的合法低轨读数返回 `ESP_ERR_INVALID_RESPONSE`；`pvdf_input_service.c:314-318` 把该码与超时一并映射为 `EWF_PVDF_SAMPLE_OUT_OF_RANGE`；`pvdf_confirm_policy.c:236-245` 对该状态**立即**终止整窗并判 `REJECTED_OUT_OF_RANGE`，且 `resolve_confirmation()` 中 `out_of_range_seen` 优先于「峰值超阈」。本 story 的硬件事实写明静止时 `PVDF_RAW` 趋近 0 V、单电源缓冲把负半周钳在 0 V，即确认窗口内出现低轨读数是设计上的正常情形；而 spec 实现口径只要求「校准后毫伏值必须落在该衰减档**可测上限**内」（只约束上限），并未要求下限拒绝。Edge Case Hunter 已用编译探针实测该后果：峰值 300 mV 的窗口一旦混入低轨采样即被判越界丢弃，`candidate_count` 为 0，候选项永远无法产出。修复存在两个方向——按 spec 意图只判上限，或保持 fail-closed 并把 0 V 明确视为异常——涉及 AC 1.3 的判定语义与尚未冻结的电气基线（HW-OI-008），超出单轮 autofix 的无歧义范围，需人工裁定后实施。

**Patch（已应用，4 项）**

- [x] [Review][Patch] 资源表阶段断言是空断言，收紧为 PVDF 专属谓词 [Embedded/tests/test_bsp_contract.py:368]
- [x] [Review][Patch] 任务体与停止分支的 PVDF 接线无断言，补 `service_task_entry` / `stop_service` 断言 [Embedded/tests/test_bsp_contract.py:372]
- [x] [Review][Patch] 恢复被本次改动误删的函数间空行（编码规范与既有风格） [Embedded/components/services/selftest_service/selftest_service.c:573]
- [x] [Review][Patch] 移除文件末尾多余空行 [Embedded/tests/run_host_tests.py][Embedded/tests/test_bsp_contract.py]

**Defer（已确认、本轮不修，15 项）**

- [x] [Review][Defer] 候选事件发布到尚无消费者的队列，第 17 条起每次发布必然超时 [Embedded/components/services/pvdf_input_service/pvdf_input_service.c:395] — deferred：`EVENT_BUS_QUEUE_DEPTH = 16`（`event_bus.c:12`），`event_bus_queue()` / `event_bus_group()` 在全仓无调用者，本 story 是唯一发布者。根因是消费者属 Story 2.1 的有效敲击队列，本 story 已按 spec 发布并保留 `ESP_LOGW` 有界告警（符合 AD-7「不得静默丢弃」），单方面加大队列深度只会推迟同一问题。
- [x] [Review][Defer] ISR 投递失败后比较器中断永久关闭且无重武装路径 [Embedded/components/BSP/PVDF/pvdf_bsp.c:521] — deferred：形态与 Story 1.1 复审已 deferred 的第 1 项完全相同（`key.c` 的 `key_pwr_isr`），spec 明确把该形态定为 `low` 且写明「本 Story 不要顺手修」，本实现按 Task 3 要求镜像了 `key.c` 语义。
- [x] [Review][Defer] 确认窗口内中断全程关闭，AD-7「合并为同一在飞确认」在固件中不可达 [Embedded/components/services/pvdf_input_service/pvdf_input_service.c:353] — deferred：`drain_merged_wake_messages()` 与策略的合并分支只能被主机用例触达。因窗口 20 ms 显著小于 20 次/秒的 50 ms 间隔，行为结果等价于「丢最新」，对已冻结的样本表无害；覆盖「窗口内到达的敲击」需样机敲击波形数据。
- [x] [Review][Defer] 自检观察预算等于本项 timeout，`INCONCLUSIVE` 会被引擎改判为超时失败 [Embedded/components/services/selftest_service/selftest_service.c:948] — deferred：与 Story 1.1 复审 deferred 第 3 项同型（spec 标 `maybe-false`）。本项 `isr_registered` 通常在首轮轮询即就绪，实际不会用满 2000 ms；仅在 PVDF 链路上电失败时触发。
- [x] [Review][Defer] 4 条 `detail_code` 出口未携带 `HW_OI_008_PENDING` 标记 [Embedded/components/services/selftest_service/selftest_service.c:958,975,984,995] — deferred：AC 2.3 的证据口径主要约束该项的观察结论（成功/降级/不匹配三条已携带），取消与内部读取失败路径是否也应携带标记需与自检口径统一后确定，属口径决策而非单点缺陷。
- [x] [Review][Defer] AC 2.2 要求的各类样本计数只落在中文日志，未落入 `detail_code` [Embedded/components/services/selftest_service/selftest_service.c:1005-1037] — deferred：spec 自身存在张力——AC 2.2 要求计数落在 48 字节的 `detail_code`，而 Task 5 要求该字段写「形如 `PVDF_POLICY_OK_HW_OI_008_PENDING` 的稳定码」，二者无法同时满足，需先统一口径。
- [x] [Review][Defer] 降级路径写死 3300 mV 满量程 [Embedded/components/BSP/PVDF/pvdf_bsp.c:169-174] — deferred：spec 点名的反模式，但仅在无校准句柄的降级路径参与上限判定，且已带「设计值、未冻结」中文注释；移除需给出运行期可得的替代量程来源。
- [x] [Review][Defer] BSP 的越界判据与错误码映射只被源码文本扫描覆盖，无宿主可执行测试 [Embedded/components/BSP/PVDF/pvdf_bsp.c:176-205] — deferred：spec Task 8 只要求源码合同扫描；把纯整数判据抽到可主机编译的纯模块属测试架构改进，会新增公共面，宜与上一条未决项一并裁定。
- [x] [Review][Defer] `EWF_PVDF_ADC_ATTEN_LEVEL` 是无人引用的公开宏，与真正生效的 `EWF_PVDF_BSP_ADC_ATTEN` 构成双份真相 [Embedded/components/BSP/PVDF/pvdf_bsp.h:38] — deferred：`12U` 改为其它值不改变行为也不使任何门禁失败；清理属去重，需确认无后续 Story 引用。
- [x] [Review][Defer] `arm_runtime` 的 `@brief` 与实现语义矛盾，且盲窗锚点落在服务 run 而非 PVDF 初始化 [Embedded/components/services/pvdf_input_service/pvdf_confirm_policy.h:1787] — deferred：`@details` 已正确描述「本调用之后仍有盲窗」，属头文件措辞不一致；锚点位置使该拒绝路径在启动流程中不被触达（仅自检重放覆盖），改动涉及 AC 1.4 的锚点定义。
- [x] [Review][Defer] 样本表注释声称盲窗类别「附盲窗结束后的对照敲击」，表中无该样本 [Embedded/components/services/pvdf_input_service/pvdf_confirm_policy.h:1748] — deferred：对照敲击仅存在于主机用例 `test_pvdf_confirm_policy.c`，未进入「固件与主机共用」的同一张表；增删样本需同步更新冻结的期望契约。
- [x] [Review][Defer] `pvdf_bsp_init()` 部分失败时 `s_wake_enabled` 已置真但 `s_initialized` 仍假，`wake_disable()` 无法回收 [Embedded/components/BSP/PVDF/pvdf_bsp.c:258-271] — deferred：该路径会使板级初始化整体失败（`bsp_board.c` 返回错误并终止启动），实际不产生可观测的残留行为。
- [x] [Review][Defer] 多条早退路径不经过 `set_error()`，`s_last_error` 会保留陈旧值 [Embedded/components/BSP/PVDF/pvdf_bsp.c:226,255,212,275,325] — deferred：`pvdf_bsp_error_code()` 在服务与自检日志中作为「本次失败原因」输出，可能打印与当次失败无关的稳定码；属诊断精度问题，不影响判定。
- [x] [Review][Defer] `publish_snapshot()` 使用非阻塞取锁，争用时静默丢弃快照 [Embedded/components/services/pvdf_input_service/pvdf_input_service.c:420-430] — deferred：既有服务样板（`power_service`）的同一做法；快照为唯一读源，最坏情形是自检观察到滞后的 `blind_window_open`。
- [x] [Review][Defer] 产品可见日志含设计/流程解释性文字（红线 14 措辞） [Embedded/components/BSP/PVDF/pvdf_bsp.c:144][Embedded/components/services/pvdf_input_service/pvdf_input_service.c:187,402] — deferred：Story 1.1 既有日志为同一风格，属跨 Story 口径不一致，统一需与日志规范一并处理。

**Rejected（1 项）**

- `false` — 交付记录称样本表为「28 条合成激励」，实际 `s_selfcheck_wakes[]` 为 27 条（1+20+3+1+1+1）。事实成立，但其唯一修复是编辑本次审查的 story 文件本身，按 triage 规则拒绝。

