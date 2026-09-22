# Story 1.1: 建立定稿电源、启动与 BSP 自检

Status: review

## Story

As a 设备开发者，
I want 按硬件事实源完成电源、启动绑带、PWR、BOOT0、EN 和基础 BSP 初始化，
so that 设备能在各供电条件下安全启动并为后续输入验证提供稳定底座。

本 Story 覆盖 FR-E-001、AD-8、AD-13，建立后续 PVDF（Story 1.2）、USB-Serial-JTAG
诊断（Story 1.3）和设备输入/同步链路所依赖的板级资源边界。这里的“定稿”只表示固件
按当前硬件事实源实现接口合同；当前原理图仍是设计基线，不能把编译或软件自检当成样机
通过。

## Acceptance Criteria

1. **Given** `docs/hardware/` 中的 GPIO、电源和启动绑带基线已载入，**When** 固件执行
   BSP 初始化和启动自检，**Then** PWR 只作为 LTC2954 `PWR_INT` 只读输入，
   BOOT0/EN 角色与硬件基线一致，固件不驱动 KILL、不模拟软件关机。
2. **And** USB-C 外部供电、电池供电和外部供电并充电三种条件下，均不因产品逻辑暂停
   输入、显示、音频或同步路径。主控不检测充电状态，也不得新增“充电中”门控或 UI
   分支；尚无样机时只能记录为待硬件验证。
3. **Given** 设备处于下载流程或运行态，**When** 分别执行“BOOT0 保持低电平后长按
   PWR 上电”和运行态 BOOT0 短按，**Then** 前者进入下载采样，后者只产生运行态自动
   模式切换，不改变下一次下载路径。Story 1.1 负责原始按键边界、去抖/事件交接和
   下载路径不被应用改写；3 秒 `automatic_tap` 定时器及业务链路属于 Story 2.6。
4. **And** 自检输出包含可追溯的引脚、供电和复位结果，且明确区分设计输入、软件观察和
   待样机验证，不把设计输入或软件编译成功误报为样机通过。

## Tasks / Subtasks

