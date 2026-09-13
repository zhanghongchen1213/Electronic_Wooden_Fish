# 电子木鱼外围电路参考资料与落图索引

本目录同时提供原厂/厂商参考资料、典型应用页码、EWF 的数量清单和逐脚 SVG 落图约束。SVG 用来理解“哪个脚通过哪条线接到哪个网络”，落图时仍需在嘉立创 EDA 中使用真实符号、封装和 ERC；所有推荐阻值/容值都必须按采购版本和样机验证。

## 0. 主板装配数量总表

下表是按当前 EWF 方案计算的**主板装配数量**。数量是主板 BOM 数量，不把 PY206 屏幕模组内部芯片重复计入主板。

| 类别 | 器件/模块 | 主板数量 | 是否在 PY206 内部 | 说明 |
| --- | --- | ---: | --- | --- |
| 主控 | ESP32-S3-WROOM-1-N16R8 | 1 | 否 | 主控模组 |
| 充电/系统电源 | BQ25895RTWR | 1 | 否 | 单节电池充电、NVDC、BQ_SYS |
| PWR 锁存 | TPS3424A11C13ADRLR | 1 | 否 | BOOT/PWR 电源控制 |
| 系统负载开关 | TPS22965DSGR | 1 | 否 | BQ_SYS → SYS_MAIN |
| 3V3 降压 | TLV62569DBVR | 1 | 否 | SYS_MAIN → 3V3 |
| 音频 Codec | ES8311 | 1 | 否 | DAC-only 播放，不接 ADC_DATA |
| 音频功放 | NS4150B | 1 | 否 | `3V3_AUDIO`，差分扬声器 |
| PVDF 缓冲 | TLV2369IDGKR | 1 | 否 | PVDF 模拟缓冲 |
| PVDF 比较器 | TLV7042DGKR | 1 | 否 | 双路比较器，A 路唤醒，B 路固定 |
| 电量计 | CW2015 | 1 | 否 | CELL 接 BAT_PROTECTED |
| IMU | QMI8658A | 1 | 否 | 预留，INT1=IO41，INT2=NC |
| 4G 载板 | M100EG-C2 / Air780EGP carrier | 1 | 否 | UART、SIM、天线 |
| 屏幕模组 | PY206-W38-V2 | 1（外部模组） | 是 | 包含 CO5300、CST9217、OCP21351 |
| 屏幕连接器 | OK-14F024-04 | 1 | 否 | 主板侧 24-pin BTB 母座 |
| USB-C | TYPEC-304-ACP16 | 1 | 否 | 单 USB-C 供电和 USB 数据 |
| 电池连接器 | B3B-PH-K-S(LF)(SN) | 1 | 否 | BATT+/BATT−/NTC |
| 扬声器连接器 | 2P 连接器 | 1 | 否 | SPK_P/SPK_N |
| PVDF 接口 | 2P 焊盘/连接器 | 1 | 否 | PVDF_RAW |

**数量结论：** 主板需要 11 颗主要 IC（ESP32、BQ25895、TPS3424、TPS22965、TLV62569、ES8311、NS4150B、TLV2369、TLV7042、CW2015、QMI8658A 各 1）以及 1 个 M100/Air780EGP 载板模块；PY206 是 1 个外部屏幕模组，CO5300/CST9217/OCP21351 已包含在该模组内，主板数量为 0。

被动器件数量按原理图分配逐项统计：USB-C CC 电阻 2 个，BQ25895 I²C 上拉 2 个，PVDF `R_SER/R_BIAS/R_ADC` 各 1 个，PVDF `C_ADC` 1 个，ES8311/功放/各电源轨去耦电容按典型应用图和最终 ERC/BOM 再汇总，不在本表用“估算数量”代替 BOM。

## 1. 电源与 USB

