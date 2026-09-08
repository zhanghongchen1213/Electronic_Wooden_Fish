---
name: Electronic_Wooden_Fish
description: "设备 OLED 轨页面、状态与组件闭包。"
type: ui-contract
surface: device
status: draft
created: 2026-09-08
updated: 2026-09-08
sources:
  - "{planning_artifacts}/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/DESIGN.md"
  - "{planning_artifacts}/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/EXPERIENCE.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
  - "{planning_artifacts}/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
authority: "UX spines (DESIGN.md / EXPERIENCE.md) > 本契约 > pen/HTML > SquareLine generated C"
---

# UI_CONTRACT · 设备轨（OLED 410×502 / LVGL 8.4）

> 本契约把 UX 契约落到**可校验闭包**：凡页面/状态/组件未入下表，或未以 `[UI]…` 帧画入 HTML，即视为设计未完成。SquareLine 导出边界：**`generated/` 只放布局/字体/事件空桩；业务渲染/路由/typed intent 在 `bindings/`；LVGL 仅 `ui_task` 独占**。

## 1. 几何与画布

| 项 | 值 | 依据 |
| --- | --- | --- |
| 画布 | 410×502（QSPI AMOLED，CO5300） | AD/legbot 形态 |
| 画布圆角 | 110 | `rounded.device.canvas`；legbot 同款 |
| 屏幕边距 | 16 | `spacing.device.screen_edge` |
| 关键净空 | ≥8 | `spacing.device.critical_clearance` |
| 状态栏高 | 24 | `components.device.statusbar.height` |
| 触区 | 木鱼页电子木鱼为唯一可敲触区（≥96×96）；滑动手势全区 | FR-E-007 |

## 2. 命名规则（pen / HTML / SquareLine 对象共用）

`[UI][PAGE:<PageId>][ST:<StateId>][CMP:<CompId>][VAR:<Name>]`

- PageId：`MUYU | JINGWEN | TONGJI | SHEZHI | SHELL | OVERLAY`
- StateId：见 §4 状态闭包；CompId 只能使用 §5 的 canonical 名称；VAR 为值占位（如 `<vol>`、`<today>`）。
- 状态栏不单独生成顶层屏幕；同步/充电/低电/故障以页面内 `statusbar` 的 `VAR` 标注。

## 3. 页面闭包表（pen/HTML 必须各有帧；绑定 epics E3 story）

| PageId | 页面 | 作用 | 帧(ST) | 绑定 |
| --- | --- | --- | --- | --- |
| MUYU | 木鱼页 | 7字带+心经进度+今日敲击+累计敲击+电子木鱼 | BASE, EMPTY, MID, FULL, DONE_OVERLAY, CHARGING_PAUSE, UNTRUSTED_TIME | FR-E-006/007；E3.2/3.3/3.9 |
| JINGWEN | 经文页 | 大号当前字+已确认前后文+心经进度百分比 | BASE | FR-E-006；E3.4 |
| TONGJI | 统计页 | 今日敲击+累计敲击 | BASE, UNTRUSTED_TIME | FR-E-006/009；E3.5 |
| SHEZHI | 设置页(下滑) | 亮度/熄屏/音量/立即同步 | BASE, SYNC_BUSY, SYNC_OK, SYNC_PENDING, SYNC_FAIL | FR-E-007；E3.7 |
| SHELL | 全局 | 状态栏（常驻于 MUYU/JINGWEN/TONGJI 帧内） | `VAR:sync-ok/pending/busy/fail/charging/lowbatt/fault/untrusted` | FR-E-008/009；E3.1/3.8 |
| OVERLAY | 完成遮罩 | 无礼花；遮罩+从头开始/退出 | DONE | FR-E-003/005；E3.9 |

## 4. 状态闭包（帧清单 = 页面 × 状态）

设备在状态栏用点+短字；正文帧覆盖以下（每帧独立 HTML 块）：

