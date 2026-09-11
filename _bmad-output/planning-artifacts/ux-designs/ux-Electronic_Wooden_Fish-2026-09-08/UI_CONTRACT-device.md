---
name: Electronic_Wooden_Fish
description: "设备 OLED 轨页面、状态与组件闭包。"
type: ui-contract
surface: device
status: draft
created: 2026-09-08
updated: 2026-09-11
selected_style: DEVICE-01
source_master: hbTEa
sources:
  - "{planning_artifacts}/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/DESIGN.md"
  - "{planning_artifacts}/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/EXPERIENCE.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
  - "{planning_artifacts}/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
style_candidates: 10
authority: "UX spines (DESIGN.md / EXPERIENCE.md) > 本契约 > pen/HTML > SquareLine generated C"
---

# UI_CONTRACT · 设备轨（OLED 410×502 / LVGL 8.4）

> 本契约把 UX 契约落到**可校验闭包**：凡页面、状态或组件未列入下表，或未以 `[UI]…` 帧画入 HTML，均视为设计未完成。SquareLine 导出边界：**`generated/` 只放布局/字体/事件空桩；业务渲染/路由/typed intent 在 `bindings/`；LVGL 仅 `ui_task` 独占**。

## 1. 几何与画布

| 项 | 值 | 依据 |
| --- | --- | --- |
| 画布 | 410×502（QSPI AMOLED，CO5300） | AD/legbot 形态 |
| 画布圆角 | 110 | `rounded.device.canvas`；legbot 同款 |
| 屏幕边距 | 16 | `spacing.device.screen_edge` |
| 关键净空 | ≥8 | `spacing.device.critical_clearance` |
| 状态栏高 | 24 | `components.device.statusbar.height` |
| 触区 | 木鱼页电子木鱼为唯一可计数触区（≥96×96）；经文页实体 PVDF 仍可推进；普通屏幕触摸只作滚动/导航 | FR-E-007 |

设备三主页由 `screen_shell` 常驻主壳承载：视口 410×502，横向 pager 内容宽 1230；`MUYU/JINGWEN/TONGJI` 根帧分别位于 0/410/820。设置页是按需加载的独立 410×502 Screen。

当前 Pencil 状态栏 master 的节点 ID 是 `VphYz`，逻辑名为 reusable `cmp_statusbar_gGgAm`；`gGgAm` 仅是沿用的组件名后缀，不是另一个节点或实例。master 外框 `x=48,y=20,w=314,h=24`，内边距 ≥8；四个信号 slot 依次位于左侧，间距 6–8px；同步状态居中；电池值与电池符号位于右侧，并保留充电/低电量状态变体。每个信号位置只保留一个图标节点，connected/no-signal/disabled 由运行时颜色或图标补丁切换，不复制隐藏节点。所有子节点必须完整落在 master 与 410×502 圆弧屏安全区内。木鱼主视觉由 reusable `cmp_woodfish_mark` 使用 `lvgl-design/assets/woodfish-reference.png` 承载；`woodfish-anatomy` 是 `woodfish` 内的非交互器物子组件，不得替换为与参考轮廓无关的图形。当前母版来自 `hbTEa`（`DEVICE-01`），full frame 不添加外层展示板或额外背景。

## 2. 命名规则（pen / HTML / SquareLine 对象共用）

`[UI][PAGE:<PageId>][ST:<StateId>][CMP:<CompId>][VAR:<Name>]`

- PageId：`MUYU | JINGWEN | TONGJI | SHEZHI | SHELL`
- StateId：见 §4 状态闭包；CompId 只能使用 §5 的 canonical 名称；VAR 为值占位（如 `<vol>`、`<today>`）。
- 状态栏不单独生成顶层屏幕；同步/充电/低电/故障以页面内 `statusbar` 的 `VAR` 标注。
- 候选风格使用 `[STYLE:<style-id>]` 命名空间；Stage 4 只允许 `[STYLE:DEVICE-01]`，每个 canonical frame 固定 410×502；结构 axes 只作审阅索引，不改变页面状态或组件语义。

