---
title: 电子木鱼系统架构
status: final
created: 2026-09-08
updated: 2026-09-08
---

# 电子木鱼系统架构

## 1. 文档定位

本文件是电子木鱼的实现架构单一事实源，承载硬件拓扑、引脚分配、电源、总线、USB 调试、音频、屏幕、网络和数据流设计。产品愿景、用户行为和分层功能需求位于 [PRD](./_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md)；产品简报不承载本文件的电路细节。

本项目是个人开发原型，只维护一台设备和一个固定 backend，不设计量产认证、多设备、换机迁移、复杂账号、CI/CD 或运维体系。

## 2. 分层边界

```text
Embedded
  设备电源、PVDF敲击识别、累计数持久化、屏幕/触摸、音频、4G活动窗口同步、USB调试

cloud/backend
  微信身份映射、设备数据接收、去重与增量计算、心经游标权威、SQLite持久化、WebSocket推送

cloud/frontend
  uni-app微信小程序、诵经动画、离线回放、完成弹窗、记录页、设备页和操作入口
```

跨层原则：Embedded 只产生可靠的本地计数和设备状态；backend 是经文游标、完成状态、统计和音频配置的权威；frontend 负责呈现和发起用户操作，不自行篡改正式进度。

## 3. 硬件拓扑

### 3.1 主控与模块

- **主控：** ESP32-S3-N16R8（16 MB Flash / 8 MB PSRAM 型号，最终以采购封装数据手册为准）。
- **蜂窝通信：** Air780EGP 完整模组组件（沿用 `main_control` 已验证带天线设计），通过 UART（IO43/44）与 ESP32-S3 连接，采用 AT/HTTPS 模式，是 MVP 唯一联网链路；GPS 保留控制能力但默认关闭；小程序关闭时不保持网络会话。
- **主敲击传感：** PVDF 压电薄膜；QMI8658C 不参与 MVP 主计数，仅保留扩展感知能力。
- **电量计：** CW2015，读取电池电压和电量百分比。
- **触摸：** CST9217，提供电容触摸中断和 I2C 读写。
- **显示：** CO5300 兼容 QSPI AMOLED，参考用户提供的约 2.06 英寸、410×502 模组；最终供电和 FPC 版本必须以供应商数据手册复核。
- **音频：** ES8311 音频编解码器 + NS4150B 单声道 D 类功放 + 扬声器；高速连击时由固件合并为节奏音效。
- **下载/调试：** ESP32-S3 原生 USB-Serial-JTAG；CH340X 不上板。
- **结构：** 单一 3D 打印、拼接、喷涂外壳；只为最终外壳做一次传感器和声学标定，不做多外壳运行时适配。

### 3.2 引脚分配

| 功能 | 器件/信号 | ESP32-S3 GPIO | 约束/说明 |
| --- | --- | ---: | --- |
| BOOT 按键 | BOOT | IO0 | 启动绑带；按下拉低进入下载流程，默认保持高 |
| PWR 按键 | PWR | IO46 | 启动绑带；板级电源电路负责硬断电/上电，复位采样时必须满足默认低电平 |
| 独立复位 | EN/RESET | EN 测试点/按键 | 与 PWR 分离，用于首烧、复位和调试；具体 RC 按 ESP32-S3 参考设计实现 |
| PVDF 主传感 | 模拟输入 | IO9（工程建议） | ADC1_CH8，需高阻保护/钳位和最终板级复核；不与 USB 或启动绑带复用 |
| PVDF 比较器 | 比较器唤醒输出 | IO11 | 低功耗比较器唤醒候选；料号/阈值按原理图与实测确认 |
| 电量计 | CW2015 SDA/SCL | IO1 / IO2 | 与 CST9217、ES8311、QMI8658C 共享 I2C |
| 触摸 | TP_RST / TP_INT | IO38 / IO39 | TP_RST 低有效；TP_INT 作为触摸中断 |
| 蜂窝（Air780EGP） | UART TX/RX | IO43 / IO44 | AT/HTTPS 串口，沿用 main_control 已验证模组 |
| 蜂窝（Air780EGP） | DTR | IO10 | 休眠控制 |
| 蜂窝（Air780EGP） | RST | IO15 | 模组复位 |
| 蜂窝（Air780EGP） | NET_STATUS | IO16 | 网络状态指示 |
| 蜂窝（Air780EGP） | GNSS_VCC | IO8 | GPS 供电控制，默认关闭 |
| IMU | QMI8658C INT1（仅） | IO41 | 上板默认不启用，仅作未来扩展；INT2 不接、不占用 GPIO45 |
| AMOLED | LCD_RST / QSPI_CS / QSPI_SCL / QSPI_D0~D3 / LCD_EN | IO4 / IO40 / IO5 / IO6 / IO7 / IO12 / IO42 / IO47 | QSPI 屏幕控制；电源脚按最终 FPC 数据手册核对 |
| 音频编解码 | ES8311 I2C | IO1 / IO2 | 配置接口 |
| 音频数据 | I2S_DO / WS / DI / BCLK / MCLK | IO13 / IO14 / IO17 / IO18 / IO21 | ESP32-S3 作为 I2S 主控的候选方案；时钟主从关系在音频联调时确认 |
| 功放 | NS4150B PA_EN | IO48 | 独立使能，静音时关闭或保持禁用 |
| RGB | RGB LED 数据 | IO3 | 状态反馈 |
| USB | D− / D+ | IO19 / IO20 | 原生 USB-Serial-JTAG；不得被其他外设占用 |

