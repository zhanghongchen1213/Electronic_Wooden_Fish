---
input: "brief-Electronic_Wooden_Fish-2026-09-07/brief.md + addendum.md + 本轮硬件讨论"
target: "prd-Electronic_Wooden_Fish-2026-09-07/prd.md + ../../../../architecture.md"
date: 2026-09-08
status: reconciled
---

> **2026-09-09 硬件基线覆盖说明：** 后续批准的 `spec-electronic-wooden-fish-hardware-schematic-baseline.md` 与 `docs/hardware/电子木鱼-硬件网络清单.json` 覆盖本文中的历史 GPIO/电源草案；本文保留为对账记录，不作为当前落图依据。

# 输入对账：电子木鱼 PRD 重构

## 已保留的产品主线

- 个人桌面原型，实体敲击连接《心经》逐字推进、离线记录和历史回放。
- 小程序打开时实时显示；小程序关闭时设备离线累计，重新打开后快速逐字回放。
- 一台设备、一个固定 backend、一个微信身份；不做量产、多设备、社交、提醒和商业化。
- 完成弹窗、从头开始/退出、音频默认静音、屏幕短显和 BLE/Wi‑Fi/4G 优先级均按本轮用户确认落入 PRD。

## 已处理的旧版问题

1. 原先混合的 FR-1～FR-17 已按 `FR-E`、`FR-B`、`FR-F` 三层重写，并拆开跨层责任。
2. GPIO、芯片、电源、USB、I2C、音频和屏幕引脚从 PRD/addendum 主文移到根目录 `architecture.md`。
3. 自动切换旧规则被更正为“《心经》完成后停在末字，礼花/弹窗期间忽略敲击；用户选择从头开始或退出”。
4. 原先的 PostgreSQL 依赖被更正为 backend 内部单文件 SQLite，部署运维不纳入产品 PRD。
5. 本轮新增的 ESP32-S3-N16R8、CW2015、QMI8658C、CO5300、CST9217、ES8311、NS4150B、原生 USB-Serial-JTAG 和 RESET 路径已写入架构文档。

## 保留为工程门禁的内容（历史对账；当前细节以硬件基线为准）

- GPIO45 启动绑带约束保留；QMI8658C 仅接 INT1→IO41，INT2 不接；IO9 为 PVDF ADC，IO8 已释放为 BQ25895 `BQ_OTG_EN`。
- Air780E 需要独立高电流电池轨，AMS1117 不承担 4G 主供电。
- CO5300 FPC 电源/时序、共享 I2C 实际地址、PVDF 模拟前端和音频电源仍需按最终器件资料与上电实测核验。
- USB 原生下载/JTAG 可去掉 CH340X，但必须保留 GPIO0、EN/RESET、VBUS/GND 和正确启动绑带时序。