| 器件/模块 | 推荐资料 | 重点页码/用途 |
| --- | --- | --- |
| BQ25895 | [TI BQ25895 Rev.C 数据手册](https://www.ti.com/lit/ds/symlink/bq25895.pdf) | 第 3–5 页脚位；第 49–55 页典型应用、SYS、BAT、VBUS、PMID、TS、ILIM 和布局 |
| TPS3424 | [TI TPS3424 Rev.C 数据手册](https://www.ti.com/lit/ds/symlink/tps3424.pdf) | 第 5–6 页 8-pin 脚位；第 19 页 PWR 按键、SPT/LPT、RESET、INT、KILL |
| TPS22965 | [TI TPS22965 Rev.F 数据手册](https://www.ti.com/lit/ds/symlink/tps22965.pdf) | 第 3 页 DSGR 脚位；第 17–19 页 VBIAS、VIN、ON、CT、VOUT 和典型应用 |
| TLV62569 | [TI TLV62569 Rev.C 数据手册](https://www.ti.com/lit/ds/symlink/tlv62569.pdf) | 第 2 页 SOT-23-5 脚位；第 7 页反馈分压、SW、电感和输入/输出电容 |
| USB Serial/JTAG | [ESP-IDF USB Serial/JTAG 官方文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/usb-serial-jtag-console.html) | GPIO20=D+、GPIO19=D−；烧录、CDC 日志和 JTAG；无需外部 USB-UART |
| USB-C 受电端 | [Espressif USB-C 硬件指南](https://docs.espressif.com/projects/esp-iot-solution/en/latest/usb/usb_overview/usb_typec_hardware_guide.html) | CC1/CC2 各 5.1kΩ Rd；受电端识别和正反插 |

EWF 最终电源网络：`USB-C VBUS/BAT → BQ25895 → BQ_SYS → TPS22965 → SYS_MAIN → 3V3 → 3V3_AUDIO`。USB-C D+/D− 直接接 ESP32；BQ25895 D+/D− 按其“关闭输入源检测”方式处理。

## 2. 音频

| 器件 | 推荐资料 | 重点页码/用途 |
| --- | --- | --- |
| ES8311 | [Everest ES8311 Rev.7.0 原理图资料](https://dl.espressif.com/dl/schematics/Audio_ES8311.pdf) | 第 2 页脚位；第 3 页典型应用；PVDD/DVDD/AVDD、VMID、ADCVREF、DACVREF、OUTP/OUTN |
| NS4150B | [Nsiway NS4150B 原理图资料](https://dl.espressif.com/dl/schematics/NS4150B.pdf) | 第 6 页脚位；第 11 页应用；CTRL、BYPASS、INP/INN、VCC、VoP/VoN、Cin+Rin |
| NS4150B 新版核对 | [LCSC NS4150B 数据手册](https://datasheet.lcsc.com/lcsc/2209161630_Shenzhen-Nsiway-Tech-NS4150B_C189961.pdf) | 与采购批次脚位和外围值交叉核对 |

EWF 音频连接：ES8311 只做 DAC 播放；`DOUT=IO13`、`WS=IO14`、`BCLK=IO18`、`MCLK=IO21`；ADC_DATA/IO17 不接。NS4150B 使用 `3V3_AUDIO`，ES8311 OUTP/OUTN 经输入耦合电容和串联 Rin 进入功放。

## 3. 传感器与模拟前端

| 器件 | 推荐资料 | 重点页码/用途 |
| --- | --- | --- |
| QMI8658A | [QST QMI8658A Rev.A 数据手册](https://www.qstcorp.com/upload/pdf/202301/13-52-25%20QMI8658A%20Datasheet%20Rev%20A.pdf) | 第 8–11 页 I²C 应用、SA0、RESV、INT1/INT2；第 13 页外部元件 |
| CW2015 | [CW2015 数据手册](https://datasheet.lcsc.com/lcsc/2201242130_Cellwise-CW2015CHBD_C881838.pdf) | TDFN-8 脚位、CELL、VDD、I²C、100nF 去耦、4.7kΩ 上拉 |
| TLV7042 | [TI TLV7042 数据手册](https://www.ti.com/lit/ds/symlink/tlv7042.pdf) | 双路开漏比较器、输出上拉和封装脚位 |
| TLV2369 | [TI TLV2369 产品/数据手册入口](https://www.ti.com/product/zh-tw/TLV2369) | VSSOP-8 缓冲器脚位和未使用通道处理 |

EWF 传感器连接：QMI8658A `INT1=IO41`，INT2 和 GPIO45 不接；PVDF 使用 `IO9` ADC 和 `IO11` 唤醒；PVDF 的 1MΩ 限流、低漏钳位、阈值和滞回必须按实测确定。

## 4. 屏幕模组

| 模块 | 资料 | 使用方式 |
| --- | --- | --- |
| PY206-W38-V2 | [`3593896291PY206-W38-V2(2).pdf`](../3593896291PY206-W38-V2(2).pdf)；[JLC OK-14F024-04/C9900019201](https://jlcpcb.com/partdetail/JLCPCBAssembly-OK_14F02404/C9900019201)；[OCN 连接器检索](https://www.panelook.com/connector_bramodlist.php?brands%5B%5D=OCN) | 屏幕排线端为 `OK-14GM024-04` 公座，主板端放 `OK-14F024-04` 母座；24P、0.4mm、2×12、0.8H。PDF 第 3 页给出 24-pin 电气映射；pin 1 接触面、pin 8 `LCD_VBAT`、`LCD_EN` 极性和 `TE` 是否启用仍需实物确认 |
| CO5300/CST9217/OCP21351 | 已集成在 PY206 模组内部 | 主板不重复放置芯片，只放 BTB 母座和保护/测试器件；CO5300 的 QSPI、CST9217 的 I²C/RST/INT 均经 24-pin 排线连接 |

### PY206 24-pin 直接落图口径

主板 J3 采用 `OK-14F024-04` 母座；屏幕 FPC 尾端的 `OK-14GM024-04` 是配对公座。它属于板对板（BTB）上下扣合结构，不是 HRS FH34SRJ 一类的 0.5mm FPC/FFC ZIF 插座。嘉立创页面明确给出 `OK-14F024-04` 的 SMD-24P 符号/封装库条目（C9900019201）；下单前仍要把供应商机械图与实物的 pin-1、接触面、焊盘和配合高度叠图核对。

逐脚网络如下：

| 脚 | 屏幕标注 | 主板网络 | 连接说明 |
| ---: | --- | --- | --- |
| 1, 7, 10 | `TP-VDD_3V` / `VDD_3V` | `3V3` | 触摸/显示逻辑供电；主板侧 `1µF + 100nF` 就近去耦 |
| 2, 3 | `TP-SCL_3V` / `TP-SDA_3V` | `I2C_SCL` / `I2C_SDA` | 接 ESP32 IO2/IO1；整板只保留一组 4.7kΩ 上拉 |
| 4, 5 | `TP-INT_3V` / `TP-RST_3V` | `TP_INT` / `TP_RST` | 接 CST9217 的 INT/RST；IO39/IO38，极性按首帧和触摸初始化验证 |
| 6, 11, 13, 15, 17 | `GND` | `GND` | 全部接连续地平面，FPC 下方保持短回流 |
| 8 | `VBAT` | `LCD_VBAT` | 接 `SYS_MAIN` 经屏幕所需偏压路径；电压范围和上电时序必须按模组资料/实物确认 |
| 9, 19 | `MTP/NC` / `NC` | `NC` | 不布线、不上拉、不下拉，丝印标 NC |
| 12 | `L_RES` | `LCD_RST` | ESP32 IO4，保留复位测试点 |
| 14, 22, 20, 18 | `L_IO0..3` | `LCD_D0..D3` | QSPI 数据，分别接 IO6/IO7/IO12/IO42 |
| 16 | `L_SCLK` | `LCD_SCLK` | ESP32 IO5；与 D0–D3、CS 成组布线 |
| 21 | `CS` | `LCD_CS` | ESP32 IO40 |
| 23 | `OLED_EN`（部分资料写 `VCI_EN`） | `LCD_EN` | ESP32 IO47；有效极性和时序必须样机验证 |
| 24 | `LTE_TE` / `L_TE` | `LCD_TE` | MVP 默认 NC，但留 `TP_LCD_TE`；需要 TE 同步时再按 CO5300 配置启用 |

`TP PIN定义` 是屏幕模组上的测试焊盘定义（TP_VDD、TP_RST、TP_INT、TP_SDA、TP_SCL、TP_GND），不是第二个必须插接的 6-pin 外部连接器；主板只需按上表连接 24-pin BTB，并可选择保留探针测试点。

## 5. 4G 载板

| 模块 | 资料 | 使用方式 |
| --- | --- | --- |
| Air780EG | [合宙 Air780EG 资料中心](https://docs.openluat.com/air780eg/product/) | 获取硬件设计手册、原理图和 PCB 封装 |
| M100/Air780EGP 载板 | 项目载板资料 + `main_control` 交叉源码 | 主板只冻结逻辑交叉：IO43→载板 RXD、IO44←载板 TXD、DTR=IO10、RST=IO15；NET_STATUS 不接 IO16 |

目标载板排针编号、方向、电平和 RST/DTR 极性没有可靠的项目内原厂图前，不得把裸模块脚号写进主板原理图。

## 6. 落图规则

1. 先打开对应官方 PDF 的典型应用页，再按页码将元件、脚号和连接复制到嘉立创 EDA。
2. 只使用 EWF 最终网络名：`VBUS_5V`、`BAT_PROTECTED`、`BQ_SYS`、`SYS_MAIN`、`3V3`、`3V3_AUDIO`、`USB_D+`、`USB_D−`、`I2C_SDA`、`I2C_SCL`。
3. 原厂资料没有明确的电阻、电容、极性或 NC 处理时，标记为待确认并保留测试点，不凭经验补值。
4. 每个模块落图后分别执行 ERC、网络表导出、BOM 导出和样机验证。

## 7. 参考图使用顺序

先按第 0 节数量表准备主板器件，再打开对应模块的原厂典型应用页，在嘉立创 EDA 中使用真实符号和封装逐条落线。对于 PY206，先确认 OK-14GM024-04 公座与 OK-14F024-04 母座的机械配合，再按 [`py206-btb-24pin-wiring.svg`](display/py206-btb-24pin-wiring.svg) 和 24-pin 表逐条落线；未完成实物插合前，不要把 `LCD_VBAT`、`LCD_EN` 和 `LCD_TE` 的状态写成“已通过”。
