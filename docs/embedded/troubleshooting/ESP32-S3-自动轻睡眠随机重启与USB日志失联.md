<!-- 来源: legbot_watch · 源文件: docs/troubleshooting/ESP32-S3自动轻睡眠随机重启与USB日志失联根因与解决方案.md · legbot HEAD: 41a5ab8b9 · 迁移: 2026-09-08 · 判定: 搬(去 LTE 专属段) · EWF 适配点: EWF 用 Air780EGP(外部UART模组) 而非板载 LTE 切频，DFS 相关可略 -->

# ESP32-S3 自动轻睡眠随机重启与 USB 日志失联

## 0. 迁移说明与 EWF 适用性

本文迁移自 `legbot_watch` 在 **ESP32-S3 + ESP-IDF v5.5.4** 组合上定位并修复的自动 light sleep 重启与 USB 日志失联故障，是一份「结论已成立、按 EWF 命名与路径改写」的设备侧排障文档。

- **EWF 状态**：设备侧（`Embedded/` ESP-IDF 工程）尚未落地，本故障未在 EWF 真机复现。本文作为 EWF 骨架落地、低功耗与联网验收时的前置参考与硬规则依据，不等同于 EWF 已完成同项验收。
- **路径改写**：`components/` → `Embedded/components/`（EWF 固件工程根为 `Embedded/`）；构建产物在 `Embedded/build/`；真机取证归档建议在 `docs/embedded/hardware_test_records/`。
- **不适用段**：原文档「LTE DFS 切频」段的根因与修复依赖**板载 LTE 射频在 CPU 降频期间的切频竞态**。EWF 唯一产品联网链路是 Air780EGP 4G（**外部 UART AT 模组**，IO43/44），无板载 LTE 射频，因此该段**不收录**，仅保留文末一句说明。其余与输入/UI/音频/同步无关的、ESP-IDF 版本级的根因与对策**原样保留**。
- **硬规则**：**USB 连接时禁用自动 light sleep，防调试串口失联**。EWF 架构 **AD-11**（USB-Serial-JTAG 为唯一调试通道）已写入该规则；本文给出其根因（原生 USB-Serial-JTAG 在自动 light sleep 下失联）与配置入口 `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`。

---

## 1. 结论与当前状态

设备可能因多种独立 PM 重启故障而表现出外观相同的「黑屏重启」。**不得因外观同为「黑屏重启」就混用根因。** 本文记录与 EWF 直接相关的两类：

- 主缺陷：自动 light sleep 的 FreeRTOS tick 补偿断言（`vTaskStepTick()`）；
- 次要边界：任务栈置于外部 PSRAM 时直接调用 NVS/Flash API（Flash 写期间 cache 禁用）。

（原 legbot 文档还有第三类「LTE 射频未确认关闭时的 DFS/切频崩溃」，因 EWF 无线链路为外部 Air780EGP UART AT、无板载 LTE 射频而**不适用**，见文末说明。）

### 1.1 自动 light sleep tick 补偿断言（EWF 直接适用）

EWF 尚未真机复现；下列定位与修复结论迁移自 legbot 在**相同 ESP32-S3 + ESP-IDF v5.5.4** 组合上的真机排障。目标板随机重启已定位到 ESP-IDF `esp_pm` 自动 light sleep 的 FreeRTOS tick 补偿路径，而不是 UI、输入、音频、同步任务、任务栈溢出、看门狗或欠压。

Core Dump 给出的直接故障为：

```text
assert failed: vTaskStepTick tasks.c:3068
(( xTickCount + xTicksToJump ) <= xNextTaskUnblockTime)
```

崩溃任务和调用链为：

```text
IDLE0
  -> prvIdleTask()
  -> vApplicationSleep()
  -> vTaskStepTick(xTicksToJump=4)
  -> __assert_func()
  -> esp_system_abort()
  -> panic_abort()
```

修复通过**项目本地 `esp_pm` 组件覆盖**，在固定使用 ESP-IDF v5.5.x 的前提下回移 Espressif 后续的两道官方保护：

1. light sleep 请求被拒绝时不再错误补偿 RTOS tick；
2. 实际休眠 tick 只比预计空闲时间多 1～2 tick 时，将补偿量限制到预计值，避免误触发 `vTaskStepTick()` 断言。

