# 嘉立创 EDA 分层原理图落图记录

> **状态：已建立页树但未放置元件；本文件与离线 HTML 复刻图是当前落图入口，嘉立创 EDA ERC 尚未运行。**
>
> 当前选择以离线 HTML 复刻为主，不再继续自动化放置；网页工程中已保留 P01–P07 页树，但未把自动生成文件当作 ERC 结果。网络名逐字采用 [`电子木鱼-硬件网络清单.json`](电子木鱼-硬件网络清单.json)。

## 1. 工程登记

| 字段 | 值 |
| --- | --- |
| 工程名 | `电子木鱼`（工程 ID `f4c99a54f36241e7a4e90c9ea8e6a1b8`） |
| 工具 | 嘉立创 EDA 专业版（目标工具） |
| 原理图状态 | `pages_created_blank_manual_html` |
| PCB/打样状态 | 未开始 |
| ERC 状态 | 未运行 |
| 电气事实源 | `_bmad-output/implementation-artifacts/spec-electronic-wooden-fish-hardware-schematic-baseline.md` |
| 人可读基线 | `docs/hardware/电子木鱼-硬件原理图设计基线.md` |
| 机器网络清单 | `docs/hardware/电子木鱼-硬件网络清单.json` |

**禁止误读：** 静态 JSON 校验通过只说明网络清单结构和合同自洽；不等价于 EDA ERC、PCB 可制造性、样机上电或通信/音频/传感器通过。

## 2. 层次页树与页间端口

```text
电子木鱼 / Schematic1
├── P01_POWER_USB       USB-C / BQ25895 / TPS3424 / TPS22965 / TPS22919 / FSW7227
├── P02_MCU_USB         ESP32-S3-WROOM-1-N16R8 / BOOT / EN / USB-Serial-JTAG
├── P03_DISPLAY_TOUCH   PY206-W38-V2 / CO5300 / CST9217 24-pin FPC
├── P04_AUDIO_RGB       ES8311 / NS4150B / 扬声器 / RGB
├── P05_PVDF            PVDF / TLV2369 / TLV7042 / 限流钳位 / ADC
├── P06_MODEM           M100EG-C2 / Air780EGP / SIM / 天线
└── P07_SHARED_SENSORS  CW2015 / QMI8658C / I²C 测试点
```

每页只通过全局标签连接，不复制或改写网络名。电源页和主控页必须同时显示 `VBAT_SW`、`3V3`、`AUDIO_5V` 的电源符号及测试点，避免层次页中形成隐含电源。

## 3. P01 电源/USB 逐网落图顺序

1. 放置 J1（`TYPEC-304-ACP16`，LCSC `C165948` 候选），合并 USB-C 多个 VBUS/GND/D+/D− 引脚。
2. J1 VBUS 经过输入保护后标 `VBUS_5V`，只进入 BQ25895 `VBUS`；CC1/CC2 各放 5.1 kΩ Rd 到 GND。
3. 放置 BQ25895RTWR（LCSC `C80200`）：`BAT`→`BAT_PROTECTED`、`SYS`→`BQ_SYS`（仅保留/测试）、`PMID`→`PMID_OTG_5V`、D+/D−→`USB_DP_BQ/USB_DM_BQ`、OTG→`BQ_OTG_EN`。
4. 按推荐初值放置 BQ 周边：`L=2.2 µH`、VBUS `1 µF+100 nF`、SYS `≈20 µF`、PMID `60 µF`、REGN `4.7 µF`、BTST `47 nF`、ILIM `180 Ω`、STAT/INT `10 kΩ`、TS `5.23 kΩ/30.1 kΩ + 10 kΩ NTC`；全部标注 `RECOMMENDED_INITIAL`。
5. 放置 TPS3424（`TPS3424A11C13ADRLR`）和 TPS22965DSGR（LCSC `C122837`）：PWR 按键→TPS3424，TPS3424 锁存输出→TPS22965 ON，输出标 `VBAT_SW`。
6. `VBAT_SW` 只连 M100 VIN 和 TLV62569 输入；**禁止**从 `BQ_SYS` 或 5 V 升压网给 M100 VIN。
7. 放置 TLV62569DBVR（LCSC `C141836`）：`L=2.2 µH`、输入 `10 µF+100 nF`、输出 `2×22 µF`、FB `453 kΩ/100 kΩ`，输出标 `3V3`。
8. 放置 TPS22919DCKR（LCSC `C2149796`）：输入 `PMID_OTG_5V`、输出 `AUDIO_5V`，ON 由 `PMID_VALID/AUDIO_EN` 逻辑驱动并用 100 kΩ 下拉；不得悬空。
9. 放置 FSW7227YMS10G/TR（LCSC `C32713318`）：公共端 `USB_DP_C/USB_DM_C`，BQ 端 `USB_DP_BQ/USB_DM_BQ`，ESP 端 `USB_D+/USB_D−`；SEL/DSEL 分别引出 `JP_USB_FORCE_BQ` 与 `JP_USB_FORCE_ESP`。