## 3. 页面闭包表（pen/HTML 必须各有帧；绑定 epics E3 story）

| PageId | 页面 | 作用 | 帧(ST) | 绑定 |
| --- | --- | --- | --- | --- |
| MUYU | 木鱼页 | 7字带+心经进度+今日敲击+累计敲击+电子木鱼 | BASE, EMPTY, MID, FULL, DONE_OVERLAY, CHARGING_PAUSE, UNTRUSTED_TIME | FR-E-006/007；E3.2/3.3/3.9 |
| JINGWEN | 经文页 | 同一篇 append-only 经文流：流尾最新大字+已确认前文可垂直回看+心经进度百分比 | BASE | FR-E-006；E3.4 |
| TONGJI | 统计页 | 今日/累计同构统计卡（跨诵读周期保留） | BASE, UNTRUSTED_TIME | FR-E-006/009；E3.5 |
| SHEZHI | 设置页(下滑) | 亮度/熄屏/音量/立即同步/木鱼版本/木鱼ID | BASE, SYNC_BUSY, SYNC_OK, SYNC_PENDING, SYNC_FAIL | FR-E-007；E3.7 |
| SHELL | 全局 | 状态栏（常驻于 MUYU/JINGWEN/TONGJI 帧内） | `VAR:sync-ok/pending/busy/fail/charging/lowbatt/fault/untrusted` | FR-E-008/009；E3.1/3.8 |

完成遮罩属于 `MUYU.DONE_OVERLAY` 帧内的 `modal-done` 组件（FR-E-003/005；E3.9），不另建 `OVERLAY.DONE` 顶层帧。

## 4. 状态闭包（帧清单 = 页面 × 状态）

设备状态栏使用点与短字表达；正文帧覆盖以下状态（每帧为独立 HTML 块）。`tap-rings`、`scripture-history` 的 idle/flash、流尾/回看属于组件状态变体，不增加页面 ID：

Pen 中只保留真实改变页面结构或输入边界的 screen：`screen_shell` 内的 `MUYU.BASE/JINGWEN.BASE/TONGJI.BASE`、`MUYU.CHARGING_PAUSE`、`MUYU.DONE_OVERLAY` 和按需 `SHEZHI.BASE`。`MUYU.EMPTY/MID/FULL` 由字带+进度组件状态板承载，`MUYU.UNTRUSTED_TIME` 与 `TONGJI.UNTRUSTED_TIME` 由今日统计组件状态板承载，`SHEZHI.SYNC_BUSY/OK/PENDING/FAIL` 由 `sync-btn` 组件状态板承载。HTML 组装时为闭包校验展开这些状态帧，并标记 `data-ewf-screen-variant="false"`；设置页亮度、自动熄屏与音量结构必须逐节点一致，首屏最多显示 4 行。