legbot 原现场完成全量构建、80 项 services 回归、全分区烧录与逐段哈希校验，随后用户完成 5 轮「点亮屏幕 → 触发交互 → 等待自动熄屏」真机复测，肉眼确认无重启；日志中会话全部正常收尾，业务控制结果到达 `APPLIED`，未再出现 assert、panic、Guru Meditation 或 WDT。因此该问题在**原项目**的当前状态为：

> **暂定修复完成。** 若以后再次出现重启，先按第 8 节恢复取证，不得直接怀疑或修改 UI、输入或音频代码。

> **EWF 提示**：若 EWF 固定到含该缺陷的 IDF 版本（见 §6），应沿用同一本地 `esp_pm` 覆盖，并复跑 §7 验收矩阵；遇不同调用栈必须按新根因处理，不得因为表现仍是「黑屏重启」就复用本文结论。

### 1.2 另一类重启：外部 PSRAM 栈上调用 NVS/Flash（cache 禁用边界）

legbot 现场另一起旧重启（对应 ELF SHA256 以 `73ecfc752` 开头）触发了 `esp_task_stack_is_sane_cache_disabled()` 类检查：当时某任务栈位于 PSRAM，任务又直接调用 NVS/Flash API；**Flash 写入期间 cache 禁用，正在外部 RAM 栈上运行不符合该边界**。修复后，初始化由内部栈把数据加载到 RAM 缓存、运行期只读缓存；运行期持久化改为 typed update，由内部栈任务串行执行 NVS 写入并返回 ACK。两条时间相关路径都遵守该边界。

这与 §1.1 的自动 light sleep 断言**不是同一故障**。EWF 使用 8MB PSRAM 型号（ESP32-S3-N16R8），若任一任务把栈放在 PSRAM 又直接调用 NVS/Flash API，须遵守同一边界：**NVS/Flash 写入应安排在内部 RAM 栈上执行**。

## 2. 故障现象

现场先后出现以下表现：

- 正常使用或没有操作时，屏幕突然变黑，随后系统重新启动。
- 在一次交互（敲击/触摸推进）结束、屏幕自动熄屏附近出现重启。
- 重启后若此前存在未同步差量或待应用命令，会进入「重新连接/待同步」页；这是启动后的正常状态投影，不是新的 UI 故障。
- 串口监控在故障发生后停止更新或报告 USB 设备句柄失效，软件侧任务日志也不再推进。
- USB 接入、串口是否打开会改变问题出现概率，但不能据此判定问题已经消失。

**USB 行为容易形成误导。** 项目启用了 `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`，USB Serial/JTAG 主机连接会**禁用自动 light sleep**；持续日志还会改变任务调度和下一个唤醒期限。因此「接 USB 后不容易重启」既可能是时序扰动，也可能是自动 light sleep 被 USB 连接关掉——两种情况都不能说明 USB 或日志「修复」了故障。另一方面，芯片 panic 后原生 USB 会重新枚举，旧监控进程可能继续持有已经失效的设备句柄，所以**监控卡住也不等于所有任务在同一时刻死锁**。

## 3. 可复现路径

本轮最接近故障的受控路径（EWF 骨架落地后按此逼近，实体/屏幕输入均进入同一有效敲击队列）：

1. 保持设备已进入正常待机（离线或在线均可，本故障与网络是否在线无直接关系）。
2. 点亮屏幕（触摸或 PWR 唤醒）。
3. 执行一段连续有效敲击（实体 PVDF 或屏幕 `device_touch`），让经文推进、音频与显示进入活跃状态；若处于活动窗口，可顺带触发一次 4G 同步/设置操作。
4. 停止操作，等待 5/15/30 秒自动熄屏，让系统进入 tickless idle / 自动 light sleep。
5. 重复执行，观察是否在熄屏附近黑屏后重新出现启动页或「待同步」页。

问题**不是每次必现**。复现时必须同时记录：屏幕现象、串口最后时间戳、USB 是否重新枚举、下一次启动的复位原因，以及 Flash Core Dump。

## 4. 证据链

EWF 尚未复现，本节描述的是**应复用的取证方法**与 legbot 原现场已经据此排除掉的干扰项；下列原始证据目录与指纹均为 **legbot 固件产物**（原目录 `docs/hardware_test_records/20260805-164214/` 被 `.gitignore` 忽略，仅作真机现场归档，可用 provenance 中的源文件/HEAD 回溯），仅作方法参照，不构成 EWF 的验收证据。