**USB ERC 检查重点：** 默认上电选 BQ 路径；强制焊盘只能一路闭合；BQ 与 ESP 数据线不能有同名短接；USB VBUS 只作输入，任何输出端禁止回灌。

## 4. P02 主控与 GPIO 页

放置 `ESP32-S3-WROOM-1-N16R8`（LCSC `C2913202`），按下表逐一放置全局标签。GPIO 编号是 EWF 合同，不得照搬参考 PDF 或 `main_control` 旧映射。

| ESP32 脚 | 全局网络 | 页/去向 | 测试点 |
| ---: | --- | --- | --- |
| IO0 | `BOOT0` | 下载按键/绑带 | `TP_BOOT0` |
| IO46 | `PWR_STATE` | TPS3424/PWR 状态输入，只读 | `TP_PWR_STATE` |
| IO8 | `BQ_OTG_EN` | BQ25895 OTG 脚 | `TP_BQ_OTG_EN` |
| IO9 | `PVDF_ADC` | P05 ADC1_CH8 | `TP_PVDF_ADC` |
| IO11 | `PVDF_CMP_WAKE` | P05 比较器输出 | `TP_PVDF_CMP` |
| IO1/IO2 | `I2C_SDA/I2C_SCL` | P03/P04/P07/P01 BQ | `TP_I2C_SDA/SCL` |
| IO38/IO39 | `TP_RST/TP_INT` | P03 CST9217 | `TP_TP_RST/INT` |
| IO41 | `IMU_INT1` | P07 QMI8658C；INT2 NC | `TP_IMU_INT1` |
| IO4/40/5/6/7/12/42/47 | `LCD_RST/CS/SCLK/D0/D1/D2/D3/EN` | P03 CO5300 | `TP_LCD_*` |
| IO13/14/17/18/21 | `I2S_DOUT/WS/DIN/BCLK/MCLK` | P04 ES8311 | `TP_I2S_*` |
| IO48 | `PA_EN` | P04 NS4150B | `TP_PA_EN` |
| IO43/44 | `M100_TXD/M100_RXD` | P06 UART（交叉） | `TP_M100_TXD/RXD` |
| IO10/15/16 | `M100_DTR/M100_RST/M100_NET_STATUS_COMPAT` | P06 | `TP_M100_DTR/RST/NET_STATUS` |
| IO3 | `RGB_DATA` | P04 状态灯 | `TP_RGB_DATA` |
| IO19/20 | `USB_D−/USB_D+` | P01/P02 FSW7227 ESP 端 | `TP_USB_D−/D+` |
| EN | `RESET_N` | 独立复位 | `TP_RESET_N` |

## 5. P03 显示/触摸页与 FPC 交叉表

J3 使用 HRS `FH34SRJ-24S-0.5SH(50)`（LCSC `C324726`）作为 24P/0.5 mm 双面接触候选。逐脚网络如下；FPC 正反面、接触方向和 pin 1 必须以实物插合复核。