GPIO45 保留为空或按 ESP32-S3 VDD_SPI 启动要求处理；不得再接 QMI8658C INT2。GPIO33–37 在 N16R8 型号上通常与 Octal Flash/PSRAM 相关，不作为通用 GPIO 使用。

## 4. USB、BOOT、PWR 与调试

### 4.1 结论

ESP32-S3 内置 USB-Serial-JTAG，可通过 GPIO19（D−）和 GPIO20（D+）完成：

- `esptool.py` / `idf.py flash` 固件烧录；
- USB CDC 串口日志；
- OpenOCD + GDB JTAG 调试。

因此不需要 CH340X。需要连接 USB VBUS、GND，并保留 BOOT 与 EN/RESET。第一次烧录可按住 BOOT 后按 RESET/上电进入下载；后续由 USB-Serial-JTAG 和复位电路完成自动下载流程。[Espressif USB-Serial-JTAG 文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html)、[ESP32-S3 内置 JTAG 文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/configure-builtin-jtag.html)

### 4.2 启动绑带约束

GPIO0、GPIO45、GPIO46 是 ESP32-S3 启动采样相关引脚。下载模式要求 GPIO0=0、GPIO46=0；GPIO45 影响 VDD_SPI 电压。PWR 按键和 QMI8658C 输出都不得在复位采样窗口把这些引脚推到错误电平。[ESP32-S3 数据手册](https://documentation.espressif.com/esp32_s3_datasheet_en.pdf)

USB-Serial-JTAG 与 USB-OTG 共用内部 PHY。本项目固定 USB-Serial-JTAG，不在同一 USB 口上开发 USB-OTG 应用；否则必须增加外部 PHY 或切换设计。

## 5. 电源设计

### 5.1 分轨原则

- **Air780EGP 电池轨：** 直接使用单节锂电池或等效 3.3–4.3V 高电流轨。合宙资料要求持续供电能力超过 1A、瞬时能力超过 2A；电源走线、去耦和储能电容按峰值设计。
- **ESP32/低压外设轨：** 为 ESP32-S3、CW2015、CST9217、QMI8658C、CO5300、ES8311 提供独立 3.3V 轨。
- **音频轨：** NS4150B 使用独立受控电源，按扬声器阻抗和目标音量决定 3–5V 供电方式。
- **AMS1117：** 不得作为 Air780EGP 主供电；如保留，只能给低电流外设，优先换成低静态电流、适合电池的稳压器。
- **充电：** USB-C 充电期间暂停敲击；总电源关闭时不得通过 USB VBUS 反向给 Air780EGP 或电池造成非预期供电。

### 5.2 低电量行为

当前产品决策是继续 4G 工作直到设备关机，不做低电量主动断网；因此必须优先保证累计计数在掉压/掉电前持久化，并在 CW2015 电量低时通过 RGB、AMOLED 和小程序提示。

## 6. 共享总线与器件约束

### 6.1 I2C

CW2015、CST9217、ES8311 和 QMI8658C 共用 IO1/IO2。预计地址不同，但最终原理图必须执行总线扫描和数据手册核对：

- CW2015：固定 7-bit 地址约为 `0x62`；
- QMI8658C：由 SA0 选择 `0x6A` 或 `0x6B`；
- CST9217：默认地址字节 `0xB4/0xB5`，对应 7-bit `0x5A`；
- ES8311：地址由 CE/CDATA 配置，按最终芯片手册确认。

只保留一组总线主上拉，确认所有器件 I/O 电压兼容、上拉阻值与总线电容满足目标速率。QMI8658C 仅接 INT1（IO41）、INT2 不接；IO8 用作 Air780EGP GNSS_VCC，GPIO45 不占用。

### 6.2 音频

ES8311 通过 I2C 配置、I2S 传输音频；NS4150B 负责功率放大。音效播放失败不得影响敲击计数、本地持久化或同步。高速连击时合并音频触发，避免 20 次/秒的音频重叠变成噪声。

### 6.3 屏幕与触摸

CO5300 使用 QSPI；CST9217 使用共享 I2C 和独立复位/中断。屏幕产品行为：

- 小程序关闭时，设备离线敲击后短暂显示本地当前字、累计数和离线状态；
- 触摸支持音量、亮度、熄屏、立即同步和完成弹窗的“从头开始/退出”；
- 屏幕与小程序同时操作时最后一次操作生效；
- 《心经》完成时两端同步显示礼花和弹窗。

图片中电商页面的屏幕电源标注只能作为选型参考，不能替代 CO5300 具体 FPC 版本的电源、时序、背光/偏压和触摸盖板资料。

## 7. 网络与同步模型

### 7.1 网络链路与活动窗口

MVP 只配置 Air780EGP 的 4G（HTTPS JSON）一条联网链路；ESP32-S3 的 BLE/Wi‑Fi 作为硬件能力保留，不作为产品通道、不参与同步或唤醒。GPS 保留控制能力但默认关闭。

设备默认处于低功耗离线记录：PVDF 敲击、`device_touch` 与设置页“立即同步”都会把设备带进一个有限的活动窗口，在窗口内复用 Air780EGP 的 HTTPS 上下文，把本地高水位与设备状态一次性上报给 backend，并取回确认、最新差量与待设备应用命令；上报完成即回到低功耗，不维持长连接或常开无线。

设备与 backend 之间采用“活动窗口 + 幂等高水位”而非常开会话：重复提交同一累计数不重复计数；弱网或 backend 不可用时设备继续本地记录，网络恢复后在下一次活动窗口补传。离线期间 backend 无法直接唤醒设备——小程序设置的命令先由 backend 按 `command_revision` 持久化，设备在下一次 HTTPS 活动中取得并应用、回 ACK；离线时前端显示“待设备应用”。

### 7.2 同步数据模型

设备内部维护两个高水位：

- `local_total`：本地累计有效敲击数；
- `acked_total`：backend 已确认的累计数。

设备不向用户暴露复杂事件序号。每次同步传送当前累计数及必要状态，backend 根据差值计算新增敲击数；重复提交相同累计数不重复计数。待同步增量上限为 1000，达到上限后拒绝新敲击并提示。

设备与 backend 同时保存固定《心经》的可消费汉字序列和 `scripture_version`。设备还保存 `round_state`、`round_cursor` 和单调递增的 `command_revision`，用于离线显示、完成锁定和离线触摸“从头开始”。backend 收到同步包后以累计数和命令修订号恢复正式游标；经文版本不一致时停止推进并报告配置错误，不自动映射到另一版文本。

同步包至少包含：`device_id`、`local_total`、`acked_total`、`scripture_version`、`round_state`、`round_cursor`、`command_revision`、`battery_percent`、`network_mode`、`audio_config_version`、`firmware_version`。设备不保存绝对敲击时间；backend 使用接收/确认时间，将离线增量归入同步当天。

AMOLED 离线显示使用设备内置的《心经》汉字序列和字形资源。MVP 只需包含这部经文所需的字形，不引入通用中文字体库或动态经文下载。

### 7.3 Cloud 接口

- **设备 → backend：** Air780EGP HTTPS JSON 短连接批量同步，在活动窗口内至少一次投递；backend 按设备身份和累计高水位幂等处理，返回最新确认高水位、差量与待设备应用命令。
- **backend → frontend：** WebSocket 推送 `delta_count`、当前字、游标、轮次、完成状态、同步状态和设备状态；frontend 断线后经查询接口补齐。
- **frontend → backend：** 微信登录、音量/亮度/熄屏/立即同步设置、从头开始和退出完成状态；backend 保存最新 `command_revision`，设备离线时命令进入待设备应用。
- **命令下发生效：** 设备不常开无线、无法被直接唤醒；命令在设备下一次 HTTPS 活动窗口内按修订号应用并回 ACK，旧命令不得覆盖新命令。

## 8. 软件工程基线

### 8.1 Embedded

- ESP32-S3 固件采用 ESP-IDF；USB-Serial-JTAG、I2S、QSPI、BLE/Wi-Fi、I2C 和低功耗能力均由 ESP-IDF 管理。
- Air780EGP 采用 AT 模式，通过 UART 由 ESP32-S3 控制；不在本项目引入 Air780EGP 独立业务云。
- PVDF 只保存事件元数据，不保存原始波形；QMI8658C 保留扩展读取入口。

### 8.2 cloud/frontend

沿用喵呜项目的 uni-app（Vue 3 + TypeScript + Vite）和 HBuilderX 调试方式，仅构建微信小程序。参考基线：[miaowu/frontend/package.json](</Users/hongchenke/Documents/Github/miaowu/frontend/package.json>)。

### 8.3 cloud/backend

沿用喵呜项目的 Spring Boot 3.3.7 + Java 17 基线，但不沿用 PostgreSQL 业务模型。个人原型使用 backend 内部单文件 SQLite（建议路径 `cloud/backend/data/wooden-fish.sqlite`），保存设备高水位、经文游标、完成轮次、音频配置、状态和统计；不部署独立数据库服务。参考基线：[miaowu/backend/pom.xml](</Users/hongchenke/Documents/Github/miaowu/backend/pom.xml>)。

本地开发沿用用户指定脚本语境：[start-local-test-backend.sh](</Users/hongchenke/Documents/Github/miaowu/env-scripts/start-local-test-backend.sh>)；生产构建沿用 [build-prod-backend.sh](</Users/hongchenke/Documents/Github/miaowu/env-scripts/build-prod-backend.sh>)。CI/CD、宝塔和云端运维不在本项目文档中重新定义。

## 9. 工程验证门禁

### 9.1 芯片与启动

- 原生 USB 完成烧录、串口日志和 OpenOCD/JTAG；不安装 CH340X 仍可工作。
- BOOT + RESET 进入下载；PWR 硬断电/上电不破坏启动绑带。
- GPIO45 在所有上电样本保持正确 VDD_SPI 采样；QMI8658C 仅接 INT1（IO41）、INT2 不接，IO8 供 Air780EGP GNSS_VCC。
- ESP32-S3-N16R8 的 Flash/PSRAM 保留脚未被复用。

### 9.2 总线与外设

- 上电 I2C 扫描确认 CW2015、CST9217、ES8311、QMI8658C 地址不冲突。
- CO5300 QSPI、CST9217 触摸、ES8311/NS4150B 音频和 CW2015 电量读取分别通过独立冒烟测试。
- PVDF 单次与 1 秒 20 次连续敲击均能稳定生成本地计数；完成弹窗期间输入被忽略。

### 9.3 功耗与联网

- Air780EGP 发射峰值下电池轨无掉压重启；AMS1117 不承担 4G 主供电。
- 小程序关闭或无网络时设备离线记录，网络恢复后在活动窗口补传；仅 Air780EGP 4G 链路，无 BLE/Wi‑Fi 会话回退。
- 离线积压重新打开后先快速逐字回放，再无缝接续实时敲击。

### 9.4 Cloud 与前端

- backend 重启后 SQLite 中的累计数、游标、历史统计和音频配置保持不变。
- 重复同步包只增加一次；WebSocket 重连不重复动画。
- 《心经》完成后礼花/弹窗出现；“从头开始”只重置游标；“退出”保留完成状态；弹窗期间敲击不计数。

## 10. 明确不做

- 不使用 CH340X；不在同一 USB PHY 上实现 USB-OTG 应用。
- 不做多设备、解绑迁移、换机、公开账号、社交、排行榜、提醒、付费和多端前端。
- 不做自动 OTA；固件升级策略留待个人原型稳定后单独设计。
- 不做 QMI8658C 驱动的 MVP 敲击主算法，不保存原始波形。
- 不把具体芯片/电源/屏幕 FPC 的未核验参数写成产品承诺；未通过供应商数据手册和上电实测前不得冻结原理图。
