---
title: 根文档 architecture.md 删除前内容保真对账
type: reconcile-review
created: 2026-09-08
scope: architecture.md → ARCHITECTURE-SPINE.md + 09-08 addendum.md
result: 有条件安全（少量 arch-only 事实建议回填后删除）
---

# 根文档 architecture.md 删除前内容保真对账

## 1. 范围与方法

对账对象：

- **旧根文档**（将删除）：`/Users/hongchenke/Documents/Github/Electronic_Wooden_Fish/architecture.md`
- **新 spine**（build-substrate 契约）：`/Users/hongchenke/Documents/Github/Electronic_Wooden_Fish/_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`
- **09-08 附录**（更新、更高权威，引脚表以此为准）：`/Users/hongchenke/Documents/Github/Electronic_Wooden_Fish/_bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/addendum.md`

权威层次假设：附录/spine 与旧根文档冲突处以附录/spine 为准（附录 §1 已声明旧 PRD/根文档为历史输入）。因此「被 09-08 附录或 spine AD 明确推翻的旧决策」**不计遗漏**，仅注记；只有**未来 builder 需要、且只存在于 architecture.md**（即 spine 与附录都未承载）的事实才算缺口。

核对粒度：逐节（GPIO/启动绑带、USB/JTAG、电源分轨与低电、I²C 地址与总线、音频/屏幕行为、4G 活动窗口、同步数据模型与字段、cloud 接口方向、软件基线、验证门禁、明确不做）。GPIO 表逐行比对。

## 2. 逐节核对结果