- [x] **1. 审计现有 Embedded 基线并收敛 EWF 工程边界（AC: 1, 4）**
  - [x] 先读取并以 docs/hardware、PRD、架构主干为准，审计当前已跟踪的
        Embedded/；不要因为交接文档称“尚未实现”就重复创建第二套 BSP。
  - [x] 更新 Embedded/CMakeLists.txt、Embedded/sdkconfig.defaults 和相关
        component CMake，使工程不再以 legbot/scenic/exoskeleton profile、BLE、GPS、
        ML307R 或旧业务云为默认启动前提；保留可复用的通用驱动时必须写明来源和
        EWF 引脚映射。
  - [x] 使 Embedded/components/BSP/BSP_INCLUDE/bsp_resources.{h,c} 成为唯一板级
        GPIO、总线、电源和初始化顺序入口；删除/禁用旧资源表中与 EWF 冲突的所有者，
        不在 service/app 层新增硬编码引脚。
  - [x] 复核 Embedded/components/BSP/BOARD/bsp_board.{c,h}、KEY/key.{c,h}、
        Embedded/main/app_main.c、Embedded/components/services/legbot_services.*
        和 selftest_service/* 的当前行为，采用最小必要改动迁移而不是平行实现。

- [x] **2. 落实定稿引脚、电源与启动绑带合同（AC: 1, 2）**
  - [x] `PWR_INT` 固定为 IO8：GPIO 只读输入，开漏低有效，按下期间持续为低；由边沿
        ISR 捕获按下/释放时间，任务上下文计算低电平时长。不得把 IO8 配成输出、按电平
        轮询、或把 PWR 当成单脉冲。
  - [x] `BOOT0` 固定为 IO0 输入启动绑带；固件不得在复位采样窗口驱动或重写它。运行态
        短按/释放只通过 typed 事件交给上层，长按、抖动、启动采样和下载流程不产生自动
        模式切换。
  - [x] `EN/RESET_N` 是独立硬件复位输入/测试点，不作为产品按键，不由应用 GPIO API
        驱动；可记录 `esp_reset_reason()` 等软件观察，但不能把它伪装成 RESET_N 波形
        验证。
  - [x] IO46 保持未分配、悬空且不初始化；不得再把它当作 PWR、`PWR_STATE`、唤醒键或
        兼容脚。GPIO45 不得连接/初始化 QMI8658A INT2；IO9/IO11 保留给 Story 1.2 的
        PVDF ADC/比较器唤醒，不得被 GPS/振动马达占用。
  - [x] `PWR_STATE`（LTC2954 EN）只在板上驱动 TPS22965/TLV62569，因最高可到 VSYS
        不接 ESP32 GPIO；`KILL` 只按硬件基线端接到 V3V3，任何固件不得驱动它。
  - [x] 保留 USB-Serial-JTAG GPIO19/20、I2C0 IO1/2 和 Air780EGP UART1
        IO43/44（DTR IO10、RST IO15）的资源所有权；本 Story 不实现完整 4G 或 USB
        诊断，但不得让旧 UART/USB 代码抢占这些脚。

- [x] **3. 建立安全启动和事件边界（AC: 1, 2, 3）**
  - [x] 按 ESP-IDF v5.5.4、ESP32-S3-N16R8 的既定初始化阶段串行启动：日志/配置、
        typed event/state、板级引脚、共享总线、后续服务、自检挂钩；每阶段失败均有
        中文日志和稳定 ASCII TAG，并停止继续启动或进入明确降级态。
  - [x] PWR/BOOT ISR 只做时间戳、原子状态更新或投递 ISR-safe typed event；禁止在 ISR
        中调用 I2C、NVS、UI、音频、网络、阻塞 API 或业务决策。服务任务负责去抖、低电平
        时长和状态路由，遵守 AD-7。
  - [x] 不调用 `esp_deep_sleep_start`、`esp_restart` 或空循环来模拟产品关机，也不以
        GPIO 驱动 LTC2954 KILL；板级 ONT/PDT 负责 PWR 长按硬件开关机。
  - [x] 三种供电条件共享同一启动/运行路径；不得读取或推断充电状态来暂停输入、显示、
        音频、持久化或同步。若旧 `CHARGING_PAUSE`/charging 分支存在，必须在代码中
        删除或隔离为非产品测试说明，并在变更记录中说明依据。
  - [x] BOOT0 下载路径只由 Boot ROM/硬件绑带决定；运行态短按只产生后续自动模式所需
        的 typed 边界事件，不修改下一次复位的 IO0/IO46 采样条件。

- [x] **4. 重建 BSP 自检和可追溯结果（AC: 1, 4）**
  - [x] 将当前 selftest_service 中 GPS UART/FIX、ML307R、vibration 和旧景区项目
        移出本 Story 的 canonical 自检清单；自检至少覆盖资源表/方向、PWR_INT/BOOT0
        原始输入边界、EN/复位软件观察、初始化阶段返回值和禁用资源。
  - [x] 复用现有 typed self-test/result 框架（或在 BSP 内提供窄接口），不要创建第二套
        自检 runner。每一项输出固定资源名、观测值、错误码和 evidence 类别：
        `design_input`、`software_observed`、`hardware_pending`、`hardware_verified`
        或 `failed`；只有附有板级回执时才允许 `hardware_verified`。
  - [x] 自检报告必须能追溯到 GPIO/电源/复位结果和运行 ID；设计文档中声明的 rail
        电压、LTC2954 时序、USB/下载和三种供电行为，在没有示波器/实板记录时标为
        `hardware_pending`，不能输出笼统 PASS。
  - [x] 禁用/保留的 legacy 组件在报告中有明确原因（EWF 非目标、脚位冲突或待后续
        Story），避免通过“跳过”掩盖未审计代码。

- [x] **5. 测试、静态门禁与证据（AC: 1–4）**
  - [x] 为资源表和 PWR/BOOT 边沿语义补 host/Unity 可重复测试：持续低电平的下降/上升
        边只生成一组按压区间，BOOT0 的启动/下载/长按/抖动样例不生成运行态自动事件。
  - [x] 对生产代码（排除文档、Git 历史和 Embedded/build/ 生成物）扫描旧
        GPIO_NUM_46 PWR、GPS/L76KB、ML307R、vibration IO11、QMI INT2 IO45、
        LEGBOT_CAP_* 默认启动、BLE/cloud 自动启动及软件关机 API；发现残留必须有
        明确迁移理由，否则测试失败。
  - [x] 在干净构建目录以项目锁定的 ESP-IDF v5.5.4、target esp32s3 执行
        idf.py reconfigure/idf.py build（或仓库 runbook 等价命令），保留完整命令和
        输出；不能把现有 Embedded/build/ 的旧产物当作证据。
  - [x] 有目标板时按 docs/embedded/guides/真机闭环runbook-macos.md 记录下载采样、
        BOOT0 运行态短按、PWR 边沿/持续低电平、复位和 USB-only/电池-only/USB+电池
        三种矩阵；无目标板时提交硬件验证缺口清单，不能伪造回执。

- [x] **6. 维护边界，避免本 Story 变成后续 Epic 的平行实现**
  - [x] 不实现 PVDF ADC 阈值/候选计数（Story 1.2）、完整 USB 烧录/CDC/JTAG runbook
        （Story 1.3）、自动模式 3 秒定时器（Story 2.6）、LVGL 页面/字体、音频业务、
        4G HTTPS、云同步或持久化 schema；只保留这些能力所需的稳定 BSP/事件接口。
  - [x] 不新增 BLE/Wi-Fi/GPS/ML307R 业务链路、第二套 backend、充电检测状态或软件
        关机方案。保留的通用 legbot 代码须有 EWF 来源、用途、所有权和验证记录。

## Dev Notes

### 真源与冲突裁决

按以下顺序读取和裁决事实，不以旧迁移代码或旧 UX 变体覆盖硬件/PRD：

1. docs/hardware/电子木鱼-硬件原理图设计基线.md、docs/hardware/电源网络命名规范.md、
   docs/hardware/电子木鱼-硬件网络清单.json 以及 docs/hardware/定稿/ 的当前导出
   （板级 GPIO、电源、启动绑带和验证状态）。
2. PRD 的 FR-E-001/FR-E-011、AD-8/AD-13 和非目标。
3. ARCHITECTURE-SPINE.md 的 AD-7～AD-11、AD-13 以及 Embedded 结构/验证门禁。
4. 本 Story 的 AC/任务；UX 只提供后续页面边界，不改变板级事实。
5. Embedded/AGENTS.md、docs/embedded/** 的编码和 runbook 约束。

架构主干 `AD-13` 仍有旧“充电期间暂停输入/音频”的文字，而 PRD final 明确“主控不检测
充电状态、三种供电条件均可用”。本 Story 必须采用 PRD/epics 的最终口径，并把旧分支
删除或隔离；不要同时实现两种语义。docs/hardware/ 当前状态是
`baseline_not_prototype_verified`，因此任何软件结果都不能替代 ERC、PCB DRC、样机、
示波器或负载测试。

### 板级合同（固件可消费的最小集合）

| 资源 | 固定事实 | 固件边界 |
| --- | --- | --- |
| `PWR_INT` | LTC2954 `INT`，IO8，开漏低有效，10 kΩ 上拉至 V3V3，按住持续低 | 只读输入；边沿捕获并在任务中测时长；不轮询、不输出 |
| `BOOT0` | IO0，启动绑带；下载时低，正常 SPI 启动为高 | 只读输入；应用不得改变复位采样或下载路径 |
| `RESET_N`/EN | 专用复位输入/测试点 | 不作产品按键，不由固件驱动 |
| IO46 | 未分配/悬空启动绑带，内部弱下拉给 0 | 不初始化、不接 PWR_STATE、不作唤醒输入 |
| `PWR_STATE` | LTC2954 EN，经 100 kΩ 上拉至 VSYS，驱动 TPS22965/TLV62569 | 不接 ESP32 GPIO；不可用作软件电源状态 |
| `KILL` | LTC2954 pin 8，100 kΩ 上拉至 V3V3 的无主控端接 | 禁止任何 GPIO/API 驱动 |
| `PVDF_ADC`/`PVDF_CMP_WAKE` | IO9/IO11，分别 ADC1_CH8/比较器唤醒 | Story 1.2 所有；本 Story 不占用 |
| USB-Serial-JTAG | IO19/20 | 保留给 Story 1.3；不接 CH340X/USB-OTG |
| Air780EGP | UART1 TX/RX IO43/44，DTR IO10，RST IO15 | 只保留资源边界；不启用 ML307R 或模组业务 |
| 电源路径 | USB→BQ25895→VSYS→TPS22965→VMAIN→V3V3/4G；电池通过 BQ power-path；USB+电池可边充边运行 | 固件不检测充电、不建立暂停门控；电气时序待样机验证 |

硬件文档还规定 IO1/2 为单一 I2C0、IO45 不分配、IO35–37 为 PSRAM 保留脚。
任何新增或换脚都必须先更新硬件事实源，再改 BSP；不可在本 Story 私自选取未冻结阈值、
ONT/PDT 电容、低功耗参数、NVS schema 或 rail ADC 阈值。

### 当前源码审计结果（实现前必须处理）

交接文档把 Embedded 描述为尚未进入实现，但 git ls-files Embedded/ 显示
eeee145 已提交一整套 legbot 迁移代码。它只是候选/排障线索，不是 EWF 已验证实现。
以下文件是本 Story 的 UPDATE 审计入口，不能再从空目录假设开始：

| 现有路径 | 已观察到的旧行为/风险 | 本 Story 的处理要求 |
| --- | --- | --- |
| Embedded/CMakeLists.txt、cmake/resolve_firmware_profile.cmake | 项目名、版本和编译宏仍为 legbot_watch，默认注入 BLE/GPS/ML307R/cloud/scenic/exoskeleton | 改为 EWF 工程边界；任何保留 profile 必须显式禁用旧业务并有测试 |
| Embedded/sdkconfig.defaults | 含 BLE/NimBLE、语音/旧 Story 注释和 ML307R UART 口径；USB/PM 锚点混杂 | 以 v5.5.4 官方 Kconfig 和 EWF 锚点重建；不得通过旧配置启动 BLE/GPS |
| Embedded/components/BSP/CMakeLists.txt | 纳管 L76KB_A58、ML307R、VIBRATION、QMI8658C 等旧目录 | 从默认 BSP 启动图移除冲突组件；QMI 仅可保留为未来 I2C 预留，INT2 不占 IO45 |
| BSP_INCLUDE/bsp_resources.{h,c} | GPS 占 IO9/8/3、ML307R 占 IO43/44/10、振动占 IO11、PWR 占 IO46、QMI INT2 占 IO45 | 重写为 EWF 资源表；PWR=IO8，PVDF=IO9/11，IO46/45 保留，Air780 命名与所有权明确 |
| BSP/BOARD/bsp_board.{c,h} | 启动时保持 GPS standby、控制 ML307R、初始化振动和旧 scenic 宏 | 只执行 EWF 允许的板级初始化；删除旧 GPS/ML307R/vibration 控制，保留安全失败/降级结果 |
| BSP/KEY/key.{c,h} | 当前注释/宏仍把 GPIO46 当 PWR，并用旧动态电平重武装语义 | 迁移到 IO8 持续低电平边沿/时长契约；BOOT0=IO0；ISR 不做业务决策 |
| main/app_main.c | 启动前调用 GPS/ML307R standby，打印 legbot/BLE/GPS，按旧 profile 启动服务 | 重排 EWF 启动阶段，去掉旧业务 side effect；不要从旧 build/ 产物判断成功 |
| components/services/legbot_services.* | 服务表默认含 BLE/GPS/cloud/voice 等旧能力和 LEGBOT 宏 | 保留通用 typed queue 只有在不改变边界时；旧服务不得默认启动或拥有 EWF 板级资源 |
| components/services/selftest_service/* | canonical 清单含 GPS UART/FIX、ML307R、vibration、旧 UI/audio 项 | 以电源/绑带/复位/BSP 资源自检替换本 Story 清单；结果标注 evidence，不伪造硬件 PASS |
| BSP/L76KB_A58/、BSP/ML307R/、BSP/VIBRATION/、相关 legacy service | 与 EWF IO 合同或产品非目标冲突 | 可删除或保留为未纳管历史代码，由审计、编译和自检结果决定；不得让其进入生产启动图 |

不要顺手重构无关驱动。对能复用的 CO5300/CST9217/CW2015/ES8311/NS4150B 组件，
只调整其板级映射/所有权；显示、音频、触摸、PVDF 和 4G 的完整行为留给对应 Story。

### 架构、编码和 API 约束

- ESP-IDF 固定在项目约束的 >=5.5.4,<5.6.0 档位，本 Story 使用 v5.5.4 文档和
  esp32s3 target；不要因网上最新版本而升级到 5.5.5/6.x。
- 使用 GPIO ISR、FreeRTOS queue/event、reset reason 和 PM/USB 配置前，先核对官方
  v5.5.4 API 签名；不要凭其它 IDF 版本经验调用。
- BSP 是 GPIO、总线、电源使能和初始化顺序的唯一 owner（AD-8）；服务只通过 typed
  event/不可变状态快照交互（AD-7/AD-9），共享 I2C/UART 不得重复初始化（AD-10）。
- 每个使用日志的 .c 定义稳定 ASCII TAG，日志正文中文；ISR-safe 路径不打印、
  不分配、不阻塞。遵守 docs/embedded/style/C编码规范-Agent版.md 的文件头、错误
  处理和命名规则。
- 固件不接管硬件关机：禁止 esp_deep_sleep_start、esp_restart、仅关屏/空循环
  模拟关机，也禁止任何 GPIO 驱动 KILL。低电策略若被相邻代码触及，只能先保证累计
  落盘，不能引入供电/充电暂停分支。
- generated UI/bindings、字体、Pen/HTML 和 cloud 目录不是本 Story 的实现目标；不要
  将诊断说明、Node ID、TODO 或内部过程文字写入产品 UI。

### 自检结果与硬件证据口径

建议沿用现有 self-test typed result，不另造并行 runner；字段/枚举名称若需调整，
必须保持调用方兼容并在结果中保留：

1. resource：如 PWR_INT_IO8、BOOT0_IO0、RESET_N_EN、IO46_RESERVED；
2. observed：原始电平/边沿、初始化返回码、reset reason 或“无软件观测”；
3. evidence：design_input、software_observed、hardware_pending、hardware_verified、
   failed；
4. source/run_id/错误码：能回到硬件文档章节或测试回执。

baseline_not_prototype_verified、record_only_not_created、must_prototype_validate
是当前硬件状态；没有目标板、示波器/电子负载、下载/USB 回执时，不得把 design_input
转换为 hardware_verified。若发现硬件事实与导出网表不一致，应停在 failed/待收敛并
报告差异，不用软件“补偿”错误接线。

### 测试建议与完成判定

最小机械门禁包括：

    cd Embedded
    idf.py set-target esp32s3
    idf.py reconfigure
    idf.py build

上述命令必须在干净或明确清理的构建目录、ESP-IDF v5.5.4 环境中执行并保留输出。
构建通过只证明源码/配置可编译；它不证明 GPIO 电气、PWR 时序、下载采样、三种供电
或音频/显示在实板上通过。真机证据按
docs/embedded/guides/真机闭环runbook-macos.md 和 docs/hardware/ 当前归档规范
回读，至少包含：

- BOOT0 低电保持后长按 PWR 的下载进入证据；
- 运行态 BOOT0 完整短按/释放和长按、抖动、启动电平的事件结果；
- PWR_INT 下降/上升边沿及持续低电平时长，不是单次脉冲假设；
- EN/RESET_N 复位观察和启动日志；
- USB-only、电池-only、USB+电池同时接入的 rail/输入/显示/音频/同步结果；
- 失败项、测量工具、板号/固件版本、时间和原始日志/波形路径。

如果没有样机，只能交付编译/静态/host 测试和明确的 hardware_pending 清单，不能将
Story 标记为“硬件已验证”。

## Project Structure Notes

目标结构仍是 Embedded/components/BSP + platform + services + app_state + main。
本 Story 允许在现有 BSP、platform/event_bus、services/selftest_service 和
main/app_main.c 内做最小迁移；不新增平行 power_bsp、key2、selftest_v2 或
第二份资源表。若 legacy 文件被删除，须同步其 CMake、头文件引用、服务注册和测试；
若暂留，须从默认构建/启动图移除并留下来源、用途、验证状态。

当前 Embedded/build/ 是被忽略的历史生成目录，不是源代码或验证凭据。修改前后用
git status --short 区分本 Story 产出与既有用户变更；不得执行 commit、branch、
stash、reset、checkout、merge 等变更性 Git 操作。

### References

- [Source: _bmad-output/planning-artifacts/epics.md#Epic-1 and #Story-1.1]（Epic 目标、
  FR-E-001/AD-8/AD-13、四条 BDD 验收条件）
- [Source: _bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md#5.1-FR-E-001]
  （三种供电条件、PWR_INT、BOOT0 下载顺序、主控不检测充电）
- [Source: _bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md#5.11-FR-E-011]
  （BOOT0 运行态短按/释放、忽略长按/启动/下载；自动模式属于后续行为）
- [Source: _bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md#9.3-遗留代码清理 and #10-非目标]
  （legacy 清理、BLE/GPS/QMI/充电暂停非目标）
- [Source: _bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md#AD-7–AD-13]
  （事件驱动、BSP 独占、共享资源、USB、旧充电语义和电源边界）
- [Source: _bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md#Stack and #Structural-Seed]
  （ESP-IDF 档位、ESP32-S3、Embedded 目录结构、复用/不沿用清单）
- [Source: docs/hardware/电子木鱼-硬件原理图设计基线.md#2.1-权威-GPIO-总表 and #3.0-最终电源方案]
  （IO0/8/9/11/46、EN、PWR_STATE/KILL、供电路径）
- [Source: docs/hardware/电源网络命名规范.md#3-器件-pin-与板级网络映射]
  （VSYS/VMAIN/V3V3、PWR_INT/PWR_STATE/KILL 的正式网络边界）
- [Source: docs/hardware/电子木鱼-硬件网络清单.json]
  （GPIO machine-readable 合同、状态 legend、HW-OI-003/004/005、样机验证门禁）
- [Source: Embedded/AGENTS.md and docs/embedded/style/C编码规范-Agent版.md]
  （ESP-IDF/API、PWR/BOOT/EN/KILL、日志、ISR、编码和证据规则）
- [Source: docs/embedded/guides/真机闭环runbook-macos.md]
  （真机烧录、监控和回执归档流程）
- [Espressif ESP-IDF v5.5.4 GPIO API](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/gpio.html)
- [Espressif ESP-IDF v5.5.4 power management API](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/power_management.html)
- [Espressif ESP-IDF v5.5.4 sleep modes](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/sleep_modes.html)
- [Espressif ESP-IDF v5.5.4 stdio/USB Serial-JTAG](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/stdio.html)

## Dev Agent Record

### Agent Model Used

Claude Code（bmad-dev-story，2026-09-22 第 4 次：闭环 `Embedded/.gitignore` 治理改动并重跑验证）

### Debug Log References

- 环境（本次已核实，前一版“idf.py 未安装”结论作废）：
  `source /Users/hongchenke/.espressif/tools/activate_idf_7355072.sh` 后
  `command -v idf.py` 返回 `idf.py`，`idf.py --version` 返回 `ESP-IDF v5.5.4`。
  前两次收据把失败一律归因为“idf.py 未安装/退出码 127”，属环境激活误判。
- 历史失败真实根因（attempt-1，见 `.autopilot/1/1-1-.../attempt-1/build-recovery-01.log`）：
  `CMake Error at tools/cmake/build.cmake:330: Failed to resolve component 'esp-sr'
  required by component 'services': unknown name.`，源自
  `Embedded/components/services/CMakeLists.txt` 中对 `esp-sr` 的 REQUIRES。
- 本次编译记录：
  `docs/embedded/build_records/20260922-104210/build-01.log`（失败，exit 1）、
  `docs/embedded/build_records/20260922-104210/build-02.log`（通过，exit 0）。
- attempt 01 最早真实错误：
  `Embedded/components/services/state_service/state_service.c:501:30: error:
  'SELFTEST_ITEM_AMOLED' undeclared (first use in this function)`。
- 产物核验：`build/bootloader/bootloader.bin`、`build/partition_table/partition-table.bin`、
  `build/electronic_wooden_fish.bin`（291488 B < factory 0x480000）、
  `build/electronic_wooden_fish.elf`、`build/spiffs.bin` 全部存在；未执行 flash/monitor。
- 第 4 次（本次）环境与编译记录：同一激活脚本下 `command -v idf.py` 返回 `idf.py`、
  `idf.py --version` 返回 `ESP-IDF v5.5.4`；`docs/embedded/build_records/20260922-104920/`
  的 `environment.txt`（16MB/dio/80m/115200/USB-Serial-JTAG Console）、
  `build-01.log`（`idf.py reconfigure` + `idf.py build`，exit 0，`build-01.log:76`
  `Project build complete.`）、`session.md`。本次日志中无 `error:`、`fatal error:`、
  `undefined reference`、`FAILED:`，无需构建恢复。
- 第 5 次（本次，无实现改动）环境与编译记录：同一激活脚本下 `command -v idf.py`
  返回 `idf.py`、`idf.py --version` 返回 `ESP-IDF v5.5.4`；
  `docs/embedded/build_records/20260922-110310/` 的 `environment.txt`
  （`CONFIG_IDF_TARGET="esp32s3"`、16MB/dio/80m/115200、USB-Serial-JTAG Console、
  `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`）、
  `build-01.log`（`idf.py reconfigure` + `idf.py build`，流水线 exit 0，
  `build-01.log:76` `Project build complete.`，应用镜像 0x472a0 B）、`session.md`。
  本次日志中无 `error:`、`fatal error:`、`undefined reference`、`FAILED:`、
  `ninja: build stopped`，无需构建恢复；未覆盖或截断 `20260922-104210` 与
  `20260922-104920` 两批历史日志。

### Completion Notes List

- **构建恢复**：`services` 组件不再 REQUIRES `esp-sr`；voice/ble/cloud/gps/modem/
  audio/ui_service/time/maintenance/radio_power 旧实现整体移出默认启动图并从磁盘删除，
  其 CMake、头文件引用、服务注册与自检清单同步清理，符合“不得通过‘保留但不编译’掩盖未审计代码”。
- **工程边界**：`Embedded/CMakeLists.txt` 去掉 `LEGBOT_CAP_*`/`SCENIC_AREA_MANAGEMENT_DEBUG`
  编译宏注入；`Embedded/sdkconfig.defaults` 以 M100EG/ESP32-S3 的 EWF 锚点重建
  （显式 `CONFIG_IDF_TARGET="esp32s3"`），重新生成后的 `Embedded/sdkconfig` 中
  `CONFIG_BT_*` 与 `CONFIG_SR_*` 计数均为 0；`Embedded/cmake/resolve_firmware_profile.cmake`
  与 `main/cloud_provision.*` 已删除。
- **电源与启动绑带（AC1/AC2）**：`bsp_resources.{h,c}` 重写为唯一板级入口，
  `EWF_BSP_PWR_INT_GPIO=IO8`（开漏低有效只读输入）、`EWF_BSP_BOOT0_GPIO=IO0`（只读启动绑带）；
  `EN/RESET_N`、`PWR_STATE`、`KILL` 全部为 `GPIO_NUM_NC`，固件无任何驱动路径；
  `IO46/IO45` 为保留脚，`QMI8658A INT2=GPIO_NUM_NC`，PVDF 归 Story 1.2 的 IO9/IO11。
- **事件边界（AC3）**：新增纯逻辑模块
  `services/power_service/power_boot_policy.{h,c}`（无 ESP-IDF 依赖，可主机测试），
  PWR 只按“下降/上升边沿 + 低电平时长”生成一组按压区间，不轮询、不单脉冲；
  BOOT0 在启动/下载采样窗口、长按（≥3000 ms）和抖动（<20 ms）下都不产生运行态自动事件；
  `power_service` 的 ISR 只做时间戳与 `xQueueSendFromISR`，去抖与分类在 `power_task` 完成；
  无 `esp_deep_sleep_start`、`esp_restart`、KILL 驱动或充电暂停分支。
- **自检（AC4）**：`selftest_service` canonical 清单替换为 6 项
  （BSP 资源表、PWR_INT/BOOT0 输入边界、EN/复位软件观察、初始化阶段返回值、
  禁用与保留资源、三种供电与 LTC2954 时序），每项固定输出资源名、观测值、错误码与
  `evidence`（`design_input`/`software_observed`/`hardware_pending`/`hardware_verified`/`failed`）；
  引擎在无板级回执（`SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE == 0`）时把任何
  `hardware_verified` 断言判为 `SELFTEST_REASON_INVALID_RESULT`，
  三种供电/rail/时序项固定为 `hardware_pending`，不输出笼统 PASS。
- **测试**：`Embedded/tests/run_host_tests.py` 一次运行 11 项源码合同用例与 9 项
  PWR/BOOT 边沿语义 C 用例（`cc -Wall -Wextra -Werror` 构建后运行），全部通过；
  无回归（旧 `test_bsp_contract.py` 用例被重写为 EWF 口径并全部保留覆盖范围）。
- **测试入库治理（第 4 次）**：`Embedded/.gitignore` 原第 10 行单独一条 `tests` 规则
  会忽略整个 `Embedded/tests/`，使本 Story AC 1–2 的正式交付物
  （`run_host_tests.py`、`test_bsp_contract.py`、`test_power_boot_policy.c`）无法进入
  变更集与代码审查。经人类裁定“让测试入库”，仅删除该一行；`Embedded/tests/` 三个文件
  现已可被 git 跟踪（`git ls-files -co --exclude-standard` 可见），
  `Embedded/tests/__pycache__/` 仍被同文件 `__pycache__/` 规则忽略，
  `Embedded/tests/` 下无编译产物或临时可执行文件（主机测试二进制走 `tempfile` 临时目录）。
  其余 ignore 行（`build/*`、`managed_components`、`__pycache__/` 等）一律未改动。
- **保留但未纳管的旧资产（含迁移理由）**：`Embedded/components/ui/` 与
  `Embedded/lvgl-design/` 是 Story 3.1 的 UI 资产，不在 Story 1.1 构建图与启动图内，
  仍引用旧的 `LEGBOT_CAP_*`/`SCENIC_AREA_MANAGEMENT_DEBUG` 宏，必须由 Story 3.1 重建后再入图；
  `main/idf_component.yml` 保留 lvgl/esp_lvgl_port/CO5300/触摸/codec 依赖，
  供 Story 3.1 与 BSP 显示/音频驱动复用。`components/control_gate` 保留为纯 reducer
  （无任务、无 GPIO、无链路），由 `contract_checks` 固定其契约常量。
- **边界**：未实现 PVDF ADC 阈值（Story 1.2）、USB-Serial-JTAG 诊断（Story 1.3）、
  自动模式 3 秒定时器（Story 2.6）、LVGL 页面/字体、音频业务、4G HTTPS 或云同步。
- **未验证（必须保留）**：无样机，未执行烧录/监控；rail 电压、LTC2954 时序、
  USB/下载采样与三种供电矩阵均为 `hardware_pending`，编译通过不代表样机或电气验收通过。

### File List

新增：

- Embedded/components/services/power_service/power_boot_policy.h
- Embedded/components/services/power_service/power_boot_policy.c
- Embedded/tests/test_power_boot_policy.c
- Embedded/tests/run_host_tests.py
- docs/embedded/build_records/20260922-104210/environment.txt
- docs/embedded/build_records/20260922-104210/build-01.log
- docs/embedded/build_records/20260922-104210/build-02.log
- docs/embedded/build_records/20260922-104210/session.md
- docs/embedded/build_records/20260922-104920/environment.txt
- docs/embedded/build_records/20260922-104920/build-01.log
- docs/embedded/build_records/20260922-104920/session.md

修改：

- Embedded/.gitignore
- Embedded/CMakeLists.txt
- Embedded/sdkconfig
- Embedded/sdkconfig.defaults
- Embedded/dependencies.lock
- Embedded/main/CMakeLists.txt
- Embedded/main/Kconfig.projbuild
- Embedded/main/app_main.c
- Embedded/main/idf_component.yml
- Embedded/components/BSP/CMakeLists.txt
- Embedded/components/BSP/BSP_INCLUDE/bsp_resources.h
- Embedded/components/BSP/BSP_INCLUDE/bsp_resources.c
- Embedded/components/BSP/BSP_INCLUDE/bsp_board.h
- Embedded/components/BSP/BSP_INCLUDE/bsp_include.h
- Embedded/components/BSP/BOARD/bsp_board.c
- Embedded/components/BSP/KEY/key.h
- Embedded/components/BSP/KEY/key.c
- Embedded/components/platform/CMakeLists.txt
- Embedded/components/app_state/app_state.h
- Embedded/components/app_state/watch_state.c
- Embedded/components/contract_checks/CMakeLists.txt
- Embedded/components/contract_checks/contract_checks.c
- Embedded/components/services/CMakeLists.txt
- Embedded/components/services/legbot_services.h
- Embedded/components/services/legbot_services.c
- Embedded/components/services/power_service/power_service.h
- Embedded/components/services/power_service/power_service.c
- Embedded/components/services/selftest_service/selftest_service.h
- Embedded/components/services/selftest_service/selftest_service.c
- Embedded/components/services/state_service/state_service.c
- Embedded/tests/test_bsp_contract.py
- _bmad-output/implementation-artifacts/sprint-status.yaml

删除：

- Embedded/cmake/resolve_firmware_profile.cmake
- Embedded/sdkconfig.old
- Embedded/main/cloud_provision.c
- Embedded/main/cloud_provision.h
- Embedded/components/platform/at_core/at_core.c
- Embedded/components/platform/at_core/at_core.h
- Embedded/components/BSP/L76KB_A58/l76kb_a58_bsp.c
- Embedded/components/BSP/L76KB_A58/l76kb_a58_bsp.h
- Embedded/components/BSP/L76KB_A58/l76kb_a58_power_hold.c
- Embedded/components/BSP/ML307R/ml307r_bsp.c
- Embedded/components/BSP/ML307R/ml307r_bsp.h
- Embedded/components/BSP/VIBRATION/vibration_bsp.c
- Embedded/components/BSP/VIBRATION/vibration_bsp.h
- Embedded/components/services/audio_service/audio_service.c
- Embedded/components/services/audio_service/audio_service.h
- Embedded/components/services/ble_service/（ble_control_packet / ble_recovery_policy / ble_service / ble_status_frame / ble_unlock 的 .c/.h）
- Embedded/components/services/cloud_service/（cloud_payment_schedule / cloud_service 的 .c/.h）
- Embedded/components/services/gps_service/（gps_acquisition_policy / gps_service 的 .c/.h）
- Embedded/components/services/log_service/log_service.h
- Embedded/components/services/maintenance_service/（.c/.h）
- Embedded/components/services/modem_service/（ml307r_https_transport / modem_recovery_policy / modem_service 的 .c/.h 与 modem_selftest_policy.h）
- Embedded/components/services/time_service/（.c/.h）
- Embedded/components/services/ui_service/（ui_display_flush_guard / ui_payment_correlation / ui_performance_metrics / ui_service 的 .c/.h）
- Embedded/components/services/voice_service/（voice_control_policy / voice_service 的 .c/.h）
- Embedded/components/services/radio_power_arbiter.c
- Embedded/components/services/radio_power_arbiter.h
- Embedded/components/services/power_service/power_interaction_policy.c
- Embedded/components/services/power_service/power_interaction_policy.h
- Embedded/components/services/power_service/wrist_raise_policy.c
- Embedded/components/services/power_service/wrist_raise_policy.h

说明：`Embedded/build/`、`Embedded/managed_components/`、`.DS_Store` 与
`lvgl-design/squareline_studio/{backup,cache,project.info}` 为被忽略的生成物/本地缓存，
不属于本 Story 产出。`Embedded/tests/` 下的三个测试文件为本 Story 正式交付物，
其 ignore 规则已在第 4 次移除，现已可被 git 跟踪；`Embedded/tests/__pycache__/`
仍按 `__pycache__/` 规则忽略。

变更集口径（第 5 次收敛）：本 Story 的权威交付集合等于外层 story-local diff 的
changed paths，共 98 条 —— 上列“新增 / 修改 / 删除”三节中，除
`docs/embedded/build_records/**`（构建证据，受根 `.gitignore` 第 49 行
`docs/embedded/build_records` 约束，不进入变更集，改由 autopilot receipt 的
`build_log_paths` 引用）与 `sprint-status.yaml`（流程状态文件，被外层 diff 显式排除）
之外的全部路径。需注意本区段“删除”一节为便于阅读按目录做了合并书写，权威逐条清单
以 story-local diff 为准；集合已用 `git check-ignore` 逐条校验，无被忽略路径混入。

### Change Log

- 2026-09-22：第 4 次。按人类裁定“让测试入库”，删除 `Embedded/.gitignore` 中单独一条
  `tests` 规则（原第 10 行），使 `Embedded/tests/run_host_tests.py`、
  `test_bsp_contract.py`、`test_power_boot_policy.c` 这三个 AC 1–2 正式证据文件可进入
  变更集并接受代码审查；未改动其他 ignore 行。主机测试重跑通过（11 项源码合同 +
  9 项 PWR/BOOT 边沿语义，共 20 项，exit 0）；`idf.py reconfigure` + `idf.py build`
  在 ESP-IDF v5.5.4 / esp32s3 下重跑通过（exit 0），五项目标产物齐全，
  新编译记录 `docs/embedded/build_records/20260922-104920/`。
- 2026-09-22：第 3 次补齐重跑。修复真实构建根因（`services` REQUIRES 未安装的 `esp-sr`），
  把 `services` 收敛为 EWF 启动图（power/state/selftest），删除 GPS/ML307R/BLE/cloud/voice/
  audio/ui_service/time/maintenance/radio_power/at_core 旧链路与其 CMake、引用和自检清单；
  重写 BSP 资源表与 PWR_INT/BOOT0 输入边界服务；新增可主机测试的 `power_boot_policy`；
  自检加入 `evidence` 类别与无回执禁止 `hardware_verified` 的校验；
  `idf.py reconfigure` + `idf.py build` 在 ESP-IDF v5.5.4 / esp32s3 下通过（exit 0），
  五项目标产物齐全；主机合同与边沿语义测试 20 项全部通过。
