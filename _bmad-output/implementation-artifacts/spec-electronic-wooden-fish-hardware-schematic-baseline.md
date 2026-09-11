---
title: '电子木鱼硬件原理图基线与嘉立创 EDA 落图'
type: 'feature'
created: '2026-09-09'
status: 'ready-for-dev'
route: 'dispatch'
review_loop_iteration: 0
context:
  - '{project-root}/AGENTS.md'
  - '{project-root}/docs/README.md'
  - '{project-root}/_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md'
  - '{project-root}/docs/hardware/电子木鱼-硬件原理图设计基线.md'
  - '{project-root}/docs/hardware/电子木鱼-硬件网络清单.json'
---

<frozen-after-approval reason="用户已明确批准的硬件落图计划；电气接口冻结，样机验证项不得被写成已确认事实">

## Intent

**Problem:** 进入原理图设计前，已确认的主控、4G 载板、屏幕、音频、PVDF、充电和 GPIO 决策分散在架构、参考 PDF 与迁移文档中，无法直接作为 ERC 和嘉立创 EDA 落图输入。

**Approach:** 建立一份中文可读硬件基线和一份机器可校验网络清单，明确“已确认 / 推荐初值 / 必须样机验证”，同步架构 spine 与设备派生文档，并在嘉立创 EDA 建立按电源、主控、显示、音频、传感器、4G 分层的原理图工程。

## Boundaries & Constraints

**Always:** 采用 ESP32-S3-WROOM-1-N16R8、单节受保护锂电、单 USB-C、Air780EGP/M100EG-C2 载板、BQ25895 NVDC/PMID OTG、VBAT_SW/3V3/AUDIO_5V 分轨；GPIO、I²C 地址、FPC 针脚和电源输入输出必须进入网络清单；硬件输出失败不得被文档描述为样机已通过。

**Never:** 不复制参考图的 CH340X、AMS1117、TPS631000、5V 4G 供电或旧 GPIO；不把 BQ SYS 当 4G 主供电；不把 GNSS_VCC 当 ESP32 输出；不并联 BQ D+/D− 与 ESP32 USB；不引入 BLE/Wi‑Fi/GPS 业务链路；不覆盖用户提供的两份 PDF。

## I/O & Edge-Case Matrix

| 场景 | 输入 / 状态 | 预期行为 | 错误处理 |
|---|---|---|---|
| 正常上电 | PWR 短按，电池或 5V VBUS 存在 | TPS3424 锁存、TPS22965 打开，VBAT_SW→3V3；4G 走电池轨 | 记录各轨上电顺序和掉压波形 |
| 充电 | USB-C 5V/2A，电池未满 | BQ25895 充电和 NVDC 供电，固件暂停输入/音频 | 禁止 VBUS 反灌；D+/D−识别异常需可隔离 |
| OTG 音频/LED | BQ PMID 有效，IO8=1 | TPS22919 输出 AUDIO_5V，先等待再拉高 PA_EN | PMID 不足时保持 PA_EN 低，计数链路继续 |
| 4G 发射峰值 | Air780EGP 瞬时约 1.5–2A | M100 VIN 由 VBAT_SW 提供，3.3–4.2V 范围内稳定 | 样机示波器验证，不以仿真替代 |
| PVDF 异常波形 | 过压、负摆幅、连续 1–20 次/秒 | 1M 限流/低漏钳位保护 ADC；IO11 仅作唤醒 | 超阈值、滞回、消抖可换阻值并记录 |
| USB 路由 | 上电、BQ 检测、ESP32 枚举、强制焊盘 | FSW7227 先接 BQ，DSEL/SEL 后切 ESP32 | 强制选路焊盘只能单一路径有效，验证无 VBUS 反灌 |

</frozen-after-approval>

## Code Map

- `docs/hardware/电子木鱼-硬件原理图设计基线.md` -- 人可读电源、GPIO、总线、器件、FPC、网络和开放项事实源。
- `docs/hardware/电子木鱼-硬件网络清单.json` -- 机器可校验的 GPIO/电源/I²C/连接器/BOM/开放项清单。
- `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` -- 跨层不变量与板级合同；必须同步最终 GPIO 和电源边界。
- `docs/embedded/bsp/外设硬件软件二开.md` -- 设备 BSP 的派生引脚/总线/外设说明；只更新与新硬件合同直接相关的段落。
- `docs/embedded/4g/Air780EGP-AT联网与HTTPS经验.md`、`docs/embedded/4g/troubleshooting/Air780EGP-AT零响应与恢复.md` -- Air780EGP AT/DTR/RST 经验；保留软件流程，修正载板信号差异。
- `/Users/hongchenke/Documents/Github/main_control/components/BSP/GPS/gps.h` -- M100EG-C2 原始宏定义的交叉核对来源，不复制其旧 GPIO。
- `docs/hardware/3593896291PY206-W38-V2(2).pdf`、`docs/hardware/SCH_遥控板_双TypeC_4G稳压_2026-09-07.pdf` -- 屏幕针脚与音频/4G 子电路参考资料，PDF 原文件不改写。

## Tasks & Acceptance

**Execution:**
- [ ] 创建 Markdown 基线和 JSON 网络清单，覆盖计划中的全部已确认硬件及开放项。
- [ ] 将最终 GPIO/I²C/电源合同同步到架构 spine、BSP 和 Air780EGP 派生文档，并更新 `docs/README.md` 索引。
- [ ] 对 JSON 运行 GPIO 合法性/重复、I²C 地址冲突、必需电源和连接器字段检查。
- [ ] 在嘉立创 EDA 专业版创建分层原理图工程；若应用自动化受限，保留可直接照清单落图的分层和网络命名记录，并明确未完成部分。

**Acceptance Criteria:**
- Given 网络清单，when 静态检查运行，then GPIO 无重复且仅使用 WROOM 可用脚、I²C 地址无冲突、所有必需电源和关键连接器均有条目。
- Given 文档与 spine，when 交叉检索 GPIO8/16、M100 RST/NET_STATUS/GNSS_VCC 和 USB D+/D−，then 三处口径一致且明确样机验证项。
- Given 原理图工程，when 按分层页检查，then 电源、主控、显示、音频/LED、PVDF、4G、传感器和测试点均有可追踪网络名。

## Implementation Notes

- 用户计划已提供电气接口决策；未定机械尺寸、RST 极性、阈值/滞回、FPC 实物接触方向和天线/电池结构作为开放项记录，不阻塞基线文档。
- `FSW7227YMS10G/TR` 作为 USB MUX 首选 MSOP-10；保留强制选路焊盘与可替代料位。
- `GNSS_VCC` 释放给 `BQ_OTG_EN=IO8`；`NET_STATUS=IO16` 仅保留兼容逻辑位，M100 载板无对应引脚时留 NC/测试点。

## Verification

**Commands:**
- `python3 -m json.tool docs/hardware/电子木鱼-硬件网络清单.json` -- 期望 JSON 解析成功。
- `python3 tools/validate_hardware_manifest.py`（若存在）或等价内联检查 -- 期望 GPIO/地址/必需字段全部通过。
- `git diff --check` -- 期望无空白错误。

**Manual checks:**
- 在嘉立创 EDA 中逐页检查 ERC；在样机阶段用示波器、电子负载和实物插合记录所有 `[必须样机验证]` 项。

