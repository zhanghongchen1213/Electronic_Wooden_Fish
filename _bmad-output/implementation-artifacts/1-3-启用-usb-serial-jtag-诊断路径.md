# Story 1.3: 启用 USB-Serial-JTAG 诊断路径

baseline_commit: fe98d7375f21b02f22ed2cbdb3824b1462940ace

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a 固件开发者，
I want 使用 ESP32-S3 原生 USB-Serial-JTAG 完成烧录、CDC 日志和 JTAG 调试，
So that 设备不需要额外 USB 转换芯片且诊断链路稳定。

## Acceptance Criteria

**覆盖需求：** FR-E-010、AD-11

### AC 1：烧录、CDC 日志与 JTAG 三路都走同一条原生 USB-Serial-JTAG 通道

**Given** 设备通过 GPIO19/20 连接 USB（`USB_D-`=IO19、`USB_D+`=IO20）
**When** 在主机上执行烧录、打开 CDC 日志并启动 JTAG 调试
**Then** 三种操作均可成功，且全部由同一条原生 USB-Serial-JTAG 通道承载：
- 烧录 = `idf.py -p <port> flash`（**不得**用 `app-flash` 替代；`app-flash` 不写分区表与已定案数据分区）
- CDC 日志 = `idf.py -p <port> monitor`，console 由 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` 独占
- JTAG 调试 = `idf.py openocd`，其参数取自构建产物 `build/project_description.json` 的 `debug_arguments_openocd`

**And** 日志模块 TAG 为稳定 ASCII 且正文为中文
**And** 工程不启用 CH340X 或 USB-OTG 业务

实现口径（本 Story 的强制解释，不得放宽）：

1. **三路同通道口径**：烧录、CDC 日志与 JTAG 调试必须由同一条原生 USB-Serial-JTAG 通道承载 —— 烧录走 `idf.py -p <port> flash`（**不得**用 `app-flash` 替代，`app-flash` 不写分区表与已定案数据分区；真机闭环 runbook §3）；CDC 日志走 `idf.py -p <port> monitor`，console 由 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` 独占（`sdkconfig.defaults:28`）；JTAG 走 `idf.py openocd`，其参数取自构建产物 `build/project_description.json` 的 `debug_arguments_openocd`（当前为 `-f board/esp32s3-builtin.cfg`）。
2. **判定分层口径**：当前仓库**无样机**，`docs/embedded/macos_esp_idf_hardware_test_runbook.md:43` 明文"**禁止** `idf.py flash`、`app-flash`、`erase-flash`、`monitor`"，第 233 行要求"看到 `To flash, run: idf.py flash` 提示后**到此停止**"。因此"三种操作均可成功"必须拆成两层判定：
   - **可在本 Story 判定**：三路入口的**软件路径与门禁**齐备（配置锚点、编译期 `#error`、端口唯一匹配门禁、`debug_arguments_openocd`、`flasher_args.json` 产物清单、脚本 fail-closed 行为），并以**预检回执**（可在无设备主机上真实运行并归档）作为证据。
   - **必须保持 `hardware_pending`**：真机上"烧录返回 0 且每个镜像出现 `Hash of data verified.`"、"CDC 日志出现完整启动且连接期间不随机失联"、"OpenOCD 连接成功"三项事实。
   - **禁止**把编译通过、主机测试通过或预检通过写成"USB 诊断链路已在真机验证"；**禁止**在无授权时要求操作员插线、按键或复位。
3. **日志口径**：所有 ESP 日志模块 `TAG` 为稳定 ASCII 串（全大写或数字下划线、无空格与中文），日志正文为中文，并**固化为可执行门禁**（`Embedded/AGENTS.md:181-190` 的规范此前无任何自动检查覆盖）。
4. **不引入第二套 USB 通道口径**：工程不启用 CH340X、USB-OTG、TinyUSB、USB Hub 或任何第二套 USB 转换/业务通道。判定依据：`docs/hardware/电子木鱼-硬件网络清单.json:35` 的 `"forbidden_components": ["CH340X","AMS1117","TPS631000"]`、硬件基线第 123 行"不需要 CH340X；…不启用 USB-OTG"、以及当前 `build/project_description.json` 的 `build_components`（74 项）中 USB 相关项**只有** `esp_driver_usb_serial_jtag`（无 `usb`、无 `tinyusb`）。同时 IO19/IO20 必须保持为 `USB_D-`/`USB_D+`（硬件基线第 105-106 行）。

### AC 2：USB 主机连接期间自动 light sleep 被项目锚点关断，且诊断脚本留下可复用回执

**Given** USB 调试连接保持期间
**When** 系统进入空闲/低功耗路径
**Then** 自动 light sleep 被按项目锚点禁用，CDC 不随机失联
**And** 诊断脚本保存烧录和日志回执，便于 Epic 2/3 自测复用

实现口径（本 Story 的强制解释，不得放宽）：

1. **关断机制口径**：架构 AD-11 与 `docs/embedded/guides/低功耗策略与实测验收.md:31,72,89` 把该规则写作"USB 连接时禁用自动 light sleep"，ESP-IDF v5.5.4 的**实际机制**是 —— `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y` 使芯片持续监测 USB Serial/JTAG 连接状态，**只要连接保持就获取 `ESP_PM_NO_LIGHT_SLEEP` 锁**（`$IDF_PATH/components/esp_driver_usb_serial_jtag/Kconfig:14-27` help 原文）。该选项**只约束自动 light sleep**；手工调用 `esp_light_sleep_start()` 即使在连接期间也不受保护。验收不得写成"系统整体禁用 light sleep"，也**不得**把 `ESP_PM_NO_LIGHT_SLEEP` 长期持有：拔掉 USB 后必须恢复自动 light sleep 合同。
2. **三项前提口径**："CDC 不随机失联"依赖三项前提同时成立，缺一即失效，本 Story 必须为三项都建立门禁：
   - console 独占 USB-Serial-JTAG：`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` **且** `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`。后者的准确理由：`choice ESP_CONSOLE_SECONDARY`（`$IDF_PATH/components/esp_system/Kconfig:296-316`）的 `default` 目标是 `ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG`，而该选项 `depends on !ESP_CONSOLE_USB_SERIAL_JTAG` —— 主 console 已是 USB-Serial-JTAG 时默认目标不可选，Kconfig 会回落到第一个可见选项 `ESP_CONSOLE_SECONDARY_NONE`。因此显式设置 NONE 的价值是**消除对 Kconfig 回落行为的隐式依赖**，并且它就是 `docs/embedded/macos_esp_idf_hardware_test_runbook.md:162-171` 已冻结的期望值（`sdkconfig.defaults:29` 已置）。**不得**据此推断"不设就会变成次级输出"；也不得改选 `ESP_CONSOLE_USB_CDC`（ROM CDC 驱动，会占用同一 USB PHY，属第二套 USB 通道）；
   - `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`；
   - `CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP` **未**启用（启用会把 GPIO/USB 模块断电，直接破坏唤醒与调试链路，属红线）。

   并且 `CONFIG_PM_ENABLE` 与 `CONFIG_FREERTOS_USE_TICKLESS_IDLE` 必须保持开启 —— `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` 的可用性 `depends on PM_ENABLE`，被关闭时该选项会被 Kconfig **静默丢弃**、从 `sdkconfig` 消失、编译期 `#error` 随即触发；门禁必须能发现这种状态。
3. **诊断脚本口径**：新建仓内可执行脚本 `Embedded/tools/usb_serial_jtag_diag.py`，子命令固定为 `ports` / `preflight` / `flash` / `monitor` / `openocd`；所有判定逻辑（端口记录解析、唯一匹配判定、`lsof` 占用判定、run 目录唯一性判定、预检结论）必须为**零 ESP-IDF、零第三方依赖**的纯函数，可被 `Embedded/tests/` 的主机用例直接导入。脚本**不进入固件构建**，不登记为 ESP-IDF 组件。
4. **回执口径**：回执目录与文件名**必须**沿用 `docs/embedded/guides/真机闭环runbook-macos.md` §5 已冻结的布局，**不得**新建第二套命名：`docs/embedded/hardware_test_records/<运行编号>/{environment.txt,build-01.log,flash-01.log,serial-01.log,openocd-01.txt,session.md}`（`openocd-01.txt` 为本 Story 为 JTAG 通道新增的回执，与既有 `flash-01.log`/`serial-01.log` 同构）。`environment.txt` 与 `session.md` 必须取自真实命令输出，**禁止凭记忆填写**（runbook:115）；本 Story 未执行的阶段必须显式写"未执行（当前无样机）"。该路径必须加入根 `.gitignore`（与既有 `docs/embedded/build_records` 同规则），避免串口日志与操作员记录入库。
5. **fail-closed 口径**：满足任一条件即以非零退出码拒绝执行 `flash`/`monitor`/`openocd`，并给出中文理由 —— 无确认端口、端口零匹配、端口多匹配、`--serial` 与枚举结果不符、运行目录已存在、IDF 版本越出 `>=5.5.4,<5.6.0`、必需构建产物缺失、目标端口已被 `idf_monitor` 占用。**禁止**按端口排序、最近出现时间或历史端口名自行猜选端口（runbook §7.1 硬门禁）。拒绝路径必须**不产生任何** `flash-*.log` / `serial-*.log`。

## Tasks / Subtasks

- [x] **Task 1：新建主机侧 USB-Serial-JTAG 诊断脚本，把真机闭环 runbook 的烧录/监控/JTAG 三路从"文档内联模板"落成仓内可执行物（AC: 1.1、1.2、2.3、2.4、2.5）**
  - [x] 1.1 新建 `Embedded/tools/usb_serial_jtag_diag.py`，子命令固定为 `ports` / `preflight` / `flash` / `monitor` / `openocd`；纯逻辑函数（端口记录解析、唯一匹配判定、`lsof` 占用判定、run 目录唯一性判定、预检结果判定）必须零 ESP-IDF、零第三方依赖（端口枚举经 `python -m serial.tools.list_ports -v` 文本解析，不 `import serial`）
  - [x] 1.2 归档布局**必须**沿用 `docs/embedded/guides/真机闭环runbook-macos.md` §5 已冻结的目录与文件名，并只新增 `openocd-01.txt` 一项：`docs/embedded/hardware_test_records/<运行编号>/{environment.txt,build-01.log,flash-01.log,serial-01.log,openocd-01.txt,session.md}`；**不得**新建第二套命名
  - [x] 1.3 fail-closed：满足任一条件即以非零码退出且**不**执行 `flash`/`monitor`/`openocd` —— 无确认端口、端口零匹配、端口多匹配、`--serial` 与枚举结果不符、运行目录已存在、IDF 版本越出 `>=5.5.4,<5.6.0`、必需构建产物缺失、目标端口已被 `idf_monitor` 占用
  - [x] 1.4 `flash` 分支只允许 `idf.py -p <port> flash`，输出经 `tee` 落 `flash-01.log` 且拒绝覆盖已存在日志；**不得**提供 `erase-flash`/`erase_flash`/`app-flash` 入口
  - [x] 1.5 `openocd` 分支调用 `idf.py openocd`（不带端口参数），并把该命令的 `build/openocd_out.txt` 归档为本次运行的 `openocd-01.txt`
  - [x] 1.6 `ports` 与 `preflight` 必须在**无设备主机**上可运行并输出确定结论（零匹配 → 明确的中文拒绝理由），使本 Story 能在无样机条件下产出真实回执