| architecture.md 节 | 事实承载落点 | 判定 |
| --- | --- | --- |
| §1 文档定位（个人原型/单设备/单 backend 边界） | spine 前言 + Deferred「多设备/换机/迁移…」行 | ✅ 已承载 |
| §2 分层边界 / 跨层原则 | spine Design Paradigm + AD-1/AD-2 | ✅ 已承载 |
| §3.1 主控与模块 | spine Stack + AD-8/AD-11/AD-14 + 附录 §3 | ✅ 已承载（含 ESP32-S3-N16R8、Air780EGP 完整模组、CO5300 410×502、QMI8658C 上板不启用、ES8311/NS4150B） |
| §3.1 **结构**：单一 3D 打印拼接喷涂外壳；只为最终外壳做一次传感器/声学标定；**不做多外壳运行时适配** | —（spine/附录均无） | ⚠️ **arch-only 缺口 G1** |
| §3.2 引脚分配 | spine 板级合同 GPIO 表 + 附录 §3.1 | ✅ 表本身一致（§3 逐行核对）；个别「理由/附加说明」arch-only → G4 |
| §4.1 USB-Serial-JTAG 结论 | spine AD-11 + 工程门禁「芯片/启动」 | ✅ 已承载 |
| §4.1 首次烧录流程（按 BOOT+RESET，后续自动下载） | spine 门禁「BOOT+RESET 可进下载」 | ✅ 门禁级承载（细节属通用 ESP-IDF 知识，不需契约） |
| §4.1 Espressif 官方文档链接 | — | ⚠️ 资源链接未随迁（低，见 G5） |
| §4.2 启动绑带（GPIO0/45/46 采样、下载需 0=0/46=0、PWR/QMI 不得推错电平） | spine GPIO 表注记 + AD-11 | ✅ 已承载 |
| §4.2 **GPIO45 → VDD_SPI 电压** 之原因 | —（spine 只有「未接或按模组要求处理」结果，无数因） | ⚠️ arch-only（低，并入 G4） |
| §5.1 电源分轨（Air780EGP 电池轨持续>1A/瞬时>2A、低压 3.3V 轨、音频独立受控轨、AMS1117 不得主供 4G、充电暂停、VBUS 反向保护） | spine AD-13 + GPIO 表电源注记 | ✅ 已承载 |
| §5.1 **NS4150B 音频轨电压 3–5V 按扬声器阻抗/目标音量决定** | —（spine/附录无电压带） | ⚠️ arch-only（低，G3 并入 Deferred 电源细分） |
| §5.1 **AMS1117 若保留只能低电流外设、优先低静态电流稳压器** | spine AD-13 只说「不得承担 4G 主供电」 | ⚠️ 半承载（低，并入 G3） |
| §5.2 低电量：不主动断网、优先累计落盘、RGB/AMOLED/小程序提示 | spine AD-13（不主动断网+先落盘）；RGB 低电语义见附录 §4.3；电量显示见附录 §5.1/§6 | ✅ 核心决策承载；三端提示散落承载 |
| §6.1 I²C 器件地址（CW2015≈0x62、QMI8658C 0x6A/6B、CST9217 7bit 0x5A、ES8311 待确认） | spine I²C 行 | ✅ 已承载 |
| §6.1 **只保留一组总线主上拉、I/O 电压兼容、上拉阻值与总线电容满足速率** | spine「共用一组总线与上拉」+ Deferred「I²C 上拉阻值」 | ⚠️ 半承载（低，G6） |
| §6.1 **ES8311 地址由 CE/CDATA 配置** | —（spine 只列 ES8311 未给机制） | ⚠️ arch-only（低，并入 G6） |
| §6.2 音频行为（失败不阻塞、高速合并） | spine AD-12 | ✅ 已承载 |
| §6.3 屏幕/触摸（QSPI；共享 I²C+独立 RST/INT） | spine AD-8/AD-10 + 附录 §3 | ✅ 已承载 |
| §6.3 屏幕产品行为（触摸支持音量/亮度/熄屏/立即同步/从头开始退出；两端同时操作后写者生效；完成两端礼花弹窗） | 附录 §5.4/§5.5 + spine AD-5/AD-17 | ✅ 已承载（附录更高权威细化） |
| §6.3 **小程序关闭时离线敲击短暂显示本地当前字/累计/离线状态** | 附录 §5.1 状态栏「同步/待同步」+ §4.2 熄屏首触语义近似但**无完全等价句** | ⚠️ arch-only（需产品确认是否已被附录取代，G2） |
| §7.1 4G 活动窗口链路与语义 | spine AD-14 + AD-3 + 序列图；附录 §7 | ✅ 已承载 |
| §7.1 弱网/backend 不可用本地续记、窗口补传、不可直接唤醒、命令持久化 | spine AD-3/AD-5 | ✅ 已承载 |
| §7.2 高水位模型（local_total/acked_total）、1000 上限、字段清单、经文版本停推、无绝对时间、离线条目归当天 | spine AD-3/AD-4/AD-6 + §Structural Seed 字段清单 | ✅ 已承载 |
| §7.2 AMOLED 内置经文与字形，不引通用字体库/不动态下载 | spine AD-4 | ✅ 已承载 |
| §7.3 cloud 接口方向与幂等/推送/查询/命令语义 | spine AD-2/3/5/16/17/18 + 附录 §6/§7 | ✅ 已承载（注：backend→frontend 通道从「既定 WebSocket」改为「WebSocket/轮询待定 = spine A-1」，为 spine 主动开放项，非内容丢失） |
| §8.1 Embedded 基线（ESP-IDF、Air780 AT 无独立云、PVDF 只存事件元数据） | spine AD-7/AD-14 + Stack | ⚠️ 「PVDF 只保存事件元数据、不保存原始波形」arch-only → G1'（并入 G1） |
| §8.2 frontend 基线 | spine AD-18 | ✅ 已承载 |
| §8.3 backend 基线（miaowu Spring Boot，**SQLite 路径**） | spine AD-16（**JSON 文件零库，覆盖 SQLite**） | ✅ 已注记为被推翻旧决策（AD-16 更高权威），非遗漏 |
| §8.3 本地/生产脚本 | spine AD-16 + Deferred A-6 | ✅ 已承载（以 assumption 形式） |
| §9 工程验证门禁（芯片/总线/功耗/Cloud 四组） | spine 门禁节（已「保真迁移」） | ✅ 已承载 |
| §10 明确不做 | spine AD-11 + Deferred | ✅ 已承载（CH340X、多设备、OTA、QMI 主算法等均在） |