| FPC | 网络 | ESP32/电源 | 备注 |
| ---: | --- | --- | --- |
| 1 | `3V3` | 触摸 VDD | 推荐初值 |
| 2 | `I2C_SCL` | IO2 | CST9217 |
| 3 | `I2C_SDA` | IO1 | CST9217 |
| 4 | `TP_INT` | IO39 | 中断极性待测 |
| 5 | `TP_RST` | IO38 | RST 极性待测 |
| 6 | `GND` | GND | |
| 7 | `3V3` | 显示 VDD | 电源时序待测 |
| 8 | `LCD_VBAT` | 首版接 `VBAT_SW`/独立调节候选 | 供电范围待测 |
| 9 | `NC` | 不接 | MTP/NC |
| 10 | `3V3` | 显示 VDD | |
| 11 | `GND` | GND | |
| 12 | `LCD_RST` | IO4 | L_RES |
| 13 | `GND` | GND | |
| 14 | `LCD_D0` | IO6 | L_IO0 |
| 15 | `GND` | GND | |
| 16 | `LCD_SCLK` | IO5 | L_SCLK |
| 17 | `GND` | GND | |
| 18 | `LCD_D3` | IO42 | L_IO3 |
| 19 | `NC` | 不接 | |
| 20 | `LCD_D2` | IO12 | L_IO2 |
| 21 | `LCD_CS` | IO40 | CS |
| 22 | `LCD_D1` | IO7 | L_IO1 |
| 23 | `LCD_EN` | IO47 | OLED_EN |
| 24 | `LCD_TE` | TP/NC | LTE/TE，MVP 不用 |

## 6. P04 音频/LED 页

- ES8311（LCSC `C962342`）挂共享 `I2C0`，地址首版 `0x18`；`I2S_DOUT/WS/DIN/BCLK/MCLK` 接 IO13/14/17/18/21。
- 参考外围：PVDD/DVDD 100 nF、AVDD 1 µF、参考端各 1 µF、输出耦合 1 µF + 0 Ω；I²S 22 pF 仅 DNP 料位。
- NS4150B（LCSC `C189961`）VCC=`AUDIO_5V`、EN=`PA_EN`、OUTP/N→`SPK_P/SPK_N`；VCC 1 µF + 22 µF + 22 µF，BYPASS 1 µF，输入耦合 100 nF，输入偏置 100 kΩ，EN 下拉 100 kΩ。
- `PA_EN` 只有在 `AUDIO_5V` 稳定后才拉高；充电、静音、故障时保持低。RGB 优先 3V3 兼容器件；若使用 5 V RGB，必须单独画电平转换。

## 7. P05 PVDF 页

```text
J6 PVDF_RAW → R_PVDF_SER 1 MΩ → PVDF_COND
                                  ├→ U15 TLV2369 缓冲/偏置
                                  ├→ R_PVDF_ADC 1 kΩ → C_PVDF_ADC 100 nF → IO9 PVDF_ADC
                                  └→ U16 TLV7042 比较器 → IO11 PVDF_CMP_WAKE
```

钳位、10 MΩ 泄放/偏置、比较器阈值/滞回和输出极性都标 `MUST_PROTOTYPE_VALIDATE`；在原理图上放 `TP_PVDF_RAW`、`TP_PVDF_ADC`、`TP_PVDF_CMP`。原始波形不落盘、不上传。

## 8. P06 M100 载板逻辑交叉与测试点

`main_control/components/BSP/GPS/gps.h` 只用于协议/信号名称交叉核对，**其 GPIO 不能复制到 EWF**。载板真实排针编号、方向、间距和缺省 NC 由 M100EG-C2 载板版本资料与实物确认。

