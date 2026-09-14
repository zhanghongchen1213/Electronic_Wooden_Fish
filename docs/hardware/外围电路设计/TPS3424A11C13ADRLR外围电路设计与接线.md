# TPS3424A11C13ADRLR 外围电路设计与接线

> 文档状态：最终设计口径已锁定（2026-09-14）。原理图 ERC、PCB DRC、时序波形和样机仍未验证。
> 实际网表位号：U5；用户确认型号：`TPS3424A11C13ADRLR`。
> 数据手册：[TI TPS3423/TPS3424 Rev.C](https://www.ti.com/lit/ds/symlink/tps3424.pdf)。

## 1. 料号配置与网络定义

该料号为 8-pin DRL、active-low PB、active-high push-pull latched RESET、SPT/LPT 外部电容可编程、INT 短按 50 ms/长按 100 ms、KILL 去抖 500 ms。

本项目将 `PWR_STATE` 定义为唯一物理网络名。它同时承担 TPS3424 RESET 输出、TPS22965 ON 控制和 ESP32 IO46 只读状态；`SYS_EN` 仅是“控制 TPS22965”的功能称呼。

## 2. 逐脚最终接线

| U5 pin | 功能 | 最终接线 | 网表证据 |
| ---: | --- | --- | --- |
| 1 | PB，active-low 输入 | `SW2.1`；SW2.3 接 `GND` | `$1N291`：SW2.1、U5.1；GND：SW2.3 |
| 2 | GND | `GND` | GND：U5.2 |
| 3 | VDD | `VSYS`；C24=100 nF 至 GND | VSYS：U5.3、C24.2 |
| 4 | SPT | C22=120 pF 至 GND | `$1N273`：U5.4、C22.2；C22.1=GND |
| 5 | LPT | C23=4.7 nF 至 GND | `$1N275`：U5.5、C23.1；C23.2=GND |
| 6 | RESET | `PWR_STATE`；接 U8.3、U1.16、TP_PWR_STATE | PWR_STATE：U5.6、U8.3、U1.16、TP_PWR_STATE.1 |
| 7 | INT，开漏低有效 | R7=10 kΩ 上拉至 `V3V3`，并接 TP_PWR_INT | `$1N277`：U5.7、R7.1、TP_PWR_INT.1；R7.2=V3V3 |
| 8 | KILL | 直接接 `VSYS/VDD` | VSYS：U5.8 |

PB 到 RESET 不存在外部 PCB 直连。PB 由 TPS3424 内部按键检测和锁存逻辑处理，pin 6 RESET 是内部逻辑输出。

## 3. 时序计算

TI 数据手册第 7.3.1.2 节给出：

```text
tSP 或 tLP（秒）= 0.422 × C（nF）
```

```text
C22 = 120 pF = 0.120 nF
tSP = 0.422 × 0.120 = 50.64 ms

C23 = 4.7 nF
tLP = 0.422 × 4.7 = 1.9834 s
```

SPT/LPT 电容必须靠近 U5。短按/长按实际误差、焊盘寄生和按键机械抖动需用示波器确认。

## 4. 按键与输出逻辑

| 操作 | PB | U5.6 RESET / PWR_STATE | TPS22965 |
| --- | --- | --- | --- |
| PWR 松开 | 内部上拉，高 | 保持当前锁存状态 | 保持当前状态 |
| PWR 按下超过约 50.6 ms | 低 | 置高 | ON 打开，系统上电 |
| PWR 持续约 1.98 s | 低 | 锁存释放为低 | ON 关闭，系统断电 |

SW2 使用四脚常开轻触按键封装。网表连接 `SW2.1→PB`、`SW2.3→GND` 的电气拓扑正确；精确按键 MPN 和封装方向仍需用 BOM 或万用表确认其两侧内部触点关系。

## 5. 首版 BOM

| 位号 | 规格/器件 | 状态 |
| --- | --- | --- |
| U5 | `TPS3424A11C13ADRLR`，SOT-5X3/DRL，8-pin | 用户已确认 |
| C22 | 120 pF ±5%，50 V，0603，库存 `C1643` | 网表值与库存匹配 |
| C23 | 4.7 nF ±10%，50 V，0603，库存 `C53987` | 网表值与库存匹配 |
| C24 | 100 nF，0603 | 网表连接正确；精确 MPN 随整板 BOM |
| R7 | 10 kΩ ±1%，0603，库存 `C25804` | 网表连接正确 |
| SW2 | 四脚常开轻触按键 | 精确 MPN/封装方向待确认 |
| TP_PWR_STATE | 测试点 | 同时观测 PWR_STATE/SYS_EN |
| TP_PWR_INT | 测试点 | 观测 INT 脉冲 |

## 6. 布局与验收门禁

1. U5、C24、C22、C23 紧凑放置；SPT/LPT 走线短，远离 `SW_BQ`、4G 和扬声器功率回路。
2. `PWR_STATE` 直接连接 TPS22965 ON；不得把 PB 线接到 RESET，也不得把 RESET 接 ESP32 EN。
3. 断电确认 SW2.1–SW2.3：松开开路、按下近 0 Ω；确认四脚按键内部 1/2、3/4 两侧连接。
4. 用示波器测 PB、RESET/PWR_STATE、TPS22965 ON、VMAIN 和 IO46，验证短按、长按和掉电。
5. 完成同版 BOM、ERC、网表复核、PCB DRC 和样机测试后，才能称为已验证。

完成嘉立创原理图后，可以提供同版原理图 PDF/截图、BOM 和网表进行复查。网表可证明引脚与网络端点，不能证明 PCB 走线、焊接、热设计或样机性能。
