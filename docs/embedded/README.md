# docs/embedded — 设备侧（Embedded/固件）文档

> 范围：EWF `Embedded/` 固件实现规格与排障知识。设备侧代码/框架以 **legbot_watch**（只读蓝本）为基础，本目录多数文档从 `legbot_watch/docs` 迁移/适配而来，每条头部带 provenance（legbot 路径 + HEAD + 判定 + 适配点）。
>
> 引脚/板级唯一权威：`ARCHITECTURE-SPINE.md` §板级合同（GPIO 冲突一律以 spine 为准，先改 spine 再调整）。固件强制规则与坑索引见根 **`AGENTS.md`**。

## 文档索引

| 文件 | 内容（一句话） |
| --- | --- |
| `troubleshooting/LVGL-运行时字体缺字根因与解决.md` | 中文 `□` 缺字五路字形闭合链路 + `font_glyph_contract.json` + 校验命令（EWF UI 必读） |
| `troubleshooting/LVGL-滚动低帧率与快照直传.md` | CO5300 QSPI 带宽→60fps 快照直传架构（滚动/三页横滑） |
| `troubleshooting/ESP32-S3-自动轻睡眠随机重启与USB日志失联.md` | esp_pm/light-sleep 随机重启 + USB-Serial-JTAG 日志失联 + `NO_AUTO_LS_ON_CONNECTION` |
| `troubleshooting/NS4150B-点击音启动时序.md` | NS4150B PA_EN 启动安定 150ms / enable→delay→PCM→drain→off |
| `troubleshooting/CO5300-熄亮屏闪屏与整帧时序.md` | 闪屏根因=复位后 GRAM 未定义即 DISPON；先 DISPOFF→整帧→DISPON |
| `guides/低功耗策略与实测验收.md` | 全链路低功耗方法学（证据分级 IMPLEMENTED/PASS/DEFERRED、CO5300/CST9217/USJ 锚点） |
| `guides/真机闭环runbook-macos.md` | macOS ESP-IDF 真机编译/烧录/监控/取证 runbook 模板 |
| `bsp/外设硬件软件二开.md` | 板级 GPIO/总线总表 + 每外设（CO5300/CST9217/CW2015/ES8311/NS4150B/PVDF/按键/USJ）「数据手册事实 + 已知坑」（按 EWF spine 引脚重写） |
| `style/C编码规范-Agent版.md` | EWF 设备侧 C 编码强制基线（显力科技 Agent 精简版） |

> 补充：**`4g/`** —— Air780EGP 4G 轨经验（源 `main_control`，HEAD `fb458b9`），含 README + `Air780EGP-AT联网与HTTPS经验.md` + `troubleshooting/Air780EGP-AT零响应与恢复.md`，见 [`4g/README.md`](4g/README.md)。

## 迁移来源与判定（legbot HEAD 41a5ab8b9，2026-09-08）

| legbot 源 | EWF 目标 | 判定 |
| --- | --- | --- |
| `docs/troubleshooting/LVGL运行时字体缺字问题根因与解决方案.md` | `troubleshooting/LVGL-运行时字体缺字根因与解决.md` | 搬(词表适配) |
| `docs/troubleshooting/LVGL滚动低帧率根因与快照直传解决方案.md` | `troubleshooting/LVGL-滚动低帧率与快照直传.md` | 搬(词表适配) |
| `docs/troubleshooting/ESP32-S3自动轻睡眠随机重启与USB日志失联根因与解决方案.md` | `troubleshooting/ESP32-S3-自动轻睡眠随机重启与USB日志失联.md` | 搬(去 LTE 专属) |
| `docs/troubleshooting/audio_click_sound_root_cause_and_solution.md` | `troubleshooting/NS4150B-点击音启动时序.md` | 搬(词表适配) |
| `docs/guides/手环全链路低功耗策略与实测验收.md` + `docs/hardware/…二开.md` | `troubleshooting/CO5300-熄亮屏闪屏与整帧时序.md` | 适配·提炼 |
| `docs/guides/手环全链路低功耗策略与实测验收.md` | `guides/低功耗策略与实测验收.md` | 适配 |
| `docs/guides/macos_esp_idf_hardware_test_runbook.md` | `guides/真机闭环runbook-macos.md` | 适配 |
| `docs/hardware/ESPWatch-S3-4G_硬件软件二次开发文档.md` | `bsp/外设硬件软件二开.md` | 适配(引脚按 spine) |
| `docs/xlkj/显力科技嵌入式C代码编写规则-Agent版.md` | `style/C编码规范-Agent版.md` | 搬(原样) |

## 排除表（legbot 文档，未迁移）

| legbot 文件/主题 | 理由 |
| --- | --- |
| `contracts/BLE_Protocol.md` | EWF 无外骨骼 BLE（MVP 非目标） |
| `contracts/cloud_api_contract.md` | EWF 无独立业务云（backend 自建） |
| `guides/GPS运行状态与云端上报策略.md`、`troubleshooting/L76KB-A58_GPS…md` | EWF GPS 默认关闭、非 MVP 业务 |
| `guides/ML307R_4G联网与低功耗退避策略.md`、`troubleshooting/ML307R…*.md` | legbot 用 ML307R；EWF 4G=Air780EGP（AT/HTTPS 经验另源 `main_control`，待单独迁移） |
| `guides/BOOT0按键离线语音控制…md` | EWF 无离线语音需求 |
| 外骨骼/步数/支付/场景切换等 watch 业务文档段 | EWF 无对应产品模块；治理范式可借鉴但内容不迁 |

## 工具与两层分离（简述；细则见根 AGENTS.md）

- SquareLine 导出 C 落 `Embedded/components/ui/generated/`（只布局/字体/事件空桩）；业务在 `bindings/`；LVGL 仅 `ui_task` 独占调用（规则见 `AGENTS.md` §设备侧强制规则）。
- 工具链：`lvgl-design/squareline_studio/tools/{generate_squareline_project,postprocess_squareline_export,validate_squareline_project,validate_font_coverage}.py`（照搬 legbot，现为 legbot 定向，待 EWF html/字体契约生成后重定向——见 `lvgl-design/README.md` 与 UX 运行目录 `.memlog.md`）。