| 载板信号 | `main_control` 旧 GPIO（仅参考） | EWF GPIO | EWF 网络 | 测试点 | 落图处理 |
| --- | ---: | ---: | --- | --- | --- |
| VIN | — | — | `M100_VIN=VBAT_SW` | `TP_VBAT_SW` | 高电流电池轨，禁止 BQ_SYS/5V |
| GND | — | — | `GND` | `TP_GND_M100` | 独立回流区域 |
| TXD | IO17 | IO44 | `M100_RXD` | `TP_M100_TXD` | 载板 TXD → ESP RX |
| RXD | IO18 | IO43 | `M100_TXD` | `TP_M100_RXD` | ESP TX → 载板 RX |
| DTR | IO7 | IO10 | `M100_DTR` | `TP_M100_DTR` | 开漏；极性待测 |
| RST | IO15 | IO15 | `M100_RST` | `TP_M100_RST` | 方向/低有效待测 |
| NET_STATUS | IO16 | IO16 | `M100_NET_STATUS_COMPAT` | `TP_NET_STATUS` | 载板无脚时 NC/TP |
| GNSS_VCC | IO8 | 无 | `M100_GNSS_VCC_NC` | `TP_GNSS_VCC_NC` | NC/TP；IO8 改 BQ_OTG_EN |
| SIM | — | — | `M100_SIM` | `TP_SIM_*` | 载板自带/ESD 待核 |
| ANT | — | — | `M100_ANT` | `TP_ANT_FEED` | 天线/匹配/机械待核 |

## 9. P07 共享传感器页

| 器件 | 地址首版 | 网络 | 中断 | 状态 |
| --- | --- | --- | --- | --- |
| CW2015 | `0x62` | `I2C_SDA/SCL` | — | 上电扫描 |
| QMI8658C | `0x6B`（SA0 高） | `I2C_SDA/SCL` | INT1→IO41，INT2 NC | MVP 不启用；上电扫描 |

共享总线还包含 BQ25895 `0x6A`、CST9217 `0x5A`、ES8311 `0x18`。五个地址必须唯一；任何冲突先改 spine/清单再改固件。

## 10. 嘉立创 EDA 建工程后的 ERC 清单

- [ ] P01–P07 层次页均存在且页端口名称与 JSON 一致。
- [ ] `VBAT_SW` 只连接 TPS22965 输出、TLV62569 输入和 M100 VIN；`BQ_SYS` 无 M100 端点。
- [ ] `USB_DP_BQ/USB_DM_BQ` 与 `USB_D+/USB_D−` 只经 FSW7227 选路，默认/强制选择互斥。
- [ ] BQ25895 所有电源/自举/REGN/TS/ILIM/STAT/INT 脚有明确终端，不留悬空关键脚。
- [ ] TPS22965/TPS22919 ON、QOD、CT 脚有明确状态；无软件模拟硬关机的替代路径。
- [ ] `IO0/IO46` 启动绑带和 `IO45` 未接约束可追踪；GPIO33–37 未误用。
- [ ] I²C 地址唯一；QMI SA0=高；上拉阻值及电压域已标注推荐/待测。
- [ ] PVDF ADC 有 1 MΩ 限流、1 kΩ+100 nF 首版网络与低漏钳位；比较器输出有 IO11 测试点。
- [ ] FPC 24 脚逐一对照 PDF 与实物，pin 1/接触面/`LCD_VBAT`/`LCD_EN`/`TE` 记录齐全。
- [ ] M100 VIN 电流能力、DTR/RST 极性、NET_STATUS/GNSS_VCC NC 处理有载板资料证据。
- [ ] 所有 `[必须样机验证]` 事项在 BOM/原理图备注中保留，不标记为 PASS。

## 11. 当前未完成与后续交付

1. 嘉立创 EDA 工程文件（`.epro/.sch`）尚未创建；需要用户桌面账号/工具连接或人工落图。
2. 未运行 EDA ERC、PCB DRC、封装 3D 检查；未进行打样。
3. M100 载板真实 header pin number、TLV7042/TPS3424 LCSC 料号、PVDF 阈值/滞回、FPC 物理方向、电池/天线/扬声器结构仍属开放项。
4. 完成工程后，应把工程导出的 BOM/网络表回填 JSON 的 `lcsc` 与 `physical_header_pin_numbers` 字段，并重跑 `python3 tools/validate_hardware_manifest.py`。