```
[MUYU][BASE]     典型亮屏：7字带已诵若干、心经进度字数+百分比、今日敲击、累计敲击、状态栏
[MUYU][EMPTY]    当前诵读起始：7字带全空位（低对比占位，不预览未来字）
[MUYU][MID]      当前诵读中途：带首行微满、标点随附、进度百分比
[MUYU][FULL]     7字带满位并继续左移（不代表心经完成）
[MUYU][DONE_OVERLAY] 完成遮罩浮层 + 从头开始/退出
[MUYU][CHARGING_PAUSE] 充电中（输入暂停，以轻量提示表达，不使用弹窗）
[MUYU][UNTRUSTED_TIME] 未校时：今日区显「待校时」
[JINGWEN][BASE]  一篇连续经文流：每行 13 个字符槽（标点计槽）、流尾最新大字 + 已确认前文回看 + 心经进度百分比
[TONGJI][BASE]   今日/累计同构统计卡（近7/30与连续不在设备）
[TONGJI][UNTRUSTED_TIME] 今日显「待校时」
[SHEZHI][BASE]   首屏四行：三档亮度·5/15/30s·音量50·立即同步；第二页：木鱼版本·木鱼ID
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

上述页面状态展开后共有 15 个设备顶层帧；`SHELL` 行与 `tap-rings`/`scripture-history` 状态是帧内变体，不另计顶层帧。7 字带容量只表示可见窗口，不是诵读字数上限。

> 熄屏/首触唤醒以低亮状态帧表达（不放解释性说明文字）。统计近7/30/连续不在设备。

### 4.1 组件优先级与变体样式

| 层级 | Pencil 真源 | 变体规则 |
| --- | --- | --- |
| 组件状态板 | `ZauNI` `cmp_muyu_flow_states` | `EMPTY`=7 个普通低对比占位；`BASE`=最新字槽 3；`MID`=最新字槽 7；`FULL`=最新字槽 4；每个状态只允许一个可见大字 |
| 组件状态板 | `Hlo77` `cmp_today_taps_variants` | 同一 `today-taps` 几何下 trusted/untrusted 两态；trusted 使用正文色，untrusted 只显示“待校时”并使用 muted 色；内容相同的变体不重复登记 |
| 组件状态板 | `JZhSx` `cmp_tap_rings_states` | `idle`/`flash` 视觉对照；母版只导出 idle 白色描边，`flash` 是描边色运行时补丁（`#e6bd69`，160ms），**不建立金色环节点变体** |
| 组件状态板 | `fWCbZ` `cmp_sync_btn_variants` | `BASE/BUSY/OK/PENDING/FAIL` 只覆写同步行的 action 文案/状态色，不复制设置屏 |
| 组件状态板 | `qLhoo` `cmp_statusbar_states` | 信号四态（connected/`signal-zero`/`wifi-off`/全禁用）、同步四态、电量满·中·低·充电，共 12 组对照；页面本体仍只用单槽运行时补丁 |
| 组件变体 | `lo9C1` / `fevaG` / `PawlG` | `lo9C1`=tail 母版、`fevaG`=review 变体、`PawlG`=tail 页面实例；三者共用 362×286、13 槽行几何 |
| 整屏变体 | `ZTLH6` / `VkR3m` | 仅因充电输入边界或完成遮罩锁定而保留；字带、进度、状态栏、木鱼与三环均复用组件母版 |

整屏状态不得因为单个颜色、数值或同步文案变化而新增；同一页面骨架先通过组件状态板表达，只有布局、信息架构或输入边界真实改变时才保留 Screen 变体。

## 5. 组件清单（CompId → DESIGN 令牌）