## 3. spine GPIO 表 vs 09-08 附录 §3.1 逐行核对

结论：**GPIO 编号逐行完全一致，无冲突**。spine 与附录的差异仅在于信号命名拼写/分组，不影响接线。

| 功能 | spine GPIO | 附录 §3.1 GPIO | 一致 |
| --- | ---: | ---: | --- |
| BOOT | IO0 | IO0 | ✅ |
| PWR | IO46 | IO46 | ✅ |
| RESET | EN | EN | ✅ |
| PVDF ADC | IO9 | IO9 | ✅ |
| PVDF 唤醒（比较器） | IO11 | IO11 | ✅ |
| 共享 I²C SDA/SCL | IO1/IO2 | IO1/IO2 | ✅ |
| CST9217 TP_RST/TP_INT | IO38/IO39 | IO38/IO39 | ✅ |
| QMI8658C INT1 | IO41 | IO41 | ✅ |
| CO5300 RST/CS/SCL/D0~D3/EN | IO4/40/5/6/7/12/42/47 | IO4/40/5/6/7/12/42/47 | ✅ |
| ES8311 I2S DO/WS/DI/BCLK/MCLK | IO13/14/17/18/21 | IO13/14/17/18/21 | ✅ |
| NS4150B PA_EN | IO48 | IO48 | ✅ |
| Air780EGP UART TX/RX | IO43/44 | IO43/44 | ✅ |
| Air780EGP DTR/RST/NET_STATUS/GNSS_VCC | IO10/15/16/8 | IO10/15/16/8 | ✅ |
| RGB DATA | IO3 | IO3 | ✅ |
| USB D−/D+ | IO19/20 | IO19/20 | ✅ |

补充说明（非缺口）：

- GPIO45 约束、GPIO33–37 不作为通用 GPIO：spine 与附录措辞一致，与旧根文档一致（旧文档另给出「N16R8 上 GPIO33–37 与 Octal Flash/PSRAM 相关」的理由，spine/附录只保留结论 → 见 G4）。
- 附录 §8.2 明示 main_control 原板引脚常量（IO7/15/16/17/18/8）不可照搬，为旧决策的正式推翻注记，spine「不沿用 main_control 原板引脚常量」亦一致。

## 4. arch-only 事实清单（未来 builder 需要、仅存在于 architecture.md）

**G1（中，建议 must-fill）—— PVDF 存储边界 + 单外壳标定边界。**
- architecture.md §8.1：PVDF **只保存事件元数据、不保存原始波形**（§10「不保存原始波形」重复）。spine AD-1 只描述比较器唤醒 + ADC 确认，未写明「不得落盘原始波形」。该边界影响固件存储/缓冲设计。
- architecture.md §3.1 结构：**单一 3D 打印/拼接/喷涂外壳，只为最终外壳做一次传感器与声学标定，不做多外壳运行时适配**。spine/附录均无对应句（spine 前言「一台设备」只覆盖产品规模，不覆盖「无多外壳运行时适配」的固件范围）。
- **建议回填**：前者并入 spine **AD-1**（或 AD-12）加一句「PVDF 仅落盘事件元数据，不保存/不上传原始波形」；后者并入 spine **Deferred「明确非目标」** 或 §Structural Seed「嵌入式复用与不沿用」旁注（单外壳一次性标定、无多外壳运行时适配）。

**G2（低~中，需产品确认）—— 离线敲击的屏幕反馈句。**
- architecture.md §6.3：「小程序关闭时，设备离线敲击后短暂显示本地当前字、累计数和离线状态」。附录 §5.1（常驻状态栏同步/待同步）与 §4.2（熄屏短亮屏）语义近似但**无等价句**，spine AD-9 亦未写明「离线敲击后短暂亮屏显示本地字/累计/离线」。
- **建议回填**：若仍属产品意图，应补进 **09-08 附录 §5**（设备屏幕合同，行为权威所在），而非 spine；spine AD-9 可加一句「离线敲击即时以本地状态短亮屏反馈」。若认定已被附录 §4.2/§5.1 取代，则忽略。