EWF 若再次遇到同类重启，应在 `docs/embedded/hardware_test_records/` 建立同等归档，至少记录：串口最后时间戳日志、重启后的启动尾部、下一次启动的复位原因、以及 512 KiB 原始 Flash Core Dump；不得直接引用本文中的旧日志与旧指纹。

取证结论要点（legbot 已用崩溃固件匹配的 ELF 验证）：

- NVS 启动复位账本记录到 `ESP_RST_PANIC`（枚举值 4）；启动时 Core Dump 校验通过，识别出 29664 字节有效崩溃数据。
- 用与崩溃固件匹配的 ELF 解码后，崩溃任务是 `IDLE0`，而不是 UI、输入、音频或同步任务。
- 各业务任务仍有充足栈余量（原现场 `ui_task` 等 free stack 均在 8 KB 以上），可排除任务栈溢出。
- 结合复位枚举、断言文本与完整调用栈，可排除：brownout 或电源毛刺、task/int watchdog、业务任务栈溢出、应用主动调用 `esp_restart()` 等。

## 5. 根本原因

ESP-IDF v5.5.4 的 `components/esp_pm/pm_impl.c` 在 `vApplicationSleep()` 中按 `esp_timer` 计算休眠经过时间，然后调用：

```c
vTaskStepTick(slept_ticks);
```

该版本缺少两项后来由 Espressif 补充的保护：

1. `esp_light_sleep_start()` 返回错误、实际没有进入预期休眠时，旧实现仍可能根据调用耗时执行 tick 补偿；
2. 真正进入 light sleep 后，cache miss、CPU 频率切换或 Flash 延迟可能让实际唤醒开销略超预计空闲时间，旧实现没有对小范围 oversleep 做有界处理。

当补偿量越过 FreeRTOS 已知的下一个任务唤醒点时，`vTaskStepTick()` 会主动断言。Core Dump 捕获到的补偿量为 **4 tick**，正是在该路径触发。

仅凭崩溃后的栈帧无法恢复 `esp_light_sleep_start()` 当时的返回值，因此不能严谨区分这一次属于「休眠请求被拒绝」还是「实际休眠轻微超时」。两种上游缺陷进入相同断言路径，因此应**同时回移两项官方保护**，避免只修一个分支后留下同类重启。

Espressif 官方依据：