```
[MUYU][BASE]     典型亮屏：7字带已诵若干、心经进度字数+百分比、今日敲击、累计敲击、状态栏
[MUYU][EMPTY]    当前诵读起始：7字带全空位（低对比占位，不预览未来字）
[MUYU][MID]      当前诵读中途：带首行微满、标点随附、进度百分比
[MUYU][FULL]     7字带满位并继续左移（不代表心经完成）
[MUYU][DONE_OVERLAY] 完成遮罩浮层 + 从头开始/退出
[MUYU][CHARGING_PAUSE] 充电中（输入暂停提示为轻点，非弹窗）
[MUYU][UNTRUSTED_TIME] 未校时：今日区显「待校时」
[JINGWEN][BASE]  大号当前字 + 已确认前后文 + 心经进度百分比
[TONGJI][BASE]   今日敲击/累计敲击大数
[TONGJI][UNTRUSTED_TIME] 今日显「待校时」
[SHEZHI][BASE]   三档亮度·5/15/30s·音量50·立即同步（默认值态）
[SHEZHI][SYNC_BUSY] 立即同步进行中
[SHEZHI][SYNC_OK]    立即同步成功
[SHEZHI][SYNC_PENDING] 待同步/未连接
[SHEZHI][SYNC_FAIL] 立即同步失败
[SHELL][VAR:signal-4g-connected] 4G 有信号
[SHELL][VAR:signal-4g-no-signal] 4G 无信号
[SHELL][VAR:signal-4g-disabled] 4G 未启用
[SHELL][VAR:signal-wifi-connected] Wi-Fi 有信号
[SHELL][VAR:signal-wifi-no-signal] Wi-Fi 无信号
[SHELL][VAR:signal-wifi-disabled] Wi-Fi 未启用
[SHELL][VAR:signal-ble-connected] 蓝牙有信号
[SHELL][VAR:signal-ble-no-signal] 蓝牙无信号
[SHELL][VAR:signal-ble-disabled] 蓝牙未启用
[SHELL][VAR:signal-gps-connected] GPS 有信号
[SHELL][VAR:signal-gps-no-signal] GPS 无信号
[SHELL][VAR:signal-gps-disabled] GPS 未启用
[SHELL][VAR:sync-ok] 已同步点+字
[SHELL][VAR:sync-pending] 待同步点+字
[SHELL][VAR:sync-busy] 同步中点+字
[SHELL][VAR:sync-fail] 同步失败点+字
[SHELL][VAR:charging] 充电中
[SHELL][VAR:lowbatt] 低电量
[SHELL][VAR:fault] 故障
[SHELL][VAR:untrusted] 待校时
```

上述页面状态展开后共有 15 个设备顶层帧；`SHELL` 行是帧内变体，不另计顶层帧。7 字带容量只表示可见窗口，不是诵读字数上限。

> 熄屏/首触唤醒以「低亮空帧 + 说明」单帧表达（验证存在即可，不画复杂状态）。统计近7/30/连续不在设备。

## 5. 组件清单（CompId → DESIGN 令牌）

| CompId | 组件 | 令牌/规格 |
| --- | --- | --- |
| statusbar | 顶部状态栏 | 透明/`{colors.device.surface.0}`；左侧 connectivity-rail，右侧 battery-status，同步状态轻量呈现 |
| connectivity-rail | 信号组件容器 | 固定承载 4G/Wi-Fi/蓝牙/GPS 四个 signal-status；每项支持 connected/no-signal/disabled |
| signal-status | 单项信号 | 图标与线条/短文案双编码；图标可见不代表业务链路启用 |
| battery-status | 电池组件 | 右侧电池符号+百分比；充电/低电量有独立图标或文案 |
| charcell | 7字带字形槽 | 字形同槽位；已诵=`{typography.device.char}`/`{colors.device.text.primary}`，空位=`{typography.device.placeholder}`/`{colors.device.text.muted}` |
| glyph-current | 当前字 | `{typography.device.glyph}`；关键帧可用 `{colors.brand.amber.400}` 微亮当前字 |
| scripture-progress | 心经进度 | `{rounded.device.pill}`；显示 `confirmed_chars / scripture_chars_total · percent%`；达到 100% 后保持 N/N |
| today-taps | 今日敲击 | `{typography.device.body}`；文案使用“今日敲击 N 次”，未校时显示“待校时” |
| total-taps | 累计敲击 | `{typography.device.value}`；跨所有 `round_id` 保留，不随从头开始清零 |
| round-index | 当前诵读标识 | 仅辅助显示“第 N 次诵读/进行中/已完成”，不表达字数上限 |
| woodfish | 电子木鱼 | 单线檀木（`{colors.brand.sandal.600}`）；敲击光晕 `{colors.brand.amber.300}` 一次淡出 |
| row | 设置行 | 图标/标题/值/箭头 |
| slider | 设置滑杆 | 轨道=`{colors.device.surface.2}`；旋钮=`{colors.brand.amber.600}` |
| sync-btn | 立即同步 | 四态文案/点色 |
| modal-done | 完成遮罩 | `components.device.modal-done`；末字确认后锁定输入，提供“从头开始/退出” |

## 6. SquareLine 导出边界与 Screen 建议（St5 落实）

- 推荐 Screen：`screen_shell`（含 statusbar + 三页容器左右滑）、`screen_settings`（下滑）、`screen_done_modal`（overlay）。
- 导出落 `Embedded/components/ui/generated/`；父 CMake 读 `filelist.txt`；`compat/lvgl/lvgl.h` 转发到 ESP-IDF lvgl；业务在 `bindings/`。
- 字体：**只嵌入该部《心经》可消费汉字 + 本页 UI 字串子集**（Noto Sans SC 500/600/700 + 数字）；`generated/fonts` 与 SquareLine 资产字节一致；禁手改 .c 位图。

## 7. FR / story 校验
- 覆盖 FR-E-006/007/009 全部页面态；FR-E-003/005/008 由状态/反馈帧覆盖；对照 epics E3.1~E3.9。