| CompId | 组件 | 令牌/规格 |
| --- | --- | --- |
| statusbar | 顶部状态栏 | reusable `cmp_statusbar_gGgAm`；透明/`{colors.device.surface.0}`；左侧 connectivity-rail，右侧 battery-status，同步状态轻量呈现；四项信号与电量均为单槽运行时补丁，状态对照见 `qLhoo` |
| connectivity-rail | 信号组件容器 | 固定承载 4G/Wi-Fi/蓝牙/GPS 四个 signal-status；每项支持 connected/no-signal/disabled |
| signal-status | 单项信号 | 图标与线条/短文案双编码；图标可见不代表业务链路启用 |
| battery-status | 电池组件 | 右侧电池符号+百分比；充电/低电量有独立图标或文案 |
| charcell | 7字带字形槽 | reusable `mDOlU` + `ZauNI` 状态板；空/基线/中段/满带分别由组件变体表达，固定 7 槽；唯一 `glyph-current` 使用大字 |
| glyph-current | 当前字 | `{typography.device.glyph}`；最新字用 `{colors.brand.amber.400}`，保持到下一有效字，旧字回到普通层级 |
| scripture-progress | 心经进度 | reusable `JmQTi` + `ZauNI` 状态板；显示 `round_consumed / scripture_chars_total · percent%`；达到 100% 后保持 N/N |
| today-taps | 今日敲击 | `{typography.device.body}`；文案使用“今日敲击 N 次”，未校时显示“待校时” |
| total-taps | 累计敲击 | `{typography.device.value}`；跨所有 `round_id` 保留，不随从头开始清零 |
| round-index | 当前诵读标识 | 仅辅助显示“第 N 次诵读/进行中/已完成”，不表达字数上限 |
| woodfish | 电子木鱼 | 使用 `woodfish-anatomy` 主体；木鱼页唯一 `device_touch` 触区；三环由 `tap-rings` 承载 |
| woodfish-anatomy | 木鱼识别结构 | 鱼身、鱼头、嘴槽、眼点、尾鳍/底座、木纹、木槌均可辨；静止与敲击两态 |
| tap-rings | 三环敲击反馈 | 正好三道透明椭圆描边；`physical_pvdf`/`device_touch` 在 MUYU 都触发；`idle→flash→idle`，flash 160ms，连续输入只重启光效。母版只导出 idle 白色描边，flash 切 `{colors.brand.amber.300}` 由 `bindings` 运行时下发，**不建节点变体**（对照板 `JZhSx`） |
| scripture-history | 经文历史流 | `lo9C1` 为 tail 母版、`fevaG` 为 review 变体，`PawlG` 是页面 tail 实例；每行固定 13 个字符槽，标点占槽；最新字在流尾；垂直滚动只浏览已确认/本地已记录内容；新字到达自动锚回流尾；不预览未来 |
| stat-card | 统计卡 | reusable `ZtT4f`；今日与累计各一张 378×64 同构卡，设备不显示近 7/30 日或连续天数 |
| reading-progress | 本次诵读进度 | 统计页第三块；环形进度 + `已诵 / 总字数` + 百分比 + 第 N 次诵读；分母取运行时 `scripture_chars_total`，不写入 canonical 经文源，也不引入新指标 |
| row | 设置行 | 图标/标题/值/箭头；每行 378×80，间隔 8 |
| settings-list | 设置列表 | `W3247` 唯一设置 Screen；378×344 纵向 viewport、520px 内容、两页 page group，首屏最多 4 行；同步态只改变 `sync-btn`，第二页为木鱼版本/木鱼 ID |
| slider | 设置滑杆 | 亮度 `low/medium/high` 与熄屏 `5s/15s/30s` 为分段胶囊；音量为 0–100 滑块 |
| sync-btn | 立即同步 | reusable `cQ5wd` 的 `BASE/BUSY/OK/PENDING/FAIL` 五态组件变体 |
| device-identity | 设备身份行 | `木鱼版本` 与 `木鱼ID` 共用同一行几何，值来自设备状态 |
| modal-done | 完成遮罩 | `components.device.modal-done`；末字确认后锁定输入，提供“从头开始/退出” |

## 6. SquareLine 导出边界与 Screen 建议（Stage 5 落实）

- 推荐 Screen：`screen_shell`（410×502 视口 + 1230×502 横向 pager）、`screen_settings`（按需下滑，378×344 纵向列表）；完成遮罩作为 MUYU 屏内 `modal-done` 组件，不另建 OVERLAY Screen。SquareLine 1.6.1 导出 8.3.11，固件重新编译到 8.4.0；配置细节见 `lvgl-design/squareline_studio/ewf_project_manifest.json`。
- 导出落 `Embedded/components/ui/generated/`；父 CMake 读 `filelist.txt`；`compat/lvgl/lvgl.h` 转发到 ESP-IDF lvgl；业务在 `bindings/`。
- 字体：**只嵌入本篇《心经》的可消费汉字与本页 UI 字符子集**（Noto Serif SC 字形 + Noto Sans SC 状态/数字）；`generated/fonts` 与 SquareLine 资产字节一致；禁手改 .c 位图。
- `generated/` 只导出布局、字体和事件空桩；三环描边色（flash）、最新字切换、经文滚动锚点、状态栏信号/电量/同步的颜色与图标补全，全部由 `bindings/` 在 `ui_task` 内驱动。金色闪光只需在 bindings 侧绑定 `{colors.brand.amber.300}` 并在 160ms 后回 idle，母版**不导出**金色描边与任何重复的信号槽节点。

## 7. FR / story 校验
- 覆盖 FR-E-006/007/009 全部页面态；FR-E-003/005/008 由状态/反馈帧覆盖；对照 epics E3.1~E3.9。