- [x] **Task 2：把"原生 USB-Serial-JTAG 独占、无 CH340X/USB-OTG"固化为源码合同门禁（AC: 1.4）**
  - [x] 2.1 在 `Embedded/tests/test_bsp_contract.py` 追加断言：`sdkconfig.defaults` 必须含 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`、`CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`（三项与 `docs/embedded/macos_esp_idf_hardware_test_runbook.md:162-171` 的期望清单逐一对应）
  - [x] 2.2 追加禁用断言：`sdkconfig.defaults` **不得**启用 `CONFIG_ESP_CONSOLE_UART_DEFAULT` / `CONFIG_ESP_CONSOLE_UART_CUSTOM` / `CONFIG_ESP_CONSOLE_USB_CDC`；`Embedded/main/CMakeLists.txt` 与 `Embedded/CMakeLists.txt` **不得**把 `usb` / `tinyusb` 列为组件依赖。**必须按组件 token 判定，不得用裸子串匹配** —— 否则会误命中 `esp_driver_usb_serial_jtag` 这类合法标识符（同"裁决 6"的判定纪律）
  - [x] 2.3 追加禁用符号断言：`Embedded/` 生产源码（`main/` + `components/**`）中不得出现 `ch340` / `usb_otg` / `tinyusb`（大小写不敏感），也不得新增 CH340X 驱动目录
  - [x] 2.4 断言 `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h` 的 `EWF_BSP_USB_SERIAL_JTAG_DM_GPIO`/`_DP_GPIO` 仍为 `GPIO_NUM_19`/`GPIO_NUM_20`，与 `contract_checks.c` 既有 `_Static_assert` 口径一致（防引脚漂移）

- [x] **Task 3：把"日志 TAG 稳定 ASCII + 正文中文"从口头规范变成可执行门禁（AC: 1.3）**
  - [x] 3.1 扫描 `main/` 与 `components/**` 全部 `static const char *TAG = "..."`：断言非空、纯 ASCII、全大写或数字下划线、无空格与中文
  - [x] 3.2 扫描全部 `ESP_LOGE/W/I/D/V(` 调用：断言格式串正文含中文（允许的例外仅限"纯技术符号串"，且必须显式列入测试内的 `ALLOWED_ASCII_ONLY_LOG_CALLS` 白名单并注明理由）
  - [x] 3.3 断言定义 `TAG` 的文件必须真的出现日志宏，且出现日志宏的文件必须定义 `TAG`（对应 `Embedded/AGENTS.md:183-189`"不使用 ESP 日志的文件不要定义 TAG"）
  - [x] 3.4 断言日志正文不含凭据类字样（`token`/`password`/`secret`/`appsecret` 大小写不敏感；对应 `docs/embedded/style/C编码规范-Agent版.md:156` 与 `Embedded/AGENTS.md:253`）

- [x] **Task 4：为 light-sleep / USB 调试锚点建立回归保护，防止后续 Story 静默破坏（AC: 2.1、2.2）**
  - [x] 4.1 断言 `Embedded/components/contract_checks/contract_checks.c` 中三处 USB 锚点 `#error` 全部存在：`!CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`、`#ifdef CONFIG_ESP_CONSOLE_UART`、`!CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION`
  - [x] 4.2 断言 `sdkconfig.defaults` **未**启用 `CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP`（启用即破坏 GPIO 唤醒与 USB 调试链路，属红线）
  - [x] 4.3 断言 `sdkconfig.defaults` 仍启用 `CONFIG_PM_ENABLE=y` 与 `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`（`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` 的可用性 `depends on PM_ENABLE`；缺失时该选项会被 Kconfig 静默丢弃，门禁必须能发现）
  - [x] 4.4 断言 `Embedded/components/esp_pm/` 覆写资产仍存在（`CMakeLists.txt`/`Kconfig`/`sdkconfig.rename`/`generate_v554_pm_impl.py`），**但不修改其任何内容**（见红线 8）

- [x] **Task 5：把诊断脚本的回执约定固化，并避免串口日志与操作员记录入库（AC: 2.4）**
  - [x] 5.1 `preflight` 必须生成 `environment.txt`，内容取自真实命令输出，至少含：IDF 版本、`git rev-parse --short HEAD`、`git status --short`、以及 `rg -n 'CONFIG_IDF_TARGET=|CONFIG_ESPTOOLPY_FLASHSIZE=|CONFIG_ESPTOOLPY_FLASHMODE=|CONFIG_ESPTOOLPY_FLASHFREQ=|CONFIG_ESPTOOLPY_MONITOR_BAUD=|CONFIG_ESP_CONSOLE_|CONFIG_USJ_' sdkconfig` 的结果（沿用 `docs/embedded/macos_esp_idf_hardware_test_runbook.md:150-158` 的取证命令，不另立口径）
  - [x] 5.2 `session.md` 按 `docs/embedded/guides/真机闭环runbook-macos.md:669-710` 的模板骨架生成，且**未执行的阶段必须显式写"未执行（当前无样机）"**，不得留空、不得凭记忆填写（runbook:115）
  - [x] 5.3 在根 `.gitignore` 追加 `docs/embedded/hardware_test_records`，与既有第 49 行 `docs/embedded/build_records` 同规则；理由：串口日志与操作员动作时间线属本地取证产物，runbook 明确"默认不擅自提交测试记录"，而当前该路径**未被**忽略（`git check-ignore` 已确认不命中）

- [x] **Task 6：接入主机门禁入口（AC: 2.3、2.5）**
  - [x] 6.1 新建 `Embedded/tests/test_usb_serial_jtag_diag.py`（纯 Python），覆盖：端口文本解析、单匹配通过、零匹配拒绝、多匹配拒绝、序列号不符拒绝、`lsof` 判定、run 目录已存在拒绝、IDF 版本越界拒绝、缺产物拒绝、以及"`flash` 分支拒绝执行"的判定函数返回值
  - [x] 6.2 在 `Embedded/tests/run_host_tests.py` 注册该用例（与 `test_bsp_contract` 同一反射发现路径，或在 `main()` 追加一条 `run_*_host_test()`；二选一并在 Change Log 说明）
  - [x] 6.3 确认 `test_bsp_contract.py` 的 `PRODUCTION_SOURCES` 白名单**覆盖新增的 `Embedded/tools/*.py`**，否则新脚本不会被任何门禁扫描（该白名单当前不含 `components/ui` 与 `components/esp_pm`，属既有缺口，本 Story 只补 `tools/`）

- [x] **Task 7：产出本 Story 的证据与结论（AC: 1.2、2.4）**
  - [x] 7.1 环境激活 `source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` → `idf.py --version` 为 `ESP-IDF v5.5.4`；执行 `idf.py reconfigure` + `idf.py build`，要求退出码 0、`error:`/`warning:` 均为 0、五个产物齐全；归档至 `docs/embedded/build_records/<运行编号>/`
  - [x] 7.2 在 `Embedded/` 执行 `python3 Embedded/tests/run_host_tests.py` 与 `python3 -m py_compile` 全部新增/修改脚本，退出码 0
  - [x] 7.3 **预检回执（本 Story 的核心可判定证据）**：在无设备主机上真实运行 `python3 Embedded/tools/usb_serial_jtag_diag.py preflight` 与 `... ports`，归档输出与退出码，证明：(a) 预检对可见门禁给出确定结论；(b) 无确认端口时 `flash`/`monitor`/`openocd` 被 fail-closed 拒绝且未产生任何 flash/serial 日志
  - [x] 7.4 在 `session.md` 与 Completion Notes 中列出 `hardware_pending` 清单，逐项写明未验证原因与复验入口（见"测试与完成判定"）

## Dev Notes

### 真源与冲突裁决

按以下顺序读取与裁决，不以旧迁移代码或旧文档陈述覆盖硬件/PRD/架构：

1. `docs/hardware/电子木鱼-硬件原理图设计基线.md`、`docs/hardware/电源网络命名规范.md`、`docs/hardware/电子木鱼-硬件网络清单.json`、`docs/hardware/定稿/`（引脚、USB 拓扑、禁用器件、开放项）。
2. `prd.md` §5.10 FR-E-010 与 §14 工程验证入口。
3. `ARCHITECTURE-SPINE.md` AD-7/AD-8/AD-11/AD-12 与 §工程验证门禁。
4. 本 Story 的 AC；UX 四份规范不改变 USB 板级事实。
5. `AGENTS.md`、`Embedded/AGENTS.md`、`docs/embedded/**`、`docs/embedded/style/C编码规范-Agent版.md`。

**裁决 1 —— AC 要求真机三路验证，但当前无样机且现行 runbook 明文禁止烧录/监控（本 Story 最重要的裁决）**

`epics.md` Story 1.3 的 BDD 写"在主机上执行烧录、打开 CDC 日志并启动 JTAG 调试 → 三种操作均可成功"。而 `docs/embedded/macos_esp_idf_hardware_test_runbook.md:3` 写"本项目尚无样机，**不执行烧录、串口监控或真机动作**"，第 43 行写"**禁止** `idf.py flash`、`app-flash`、`erase-flash`、`monitor`"，第 233 行写"看到 `To flash, run: idf.py flash` 提示后**到此停止**"。
→ **裁决**：本 Story 交付**软件路径 + 门禁 + 预检回执**，真机三路成功一律标 `hardware_pending`，复验入口是 `docs/embedded/guides/真机闭环runbook-macos.md`（样机到位后执行）。**不得**以任何方式执行 `idf.py flash`/`monitor`/`erase-flash`，**不得**要求操作员插线、按键或复位。这与 Story 1.1/1.2 在无样机条件下的既有交付口径一致（`hardware_pending`、`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 保持 `0U`）。
→ 同时：本 Story **不修改**该 runbook 的"禁止执行"条款。样机到位后由 runbook 的维护方（或其对应 Story）改口径，不由 1.3 顺手放宽。

**裁决 2 —— "诊断脚本"在仓库中零命中，AC 指的是要新建它**

`grep -rn "诊断脚本"` 在 `docs/` 与 `Embedded/` 零命中；`Embedded/` 下**不存在**任何 flashing/monitor 脚本（全仓只有 `cloud/env-scripts/*.sh` 四个后端脚本）；烧录/监控只以**文档内联 bash 模板**形式存在于两份 runbook 中。
→ **裁决**：本 Story 新建仓内可执行诊断脚本（Task 1），且**不得复制第二套归档约定** —— 必须沿用 `docs/embedded/guides/真机闭环runbook-macos.md` §5 已冻结的 `docs/embedded/hardware_test_records/<运行编号>/` 与既有固定文件名（仅为 JTAG 通道新增 `openocd-01.txt`）。

**裁决 3 —— "禁用自动 light sleep"的准确机制（旧文档措辞不够精确）**

`ARCHITECTURE-SPINE.md:128`、`docs/embedded/guides/低功耗策略与实测验收.md:31/72/89`、`docs/embedded/troubleshooting/ESP32-S3-自动轻睡眠随机重启与USB日志失联.md:12/264` 都写作"USB 连接时禁用自动 light sleep"。ESP-IDF v5.5.4 Kconfig 原文（`$IDF_PATH/components/esp_driver_usb_serial_jtag/Kconfig:14-27`）的实际语义是"**只要 USB Serial/JTAG 处于连接状态就获取 `ESP_PM_NO_LIGHT_SLEEP` 锁**"，并且"**只能控制自动 Light-Sleep 行为**；若程序手工调用 `esp_light_sleep_start()`，即使本选项启用也不会阻止进入 light sleep"。
→ **裁决**：实现与验收都按 Kconfig 原文口径（见 AC 2 实现口径 1），并在门禁断言中锁定"USB 连接期间持有 PM 锁"这一机制而非"全局禁用"。拔线后仍须按自动 light sleep 合同运行，不得把 `ESP_PM_NO_LIGHT_SLEEP` 长期持有。

**裁决 4 —— Flash Core Dump 与诊断分区：本 Story 不新增**

排障文档 §8.2 给出"临时把 `coredump` 分区与 `CONFIG_ESP_COREDUMP_*` 加回"的取证流程；而 `Embedded/partitions.csv:4` 明文"Flash Core Dump 尚未启用，因此不为未使用功能占分区"，第 10 行明文"未来真正启用 OTA/诊断时再按实际镜像重新规划"。
→ **裁决**：本 Story **不修改 `partitions.csv`、不新增 coredump/诊断分区、不启用 `CONFIG_ESP_COREDUMP_*`**；只在 `session.md` 与排障提示中引用排障文档 §8.2 的一次性流程。分区变更需另立 Story。

**裁决 5 —— `Embedded/components/esp_pm` 覆写的归属**

`Embedded/components/esp_pm/` 是**已存在**的 ESP-IDF 同名组件覆盖（构建期由 `generate_v554_pm_impl.py` 生成修补后的 `pm_impl.c`），Story 1.1 复审把它记为 deferred 第 4 项（"Story 1.1 未记录该覆写"）。排障文档 §6/§9 规定"**永久 `esp_pm` 修复、版本哈希门禁和 PM 回归测试不会删除**"。
→ **裁决**：本 Story **不修改**该覆写的任何文件、**不删除**其哈希门禁与构建期 `message()`；只为"它仍然存在"加一条存在性断言（Task 4.4），并把它的真实身份写入本 Story 的 Dev Notes 与 File List 说明。**不关闭** Story 1.1 的该 deferred 项（属跨 Story 治理）。

**裁决 6 —— `CONFIG_ESP_CONSOLE_UART` 的"必须未定义"口径必须按精确 token 判定**

`sdkconfig` 中 `CONFIG_ESP_CONSOLE_UART`（裸 token）确实未定义，但 `CONFIG_ESP_CONSOLE_UART_NUM=-1`、`CONFIG_ESP_CONSOLE_UART_DEFAULT is not set`、`CONFIG_ESP_CONSOLE_UART_CUSTOM is not set` 均存在。`contract_checks.c:48` 用 `#ifdef CONFIG_ESP_CONSOLE_UART`，在 C 预处理中是**精确 token 匹配**，因此安全。
→ **裁决**：新增门禁**禁止**用子串 grep 判定"`CONFIG_ESP_CONSOLE_UART` 未定义"（会命中 `_NUM`/`_DEFAULT`/`_CUSTOM` 而误报）。必须用精确 token 判定（如正则 `^CONFIG_ESP_CONSOLE_UART(=| is not set)`），或改为扫描 `CONFIG_ESP_CONSOLE_UART_DEFAULT`/`_CUSTOM` 族并断言其均未启用。

**裁决 7 —— 硬件文档之间的三处不一致（不影响固件引脚，但不得写成已冻结事实）**

1. USB-C 连接器 LCSC 号：硬件基线 §5.6（第 333/356 行）与网络清单 JSON 第 784 行为 `C165948`，定稿 BOM 为 `C2982555`。
2. USB D± 拓扑：基线 §5.6 第 329 行写"先进入 USB-C 直连公共端，再单选到 BQ 或 ESP32"，而定稿网表第 151–152 行 `'USB_D-' ; D7.2 U1.13 USB1.A7 USB1.B7` / `'USB_D+' ; D5.2 U1.14 USB1.A6 USB1.B6` 是 ESP32（U1）与 USB-C（USB1）**直接同网**，无中间网络。
3. USB-C shell：网络清单 JSON 第 841 行为独立网络名 `USB_SHIELD`，定稿网表第 159–164 行把 `USB1.EP` 列入 GND。
→ **裁决**：这三处均**不影响固件引脚与协议**（D−=IO19、D+=IO20 三处一致），本 Story 只依赖引脚与通道；**不得**在任何产出中断言上述三项已收敛，也不在本 Story 回写 `docs/hardware/`（属硬件事实源维护方职责）。

**裁决 8 —— 文档漂移，本 Story 不改写**

- `AGENTS.md:9` 仍写"`Embedded/` **尚未创建工程**"，`docs/embedded/guides/低功耗策略与实测验收.md:4` 仍写"EWF `Embedded/` 仍为绿地（未进入实施）"，`Embedded/AGENTS.md:83` 引用的字体排障文档路径已过期 —— 与仓库现状不符。
- `Embedded/AGENTS.md:57` 把 PVDF 归属写作"固件 story E2.5"，`epics.md` 的权威编号是 Story 1.2（Story 1.2 已记录该漂移）。
→ **裁决**：属跨 Story 文档治理，记录在 epic retrospective，**不在本 Story 改写**上述文件。

### 上游故事与仓库情报（Story 1.1 / 1.2 现状）

**Story 1.1、1.2 均已完成（`done`）。与 USB-Serial-JTAG 直接相关的既有事实如下，本 Story 必须视为"已就位"而不是"待新建"：**

| 位置 | 已存在的事实 |
|---|---|
| `Embedded/sdkconfig.defaults:27-31` | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`、`CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`，含中文注释"ESP32-S3 原生 USB Serial/JTAG 独占应用日志，释放 GPIO43/GPIO44 给 Air780EGP UART1" |
| `Embedded/sdkconfig.defaults:19-25` | `CONFIG_PM_ENABLE=y`、`CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`、`CONFIG_FREERTOS_HZ=1000`、`CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP=3`、`CONFIG_RTC_CLK_SRC_INT_RC=y` |
| `Embedded/sdkconfig:1163-1171` | 派生结果：`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`、`CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`、`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED=y`、`CONFIG_ESP_CONSOLE_UART_NUM=-1`、`CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM=4`；`CONFIG_ESP_CONSOLE_UART_DEFAULT/_CUSTOM/_USB_CDC` 均 `is not set` |
| `Embedded/sdkconfig:879-882` | `CONFIG_USJ_ENABLE_USB_SERIAL_JTAG=y`（Kconfig 默认，非显式）、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`（显式） |
| `Embedded/components/contract_checks/contract_checks.c:44-54` | 三处 `#error`：`!CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`、`#ifdef CONFIG_ESP_CONSOLE_UART`、`!CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` |
| `Embedded/components/contract_checks/contract_checks.c:112-119` | `_Static_assert(EWF_BSP_USB_SERIAL_JTAG_DM_GPIO == GPIO_NUM_19, ...)`、`DP == GPIO_NUM_20` |
| `Embedded/main/app_main.c:51-56` | 与 contract_checks 等价的第二份 USB 锚点 `#error` |
| `Embedded/main/app_main.c:103-105` | 中文日志"应用日志使用原生 USB Serial/JTAG；Air780EGP UART1 IO43/IO44 只保留资源边界，本 Story 不启用任何模组业务" |
| `Embedded/main/app_main.c:440-446` | 释放启动 PM 锁时中文日志"自动 light sleep 启动锁已释放：空闲阈值=3 tick，唤醒源=PWR_INT IO8/BOOT0 IO0/PVDF 比较器 IO11；USB 主机连接期间保持调试可用" |
| `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h:119-122` | `EWF_BSP_USB_SERIAL_JTAG_DM_GPIO GPIO_NUM_19`、`EWF_BSP_USB_SERIAL_JTAG_DP_GPIO GPIO_NUM_20`，注释"Story 1.3 独占" |
| `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.c:260-267` | `[LEGBOT_BSP_RESOURCE_USB_SERIAL_JTAG]`，`owner = "esp-idf usb/download"`，`gpio_primary = DM`、`gpio_secondary = DP` |
| `Embedded/components/services/selftest_service/selftest_service.c:633-679` | 自检读取该 BSP 资源条目、断言引脚，日志"IO46/IO45 保留，USB=IO%d/IO%d，Air780EGP=IO%d/IO%d" |
| `Embedded/components/services/selftest_service/selftest_service.c:932-933` | 待样机验证项中文声明已含"USB/下载采样…无示波器或实板回执" |
| `Embedded/build/project_description.json` | `debug_arguments_openocd = "-f board/esp32s3-builtin.cfg"`；`target=esp32s3`；`project_name=electronic_wooden_fish`；`project_version=0.1.0`；`build_components` 共 74 项，USB 相关仅 `esp_driver_usb_serial_jtag` |
| `Embedded/build/flasher_args.json` | `flash_files`：`0x0` bootloader、`0x8000` partition-table、`0x10000` app、`0x490000` spiffs；`extra_esptool_args.after = hard_reset`、`chip = esp32s3` |
| `Embedded/components/esp_pm/{CMakeLists.txt,Kconfig,sdkconfig.rename,generate_v554_pm_impl.py}` | ESP-IDF 同名组件覆盖，构建期生成修补 `pm_impl.c`；构建 `message()`"已启用 ESP-IDF v5.5.4 light sleep 修复回移：拒绝休眠不补 tick，超时 2 tick 内有界补偿" |
| `Embedded/tests/run_host_tests.py` | 主机门禁入口，**不依赖 ESP-IDF、不执行烧录或串口动作**；反射发现 `test_bsp_contract.py` 内全部 `test_*`，另经 `cc/gcc/clang`（`-std=c11 -Wall -Wextra -Werror`）构建两个纯逻辑 C 用例 |
| `Embedded/tests/test_bsp_contract.py:54-66` | `PRODUCTION_SOURCES` 白名单：根 `CMakeLists.txt`、`sdkconfig.defaults`、`main/CMakeLists.txt`、`main/app_main.c`、`main/Kconfig.projbuild` + `components/{BSP,platform,app_state,contract_checks,control_gate,services}` 的 `.c/.h`；**不含** `components/ui`、`components/esp_pm`、`tools/` |
| `Embedded/partitions.csv:4,10` | "Flash Core Dump 尚未启用，因此不为未使用功能占分区"、"未来真正启用 OTA/诊断时再按实际镜像重新规划" |
| 日志 TAG 现状 | 18 个文件定义 `static const char *TAG`，全部 ASCII 全大写（`MAIN`、`BSP_BOARD`、`BSP_KEY`、`BSP_PVDF`、`SVC_PVDF`、`PLAT_I2C`、`SELFTEST` 等）；约 187 处 ESP_LOG 调用，正文为中文 |
| 缺失 | `Embedded/` 下**无**任何 flashing/monitor 脚本、**无** CDC 驱动代码、**无** OpenOCD 配置、**无** `usb_serial_jtag_*` 调用、**无** `tools/` 目录 |

**Story 1.1 复审的 5 项 deferred（本 Story 不要顺手修，其中第 4 项与本 Story 相邻）**：ISR 投递失败无重武装、`bsp_resources.h` 把 IO17 记为 ES8311 I2S 输入、自检观察预算等于 timeout、`esp_pm` 覆写未记录（裁决 5 只加存在性断言，不改内容）、`partitions.csv` 保留 6 MiB model 分区且 spiffs 仍打包已删资源。

**Story 1.2 复审 disposition（与 1.3 相关者）**：`EVENT_BUS_QUEUE_DEPTH=16` 无消费者、ISR 投递失败后中断永久关闭、确认窗口内中断全程关闭、`publish_snapshot()` 非阻塞取锁、`s_last_error` 陈旧值、**产品可见日志含设计/流程解释性文字**（`deferred-work.md:29`，本 Story 若新增日志须注意同一口径张力：日志是诊断通道，正文应写"发生了什么"的状态短句，不写设计解释或过程说明）。

**Git 情报（近 5 次提交与工作区状态）**

- `git log --oneline -5`：`fe98d73`（bmad 技能与工作流文档更新 + `.gitignore`）、`47e4f18`（修正 PWR 引脚为 LTC2954 `PWR_INT`、新增 `Embedded/lvgl-design/ewf-device-ui-export.html`、`cloud/AGENTS.md` 与 `cloud/**` 配置/脚本、`docs/hardware/定稿/` 导出）、`91467c8`（清理 bmad 过程文件）、`eeee145`（新增 `Embedded/components` BSP 组件与板级初始化、CMake 构建设置，删除旧临时文件）、`937a3cc`（外围电路设计文档、电容需求与库存、跨文档口径）。
- 最近三次提交**均为文档/技能层改动**，不含 Story 1.1/1.2 的实现代码 —— 那部分仍在工作区/暂存区未提交（`git diff HEAD --stat -- Embedded/` 显示 99 文件变更、+3179/−54203，含 52 个旧服务文件的暂存删除）。
- 提交历史中**未见任何** USB / JTAG / CDC / OpenOCD 相关内容 → 本 Story 的诊断脚本属全新引入，无历史先例可复用（与"裁决 2"一致）。
- 由此得到两条实现约束：(a) 本 Story 的改动必须叠加在**当前工作区内容**之上，不能以 `HEAD` 为基线假设文件不存在（例如 `Embedded/components/BSP/PVDF/`、`services/pvdf_input_service/` 在 `HEAD` 中不存在、但在工作区中存在）；(b) 变更集判定必须用工作区 diff，不能只看已提交历史。

**工作区状态提醒：** Story 1.1/1.2 的实现路径尚未提交（停在暂存区/工作区），`Embedded/` 处于 legbot → EWF 重构的暂存状态（`git diff HEAD --stat -- Embedded/` 显示 99 文件变更）。**本 Story 的编辑必须基于当前工作区内容，不得回退这些改动。** 工作区中 `cloud/`、`docs/contracts/canonical/`、`_bmad-output/implementation-artifacts/4-1-*.md` 属另一 epic 的并发活动，**不是本 Story 的内容，不得修改或清理**。

### 硬件事实（USB-Serial-JTAG 通道）

**引脚与拓扑（三处一致，可直接使用）**

| 信号 | 引脚 | 方向 | 约束原文 |
|---|---|---|---|
| `USB_D-` | IO19 | `bidirectional_usb` | "USB-Serial-JTAG；**不得与 BQ D- 并联**"（基线 §2.1 第 105 行） |
| `USB_D+` | IO20 | `bidirectional_usb` | "USB-Serial-JTAG；**不得与 BQ D+ 并联**"（基线 §2.1 第 106 行） |

- 基线 §1/§3.3 第 43 行："单 USB-C 的 D+/D− **直连 ESP32 USB-Serial-JTAG**；BQ25895 D+/D− 不接 USB-C 数据线，固定配置输入限流并关闭 BC1.2 自动检测。"
- 基线 §3.3 第 198 行：`D+ / D− | USB-C 连接器 → ESP32 IO20 / IO19 | USB Serial/JTAG、烧录、CDC 日志`。
- 基线 §5.6 第 327-333 行：单 USB-C 16P、USB2.0、SMT，候选 `TYPEC-304-ACP16`；CC1/CC2 各 5.1 kΩ Rd 到 GND；"USB-C 数据路径与 4G UART 完全独立，**USB 连接不能改变 Air780EGP 的 AT 所有权**"。
- 定稿网表第 151-152 行（权威端点）：`'USB_D-' ; D7.2 U1.13 USB1.A7 USB1.B7`、`'USB_D+' ; D5.2 U1.14 USB1.A6 USB1.B6`。
- USB 数据线 ESD：定稿 BOM D5/D7 = `RCLAMP0521T-ES`（DFN1006-2L-BI，双向）；VBUS 侧 D1 = `SP1005-01ETG` + F2 = `BSMD1206-300-6V` 自恢复保险丝（与 BQ 文档一致）。
- 无独立 USB-C 外围电路设计文档（`docs/hardware/外围电路设计/` 下不存在），USB-C 事实只在基线与网络清单中。

**"不使用"原文（AC 1.4 的硬件依据）**

- 基线第 123 行：`| USB Serial/JTAG | 烧录、CDC、OpenOCD | D−=IO19、D+=IO20 | USB-C 连接器直连 ESP32 | **不需要 CH340X**；USB 数据与 BQ VBUS 充电输入并行；**不启用 USB-OTG** |`
- `docs/hardware/电子木鱼-硬件网络清单.json:35`：`"forbidden_components": ["CH340X","AMS1117","TPS631000"]`；第 30 行 `"reason": "BQ D+ 与 ESP32 USB D+ 不得并联"`。
- 基线第 431 行：旧参考图 `docs/hardware/参考原理图/SCH_遥控板_双TypeC_4G稳压_2026-09-07.pdf` **只作反例**，不改写不复制其 CH340X/AMS1117/TPS631000。
- `docs/hardware/外围电路设计/M100EG-C2外围电路设计与接线.md:62,154`："本项目经主控侧 USB-Serial-JTAG 下载与看日志，**载板 USB 不使用，四脚全部不接**"、"下载与调试一律走主控侧 USB-Serial-JTAG"。
- `docs/hardware/外围电路设计/ESP32-S3复位与启动控制设计.md:114` 提到"Joint Download Boot 模式下支持 USB-Serial-JTAG、USB-OTG 与 UART0 三种下载通道（手册 p.13）" —— 这是**芯片能力陈述**，不是启用 OTG 业务；不得据此认定 OTG 已启用。

**启动绑带与 JTAG 信号源前提**

- 基线第 108 行："正常 SPI 启动要求 GPIO0=1；进入联合下载模式要求 GPIO0=0 且 GPIO46=0。**GPIO3 用于 JTAG 信号源选择，若保持默认 eFuse 配置则 USB Serial/JTAG 不受 GPIO3 影响**；一旦烧录 `EFUSE_STRAP_JTAG_SEL`，RGB_DATA 外围必须提供确定的复位电平。"
- `docs/hardware/外围电路设计/ESP32-S3复位与启动控制设计.md:114`：下载操作序列"**先**按住 BOOT 键（GPIO0=0）不放，**再**长按 PWR 键使系统上电"；第 112 行 strapping 保持时间 `tH` 最小 3 ms；第 116 行"GPIO0=0、GPIO46=0 的联合下载组合**仍需样机验证**"。
- `Embedded/AGENTS.md:169`："硬件关机后 USB 停止枚举；长按 PWR 重新开机后 USB 恢复枚举。" → 烧录前若设备处于关机态，需按上述序列上电；这是**板级**行为，固件不参与。
- 同文件第 168、170 行：BOOT0=IO0、IO46 已悬空（不得恢复为 PWR 输入）；固件不实现、不模拟也不接管下载路径。

**硬件开放项（不得在本 Story 冒充已闭合）**

- `docs/hardware/电子木鱼-硬件网络清单.json:2195-2200`：`HW-OI-005` = "USB-C 直连 ESP32 与 BQ D+/D− 不使用的兼容性"，验证方式"USB 枚举、不同充电器、BQ 输入限流与反灌测试"，`status = "must_prototype_validate"`。
- 基线 §6.2 样机状态声明（第 397 行）：未放置元件、未运行 ERC、未 PCB 打样、无实物插合与示波器/电子负载测试；"任何输出、代码编译或静态检查通过都不能替代上述样机证据"。

### ESP-IDF v5.5.4 API / Kconfig 证据（本次已核验；实现时仍以官方页面为准）

`Embedded/AGENTS.md:124-164` 强制"新增或修改 API/Kconfig 前必须先查证"。以下为本次已核验结论，**实现时不得凭记忆改动**。

**本机 IDF 安装（事实锚点）**

| 项 | 值 |
|---|---|
| `IDF_PATH` | `/Users/hongchenke/.espressif-v5.5.4/v5.5.4/esp-idf` |
| 版本（`tools/cmake/version.cmake`） | MAJOR 5 / MINOR 5 / PATCH 4 |
| 激活脚本 | `/Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` |
| `OPENOCD_SCRIPTS` | `/Users/hongchenke/.espressif/tools/openocd-esp32/v0.12.0-esp32-20251215/openocd-esp32/share/openocd/scripts` |

**`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION`（本 Story 的核心锚点，原文见 `$IDF_PATH/components/esp_driver_usb_serial_jtag/Kconfig:14-27`）**

```
config USJ_NO_AUTO_LS_ON_CONNECTION
    bool "Don't enter the automatic light sleep when USB Serial/JTAG port is connected"
    depends on PM_ENABLE && ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED && !SOC_USB_SERIAL_JTAG_SUPPORT_LIGHT_SLEEP \
        && USJ_ENABLE_USB_SERIAL_JTAG
    default n
    help
        If enabled, the chip will constantly monitor the connection status of the USB Serial/JTAG port. As long
        as the USB Serial/JTAG is connected, a ESP_PM_NO_LIGHT_SLEEP power management lock will be acquired to
        prevent the system from entering light sleep.
        This option can be useful if serial monitoring is needed via USB Serial/JTAG while power management is
        enabled, as the USB Serial/JTAG cannot work under light sleep and after waking up from light sleep.
        Note. This option can only control the automatic Light-Sleep behavior. If esp_light_sleep_start() is
        called manually from the program, enabling this option will not prevent light sleep entry even if the
        USB Serial/JTAG is in use.
```

由原文可确认的四条硬事实：

1. **`default n`** → 必须由 `sdkconfig.defaults` 显式置 `y`（已置，`sdkconfig.defaults:31`）。
2. **`depends on PM_ENABLE`** → 若 `CONFIG_PM_ENABLE` 被关闭，该选项会被 Kconfig 静默丢弃，锚点在 `sdkconfig` 中消失、`#error` 随即触发。这是 Task 4.3 门禁的理由。
3. **机制是"连接期间获取 `ESP_PM_NO_LIGHT_SLEEP` 锁"**，不是全局禁用自动 light sleep（裁决 3）。
4. **只约束自动 light sleep**；手工 `esp_light_sleep_start()` 不受保护。本工程既有代码未手工调用 light sleep（`app_main.c:451-456` 的失败收尾是把 `light_sleep_enable` 设为 `false` 的 `esp_pm_configure()`，属自动路径配置，不属手工休眠）。

**`SOC_USB_SERIAL_JTAG_SUPPORT_LIGHT_SLEEP` 在 v5.5.4 全部 target 均为注释态**

`grep -rn "SOC_USB_SERIAL_JTAG_SUPPORT_LIGHT_SLEEP" $IDF_PATH/components/soc` 的命中全部是被注释掉的 `// #define ... (1)`（`esp32c5`/`esp32c6`/`esp32h2`/`esp32h4`/`esp32h21`/`esp32c61`/`esp32p4` 的 `soc_caps.h`，均带 `// TODO: IDF-6395`；`esp32s3/include/soc/soc_caps.h` 无该宏）。
→ 依赖条件 `!SOC_USB_SERIAL_JTAG_SUPPORT_LIGHT_SLEEP` 成立，**ESP32-S3 不支持在 light sleep 期间维持 USB 通信** —— 这是"USB 连接期间必须禁止自动 light sleep"的芯片级根因，也是排障文档 §5 与本 Kconfig help 的共同结论。

**console 相关 Kconfig（`$IDF_PATH/components/esp_system/Kconfig:296-322`）**

- `choice ESP_CONSOLE_SECONDARY`（第 296-316 行）的 `prompt` 为 "Channel for console secondary output"，`default ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG`；但该默认目标带 `depends on !ESP_CONSOLE_USB_SERIAL_JTAG`，**主 console 已是 USB-Serial-JTAG 时它不可选**，Kconfig 会回落到第一个可见选项 `ESP_CONSOLE_SECONDARY_NONE`。因此本工程实际默认就是 NONE；`sdkconfig.defaults:29` 仍**显式**设置 `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`，价值是消除对 Kconfig 回落行为的隐式依赖，并且它就是 runbook:162-171 已冻结的期望值。**不得**据此推断"不设就会变成次级输出"。
- 主 console 的 choice（第 279-293 行）另含 `ESP_CONSOLE_USB_CDC`（ROM CDC 驱动，`depends on (IDF_TARGET_ESP32S2 || IDF_TARGET_ESP32S3) && !TINY_USB`）—— 它与 USB-Serial-JTAG 共用同一 USB PHY，**不得**改选它（属第二套 USB 通道，AC 1.4 禁止）。
- `config ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED` 是**内部派生符号**（`default y if ESP_CONSOLE_USB_SERIAL_JTAG || ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG`），同时 `select USJ_ENABLE_USB_SERIAL_JTAG`；不需要也不应手工设置。
- `config ESP_CONSOLE_UART` 同样是**内部派生符号**（`default y if ESP_CONSOLE_UART_DEFAULT || ESP_CONSOLE_UART_CUSTOM`）—— 这解释了裁决 6：裸 token 未定义但 `_NUM`/`_DEFAULT`/`_CUSTOM` 存在。
- 官方页面佐证（设备在 light sleep 期间的行为）：USB Serial/JTAG 在 light sleep 下"cannot receive or respond to any USB transactions"，主机"may then report the USB Serial/JTAG device as disconnected or erroneous"，唤醒后主机**可能不会重新枚举**，"Users may need to physically disconnect and then reconnect the USB cable"。另注："If the application accidentally reconfigures the USB peripheral pins or disables the USB Serial/JTAG Controller, the device disappears from the system"，恢复需手工拉低 GPIO0 后复位。

**USB Serial/JTAG 驱动 API（`$IDF_PATH/components/esp_driver_usb_serial_jtag/include/driver/usb_serial_jtag.h`）**

```
esp_err_t usb_serial_jtag_driver_install(usb_serial_jtag_driver_config_t *usb_serial_jtag_config);   // :48
int       usb_serial_jtag_read_bytes(void* buf, uint32_t length, TickType_t ticks_to_wait);          // :60
int       usb_serial_jtag_write_bytes(const void* src, size_t size, TickType_t ticks_to_wait);       // :75
esp_err_t usb_serial_jtag_wait_tx_done(TickType_t ticks_to_wait);                                    // :84
esp_err_t usb_serial_jtag_driver_uninstall(void);                                                    // :92
bool      usb_serial_jtag_is_connected(void);                                                        // :106
bool      usb_serial_jtag_is_driver_installed(void);                                                 // :113
```

**本 Story 明确不安装该驱动。** 应用日志经 console VFS 输出，**不需要** `usb_serial_jtag_driver_install()`；在 console 已使用 USB-Serial-JTAG 的前提下再安装驱动会形成两套读取者，且 `usb_serial_jtag_is_connected()` 的瞬时真值无法证明"CDC 不随机失联"（那需要真机连续日志证据，属 `hardware_pending`）。因此本 Story **不新增任何 USB 驱动调用、不新增服务、不新增自检项**（见"实现边界"）。

**PM 锁 API（`$IDF_PATH/components/esp_pm/include/esp_pm.h`；官方 Power Management 页面已核验）**

```
esp_err_t esp_pm_configure(const void *config);
esp_err_t esp_pm_lock_create(esp_pm_lock_type_t lock_type, int arg, const char *name, esp_pm_lock_handle_t *out_handle);
esp_err_t esp_pm_lock_acquire(esp_pm_lock_handle_t handle);
esp_err_t esp_pm_lock_release(esp_pm_lock_handle_t handle);
esp_err_t esp_pm_lock_delete(esp_pm_lock_handle_t handle);
esp_err_t esp_pm_dump_locks(FILE *stream);
```

- 锁类型：`ESP_PM_CPU_FREQ_MAX`、`ESP_PM_APB_FREQ_MAX`、`ESP_PM_NO_LIGHT_SLEEP`（"Prevent the system from going into light sleep."）、`ESP_PM_LOCK_MAX`。
- 新锁初始未持有；`acquire`/`release` 可递归；`acquire` 可在 ISR 中调用；`create`/`delete` **不得**在 ISR 中调用；删除前必须释放。
- `CONFIG_PM_ENABLE` 关闭时多个 PM API 返回 `ESP_ERR_NOT_SUPPORTED`；`CONFIG_FREERTOS_USE_TICKLESS_IDLE` 未开时 `esp_pm_configure()` 对 light sleep 请求返回 `ESP_ERR_NOT_SUPPORTED`。
- **本 Story 不新增 PM 锁调用**：`ESP_PM_NO_LIGHT_SLEEP` 由 IDF 的 USJ 连接监测路径自行获取/释放（Kconfig help 原文）。应用侧只保留既有的启动锁 `s_startup_light_sleep_lock`（`app_main.c:60-63`）。

**OpenOCD / JTAG（本机已核验存在）**

- `board/esp32s3-builtin.cfg` 存在于 `$OPENOCD_SCRIPTS/board/`，内容为 `source [find interface/esp_usb_jtag.cfg]` + `source [find target/esp32s3.cfg]`。
- 本工程 `build/project_description.json` 已含 `debug_arguments_openocd = "-f board/esp32s3-builtin.cfg"` → **`idf.py openocd` 无需额外配置即可使用内置 USB-JTAG**。
- `idf.py openocd` 由 `$IDF_PATH/tools/idf_py_actions/debug_ext.py` 提供，支持 `--openocd-scripts` 与 `--openocd-commands`；其 stdout/stderr 写入构建目录的 `openocd_out.txt`；**不接受 `-p/--port` 参数**（端口由 OpenOCD 自身按 USB 枚举发现）。
- 官方 JTAG 调试索引页给出的等价手工命令为 `openocd -f board/esp32s3-builtin.cfg`；内置 JTAG 只需一根连到 D+/D− 的 USB 线、无需外接适配器；Linux 需 udev 规则，Windows 需 WinUSB 驱动，**macOS 无额外步骤**。官方"Configure ESP32-S3 Built-in JTAG Interface"页未要求任何 Kconfig，也未声明需 eFuse 配置（默认 eFuse 下 USB Serial/JTAG 不受 GPIO3 影响，见硬件基线第 108 行）。

**USB 主机侧标识（来自真机闭环 runbook §2/§7.1）**

- VID:PID `303A:1001`，描述 `USB JTAG/serial debug unit`。
- 枚举命令 `python -m serial.tools.list_ports -v`，期望形如 `/dev/cu.usbmodemXXXXXX`。
- 监控波特率 `CONFIG_ESPTOOLPY_MONITOR_BAUD=115200`。
- **端口尾号会因 USB 拓扑/重插/枚举而变**；runbook §7.1 硬门禁："仅当**一个**端口同时匹配描述、VID:PID 且设备序列号经操作员确认时才可继续"；"零个匹配、多个匹配、序列号缺失或与预期不符时，列出全部候选并请求操作员确认；**不得按端口排序、最近出现时间或历史端口名自行猜选**"。该规则必须固化进诊断脚本（Task 1.3）。
- EWF 设备序列号**尚未登记**（runbook §2）→ 首次真机枚举后由操作员确认，本 Story 不预设该值。

### 本 Story 冻结的接口契约

以下为 Story 1.3 结束时必须成立的、可被后续 Story 复用的稳定事实：

1. **诊断脚本入口**：`Embedded/tools/usb_serial_jtag_diag.py`，子命令 `ports` / `preflight` / `flash` / `monitor` / `openocd`；纯逻辑函数可在主机测试中直接导入调用。
2. **回执布局**：`docs/embedded/hardware_test_records/<运行编号>/{environment.txt,build-01.log,flash-01.log,serial-01.log,openocd-01.txt,session.md}`，与真机闭环 runbook §5 一致（`openocd-01.txt` 为本 Story 新增的 JTAG 通道回执，命名与既有 `flash-01.log`/`serial-01.log` 同构）。
3. **fail-closed 判定**：无确认端口 / 零匹配 / 多匹配 / 序列号不符 / run 目录已存在 / IDF 版本越界 / 缺产物 / 端口被占用 ⇒ 拒绝执行并给中文理由；退出码非零。
4. **主机门禁新增项**：sdkconfig USB 锚点三项、console 禁用族、CH340X/USB-OTG 禁用符号、TAG ASCII 与中文日志、`PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP` 未启用、`esp_pm` 覆写存在性。
5. **不变式**：Epic 2/3 的自测可以复用同一脚本与同一回执布局，**不得**另建第二套烧录/监控脚本或第二套归档命名。

### 架构、编码与 API 约束

**架构不变量（`ARCHITECTURE-SPINE.md`）**

- **AD-11**（本 Story 的主约束）：原生 USB-Serial-JTAG（GPIO19 D− / GPIO20 D+）承担 `esptool`/`idf.py flash`、USB CDC 日志与 OpenOCD 调试；**不上 CH340X、不在同一 USB PHY 上开发 USB-OTG**；因此 GPIO43/44 让给 Air780EGP UART；**USB 连接时禁用自动 light sleep，防调试串口失联**。
- **AD-7**：ISR 只投递事件、不做产品决策；命令型路径不得静默丢弃，必须 busy/error + 日志。→ 诊断脚本的拒绝路径必须打印明确中文理由并非零退出，不得静默返回 0。
- **AD-8**：只有 BSP 定义 GPIO、总线、电源使能与初始化顺序。→ 本 Story 不得在 scripts 或新代码中重新定义 IO19/IO20；必须引用 `EWF_BSP_USB_SERIAL_JTAG_DM_GPIO`/`_DP_GPIO` 或用其已冻结值比对。
- **AD-12**：反馈/外设失败不阻塞核心链路。→ 本 Story 不在运行路径新增任何 I/O。
- **AD-13**（部分被 PRD 取代）：低电先落盘、PWR 长按由板级负责 —— 有效；"充电期间暂停输入" —— **作废**（见 Story 1.2 的裁决 1 与 PRD §13.1-OQ4）。本 Story 不涉及供电分支，也不得引入。

**一致性约定（spine §Consistency Conventions）**：命名全小写下划线；固件日志中文、模块 TAG 用 ASCII；读/写仓库文本默认无 BOM UTF-8。

**编码规范（`Embedded/AGENTS.md` 与 `docs/embedded/style/C编码规范-Agent版.md`）**

- 本 Story 预期**不修改任何 `.c`/`.h`**。若实现时确需改动固件源码，必须完整遵守：Doxygen 文件头（`@file`/`@brief`/`@details`/`@author ZHC`/`@date YYYY-MM-DD`）、`.c` 七段布局、`.h` 六段布局、每个用日志的 `.c` 定义 `static const char *TAG` 且正文中文、公开函数中文 Doxygen、业务宏/枚举/字段中文注释、`snake_case`、`EWF_*` 前缀、4 空格缩进、禁 Tab、Allman 花括号、120 列软上限、禁 VLA、ISR/硬实时路径禁动态分配、断言只验证程序员不变量。
- Python 侧：脚本统一无 BOM UTF-8；`if __name__ == "__main__"` 入口；纯逻辑函数无副作用、无全局可变状态；错误用非零退出码 + 中文 stderr。
- 不得修改 `lvgl-design/`、`Embedded/components/ui/`、字体合同或 UX 资产（与本 Story 无关）。

**构建与测试环境**

- 环境激活：`source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` → `idf.py --version` 应为 `ESP-IDF v5.5.4`；越界即停止。
- `idf.py` **必须在 `Embedded/` 执行**，不能在仓库根执行（`macos_esp_idf_hardware_test_runbook.md:37`）。
- 构建门禁：`idf.py reconfigure` + `idf.py build`；**不要**为保险执行 `idf.py set-target esp32s3`（runbook:45），也**不要**在未确认缓存损坏时 `idf.py fullclean`（runbook:46）。
- 主机门禁：`python3 Embedded/tests/run_host_tests.py`（不依赖 ESP-IDF，不执行烧录或串口动作）。
- 构建证据：`docs/embedded/build_records/<YYYYMMDD-HHMMSS>/{environment.txt,build-01.log,session.md}`；该路径被根 `.gitignore:49` 忽略，不入变更集。**禁止凭记忆填写** `environment.txt`/`session.md`（runbook:115）。

### 实现边界（不属于本 Story）

**必须留给后续 Story，不得顺带实现：**

| 能力 | 归属 |
|---|---|
| 统一有效敲击队列与输入闸门、正式计数 | Story 2.1、2.2 |
| 木鱼音频、RGB 反馈 | Story 2.3 |
| 触摸/PWR 导航、熄屏首触只唤醒的门控落点 | Story 2.4 |
| BOOT0 自动模式 | Story 2.6 |
| 低电与故障下的核心链路保护 | Story 2.7 |
| 设备 UI（状态栏 4G/Wi-Fi/蓝牙/GPS 被动状态、三环、7 字带） | Story 3.1–3.6 |
| Air780EGP 活动窗口与 HTTPS 同步 | Epic 2/5/7（`docs/embedded/4g/`） |
| coredump / 诊断分区的实际启用与 Flash 取证设施 | 另立 Story（见裁决 4；排障文档 §8.2 为一次性流程） |
| 真机三路验证（烧录/CDC 监控/OpenOCD 连接）与样机电气验收 | 样机到位后按 `docs/embedded/guides/真机闭环runbook-macos.md` 执行；`HW-OI-005` 属样机阶段 |
| 设备序列号登记与操作员确认 | 首次真机枚举时（runbook §7.1） |

**本 Story 也不做**：修改 `partitions.csv`；修改 `Embedded/components/esp_pm/**` 任何内容；修改两份 runbook 的既有条款（含"禁止 flash/monitor"）；修复 Story 1.1 的 5 项 deferred 与 Story 1.2 的 13 项 deferred；统一 `deferred-work.md:29` 的日志措辞口径；改写 `AGENTS.md:9`、`低功耗策略与实测验收.md:4`、`Embedded/AGENTS.md:83` 的过期陈述；回写 `docs/hardware/` 的三处不一致（裁决 7）。

### 红线与禁止事项

1. **禁止**执行 `idf.py flash`、`app-flash`、`erase-flash` / `erase_flash`、`monitor`，也**禁止**猜测串口或要求操作员插线、按键、复位（`macos_esp_idf_hardware_test_runbook.md:43`）。诊断脚本可以**实现**这些命令，但**本 Story 不得运行**它们；脚本必须 fail-closed。
2. **禁止**让诊断脚本提供 `erase-flash`/`erase_flash`/`app-flash` 子命令或分支；`app-flash` 不得作为完整烧录的替代（真机闭环 runbook §3）。
3. **禁止**在无确认端口时"按端口排序、最近出现时间或历史端口名"自行猜选端口（runbook §7.1 硬门禁）。
4. **禁止**新建第二套烧录/监控脚本或第二套回执命名；回执必须落在 `docs/embedded/hardware_test_records/<运行编号>/`。
5. **禁止**启用 `CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP`（会让 GPIO/USB 模块断电，直接破坏唤醒与调试链路）。**禁止**关闭 `CONFIG_PM_ENABLE` 或 `CONFIG_FREERTOS_USE_TICKLESS_IDLE`（会使 `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` 被 Kconfig 静默丢弃）。
6. **禁止**把 `ESP_PM_NO_LIGHT_SLEEP` 长期持有或全局关闭自动 light sleep；关断只允许是"USB 主机连接期间"的临时状态，拔线后必须恢复自动 light sleep 合同。
7. **禁止**引入 CH340X、USB-OTG、TinyUSB、USB Hub 或任何第二套 USB 转换/业务通道；**禁止**在 `REQUIRES`/`PRIV_REQUIRES` 中加入 `usb` 或 `tinyusb`。
8. **禁止**修改或删除 `Embedded/components/esp_pm/**`（含其哈希门禁、`Kconfig`、`sdkconfig.rename`、`generate_v554_pm_impl.py` 与构建 `message()`）；本 Story 只允许为"其仍然存在"加存在性断言（排障文档 §6/§9："永久 `esp_pm` 修复、版本哈希门禁和 PM 回归测试不会删除"）。
9. **禁止**新增 coredump 或诊断分区、修改 `partitions.csv`、启用 `CONFIG_ESP_COREDUMP_*`。
10. **禁止**安装 USB Serial/JTAG 驱动（`usb_serial_jtag_driver_install`）或在 console 已独占该外设时再加第二个读取者；**禁止**新增 USB 驱动调用、新服务或新自检项（见"实现边界"）。
11. **禁止**在任何脚本输出、日志与回执中携带凭据：不得出现 Token、密码、私钥、认证头、密钥材料或个人敏感数据（`C编码规范-Agent版.md:156`、`Embedded/AGENTS.md:253`、真机闭环 runbook §3）。
12. **禁止**把三处硬件文档不一致（USB-C LCSC 号、D± 拓扑、shell 网络名）写成已收敛事实，也**禁止**在本 Story 回写 `docs/hardware/`。
13. **禁止**声称样机验证：`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 保持 `0U`；自检证据不得升级为 `HARDWARE_VERIFIED`；编译、主机测试与预检通过**都不等于**真机通过。
14. **禁止**未查 ESP-IDF v5.5.4 官方文档即调用新 API 或引用新 Kconfig（不得用 freertos.org、vanilla FreeRTOS、`latest`/`stable` 页面或其它芯片/版本页面作为依据）。
15. **禁止**把 AI 思维链、内部推理、设计过程、调试说明、Node ID、`data-pencil-id`、`TODO`、`draft`、`placeholder` 或作者提示写进任何用户可见文本、设备 UI、Pen 画面、静态 HTML 或前端文案（本 Story 无 UI 产出，但脚本输出、`session.md` 与日志属产品可见面，同样受约束：只写状态短句与稳定 ASCII 码，不写设计解释）。
16. **禁止**任何变更性 git 操作（commit / branch / stash / reset / checkout / merge）；只允许只读 `git` 查询。File List 口径沿用 autopilot 的 `story-local.diff` 排除规则（故事文件与 `sprint-status.yaml` 不计入变更集；`docs/embedded/build_records/**` 与新增忽略的 `docs/embedded/hardware_test_records/**` 同样不计入）。

### 测试与完成判定

**无样机条件下可判定的（本 Story 的完成证据）**

1. `python3 Embedded/tests/run_host_tests.py` 退出码 0，且新增诊断脚本纯逻辑用例全绿：端口单匹配→通过；零匹配→拒绝；多匹配→拒绝；`--serial` 不符→拒绝；run 目录已存在→拒绝；IDF 版本越界→拒绝；缺产物→拒绝；端口被 `idf_monitor` 占用→拒绝；任一拒绝分支的 `flash`/`monitor`/`openocd` 判定均为"不执行"。
2. 新增源码合同门禁全绿：sdkconfig 三项 USB 锚点存在、console 禁用族未被启用、`ch340`/`usb_otg`/`tinyusb` 零命中、`PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP` 未启用、`PM_ENABLE`/`TICKLESS_IDLE` 仍启用、`esp_pm` 覆写四件套存在、IO19/IO20 断言一致。
3. 日志门禁全绿：全部 `TAG` 为 ASCII 稳定串；全部 ESP_LOG 正文含中文（或落在显式白名单内）；`TAG` 定义与日志使用一致；无凭据类字样。
4. `idf.py reconfigure` + `idf.py build` 退出码 0，`error:`/`warning:` 均为 0，五个产物齐全（`build/bootloader/bootloader.bin`、`build/partition_table/partition-table.bin`、`build/electronic_wooden_fish.bin`、`build/electronic_wooden_fish.elf`、`build/spiffs.bin`），且 `build_components` 中 USB 相关项仍只有 `esp_driver_usb_serial_jtag`。
5. 预检回执：在无设备主机上真实运行 `ports` 与 `preflight`，归档输出与退出码，能证明零匹配时给出明确中文拒绝理由、且未生成任何 `flash-*.log` / `serial-*.log`。
6. `docs/embedded/hardware_test_records/` 路径已被根 `.gitignore` 忽略（`git check-ignore` 命中）。

**无样机条件下不可判定的（必须保持 pending，不得写入完成声明）**

- 真机烧录返回 0 且每个镜像出现 `Hash of data verified.`、输出以正常复位结束。
- 真机 CDC 日志出现完整启动过程，且**保持连接期间不随机失联**（含长时间空闲 + 自动熄屏循环后仍不失联）。
- OpenOCD 真机连接成功（`idf.py openocd` / `openocd -f board/esp32s3-builtin.cfg`）与 GDB 会话可用。
- 端口枚举在真机上的真实 VID:PID 与设备序列号（EWF 尚未登记）。
- 拔掉 USB 用电池运行时的自动 light sleep 恢复行为与"拔线后唤醒链正常"。
- `HW-OI-005`（USB-C 直连 ESP32 与 BQ D± 不使用的兼容性）所需的 USB 枚举、不同充电器、BQ 输入限流与反灌测试。
- 下载进入序列（先按住 BOOT0、再长按 PWR 约 2 s）与 `GPIO0=0、GPIO46=0` 联合下载组合（`ESP32-S3复位与启动控制设计.md:116` 明文仍需样机验证）。
- USB-C 机械沉板高度、外壳开孔与屏蔽脚布局。

**建议的验证顺序**：先跑主机测试（纯逻辑 + 源码合同）→ 再 `py_compile` 全部脚本 → 再 `idf.py reconfigure` + `build` → 最后跑无设备预检并归档。任一失败必须如实报告为诊断路径失败，**不得**改写为通过，也不得为了让预检"通过"而放宽门禁。

## Project Structure Notes

**新增（本 Story 拥有的路径）**

```
Embedded/tools/                                            # 新增目录（host 侧诊断工具）
  usb_serial_jtag_diag.py                                  # 诊断脚本入口与纯逻辑函数
Embedded/tests/test_usb_serial_jtag_diag.py                # 新增主机用例（纯 Python）
```

**修改（必须触碰的最小集合，每处都有明确理由）**

```
Embedded/tests/run_host_tests.py                           # 注册诊断脚本纯逻辑用例（Task 6.2）
Embedded/tests/test_bsp_contract.py                        # 追加 USB/console/CH340X/OTG/TAG/PM 门禁（Task 2、3、4）
.gitignore                                                 # 追加 docs/embedded/hardware_test_records（Task 5.3）
```

**预期不修改**：`Embedded/` 下任何 `.c`/`.h`、`sdkconfig.defaults`、`sdkconfig`、`CMakeLists.txt`、`main/**`、`components/**`、`partitions.csv`、`docs/hardware/**`、两份 runbook、`docs/embedded/**` 既有文档。

> 若实现时发现必须改动固件源码才能满足某条 AC（例如确需新增一个 USB 连接状态可观测项），**必须在 Change Log 中说明理由**，并同步评估是否引发 `LEGBOT_SERVICE_COUNT`/`SELFTEST_ITEM_COUNT` 与既有 `_Static_assert` 的连锁变更。默认路径是**不改固件源码** —— 本 Story 的 AC 由"锚点已就位 + 新增门禁 + 新增诊断脚本"三部分闭合。

**对齐与偏离说明**

- `Embedded/tools/` 是新增顶层目录。`Embedded/CMakeLists.txt:5-7` 的 `EXTRA_COMPONENT_DIRS` 只纳管 `components/`，`tools/` 不会被 ESP-IDF 组件系统拾取，因此**不需要**修改任何 `CMakeLists.txt`；`Embedded/.gitignore` 也未忽略 `tools/`（已核验）。这与 `cloud/env-scripts/*.sh` 作为"host 侧脚本"的定位同构。
- 诊断脚本**不进入固件构建**：`main/CMakeLists.txt` 的 `PRIV_REQUIRES` 与 `MINIMAL_BUILD ON` 均不涉及它；不允许把它误登记为组件。
- `Embedded/tests/` 是扁平的单一测试目录，**没有** per-component `test/` 或 `host_test/` 约定（已核验）。本 Story 不引入新的测试目录结构。
- `test_bsp_contract.py` 的 `PRODUCTION_SOURCES`（第 54-66 行）当前**不含** `components/ui`、`components/esp_pm`、`tools/`。本 Story 只补 `tools/*.py`（新增物必须被扫描），**不顺手**把 `ui`/`esp_pm` 纳入（属既有缺口，Story 1.1 已 deferred）。
- 变更集口径：故事文件本身与 `sprint-status.yaml` 不计入变更集（autopilot `story-local.diff` 排除）；`docs/embedded/build_records/**` 与 `docs/embedded/hardware_test_records/**` 被 `.gitignore` 忽略，同样不计入。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md`#Story 1.3: 启用 USB-Serial-JTAG 诊断路径]（本 Story 的 BDD 验收与覆盖需求 FR-E-010、AD-11 的唯一真源）
- [Source: `_bmad-output/planning-artifacts/epics.md`#Functional Requirements#FR-E-010]（原生 USB-Serial-JTAG 完成烧录、CDC 日志与 JTAG 调试；不使用 CH340X 或 USB-OTG 业务；PC 可识别并完成烧录/日志）
- [Source: `_bmad-output/planning-artifacts/epics.md`#Additional Requirements#AD-11]（ESP32-S3 原生 USB-Serial-JTAG（GPIO19/20）负责烧录、CDC 日志和 JTAG；USB 连接时禁用自动 light sleep 以防日志失联）
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#5.10 FR-E-010]（需求与验收原文；"同一 USB 口不启用 USB-OTG；不依赖 CH340X"）
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#14 工程验证入口]（第 423 行把 `GPIO19/20 USB` 列入验证入口；第 428 行三种供电条件下各链路可用）
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`#AD-11]（唯一调试通道；`esptool`/`idf.py flash`、USB CDC 日志、OpenOCD；不上 CH340X、不做 USB-OTG；GPIO43/44 让给 Air780EGP）
- [Source: 同上#AD-7、AD-8、AD-12]（服务/ISR 边界、BSP 独占资源、失败不阻塞）
- [Source: 同上#Consistency Conventions]（固件日志中文、模块 TAG 用 ASCII）
- [Source: 同上#工程验证门禁]（"按硬件主基线的启动绑带、USB、PWR 和保留脚合同完成原生 USB 烧录/CDC 日志/OpenOCD 验证"）
- [Source: `docs/hardware/电子木鱼-硬件原理图设计基线.md`#2.1 权威 GPIO 总表 第 105-106 行]（`USB_D-`=IO19、`USB_D+`=IO20；不得与 BQ D± 并联）
- [Source: 同上#第 43、118、123、134、193-202、327-333、374、397、402、407、420、431 行]（USB-C 直连、UART0 保留、USB Serial/JTAG 行（烧录/CDC/OpenOCD、不需要 CH340X、不启用 USB-OTG）、USB 已冻结拓扑、USB-C 供电与数据表、§5.6 USB-C 与连接器、P02 页、样机状态声明、差分等长与禁止并联、测试点、缺证据不得宣称门禁通过、旧参考图只作反例）
- [Source: `docs/hardware/电子木鱼-硬件网络清单.json`#第 30、35、422-445、780-845、1626-1651、1845-1866、1963-1968、2010-2027、2195-2200 行]（BQ D± 不得并联、`forbidden_components` 含 CH340X、USB_D± 引脚条目、USB-C 连接器逐脚与测试点、网络类别、CC Rd、测试点白名单、P02_MCU_USB、`HW-OI-005` must_prototype_validate）
- [Source: `docs/hardware/定稿/Netlist_Schematic1_2026-09-21.tel`#第 151-152 行]（`USB_D-` / `USB_D+` 的权威端点定义）
- [Source: `docs/hardware/外围电路设计/ESP32-S3复位与启动控制设计.md`#第 40、48-50、98-116 行]（网络名 `RESET`/`RESET_N` 未闭合、逐脚表、启动模式与 strapping 保持时间、下载操作序列、`GPIO0=0、GPIO46=0` 仍需样机验证、`EFUSE_STRAP_JTAG_SEL` 相关前提见基线第 108 行）
- [Source: `docs/hardware/外围电路设计/M100EG-C2外围电路设计与接线.md`#第 62、154 行]（载板 USB 四脚不接；下载与调试一律走主控侧 USB-Serial-JTAG）
- [Source: `docs/embedded/macos_esp_idf_hardware_test_runbook.md`#第 3、9、20、28-37、43-48、109-115、131-137、150-173、228-233、282、356 行]（范围与无样机禁烧录、锚点表、安全边界、编译记录布局、IDF 版本门禁、sdkconfig 取证命令与期望清单、成功判据后停止、失败日志纪律）
- [Source: `docs/embedded/guides/真机闭环runbook-macos.md`#第 9-10、35、39-46、50-57、99-129、186-223、302-338、361-363、369-390、407-451、453-466、524、562-571、582、615-622、665-710 行]（流程模板与"待回填"纪律、IDF 约束、监控波特率/USB 标识、安全边界、标准命令前缀、端口唯一匹配硬门禁、`idf.py flash` 原文与成功判据、下载模式提示、monitor PTY 与文件日志、轮询规则、监控中断保全、E1 平台底座动作清单含"烧录→看日志→JTAG 通道可用、USB 连接时日志不因自动 light-sleep 失联"、日志检索命令、故障分类、临时测试 console 配置、`session.md` 模板）
- [Source: `docs/embedded/troubleshooting/ESP32-S3-自动轻睡眠随机重启与USB日志失联.md`#第 12、57、63、72-75、87、93、104-148、180-198、254-266 行]（硬规则与 `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`、`vTaskStepTick()` 根因、PSRAM 栈上调用 NVS/Flash 边界、USB 行为误导性、取证要求、`pm_impl.c` SHA256 基线门禁、2 tick 容差、真机复测矩阵含"拔掉 USB 用电池运行至少 2 小时"、验收失败条件、临时取证清理与"永久 esp_pm 修复不会删除"、对 EWF 的硬规则）
- [Source: `docs/embedded/bsp/外设硬件软件二开.md`#第 107、111、311、318-327 行]（USB-Serial-JTAG 设备侧事实行、电源/USB 落图合同、GPIO43/44 是 UART0 默认映射因此 console 必须改到 USB-Serial-JTAG、§9 全文）
- [Source: `docs/embedded/guides/低功耗策略与实测验收.md`#第 19、31、44、72、89、106、127、132 行]（状态词表 `DEFERRED`、USB-Serial-JTAG 唯一调试通道、整机电流不得用数据手册典型值、PM 锁与拔线后合同、USB 调试不自动休眠行、功耗采集必须断开 USB、拔线唤醒链门禁、编译与单测不替代真机验收）
- [Source: `docs/embedded/style/C编码规范-Agent版.md`#第 156、164 行]（日志级别/限频与禁止输出凭据；释放失败不得伪报成功）
- [Source: `docs/embedded/README.md`#第 5、13、16、18 行]（引脚唯一权威；排障文档索引含 light-sleep/USB 日志失联）
- [Source: `AGENTS.md`#3.2、3.3、3.4、3.5、3.7、4、5]（日志中文+ASCII TAG、先查证后使用、sdkconfig 锚点清单含 `CONFIG_USJ_*`、EWF 差异红线、文案硬禁令、症状索引、只读蓝本）
- [Source: `Embedded/AGENTS.md`#ESP-IDF v5.5.4 ESP32-S3 API 校验规则、#PWR 实体键与板级电源所有权、#ESP 日志语言与标签、#文件头、#.c/.h 布局、#中文注释、#EWF 嵌入式与 cloud 边界]（固件层唯一强制规则；API 校验顺序；"硬件关机后 USB 停止枚举"；日志与文件头模板；Token 不得进入普通日志）
- [Source: `Embedded/sdkconfig.defaults`#第 19-31、41-42 行]（PM/light-sleep 锚点与 USB-Serial-JTAG 三行锚点）
- [Source: `Embedded/sdkconfig`#第 879-882、1023-1036、1163-1171、1247 行]（USJ 段、PM 段、console 段、tickless idle）
- [Source: `Embedded/components/contract_checks/contract_checks.c`#第 35-54、112-119 行]（USB 三处 `#error` 与 IO19/IO20 `_Static_assert`）
- [Source: `Embedded/main/app_main.c`#第 35-56、60-63、70-72、87-105、440-482 行]（锚点 `#error` 第二份、启动 PM 锁、USB 中文日志、`abort_startup_power_management()` 的 `light_sleep_enable=false` 收尾）
- [Source: `Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h`#第 119-122 行] 与 [`bsp_resources.c`#第 260-267 行]（`EWF_BSP_USB_SERIAL_JTAG_DM_GPIO`/`_DP_GPIO` 与 BSP 资源条目）
- [Source: `Embedded/components/services/selftest_service/selftest_service.c`#第 633-679、932-933 行]（USB 资源断言与"待样机验证项"声明）
- [Source: `Embedded/components/esp_pm/CMakeLists.txt`、`Kconfig`、`sdkconfig.rename`、`generate_v554_pm_impl.py`]（IDF 同名组件覆盖与哈希门禁，本 Story 只做存在性断言）
- [Source: `Embedded/tests/run_host_tests.py`、`tests/test_bsp_contract.py`（含 `PRODUCTION_SOURCES` 与 `FORBIDDEN_TOKENS`）、`tests/test_power_boot_policy.c`、`tests/test_pvdf_confirm_policy.c`]（主机门禁与源码合同扫描范式、白名单与禁用符号表）
- [Source: `Embedded/CMakeLists.txt`、`Embedded/main/CMakeLists.txt`、`Embedded/partitions.csv`、`Embedded/.gitignore`]（`EXTRA_COMPONENT_DIRS` 与 `MINIMAL_BUILD`、main 的 `PRIV_REQUIRES`、分区与"Core Dump 尚未启用"、忽略规则）
- [Source: `Embedded/build/project_description.json`、`Embedded/build/flasher_args.json`]（`debug_arguments_openocd`、`build_components`、`flash_files` 与 `extra_esptool_args`）
- [Source: `_bmad-output/implementation-artifacts/1-1-建立定稿电源-启动与-bsp-自检.md`]（上游 Story 情报、USB 保留给 Story 1.3 的边界声明、5 项 deferred、未验证缺口）
- [Source: `_bmad-output/implementation-artifacts/1-2-闭合-pvdf-唤醒与-adc-二次确认.md`]（"USB-Serial-JTAG 诊断路径与 light sleep 关断策略 → Story 1.3"的边界表、锚点清单、日志措辞口径、13 项 deferred）
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md`#第 29 行]（产品可见日志含设计/流程解释性文字的未决口径）
- [Source: `_bmad-output/implementation-artifacts/sprint-status.yaml`#第 38-41 行]（epic-1 为 `in-progress`；1-1/1-2 为 `done`；1-3 为 `backlog`）
- ESP-IDF v5.5.4 官方依据（`Embedded/AGENTS.md` 强制）：USB Serial/JTAG Console <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/usb-serial-jtag-console.html>；JTAG 调试索引 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/jtag-debugging/index.html>；内置 JTAG 配置 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/jtag-debugging/configure-builtin-jtag.html>；电源管理 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/power_management.html>；睡眠模式 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/sleep_modes.html>；Kconfig 参考 <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/kconfig-reference.html>
- 本机 IDF 源码依据（已核验）：`$IDF_PATH/components/esp_driver_usb_serial_jtag/Kconfig:4-27`、`$IDF_PATH/components/esp_driver_usb_serial_jtag/include/driver/usb_serial_jtag.h:48-113`、`$IDF_PATH/components/esp_system/Kconfig:296-322`、`$IDF_PATH/components/esp_pm/include/esp_pm.h`、`$IDF_PATH/tools/idf_py_actions/debug_ext.py`、`$OPENOCD_SCRIPTS/board/esp32s3-builtin.cfg`、`$IDF_PATH/tools/cmake/version.cmake`

## Dev Agent Record

### Agent Model Used

deepseek-v4-flash[1m]（BMad dev-story，无人监管单轮执行）

### Debug Log References

**1. 主机门禁（全量回归）**

```
$ cd Embedded && python3 tests/run_host_tests.py        # 退出码 0
source-contract: 40/40 通过          （Story 1.3 新增 12 条 USB/日志/PM 门禁）
usb-serial-jtag-diag: 45/45 通过     （新增主机用例，纯 Python）
power_boot_policy: 9 个主机用例全部通过
pvdf-confirm-policy: 全部断言通过
host tests: 全部通过
```

```
$ python3 -m py_compile Embedded/tools/usb_serial_jtag_diag.py \
      Embedded/tests/test_usb_serial_jtag_diag.py \
      Embedded/tests/run_host_tests.py Embedded/tests/test_bsp_contract.py   # 退出码 0
```

**2. ESP-IDF 构建（runbook 流程，`Embedded/`）**

- 运行编号 `20260922-124314`，记录目录 `docs/embedded/build_records/20260922-124314/`（被 `.gitignore:49` 忽略，不入变更集）。
- `source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` → `idf.py --version` = `ESP-IDF v5.5.4`（唯一 attempt，无失败重试）。
- `idf.py reconfigure` + `idf.py build` 退出码 **0**；`build-01.log` 中 `error:` 计数 0、`warning:` 计数 0；五个产物齐全；`build_components` 74 项，USB 相关项只有 `esp_driver_usb_serial_jtag`。
- 看到 `Project build complete. To flash, run:` 后按 runbook:233 **到此停止**，未执行 `flash`/`app-flash`/`erase-flash`/`monitor`。

**3. 预检回执（本 Story 的核心可判定证据，无设备主机真实运行）**

运行编号 `20260922-124540`，回执目录 `docs/embedded/hardware_test_records/20260922-124540/`（被根 `.gitignore:50` 忽略）。

```
$ python3 Embedded/tools/usb_serial_jtag_diag.py ports                         # 退出码 1
/dev/cu.Bluetooth-Incoming-Port	desc=n/a	hwid=n/a
/dev/cu.UGREENHiTuneS3	desc=n/a	hwid=n/a
/dev/cu.debug-console	desc=n/a	hwid=n/a
stderr: 端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口

$ python3 Embedded/tools/usb_serial_jtag_diag.py preflight                     # 退出码 1
已写入回执：…/environment.txt
已写入回执：…/session.md
IDF 版本：通过
构建产物：通过
端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口
stderr: 预检未通过：端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口

$ python3 Embedded/tools/usb_serial_jtag_diag.py flash   --run-id 20260922-124540 --serial DC:DA:0C:12:34:56   # 退出码 1
stderr: 拒绝执行 flash：端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口
$ python3 Embedded/tools/usb_serial_jtag_diag.py monitor --run-id 20260922-124540 --serial DC:DA:0C:12:34:56   # 退出码 1
stderr: 拒绝执行 monitor：端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口
$ python3 Embedded/tools/usb_serial_jtag_diag.py openocd --run-id 20260922-124540                             # 退出码 1
stderr: 拒绝执行 openocd：端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口
```

拒绝路径证据：`find docs/embedded/hardware_test_records -name 'flash-*.log' -o -name 'serial-*.log' -o -name 'openocd-*.txt'` 命中 **0**；`Embedded/build/openocd_out.txt` **不存在**（即未启动过 `idf.py openocd`）。回执目录只有 `environment.txt` 与 `session.md`，符合冻结布局。

`environment.txt` 的 runbook:162-171 期望清单逐项核对：`CONFIG_IDF_TARGET="esp32s3"`、`CONFIG_ESPTOOLPY_FLASHSIZE="16MB"`、`FLASHMODE="dio"`、`FLASHFREQ="80m"`、`MONITOR_BAUD=115200`、`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`、`CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y` —— 全部 OK。凭据字样扫描（`token|password|secret|appsecret`）无命中。

**4. 实现期间修正的两处缺陷（均有回归门禁覆盖）**

- **`idf.py` 解析缺陷**：本机激活脚本把 `idf.py` 定义为 shell 函数（`$IDF_PATH/tools/idf.py` 的转发），PATH 上没有同名可执行文件，`shutil.which("idf.py")` 恒为 `None`，导致首版预检把 IDF 版本误判为"无法读取"。修正为 `_idf_invocation()`：优先取 PATH 上的真实可执行文件，否则回落 `$IDF_PYTHON_ENV_PATH/bin/python $IDF_PATH/tools/idf.py`（同一入口脚本、同一组参数，不增删任何子命令），解析失败即 fail-closed 拒绝且不产生回执；新增 `test_idf_invocation_resolves_the_activation_script_shell_function` 与 `test_unresolvable_idf_entry_point_is_refused_before_writing_a_receipt` 两条门禁。
- **`openocd` 门禁集缺陷**：首版把 `openocd` 的门禁集做成"与端口无关"（因为 `idf.py openocd` 不带端口参数），于是在无设备主机上放行了 `idf.py openocd`，OpenOCD 真的启动并报 `Error: esp_usb_jtag: could not find or open device!`。这违反 Task 1.3 / AC 2 实现口径 5"同一 fail-closed 条件集覆盖 `flash`/`monitor`/`openocd`"。修正为 `openocd_phase_decision()` 同样先确认唯一设备端口（JTAG 与烧录、监控共用同一 USB PHY），命令本身仍为不带端口参数的 `idf.py openocd`；新增 `test_openocd_phase_decision_is_refused_without_a_confirmed_port` 与 `test_openocd_phase_decision_is_refused_when_the_port_is_busy`。修正后已删除该次中间回执与 `Embedded/build/openocd_out.txt`，并用修正版重新产出回执。

**5. code-review 裁定后的缺陷修正（方向 a）：序列号确认硬门禁（本轮拒绝路径实跑证据）**

运行编号 `20260922-130253`（预检回执）、`20260922-130311`（构建记录），均在 `source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` 后执行（`idf.py --version` = `ESP-IDF v5.5.4`）。

```
$ cd Embedded && PYTHONDONTWRITEBYTECODE=1 python3 tests/run_host_tests.py      # 退出码 0
source-contract: 41/41 通过
usb-serial-jtag-diag: 53/53 通过      （本轮新增 6 条序列号门禁用例）
power_boot_policy: 9 个主机用例全部通过
pvdf-confirm-policy: 全部断言通过
host tests: 全部通过

$ python3 -m py_compile Embedded/tools/usb_serial_jtag_diag.py \
      Embedded/tests/test_usb_serial_jtag_diag.py \
      Embedded/tests/run_host_tests.py Embedded/tests/test_bsp_contract.py      # 退出码 0

$ python3 Embedded/tools/usb_serial_jtag_diag.py flash   --run-id 20260922-130253   # 退出码 1（缺 --serial）
$ python3 Embedded/tools/usb_serial_jtag_diag.py monitor --run-id 20260922-130253   # 退出码 1（缺 --serial）
$ python3 Embedded/tools/usb_serial_jtag_diag.py openocd --run-id 20260922-130253   # 退出码 1（缺 --serial）
stderr: 未提供操作员确认的设备序列号（--serial）：runbook §7.1 要求设备序列号经操作员确认后才可继续，flash/monitor/openocd 不得在无确认序列号时执行
（三条均在枚举端口之前拒绝：未创建运行目录、未写任何回执）

$ python3 Embedded/tools/usb_serial_jtag_diag.py preflight --run-id 20260922-130253 --serial DC:DA:0C:12:34:56   # 退出码 1
已写入回执：…/environment.txt / …/session.md
IDF 版本：通过 / 构建产物：通过 / 端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口 / 退出码：1
$ python3 Embedded/tools/usb_serial_jtag_diag.py flash   --run-id 20260922-130253 --serial 11:22:33:44:55:66   # 退出码 1
$ python3 Embedded/tools/usb_serial_jtag_diag.py monitor --run-id 20260922-130253 --serial 11:22:33:44:55:66   # 退出码 1
$ python3 Embedded/tools/usb_serial_jtag_diag.py openocd --run-id 20260922-130253 --serial DC:DA:0C:12:34:56   # 退出码 1
stderr: 拒绝执行 <phase>：端口：端口零匹配：未发现 USB JTAG/serial debug unit（303A:1001）候选端口

$ python3 Embedded/tools/usb_serial_jtag_diag.py ports                                   # 退出码 1（无 --serial）
（先列出全部候选端口，再给中文拒绝理由）
```

独立复现（纯函数层，对应 code-review 的两条原始复现输入；决策层只算结论，不触发任何设备动作）：

```
无 SER= 且不传 --serial            → allowed=False（未提供操作员确认的设备序列号）
SER=FF:FF:FF:FF:FF:FF 且不传 --serial → allowed=False（同上）
无 SER= 且 --serial 已提供          → allowed=False（端口未枚举到设备序列号（SER= 缺失））
SER=FF:FF:… 且 --serial 不符        → allowed=False（端口序列号不符：枚举 FF:…，期望 11:…）
SER=FF:FF:… 且 --serial 一致        → allowed=True
确认序列号后 flash / openocd 决策    → allowed=True（决策层，未执行任何动作）
```

拒绝路径零泄漏：`find docs/embedded/hardware_test_records -name 'flash-*.log' -o -name 'serial-*.log' -o -name 'openocd-*.txt'` 计数 **0**；`Embedded/build/openocd_out.txt` **不存在**；运行目录 `20260922-130253` 内只有 `environment.txt` 与 `session.md`。本轮在未激活 ESP-IDF 环境下误跑的一次 preflight 回执 `20260922-130244`（结论为"未读取到版本"）已删除，不留误导性证据。构建记录 `docs/embedded/build_records/20260922-130311/`：`idf.py reconfigure` + `idf.py build` 退出码 **0**（唯一 attempt，无失败重试），`error:`/`warning:` 均 0，五个产物齐全，`build_components` 74 项且 USB 相关项仅 `esp_driver_usb_serial_jtag`；看到 `To flash, run: idf.py flash` 后按 runbook:233 到此停止。

### Completion Notes List

**已完成（可在无样机条件判定）**

1. **Task 1 — 诊断脚本**：`Embedded/tools/usb_serial_jtag_diag.py`，子命令固定 `ports` / `preflight` / `flash` / `monitor` / `openocd`。全部判定为纯函数、零 ESP-IDF、零第三方依赖（端口枚举只经 `python -m serial.tools.list_ports -v` 文本解析；由 `test_diagnostic_tool_stays_free_of_idf_and_third_party_imports` 以 AST 导入集判定，只允许 12 个标准库模块）。回执沿用真机闭环 runbook §5 冻结布局，只为 JTAG 通道新增 `openocd-01.txt`，未新建第二套命名。`flash` 只允许 `idf.py -p <port> flash`（经真实 `tee` 落 `flash-01.log`，`O_EXCL` 拒绝覆盖），`monitor` 在 PTY 中运行并原样落 `serial-01.log`（`Ctrl+]` 正常退出），`openocd` 归档 `build/openocd_out.txt` 为 `openocd-01.txt`。未提供 `erase-flash` / `erase_flash` / `app-flash` 入口（argparse 拒绝，退出码 2）。
2. **fail-closed 八条**：无确认端口 / 零匹配 / 多匹配 / `--serial` 不符 / 运行目录已存在（建目录门禁）/ IDF 版本越出 `>=5.5.4,<5.6.0` / 必需构建产物缺失 / 端口被 `idf_monitor` 占用，全部有主机用例，且拒绝路径不产生任何回执文件（`test_refused_*` 三条已断言）。端口选择无任何猜测路径（`test_multiple_matching_ports_are_refused_instead_of_guessed`）。
3. **Task 2/3/4 — 源码合同门禁**：`Embedded/tests/test_bsp_contract.py` 新增 12 条 —— sdkconfig 三项 USB 锚点；console 禁用族（按 `=y` 与精确 token 判定，不用裸子串）；`REQUIRES`/`PRIV_REQUIRES` 按组件 token 判定（不含 `usb`/`tinyusb`）；`main/` + `components/**` 的 `ch340`/`usb_otg`/`tinyusb` 零命中；IO19/IO20 与 `_Static_assert` 一致；`contract_checks.c` 三处 USB `#error`；`PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP` 未启用 + `PM_ENABLE`/`TICKLESS_IDLE` 仍启用；`esp_pm` 覆写四件套存在性（未改其内容）；TAG 稳定 ASCII；日志正文中文（跨行调用与拼接宏按括号配平解析，不按行子串匹配）；TAG 定义与日志使用一一对应；日志正文无凭据字样。
4. **Task 3.2 的中文判定口径（需评审知悉的解释）**：`ALLOWED_ASCII_ONLY_LOG_CALLS` 机制已实现但当前为**空集**。"正文含中文"的机械门槛取"汉字、中文标点或全角形式"三类码点，因此唯一的近似命中行 `main/app_main.c:327` 的 `ESP_LOGI(TAG, "Flash JEDEC ID：0x%06" PRIx32, flash_id)` 由全角冒号满足（该行是 `Embedded/AGENTS.md:182` 允许保留英文的"技术标识符"行），无需把它写成并非"纯技术符号串"的白名单条目。187 处日志调用现全部通过；纯 ASCII 英文正文（如 `"USB connected: %d"`）仍会被该门禁拒绝。
5. **Task 5 — 回执约定**：`preflight` 生成 `environment.txt`（IDF 版本、`git rev-parse --short HEAD`、`git status --short`、runbook:156 的 `rg -n` sdkconfig 取证命令）与 `session.md`（runbook §15.3 模板骨架，未执行阶段逐项写 `未执行（当前无样机）`；工作区状态压成一行摘要，完整列表由 `environment.txt` 承载）。根 `.gitignore` 追加 `docs/embedded/hardware_test_records`（`git check-ignore` 已命中）。
6. **Task 6 — 主机门禁接入**：新建 `Embedded/tests/test_usb_serial_jtag_diag.py`（45 条用例）；在 `run_host_tests.py` 追加 `run_usb_serial_jtag_diag_tests()`（`main()` 注册方式，与 `test_bsp_contract` 同一反射发现路径，声明了参数的用例注入临时目录）；`PRODUCTION_SOURCES` 增加 `tools/**/*.py`，使新脚本进入源码合同扫描（未顺手补 `components/ui`、`components/esp_pm` —— 属 Story 1.1 既有缺口）。
7. **Task 7 — 证据**：构建与预检证据见 Debug Log；两者都不构成真机验证。

**未修改项（按红线与"实现边界"保持原状）**

- **未改动任何固件 `.c`/`.h`**（`Embedded/` 下 0 处源码改动），因此无需评估 `LEGBOT_SERVICE_COUNT`/`SELFTEST_ITEM_COUNT` 连锁变更；`sdkconfig.defaults`、`sdkconfig`、`partitions.csv`、两份 runbook、`docs/embedded/**` 既有文档、`docs/hardware/**` 全部未改。
- 未新增 coredump 或诊断分区、未启用 `CONFIG_ESP_COREDUMP_*`、未修改 `Embedded/components/esp_pm/**`、未安装 USB Serial/JTAG 驱动、未新增 PM 锁调用或自检项。
- `SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE` 保持 `0U`，自检证据未升级为 `HARDWARE_VERIFIED`。

**`hardware_pending` 清单（本 Story 不可判定，逐项写明未验证原因与复验入口）**

| 未验证事实 | 未验证原因 | 复验入口 |
|---|---|---|
| 真机 `idf.py -p <port> flash` 返回 0 且每个镜像出现 `Hash of data verified.` | 当前无样机；runbook:43 禁止烧录 | 样机到位后 `docs/embedded/guides/真机闭环runbook-macos.md` §9 |
| 真机 CDC 日志出现完整启动且连接期间不随机失联（含长时间空闲与自动熄屏循环） | 当前无样机；runbook:43 禁止 `monitor` | 同上 §10–§11 |
| 真机 OpenOCD 连接成功与 GDB 会话可用 | 当前无样机；且 JTAG 需实际枚举到 `303A:1001` | 同上 §13 与 `idf.py openocd`（`-f board/esp32s3-builtin.cfg`） |
| 端口真实 VID:PID 与设备序列号（EWF 尚未登记） | 需首次真机枚举后由操作员确认 | runbook §7.1 唯一设备匹配门禁 |
| 拔掉 USB 用电池时的自动 light sleep 恢复与唤醒链 | 需真机电流与唤醒实测 | `docs/embedded/guides/低功耗策略与实测验收.md` |
| 下载进入序列（先按 BOOT0、再长按 PWR 约 2 s）与 `GPIO0=0、GPIO46=0` 联合下载组合 | `ESP32-S3复位与启动控制设计.md:116` 明文仍需样机验证 | 样机到位后按该文档逐项实测 |
| `HW-OI-005`（USB-C 直连 ESP32 与 BQ D± 不使用的兼容性） | 属样机阶段电气验收 | USB 枚举 / 不同充电器 / BQ 输入限流与反灌测试 |
| USB-C 机械沉板高度、外壳开孔与屏蔽脚布局 | 属机械件验收 | 结构与装配验收 |
| `monitor` 的 PTY 分支与 `openocd` 的 `build/openocd_out.txt` 归档路径 | 无设备主机上两者均被 fail-closed 拒绝，实机路径未被触发 | 样机到位后首轮三路闭环 |

### File List

- `.gitignore`（修改：追加 `docs/embedded/hardware_test_records`）
- `Embedded/tools/usb_serial_jtag_diag.py`（新增：诊断脚本与全部纯逻辑判定）
- `Embedded/tests/test_usb_serial_jtag_diag.py`（新增：53 条主机用例，含序列号确认硬门禁的拒绝用例）
- `Embedded/tests/run_host_tests.py`（修改：注册 `run_usb_serial_jtag_diag_tests()`）
- `Embedded/tests/test_bsp_contract.py`（修改：追加 12 条 USB/日志/PM 门禁，`PRODUCTION_SOURCES` 补 `tools/**/*.py`）

构建证据与预检回执落在被忽略路径，不进变更集：`docs/embedded/build_records/20260922-124314/`、`docs/embedded/hardware_test_records/20260922-124540/`。

### Change Log

- 2026-09-22（Story 1.3 实现，dev-story）：新建主机侧 USB-Serial-JTAG 诊断脚本（五子命令、纯函数判定、fail-closed 八条、`tee`/PTY 回执归档），把日志 TAG/正文、USB 独占与 light-sleep 调试锚点从口头规范落成 12 条源码合同门禁，新建 45 条主机用例并接入 `run_host_tests.py`，追加 `docs/embedded/hardware_test_records` 忽略规则。`Embedded/` 固件源码零改动。
- 2026-09-22（同批修正）：`idf.py` 解析回落 `$IDF_PYTHON_ENV_PATH/bin/python $IDF_PATH/tools/idf.py`（激活脚本把 `idf.py` 定义为 shell 函数）；`openocd` 门禁集补齐"唯一设备端口确认"，与 `flash`/`monitor` 共用同一 fail-closed 条件集。两条修正均有对应主机门禁。
- **Task 6.2 选择说明**：采用"在 `main()` 追加 `run_usb_serial_jtag_diag_tests()`"分支（非 `PRODUCTION_SOURCES` 反射），因为新增用例声明了 `tmp_path` 参数，需在反射调用时注入临时目录；与 `test_bsp_contract` 仍是同一"反射发现 `test_*` 函数"范式。
- 2026-09-22（code-review 裁定后的缺陷修正，方向 a —— 依据：story 冻结的 runbook §7.1 原文「仅当**一个**端口同时匹配描述、VID:PID **且设备序列号经操作员确认**时才可继续」「序列号缺失或与预期不符时，列出全部候选并请求操作员确认」，AC 2 实现口径 5 首条 fail-closed 条件「无确认端口」，以及 story 自述「EWF 设备序列号尚未登记（runbook §2）→ 首次真机枚举后由操作员确认」，故无预登记值可自行比对、必须由操作员显式确认）：把序列号确认从"`expected_serial is not None` 才比对"上升为**硬门禁**。
  - 新增常量 `SERIAL_REQUIRED_SUBCOMMANDS = ("flash", "monitor", "openocd")` 与纯函数 `serial_requirement_decision()`：`serial` 为 `None`/空/纯空白即拒绝并给出中文理由。`flash`/`monitor`/`openocd` 在 `_require_operator_serial()`（子命令入口首行）校验，缺 `--serial` 时以退出码 1 拒绝；此时**不枚举端口、不解析或创建运行目录、不写任何回执**，拒绝路径零副作用。`ports`/`preflight` 两个只读子命令的 `--serial` 仍非必需，参数面未变。
  - `select_port()` 删除"`expected_serial is None` 时无条件放行"路径：候选为零、候选多于一、候选端口未枚举到 `SER=`、`--serial` 缺失、`--serial` 与枚举结果不符，五类情形一律拒绝；放行条件收敛为「候选唯一 且 该端口枚举到 `SER=` 且与 `--serial` 逐字一致」。
  - 门禁顺序保留信息量：先判候选（零匹配 / 多匹配 / 指定端口不在候选内），再判序列号，使 `ports`/`preflight` 在无设备主机上仍先列出全部候选再给拒绝理由（runbook §7.1 要求的「列出全部候选并请求操作员确认」）。
  - 用例同步：`test_single_matching_port_is_confirmed` → 更名 `test_single_matching_port_is_confirmed_only_with_the_operator_serial` 并传入 `--serial`；新增 `test_single_matching_port_without_an_operator_serial_is_refused`、`test_port_without_an_enumerated_serial_is_refused`、`test_operator_serial_requirement_is_a_hard_gate`、`test_serial_phases_require_an_explicit_operator_serial`（三路 handler 缺 `--serial` 一律拒绝且运行目录零残留），并在 flash/openocd 的 fail-closed 汇总用例中补「缺 `--serial`」「无 `SER=`」两条；`test_requested_port_inside_the_candidates_is_confirmed` 与 `test_openocd_phase_decision_is_refused_when_the_port_is_busy` 补传已确认序列号以继续覆盖其原本意图。
  - spec 张力处置：本 Story 完成判定第 1 条「端口单匹配→通过」写窄了，**以冻结门禁为准**（该条所在区段不在本次授权的改动范围内，故不改写正文）；`test_single_matching_port_is_confirmed` 已按冻结门禁更新，即"单匹配"必须在序列号经操作员确认的前提下才成立。
  - 本轮未改动 `Embedded/` 固件源码、`sdkconfig*`、`partitions.csv`、`.gitignore`、`run_host_tests.py`、`test_bsp_contract.py`（`.gitignore`/`run_host_tests.py`/`test_bsp_contract.py` 的上一轮改动原样保留），变更集仍为 5 条路径。
- 2026-09-22（attempt-2 复审，`bmad-code-review` depth=deep / mode=full / autofix / unattended 单轮）：上一轮唯一 unresolved MEDIUM（runbook §7.1 序列号确认硬门禁）经独立复现与 7 组变异测试确认已闭合；零 patch（本轮无「无歧义且不削弱门禁」的可自动应用项）、零 unresolved HIGH/MEDIUM，强制门禁命令退出码 0，story 推进为 `done`。4 条 low deferred（`SERIAL_REQUIRED_SUBCOMMANDS` 无实现读取者、串号已确认但端口门禁拒绝时先建空运行目录、`--port` 先收窄后判多匹配、`story-local.diff` 仍不可直接应用）与 4 条 `false` 记录见 `attempt-2/code-review.json`。

### Review Findings

代码审查（`bmad-code-review`，depth=deep，mode=full，四层：Blind Hunter / Edge Case Hunter / Verification Gap / Acceptance Auditor；unattended + autofix 单轮）。四层原始发现 31 条，按共同根因归并为 23 条，逐条核验后结论如下；`patch` 项已在本轮应用并通过门禁复验（`run_host_tests.py` 退出码 0，source-contract 41/41、usb-serial-jtag-diag 49/49）。

唯一的 Decision 项经人类裁定（方向 a）后已实施并通过门禁复验（`run_host_tests.py` 退出码 0，source-contract 41/41、usb-serial-jtag-diag 53/53）；裁定口径与实施细节见下方条目与 Change Log。

**Decision（已裁定并解决，1 项）**

- [x] [Review][Decision] runbook §7.1「设备序列号经操作员确认」这条硬门禁未被脚本强制 [Embedded/tools/usb_serial_jtag_diag.py:select_port] — **已裁定：采用方向 (a)，并已实施。** 原缺陷：`select_port()` 只在 `expected_serial is not None` 时比对序列号，随后无条件 `return PortSelection(True, "", record.device)`；而 `--serial` 在五个子命令中全部 `default=None`，因此不传 `--serial` 时，`hwid: USB VID:PID=303A:1001 LOCATION=20-1`（**根本没有 `SER=`**）与 `SER=FF:FF:FF:FF:FF:FF`（存在但从未被操作员确认）两种输入都返回 `allowed=True`，`flash`/`monitor`/`openocd` 随即放行。裁定依据：spec 的 Dev Notes 原文引用并冻结了该门禁——「仅当**一个**端口同时匹配描述、VID:PID **且设备序列号经操作员确认**时才可继续」「序列号缺失或与预期不符时，列出全部候选并请求操作员确认」，并明写「该规则必须固化进诊断脚本（Task 1.3）」；AC 2 实现口径 5 的首条 fail-closed 条件正是「无确认端口」；且 story 自述「EWF 设备序列号**尚未登记**（runbook §2）」——既无预登记值，操作员必须显式确认。**实施结果**：(1) `--serial` 成为 `flash`/`monitor`/`openocd` 的必需参数，缺省即以退出码 1 拒绝并给出中文理由（`_require_operator_serial()` / `serial_requirement_decision()`），且拒绝发生在枚举端口与建运行目录之前，零副作用；`ports`/`preflight` 只读子命令保持原状。(2) `select_port()` 删除「`expected_serial is None` 时无条件放行」路径，放行条件收敛为「候选唯一 且 枚举到 `SER=` 且与 `--serial` 逐字一致」，零匹配/多匹配/无 `SER=`/缺 `--serial`/序列号不符五类一律拒绝。(3) 既有 fail-closed 语义与上一轮 9 项 patch 的效果保持不变：拒绝路径仍不产生任何 `flash-*.log` / `serial-*.log` / `openocd-*.txt`。spec 自身的张力（完成判定第 1 条「端口单匹配→通过」及其对应的 `test_single_matching_port_is_confirmed`）以冻结门禁为准：该用例已改为传入与枚举结果一致的 `--serial` 才通过，并新增覆盖「缺 `--serial`」「候选端口无 `SER=`」「`--serial` 与枚举不符」的拒绝用例。实跑证据见 Debug Log References 第 5 节（三条缺 `--serial` 命令退出码均为 1、回执零泄漏、主机门禁 exit 0、构建 exit 0）。

**Patch（已应用，9 项）**

- [x] [Review][Patch] 回执先建后用：执行失败路径留下 0 字节回执，并永久堵死同一次运行编号的重试 [Embedded/tools/usb_serial_jtag_diag.py:_execute_tee/_execute_openocd/_execute_monitor] — 复现（修复前）：`_execute_openocd` 先 `_create_receipt` 再判 `returncode != 0`，失败时留下 0 字节 `openocd-01.txt`，随后 `receipt_target_decision` 以「拒绝覆盖」挡死重试，而 `session.md` 的阶段表仍把该空文件当作 JTAG 通道证据；`_execute_tee` 在 `shutil.which("tee")` 判空之前就建出 `flash-01.log`，同一形态。修复：前置条件（`tee` 是否存在、命令是否成功、`build/openocd_out.txt` 是否存在）先判完再以 `xb` 独占建回执；`monitor` 仅在「零字节捕获且子进程失败」时回收空回执。复验：openocd 失败 → 退出码 2 且无回执；openocd 成功 → 归档 21 字节真实输出；缺 `tee` → 退出码 2 且无回执；monitor 子进程立即失败且零输出 → 无回执。
- [x] [Review][Patch] `monitor` 的 `Ctrl+]` 正常退出返回 `-15`（shell 241），与脚本「0 = 通过」的契约冲突 [Embedded/tools/usb_serial_jtag_diag.py:_execute_monitor] — 复现（修复前）：读到 `0x1d` 即 `break`，`finally` 随即 `terminate()` 尚未退出的监控器，`child.returncode` 为 `-15`，`SystemExit(-15)` 使进程退出码为 241；runbook 明文以 `Ctrl+]` 作为监控阶段的正常收尾，操作员会把成功会话读成失败。修复：新增 `_stop_child()`（先给 `MONITOR_SHUTDOWN_GRACE_SECONDS=3` 自行收尾，超时才 `terminate`）与 `operator_stopped` 标记，操作员主动停止一律按正常退出返回 `EXIT_OK`。复验：同一 PTY 探针下退出码由 `-15` 变为 `0`，回执保留 30 字节真实输出。
- [x] [Review][Patch] `SUBCOMMANDS` 与 `RECEIPT_FILENAMES` 两个「冻结契约」常量没有任何实现读取者，对应门禁永不失败 [Embedded/tests/test_usb_serial_jtag_diag.py:345-346,565-569] — 解析器用字面量建 5 个子命令、4 处硬编码回执文件名，而门禁只断言同名常量本身：加入第 6 个子命令（例如红线 2 明令禁止的 `app-flash`）或改名回执文件时该「冻结」门禁依然全绿。修复：新增 `test_parser_exposes_exactly_the_frozen_subcommands`（断言 `_build_parser()` 真实 choices）、`test_parser_refuses_the_banned_flash_subcommands`（`erase-flash`/`erase_flash`/`app-flash` 必须 `SystemExit`）、`test_receipt_filenames_are_the_ones_the_tool_actually_writes`（剔除 `RECEIPT_FILENAMES` 声明体后断言实现内落盘字面量）。逐项注入验证：加入 `app-flash` 子命令、把落盘名改为 `flash.log` 均被捕获；复原后通过。
- [x] [Review][Patch] `REQUIRED_ARTIFACTS["flash"]` 的用例夹具取自被测常量，完整烧录清单内容未被固定 [Embedded/tests/test_usb_serial_jtag_diag.py:78-79,276-279] — 夹具与「全绿」用例都读 `diag.REQUIRED_ARTIFACTS`，删掉 `spiffs.bin` 或 `partition_table/partition-table.bin` 后套件仍全绿，AC 1「不得用 `app-flash` 替代、必须写分区表与已定案数据分区」的意图失去自动保护。修复：夹具改为字面量五元组并新增 `test_flash_artifact_list_is_the_frozen_full_flash_set` 钉住 AC 的五个产物；注入验证：删掉 `spiffs.bin` 即被捕获。
- [x] [Review][Patch] `usb`/`tinyusb` 组件依赖门禁只覆盖根与 `main/CMakeLists.txt`，且 `REQUIRES ${requires}` 未展开 [Embedded/tests/test_bsp_contract.py:678-691] — 真实依赖声明在 `components/**/CMakeLists.txt`：`components/BSP/CMakeLists.txt` 经 `set(requires …)` + `REQUIRES ${requires}` 登记，判定结果退化成无效 token `${requires}`（实测原实现的 token 集为 `{'${requires}','PRIV_REQUIRES','esp_timer'}`），而红线 7 与 AC 1 判定依据 4（`build_components` 中 USB 相关项只有 `esp_driver_usb_serial_jtag`）在声明处完全没有自动门禁。修复：`_cmake_requires_tokens` 增加 `set(<变量> …)` 展开、按词边界排除 `PRIV_REQUIRES` 噪声并剔除 CMake 关键字；门禁改为扫描构建图内全部 `CMakeLists.txt`（跳过 `build/`、`managed_components/`），并新增 `test_cmake_requires_tokens_expands_set_variables_and_drops_keywords`。注入验证：给 `components/services` 加 `usb`、把 `set(requires` 改名均被捕获。
- [x] [Review][Patch] 日志门禁（正文中文 / 凭据字样）缺扫描计数下限，解析回归时会静默空集通过 [Embedded/tests/test_bsp_contract.py:716-728,745-755] — 同文件的 TAG 门禁已有 `found >= 1`，而这两条只断言 `offenders` 为空：`_log_calls()` 一旦因解析回归返回空集，两条门禁都会静默通过。修复：新增 `scanned` 计数与 `assert scanned >= 1, "未扫描到任何 ESP_LOG 调用，门禁失效"`。
- [x] [Review][Patch] 反射发现 0 个用例时门禁静默通过 [Embedded/tests/run_host_tests.py:52-80] — `run_usb_serial_jtag_diag_tests()` 在 `executed == 0` 时打印「0/0 通过」并返回 0：用例文件被清空后本 Story 新增的 45（现 49）条门禁整体消失而不报错。修复：`executed == 0` 时打印失效行并计一次失败。注入验证：以空模块替换被反射模块后返回 1。
- [x] [Review][Patch] `session.md` 声称「预检结论见 `environment.txt`」，但该文件不含任何门禁结论，Task 7.3 要求的「归档输出与退出码」在回执内无处可查 [Embedded/tools/usb_serial_jtag_diag.py:environment_report/_command_preflight] — `environment.txt` 原只有 IDF 版本、git HEAD、`git status --short`、sdkconfig 抽取四段；三条门禁结论与退出码仅 `print` 到 stdout。修复：`_command_preflight` 把门禁判定移到回执写入之前，`environment_report` 新增 `preflight_conclusion` 参数并追加 `--- 预检结论 ---` 段（逐条门禁结论 + 退出码）。复验：无设备主机上 `preflight` 产出的 `environment.txt` 现含「IDF 版本：通过 / 构建产物：通过 / 端口：端口零匹配… / 退出码：1」。
- [x] [Review][Patch] 回执把取证命令出处标注为「真机闭环 runbook §6.2」，实际来自另一份 runbook 的同名小节 [Embedded/tools/usb_serial_jtag_diag.py:environment_report] — `docs/embedded/guides/真机闭环runbook-macos.md:163` 的 `rg` 模式止于 `CONFIG_ESP_CONSOLE_`，**不含 `CONFIG_USJ_`**；带 `|CONFIG_USJ_` 的那份在 `docs/embedded/macos_esp_idf_hardware_test_runbook.md:158`，也正是 Task 5.1 指定的来源。按原标注去复核的操作员会拿到一份缺 `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION`（AC 2 的核心锚点）的期望清单。修复：标注改为 `macos_esp_idf_hardware_test_runbook.md:150-158 取证命令`。

**Defer（已记录，2 项）**

- [x] [Review][Defer] 交付物 `story-local.diff` 本身不是可应用的补丁 [\_bmad-output/implementation-artifacts/.autopilot/1/1-3-…/attempt-1/story-local.diff:7-8] — deferred：生成器丢失了 `\ No newline at end of file` 标记行，导致 `.gitignore` 的删除行内容被拼成 `docs/embedded/build_records+docs/embedded/build_records`、下一文件的 `--- a/Embedded/tests/run_host_tests.py` 头被粘进 `.gitignore` 新增行，该 hunk 单独取出后 `git apply --check` 报 corrupt patch。**不影响本 Story 的代码**：工作区的 `.gitignore` 第 49/50 行（`build_records` + `hardware_test_records`）与 `run_host_tests.py` 均经独立核对为正确，File List 与该产物解析出的 5 个文件一致。修复归属 autopilot 的 diff 生成器，超出本 Story 的文件集与授权范围。
- [x] [Review][Defer] 故事 Debug Log 自述实现期真的启动过一次 `idf.py openocd`（无设备主机），并删除了该次中间回执 [\_bmad-output/implementation-artifacts/1-3-启用-usb-serial-jtag-诊断路径.md:572] — deferred：该次执行**不构成红线 1 违规**（红线 1 枚举的是 `flash`/`app-flash`/`erase-flash`/`monitor`），最终代码也已修正为先过端口门禁；但它违反了本 Story 自己冻结的「同一 fail-closed 条件集覆盖三路」（Task 1.3 / AC 2 口径 5），且删除产物使该次设备侧动作失去可追溯回执。属记录与流程治理，修复需改动 story 记录或 runbook 纪律，归 epic retrospective。

**Rejected（11 项）**

- `false` — `project_description.json` 为合法 JSON 但非对象时 `.get` 抛 `AttributeError`：该文件由 IDF cmake 生成，恒为对象；未展示程序可达该状态。且即使抛出，进程仍以 traceback 非零退出且不写回执，fail-closed 未被破坏。
- `false` — 非 tty 时 `monitor` 无总超时、可能无限挂起：子进程退出即 `break`，「跟随被监控进程直到它结束」正是监控阶段的契约，`idf.py monitor` 本身行为相同，非缺陷。
- `false` — `assert anchor in defaults` 亦匹配注释行，锚点被注释关闭时门禁仍通过：补偿控制在同批门禁内已存在——`contract_checks.c` 的三处 `#error`（Task 4.1 已断言其存在）会在锚点丢失时编译失败；`CONFIG_ESP_CONSOLE_SECONDARY_NONE` 一项即使被注释也因 Kconfig 回落而仍为 NONE（spec AC 2 口径 2 已言之）。
- `false` — 「运行目录已存在 ⇒ 拒绝」在 `flash`/`monitor`/`openocd` 决策路径上不可达：条件在 `--run-id` 缺省路径上确实生效——`_phase_context` → `_resolve_run_dir(None)` 走 `run_dir_decision(create=True)`，已存在即「运行目录已存在，拒绝覆盖」。审查者只看了 `serial_phase_decision` 的门禁表。
- `false` — Task 2.3「不得新增 CH340X 驱动目录」只覆盖单个硬编码路径：同名门禁的大小写不敏感 `ch340` 符号扫描覆盖 `main/` + `components/**` 全部 `.c/.h`，任何带源码的 CH340X 驱动目录都会被该扫描命中；单路径断言只是冗余的加强项。
- `false` — `_idf_invocation` 落盘命令与 AC 1 冻结字面形式不同：解析只把 `idf.py` 前缀展开为 `$IDF_PYTHON_ENV_PATH/bin/python $IDF_PATH/tools/idf.py`，子命令与参数一个未增删（`flash_command` / `monitor_command` / `openocd_command` 是唯一构造处并有断言钉住），AC 1 的三路命令语义成立。
- `low` — `lsof` 因设备路径消失而失败（stdout 空）与「端口无占用」不可区分：触发需端口在枚举与 `lsof` 之间被拔掉，此时后续 `idf.py` 会响亮失败；修复要新增一条拒绝理由与分支，超出直接更正的范围。
- `low` — 交互态 `os.read(sys.stdin.fileno())` 的 `OSError`（终端断开）未捕获：结果是带 traceback 的非零退出，已捕获部分仍写入 `serial-01.log`，fail-closed 未破坏；修复需新增分支。
- `low` — `_create_receipt` 的 `PermissionError`/`IsADirectoryError` 未捕获：需宿主回执目录被置只读或存在同名目录才可达，且结果仍是非零退出且不写回执（fail-closed 保留），差别仅在错误消息形态；修复需新增分支。
- `low` — 「日志正文为中文」门禁的门槛含全角与中文标点（`LOG_BODY_CJK_PATTERN` 覆盖 U+FF00–U+FFEF），Task 3.2 的白名单机制被架空：AC 1「正文为中文」就当前 187 处调用**已成立**（唯一无汉字的 `main/app_main.c:327` 是 `Embedded/AGENTS.md:182` 允许保留英文的技术标识符行）；收紧门槛并为其增加白名单条目属判定口径的政策选择，需与日志规范一并裁定，超出直接更正的范围。
- `low` — `session.md` 的操作者列写作「模型」：不属于红线 15 枚举的类别（思维链/内部推理/设计过程/调试说明/Node ID/`TODO`/`draft`/`placeholder`/作者提示），且该回执落在被 `.gitignore` 忽略的本地路径、不入库；runbook 模板的操作者列也没有冻结词表，改动属措辞偏好。
- `low` — 两条日志门禁缺计数下限、Task 2.3 的目录断言偏窄同属「门禁强度」类：前者已在本轮按 patch 处理（见上），后者已判 `false`；此处不重复计入。

**审查者未触及的既有事实核对（本轮独立复核，均成立）** — `test_exit_codes` 的 5 个 `1` 确为设计内的 fail-closed 拒绝，而非功能失败：在无设备主机上以激活后的 IDF 环境复跑 `ports`/`flash`/`monitor`/`openocd` 全部退出 1 且 stderr 为中文拒绝理由，`flash-*.log`/`serial-*.log`/`openocd-*.txt` 计数为 0，`Embedded/build/openocd_out.txt` 不存在（即从未真正执行过三路动作）；`preflight` 退出 1 同为设计内结论（AC 2 口径 5 与 Task 1.6/7.3 要求「零匹配 → 明确的中文拒绝理由」）。`erase-flash`/`erase_flash`/`app-flash` 经 argparse 一律退出 2。五项构建产物齐全、`build_components` 74 项且 USB 相关项仅 `esp_driver_usb_serial_jtag`、`debug_arguments_openocd = -f board/esp32s3-builtin.cfg`、`CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP` 在 `sdkconfig` 与 `sdkconfig.defaults` 中均不存在，均已实测确认。

**attempt-2 复审（序列号硬门禁修复后的独立复审，结论：闭合且无新增阻塞）**

上一轮唯一的 unresolved MEDIUM（`select_port()` 未强制 runbook §7.1「设备序列号经操作员确认」）经以下独立复核确认为已闭合，且未引入新问题：

1. **原缺陷反向**：纯函数层复现（不触发任何设备动作）—— 无 `SER=` 且不传 `--serial`、`SER=FF:FF:FF:FF:FF:FF` 且不传 `--serial` 均为 `allowed=False`；缺 `--serial` 在候选与 `SER=` 判定之前拒绝。`--serial` 已是 `flash`/`monitor`/`openocd` 的 CLI 前置门禁，三条子命令缺参退出码 1，且不枚举端口、不建运行目录、不写回执；`ports`/`preflight` 参数面未变。
2. **无绕过路径**：CLI 分派只有 5 个子命令，动作型三路 handler 首行即门禁；全仓无第二条 `idf.py` 执行构造。
3. **拒绝路径零回执**：复跑缺 `--serial` 三条后 `docs/embedded/hardware_test_records` 内容 md5 不变、无新目录，`flash-*.log`/`serial-*.log`/`openocd-*.txt` 计数 0，`Embedded/build/openocd_out.txt` 不存在。
4. **新增用例是可真断言**：7 组变异（撤销三路/只撤销 openocd/回退旧 `select_port` 语义/门禁恒放行/删常量一项/丢掉 openocd 端口门禁/阶段门禁不传 `--serial`）全部被 53 条用例中的对应断言捕获为 FAIL；基线 53/53 通过，与 dev receipt 一致。
5. **`test_exit_codes` 非零码复核**：8 个 `1` 全部为设计内拒绝（缺 `--serial`、零匹配、无 `SER=`、序列号不符、运行目录已存在），`app-flash`/`erase-flash` 为 argparse 退出 2；既有回执 `20260922-130253/environment.txt` 的预检结论段含「退出码：1」。

本轮零 patch、零 unresolved HIGH/MEDIUM、强制门禁命令 `cd Embedded && PYTHONDONTWRITEBYTECODE=1 python3 tests/run_host_tests.py` 退出码 0、File List 与实际 diff 一致（5/5），story 状态由 `review` 推进为 `done`。4 条 low deferred 与 4 条 `false` 记录（含各自的驳回证据）见 `.autopilot/1/1-3-…/attempt-2/code-review.json`。