- [跳过被拒绝 light sleep 的 `vTaskStepTick()` 补偿](https://github.com/espressif/esp-idf/commit/47651df5677f4397a90e95db43cf36dfff2f2fbe)
- [增加 light sleep tick overflow 保护与容差配置](https://github.com/espressif/esp-idf/commit/cf94981732ff02d638b383ae5b2a3542d89bd0c6)
- [ESP-IDF v5.5.4 ESP32-S3 Power Management](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/power_management.html)
- [ESP-IDF v5.5.4 ESP32-S3 Core Dump](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/core_dump.html)

## 6. 永久修复

不修改机器上的全局 ESP-IDF，也不关闭自动 light sleep。落地方式为**项目本地同名组件覆盖**，EWF 对应文件位置：

- `Embedded/components/esp_pm/CMakeLists.txt`
- `Embedded/components/esp_pm/Kconfig`
- `Embedded/components/esp_pm/sdkconfig.rename`
- `Embedded/components/esp_pm/generate_v554_pm_impl.py`（按实际固定版本命名）
- `Embedded/components/services/test/test_pm_tick_overflow_backport.py`

构建过程执行以下约束：

1. 使用项目本地同名组件覆盖 ESP-IDF 的 `esp_pm`。
2. 继续复用所固定版本原始的 `pm_locks.c`、`pm_trace.c`、头文件、Kconfig 和 linker fragment。
3. 只在构建目录生成修复后的 `pm_impl.c`，不编辑 `$IDF_PATH`。
4. 先校验原始 `pm_impl.c` 的 SHA256 必须与构建基线一致（legbot 在 v5.5.4 的基线为 `235a2c89abb161ba48f453e5c196860be099d2e015b9c1e3a7a59fdf1d3f8d2c`）；版本或源码不匹配时直接停止构建，禁止把文本补丁错用到其他 IDF 版本。
5. light sleep 请求失败时不补 tick。
6. `slept_ticks` 超出 `xExpectedIdleTime` 不超过 2 tick 时，将补偿量限制为 `xExpectedIdleTime`。
7. 超过 2 tick 的严重异常仍输出错误并保留原断言行为，不隐藏真实的新故障。

2 tick 容差与上游提交中的默认值一致。代价是极少数轻微超时场景可能让 FreeRTOS tick 相对 `esp_timer` 少 1～2 tick；应接受这一微小误差，以避免已在真机发生的误断言重启。业务绝对时间仍应使用 `esp_timer` 或项目时间服务，不依赖 RTOS tick 作为长期墙上时钟。

> **EWF 版本提示**：EWF 架构约束为 ESP-IDF `≥5.5.4,<5.6.0`。若固定到已包含上述两道保护的较新 5.5.x，需先核对 `pm_impl.c` 是否已含保护、再决定是否仍需要本地覆盖；覆盖层一律以第 4 条哈希门禁为准，禁止凭版本号猜。

## 7. 验证结果与固定复测方案

### 7.1 自动验证

在 `Embedded/` 工程目录下执行：

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  components.services.test.test_pm_tick_overflow_backport -v

PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s components/services/test -p 'test_*.py' -v

source <EWF 所固定 ESP-IDF v5.5.x 的 export.sh>   # 激活对应 IDF 环境
idf.py reconfigure
idf.py build
idf.py size
```

下列数值为 **legbot 原项目基线，仅作方法与量级参照**；EWF 落地后以自身 services 回归与固件指纹为准，不得照抄为 EWF 结果。

| 验证项 | 原 legbot 结果（参照） |
| --- | --- |
| PM 定向回归 | 2/2 通过 |
| 取证固件 services 全量回归 | 80/80 通过（含临时取证合同） |
| 清理后生产固件 services 全量回归 | 78/78 通过 |
| ESP-IDF v5.5.4 完整构建 | 通过 |
| 全分区烧录与镜像哈希校验 | 通过 |
| 真机复测固件 ELF / BIN 指纹 | 已记录于原项目（ELF SHA256 见原现场归档） |

### 7.2 真机复测

修复后先执行 UI 连续点击/滑动、三主页与设置切换、实体敲击与屏幕 `device_touch` 混合输入、以及一次 4G 立即同步回显，再执行多轮「点亮 → 敲击推进 → 自动熄屏」循环。EWF 骨架落地后按第 3 节固定路径完成至少 5 轮「点亮 → 有效敲击 → 自动熄屏」，确认无重启、日志正常收尾、业务控制到达预期终态。

若监控出现 `Device not configured` 而肉眼确认无重启、设备也没有像 panic 复位那样重新枚举，则该事件**不能**计为本问题复现——它可能是拔线、物理断电或独立的 USB 链路事件。若在确认线缆持续连接时再次出现相同现象，应把板级供电与 USB 链路作为独立问题取证，不要自动归因于本文的 `vTaskStepTick()` 缺陷。

重新刷机前应再次核对固件指纹；任何重新构建都会改变 ELF/BIN 指纹，Core Dump 解码必须使用与崩溃固件同一次构建的 ELF。

以后修改低功耗、显示熄屏、输入唤醒（PVDF/触摸）、音频、4G 活动窗口、同步任务或 PM lock 后，至少执行：

1. 连续 10 轮「点亮 → 有效敲击/触摸交互 → 等待自动熄屏」，其中至少 5 轮包含经文推进与音频。
2. 每轮等待输入回声、持久化完成（如遇同步窗口则含 4G 上报 ACK）与屏幕自动熄灭完整结束。
3. 交叉执行 50 次屏幕电子木鱼点击 / 滑动与设置切换。
4. 执行 20 次亮屏/自动熄屏循环。
5. 保持熄屏空闲至少 30 分钟，覆盖大量 tickless idle/light sleep 周期。
6. 拔掉 USB 后用电池运行至少 2 小时；USB 会改变 light sleep 行为，不能只验收有线场景。
7. 重新接入 USB 后先读取复位原因，再检查是否存在新的 Core Dump。

验收失败条件：出现任何 `ESP_RST_PANIC`、同一断言、新 Core Dump、Guru Meditation、WDT、brownout、黑屏后重新启动，或业务任务停止推进。任何一项出现都必须重新打开问题，不得仅凭「之后又能使用」标记通过。

## 8. 再次发生时的最快定位流程

### 8.1 先保护匹配的 ELF

Core Dump 必须使用发生崩溃时的**同一份 ELF** 解码（ELF 名 = `Embedded/` 工程的实际 CMake 项目名，骨架未冻结前按实际替换）。重新构建或刷机前先复制 ELF 并记录 SHA256：

```bash
cd Embedded
shasum -a 256 build/<CMake 项目名>.elf
```

### 8.2 临时恢复 Flash Core Dump

如果生产配置已移除取证项，可临时在 `partitions.csv` 尾部加入（16 MB Flash，末 512 KiB）：

```csv
coredump,data,coredump,0xF80000,0x80000,,
```

并在 `sdkconfig.defaults` 临时加入：

```text
CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y
CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=y
CONFIG_ESP_COREDUMP_CHECKSUM_SHA256=y
CONFIG_ESP_COREDUMP_CHECK_BOOT=y
CONFIG_ESP_COREDUMP_LOGS=y
CONFIG_ESP_COREDUMP_MAX_TASKS_NUM=32
CONFIG_ESP_COREDUMP_FLASH_NO_OVERWRITE=y
CONFIG_ESP_COREDUMP_STACK_SIZE=1792
```

`NO_OVERWRITE` 只保留第一次 panic。提取完成前不要擦除该分区，也不要用新 ELF 覆盖匹配的旧 ELF。

### 8.3 提取和解码

```bash
python -m esptool --chip esp32s3 --port /dev/cu.usbmodemXXXX \
  read_flash 0xF80000 0x80000 coredump.bin

python -m esp_coredump --chip esp32s3 info_corefile \
  --core coredump.bin --core-format raw build/<CMake 项目名>.elf
```

若 Core Dump 显示同一 `vTaskStepTick()` 调用链：

1. 确认构建日志出现「已启用 ESP-IDF v5.5.x light sleep 修复回移」；
2. 检查 `Embedded/build/compile_commands.json` 中 `esp_pm` 编译输入是否为本地覆盖生成的 `pm_impl.c`（构建目录内），而非 `$IDF_PATH` 原件；
3. 运行 `test_pm_tick_overflow_backport.py`；
4. 核对 `$IDF_PATH` 是否仍是所固定的 v5.5.x，原始 `pm_impl.c` SHA 是否匹配；
5. 检查是否有其他组件绕过项目构建系统或重新覆盖 `esp_pm`。

若调用栈不同，按新栈重新定位，不要因为表现仍是「黑屏重启」就复用本文结论。

## 9. 临时诊断清理说明

根因确认后，一次性取证设施应从生产源码和默认配置移除（legbot 原现场移除项：NVS 最近八次复位账本、启动账本调用、512 KiB Flash Core Dump 默认分区与配置、对应的临时诊断合同测试）。EWF 若按第 8 节临时恢复了取证设施，取证完成、根因确认后也应做同等清理。

**永久 `esp_pm` 修复、版本哈希门禁和 PM 回归测试不会删除。** 这样既恢复原有分区与运行时配置，又保证根因修复不会在后续重构中静默丢失。

---

## 10. 对 EWF 的硬规则与不适用说明

1. **硬规则（EWF 直接适用）**：**USB 连接时禁用自动 light sleep，防调试串口失联**。原生 USB-Serial-JTAG（GPIO19/20）作为 EWF 唯一烧录/日志/JTAG 通道（架构 AD-11），自动 light sleep 会使调试串口在主机连接期间失联；对应配置入口为 `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`。这不仅保护调试链路，也会改变（通常降低）自动 light sleep 相关崩溃的复现概率——因此验收必须覆盖「拔掉 USB 用电池运行」的场景，不能只验收有线场景。

2. **不适用段说明**：EWF 无线为 **Air780EGP（外部 UART AT 模组，IO43/44）**，无板载 LTE 射频；原文档「LTE DFS 切频（CFUN=0/4/1 转换期 × CPU 降频竞态）」段与 EWF 无关，本文不收录。Air780EGP 相关的 AT/HTTPS/退避/PM lock 经验由 EWF BSP 另按架构 AD-9/AD-14 与 BSP 文档落位。