**G3（低，并入 Deferred）—— 音频/电源两处电气细分。**
- §5.1 NS4150B 音频轨为独立受控电源，**3–5V 供电按扬声器阻抗与目标音量决定**；AMS1117 若保留只给低电流外设、优先低静态电流稳压器（spine AD-13 仅保留「不得承担 4G 主供电」）。
- **建议回填**：spine **Deferred 硬件细分 seed** 行补「NS4150B 音频轨电压带（3–5V，按扬声器阻抗/目标音量）」，AMS1117 半句可并入 AD-13 或同 Deferred 行。

**G4（低，纯理由）—— GPIO 保留脚的两条「为什么」。**
- §4.2 GPIO45 **影响 VDD_SPI 电压**（spine/附录只有「未接或按模组要求处理」的结果）；§3.2 GPIO33–37 **在 N16R8 型号与 Octal Flash/PSRAM 相关**（spine/附录只有「不作通用 GPIO」的结论）。
- **建议回填**：spine GPIO 基线表下注记追加一句即可，成本极低。

**G5（极低）—— Espressif 官方文档链接**（USB-Serial-JTAG / 内置 JTAG 两则 URL）未随迁。属资源引用而非事实，builder 可自行检索；如需可并入 AD-11。

**G6（低）—— I²C 电气附注。**
- §6.1「只保留**一组总线主上拉**，确认所有器件 I/O 电压兼容、上拉阻值与总线电容满足目标速率」；§6.1 ES8311 地址由 **CE/CDATA** 配置。
- spine 已有「共用一组总线与上拉」+ Deferred「I²C 上拉阻值」+ 门禁「上电 I²C 扫描」，主体覆盖；「CE/CDATA 机制」与「I/O 电压兼容/总线电容速率」两句未现。
- **建议回填**：spine I²C 行尾补「ES8311 地址由 CE/CDATA 配置」；「上拉阻值/总线电容/IO 电压兼容」已落在 Deferred 硬件细分 seed，可不另加。

**被推翻旧决策（已注记、不计遗漏）**：
- §8.3「backend 单文件 SQLite（`cloud/backend/data/wooden-fish.sqlite`）」→ 被 **AD-16（JSON 原子文件、零数据库）** 正式覆盖，spine 顶部「本轮覆盖决策」已声明。
- 旧「三模/多链路」概念（历史 PRD 曾设想 BLE/Wi‑Fi）→ spine AD-14 明确仅 Air780EGP 4G 一条业务链路，附录 §7 一致。
- PWR 从「纯硬断电按钮」扩展为「短按唤醒/页面循环、长按由板级电源关机」→ 附录 §3.1/§5.1 + AD-13 为更高权威。
- backend→frontend 实时通道「既定 WebSocket」→ spine A-1 降级为待定（WebSocket/轮询），属主动开放项。

## 5. 结论

**删除 architecture.md 基本安全，但建议先回填 G1（两条）后再删；G2 需产品确认归属；G3/G4/G6 属低成本补强，G5 可忽略。** spine 引脚表与 09-08 附录 §3.1 逐行一致，已无历史引脚漂移风险；绝大部分契约性内容已在 spine AD/Seed 或附录中保真承载（含验证门禁整节迁移）。剩余 arch-only 项均为「边界/理由/电气细分」级别，不构成阻断，但 G1 中「PVDF 不保存原始波形」与「单外壳一次性标定、无多外壳运行时适配」是当前任何文档都未覆盖的产品/固件边界，删除前回填成本最低。

### must-fill 建议回填清单

1. **PVDF 不保存原始波形** → spine AD-1 或 AD-12 加一句。
2. **单外壳一次性标定、不做多外壳运行时适配** → spine Deferred「明确非目标」行或 §Structural Seed「嵌入式复用与不沿用」。
3. **（待产品确认）离线敲击短亮屏显示本地字/累计/离线** → 附录 §5 或 spine AD-9。
