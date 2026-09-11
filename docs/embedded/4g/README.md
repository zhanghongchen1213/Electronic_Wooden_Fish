# docs/embedded/4g — Air780EGP 4G 轨（设备侧）

> 范围：EWF 唯一 4G 业务链路 = **Air780EGP**（架构 AD-14；UART IO43/44 + DTR IO10/RST IO15/NET_STATUS 兼容位 IO16，引脚以 ARCHITECTURE-SPINE 板级合同为准）。`IO8` 已释放为 `BQ_OTG_EN`，M100 `GNSS_VCC` 留 NC/TP。经验源= `main_control`（HEAD fb458b9，2026-09-08 迁移），其 `components/BSP/GPS/gps.*` 底层即 Air780E 系 AT 固件（`GPS_HTTP_SSL_CONTEXT_ID=153` 直标 Air780EGP）。固件接入实现归 story E4.1/E4.2。

## 文档索引
| 文件 | 内容 |
| --- | --- |
| `Air780EGP-AT联网与HTTPS经验.md` | 主经验：波特率握手不写 NVRAM、AT 恢复与退避、DTR、PDP 链、HTTPS 单事务、GNSS、日志防泄露、「EWF 参数基线表(DEFERRED)」、EWF 自建项 |
| `troubleshooting/Air780EGP-AT零响应与恢复.md` | AT 零响应故障树与恢复/重试语义 |

## EWF 边界
- 整颗 Air780EGP 模组（无 combo/外骨骼封装）；**无 MQTT、无 BLE 业务**；GPS 默认关闭，M100 `GNSS_VCC` 在 EWF 板级留 `NC/TP`，不接 ESP32；`IO16` 无对应载板脚时同样留 `NC/TP`（详见主文档与硬件基线）。
- 单事务串行所有权：UART 是模组唯一命令通道，应用层不得直读写 UART，只能过「有界上报调用」（EWF 仿 `gps_https_post_json` 语义）。

## 代码照搬清单（main_control，供 E4 实现期使用）
- `components/BSP/GPS/gps.c`：`gps_uart_init/set_baud_rate`、`gps_send_at_internal/send_at`(sensitive)、`gps_sync_modem/normalize_modem_baud_rate`、`gps_http_ensure_bearer/wait_action/read_body/execute_https_post/process_http_queue`、`gps_poll_network/poll_gnss`、`gps_recover_modem/prepare_sleep/driver_task`。
- `gps_config.h`：UART/任务/时序/PDP/HTTPS/DTR 数值宏（抄录见主文档 §参数基线表）。
- 公开 API 形态：`start / get_status / https_post_json / suspend`。

## 排除表（main_control 未迁）
| 主题 | 理由 |
| --- | --- |
| BLE(`docs/Bluetooth/`、app_ble/app_blue_control) | EWF 无 BLE 业务 |
| 外骨骼/电机/景区四态授权 payload | EWF 仅单条 HTTPS 业务链 |
| STM32 遥控 UART 协议族 | EWF 用完整 Air780EGP 走 DTR 唤醒，非双机 UART |
| 多型号分区/构建发布/语音/OBS 等 plans | 与 4G 轨无关 |
| 发射峰值/掉压、SIM 实名资费/天线 | main_control 未覆盖 → **EWF 自建项**（见主文档 §EWF 自建） |
