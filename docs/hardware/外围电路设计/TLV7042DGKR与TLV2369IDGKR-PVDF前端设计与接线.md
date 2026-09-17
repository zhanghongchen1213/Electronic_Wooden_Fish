# TLV7042DGKR 与 TLV2369IDGKR PVDF 前端设计与接线

> **本版已失效，待重新收敛。** 依据网表 `Netlist_Schematic1_2026-09-17 (3).tel`（SHA-256 `18e8f80edee5e2a9c78f1a72e781da81276be7833e9795e958a9b3b2246d9ef3`）；未通过项：`PVDF_CMP_WAKE` 网络缺 `TP_PVDF_CMP` 测试点（网络清单与硬件基线均要求该端点），修正后须重新导出网表复核。TLV2369IDGKR 脚 5/6/7 已按单位增益跟随器端接、TLV7042DGKR 脚 5/6 已接确定电平，端接电平落在手册共模输入范围内。

## 1. 最终版状态

- **版本**：EWF-PVDF-FINAL-2026-09-16
- **最近核验**：未通过
- **状态**：设计参数已收口；A 通道、阈值分压、ADC RC、供电和开漏输出已由最新网表核对。B 通道处理、输入钳位和样机参数仍是验收门禁，不能标为已通过。
- **最新网表**：`Netlist_Schematic1_2026-09-17 (3).tel`
- **网表 SHA-256**：`18e8f80edee5e2a9c78f1a72e781da81276be7833e9795e958a9b3b2246d9ef3`
- **项目网络**：`PVDF_RAW`、`TLV_AND`、`PVDF_ADC`、`PVDF_CMP_WAKE`、`IO9/ADC1_CH8`、`IO11`。

## 2. 最终设计参数

| 项目 | 已收口值 | 证据/状态 |
|---|---|---|
| 电源 | TLV7042/TLV2369 VCC/V+→`V3V3`；VEE/V−→`GND` | 最新网表已核对 |
| PVDF 接口 | PVDF 连接器 1 脚→输入限流 1 MΩ；PVDF 连接器 2 脚→GND | 最新网表已核对；连接器精确 MPN 需 BOM 核对 |
| 输入限流 | 输入限流 1 MΩ, 0603, 1% | 网表已核对；库存 C22935 |
| 输入泄放 | 输入泄放 10 MΩ, 0603 | 网表已核对；传感器负摆幅仍需验证 |
| 运放 A | TLV2369.3=`PVDF_RAW`；TLV2369.1 与 TLV2369.2 同网=`TLV_AND` | 最新网表已核对 |
| ADC 隔离/滤波 | ADC 隔离 1 kΩ；ADC 滤波 100 nF 到 GND；`PVDF_ADC`→ESP32-S3 模组 17 脚 | 最新网表已核对；名义截止约 1.59 kHz |
| 比较器阈值 | 阈值上分压 1 MΩ 从 V3V3；阈值下分压 47 kΩ 到 GND；阈值滤波 100 nF 到 GND；节点→TLV7042.2 | 最新网表已核对；V3V3=3.3 V 时 VTH≈0.148 V |
| 比较器 A | TLV7042.3=`TLV_AND`；TLV7042.1→`PVDF_CMP_WAKE` | 最新网表已核对 |
| 开漏上拉 | 开漏上拉 10 kΩ，`PVDF_CMP_WAKE`→V3V3 | 最新网表已核对；静态低电平约 0.33 mA |
| 去耦 | TLV7042 去耦、TLV2369 去耦=100 nF，各自电源脚到 GND | 网表已核对；介质/DC bias 需 BOM/规格书确认 |

## 3. 接线图

```mermaid
flowchart LR
  HDR[PVDF 连接器 1 脚 PVDF+] --> RSER[输入限流 1MΩ]
  RSER --> RAW[PVDF_RAW / TP_PVDF_RAW]
  HDRN[PVDF 连接器 2 脚 PVDF-] --> GND((GND))
  RAW --> RBIAS[输入泄放 10MΩ] --> GND
  RAW --> BUFINP[TLV2369.3 +INA]
  BUFOUT[TLV2369.1 OUTA] --- BUFINN[TLV2369.2 -INA]
  BUFOUT --> AND[TLV_AND]
  AND --> RADC[ADC 隔离 1kΩ]
  RADC --> ADC[PVDF_ADC / IO9 / ADC1_CH8]
  ADC --> CADC[ADC 滤波 100nF] --> GND
  AND --> CMPINP[TLV7042.3 INA+]
  V3V3[V3V3] --> RTH[阈值上分压 1MΩ] --> VTH[PVDF_VTH]
  VTH --> RTHB[阈值下分压 47kΩ] --> GND
  VTH --> CTH[阈值滤波 100nF] --> GND
  VTH --> CMPINN[TLV7042.2 INA−]
  CMPOUT[TLV7042.1 OUTA 开漏] --> CMP[PVDF_CMP_WAKE / IO11]
  V3V3 --> RPU[开漏上拉 10kΩ] --> CMP
  V3V3 --> CMPV[TLV7042.8]
  V3V3 --> BUFV[TLV2369.8]
  CMPV --> CDEC1[TLV7042 去耦 100nF] --> GND
  BUFV --> CDEC2[TLV2369 去耦 100nF] --> GND
  CMPG[TLV7042.4] --> GND
  BUFG[TLV2369.4] --> GND
```

## 4. 逐脚接线

### TLV2369IDGKR（PVDF 缓冲运放）：VSSOP-8

| 脚 | 最终连接 | 状态 |
|---:|---|---|
| 1 OUTA | `TLV_AND`；与脚 2 同网 | 网表已核对 |
| 2 −INA | 回接脚 1，不能接 ADC 隔离 1 kΩ 后的 ADC 节点 | 网表已核对 |
| 3 +INA | `PVDF_RAW` | 网表已核对 |
| 4 V− | GND | 网表已核对 |
| 5 +INB | **待补固定电平** | 最新网表未出现 |
| 6 −INB | **待补** | 最新网表未出现 |
| 7 OUTB | **待补；建议与脚 6 同网形成跟随器** | 最新网表未出现 |
| 8 V+ | V3V3，缓冲器去耦 100 nF | 网表已核对 |

### TLV7042DGKR（PVDF 比较器）：VSSOP-8

| 脚 | 最终连接 | 状态 |
|---:|---|---|
| 1 OUTA | `PVDF_CMP_WAKE`，开漏上拉 10 kΩ 上拉至 V3V3 | 网表已核对 |
| 2 INA− | `PVDF_VTH`，阈值上分压 1 MΩ/阈值下分压 47 kΩ/阈值滤波 100 nF 分压滤波点 | 网表已核对 |
| 3 INA+ | `TLV_AND` | 网表已核对 |
| 4 VEE | GND | 网表已核对 |
| 5 INB+ | **待补确定固定电平** | 最新网表未出现 |
| 6 INB− | **待补确定固定电平** | 最新网表未出现 |
| 7 OUTB | **NC** | 最新网表未出现 |
| 8 VCC | V3V3，比较器去耦 100 nF | 网表已核对 |

## 5. 最终 BOM 与来源

库存快照：`库存列表-20260913211039.xlsx`，2026-09-13 21:10:39；可用量按在库减占用计算。嘉立创库存为 2026-09-15 页面检索时点。

| 用途 | 精确 MPN/规格 | 数量 | LCSC | 来源/状态 |
|---|---|---:|---|---|
| PVDF 比较器 | TLV7042DGKR，VSSOP-8 | 1 | C2760466 | 库存文件无精确料，嘉立创采购候选 |
| PVDF 缓冲运放 | TLV2369IDGKR，VSSOP-8 | 1 | C2057867 | 库存文件无精确料，嘉立创采购候选 |
| 输入限流 1 MΩ | 0603WAF1004T5E，1 MΩ ±1% | 1 | C22935 | 库存可用 50 |
| 输入泄放 10 MΩ | 0603WAF1005T5E，10 MΩ ±1% | 1 | C7250 | 库存可用 50 |
| ADC 隔离 1 kΩ | 0603WAF1001T5E，1 kΩ ±1% | 1 | C21190 | 库存可用 50 |
| 开漏上拉 10 kΩ | 0603WAF1002T5E，10 kΩ ±1% | 1 | C25804 | 库存可用 50 |
| 阈值上分压 1 MΩ | 0603WAF1004T5E，1 MΩ ±1% | 1 | C22935 | 与输入限流 1 MΩ 共用库存，可用量足够 |
| 阈值下分压 47 kΩ | 0603WAF4702T5E，47 kΩ ±1% | 1 | C25819 | 库存可用 87 |
| 去耦/ADC 与阈值滤波 100 nF | 0603B104K500NT，100 nF ±10%，50 V | 4 | C30926 | 库存可用 89；介质/DC bias 需规格书确认 |
| PVDF 传感器连接器 | 2P，2.54 mm 通孔接口 | 1 | 需按实际 BOM 核对 | 最新网表为 2.54 mm HDR 封装；前版 1.25 mm 候选不适用 |

## 6. 未关闭的最终验收门禁

1. 补齐 TLV7042 B 通道和 TLV2369 B 通道，重新导出网表；当前不能将悬空写成通过。
2. 为 PVDF_RAW/TLV2369.3 选择并核对低漏钳位，验证正负摆幅、ESD 和 ADC 安全范围。
3. 通过 ERC、PCB DRC；检查高阻节点、模拟地回流和 4G/音频噪声隔离。
4. 实测 PVDF_VTH 上下翻转电压、单次敲击、1 秒 20 次敲击、环境振动误触发和 ADC 不越界。
5. 复核 BOM 中 TLV7042/TLV2369 精确 MPN、PVDF 传感器连接器型号、电容介质及额定电压。

当前文档已作为设计参数收口版保存；上述门禁关闭前，不宣称 PCB 或样机验证通过。

完成嘉立创原理图后，请提供同版本原理图 PDF/截图、BOM，并强烈建议提供包含元件、引脚和网络端点的网表，以便继续关闭剩余问题。
