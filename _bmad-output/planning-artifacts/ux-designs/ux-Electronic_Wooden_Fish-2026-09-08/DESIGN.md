---
name: Electronic_Wooden_Fish
description: "一敲一字的电子木鱼双端体验：设备诵经与小程序阅读。"
type: design
status: draft
created: 2026-09-08
updated: 2026-09-08
sources:
  - "{planning_artifacts}/briefs/brief-Electronic_Wooden_Fish-2026-09-08/brief.md"
  - "{planning_artifacts}/briefs/brief-Electronic_Wooden_Fish-2026-09-08/addendum.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/addendum.md"
  - "{planning_artifacts}/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
selected_style:
  device: null
  miniapp: null
style_candidates:
  device: [DEVICE-01, DEVICE-02, DEVICE-03, DEVICE-04, DEVICE-05, DEVICE-06, DEVICE-07, DEVICE-08, DEVICE-09, DEVICE-10]
  miniapp: [MINI-01, MINI-02, MINI-03, MINI-04, MINI-05, MINI-06, MINI-07, MINI-08, MINI-09, MINI-10]
colors:
  brand:
    amber:
      "600": "#a56f1f"
      "400": "#d9a441"
      "300": "#e6bd69"
    sandal:
      "900": "#2a1e12"
      "600": "#8a5a2b"
  device:
    surface:
      "0": "#17130f"
      "1": "#211b14"
      "2": "#2b241a"
    text:
      primary: "#f5efe2"
      secondary: "#cfc4b0"
      muted: "#8f8674"
    divider: "#F5EFE21A"
    sync:
      amber: "#d9a441"
      ok: "#7fa98f"
    signal:
      connected: "#b8c7a4"
      no_signal: "#81786a"
      disabled: "#5f564b"
    battery:
      fill: "#c9a66a"
      low: "#c77b55"
    state:
      danger: "#b3705c"
      lowbatt: "#c9a66a"
  mini:
    surface:
      "0": "#faf7f2"
      "1": "#ffffff"
      "2": "#f1ece2"
    text:
      primary: "#2a2620"
      secondary: "#6f675c"
      muted: "#a39a89"
    divider: "#2A26201A"
    focus: "#d9a441"
typography:
  family:
    device: "Noto Sans SC"
    miniapp: "Noto Sans SC"
  device:
    glyph: { size: 52, weight: 700, usage: "7字带当前字/经文页大字" }
    char: { size: 26, weight: 500, usage: "7字带已诵字" }
    placeholder: { size: 26, weight: 400, usage: "7字带空位低对比占位" }
    value: { size: 34, weight: 600, usage: "统计大数" }
    title: { size: 22, weight: 500 }
    body: { size: 16, weight: 400 }
    label: { size: 13, weight: 400 }
  miniapp:
    reading: { size: 30, weight: 600, usage: "阅读行当前字放大" }
    body: { size: 16, weight: 400, lineHeight: 1.7 }
    caption: { size: 12, weight: 400 }
    stat: { size: 28, weight: 700 }
rounded:
  device:
    canvas: 110
    card: 16
    control: 12
    pill: 999
  miniapp:
    card: 16
    control: 12
    pill: 999
spacing:
  device:
    unit: 8
    screen_edge: 16
    critical_clearance: 8
    statusbar: 24
  miniapp:
    unit: 4
    page_pad: 20
components:
  device:
    statusbar:
      height: 24
      surface: "{colors.device.surface.0}"
      elements: [signal-4g, signal-wifi, signal-ble, signal-gps, battery, charging, sync]
      signalStates: [connected, no_signal, disabled]
    connectivity-rail:
      slots: [signal-4g, signal-wifi, signal-ble, signal-gps]
      slotWidth: 22
      gap: 6
    battery-status:
      value: "{colors.device.text.primary}"
      fill: "{colors.device.battery.fill}"
      low: "{colors.device.battery.low}"
    charcell:
      width: "7cols"
      glyph: "{typography.device.char}"
      onChar: "{colors.device.text.primary}"
      offChar: "{colors.device.text.muted}"
    glyph-current:
      type: "{typography.device.glyph}"
      color: "{colors.brand.amber.400}"
    scripture-progress:
      radius: "{rounded.device.pill}"
      accent: "{colors.brand.amber.600}"
      fields: [confirmed_chars, scripture_chars_total, percent, round_state]
    today-taps:
      type: "{typography.device.body}"
      color: "{colors.device.text.secondary}"
    total-taps:
      type: "{typography.device.value}"
      color: "{colors.device.text.primary}"
    round-index:
      type: "{typography.device.label}"
      color: "{colors.device.text.muted}"
    signal-status:
      states: [connected, no_signal, disabled]
      connected: "{colors.device.signal.connected}"
      no_signal: "{colors.device.signal.no_signal}"
      disabled: "{colors.device.signal.disabled}"
    signal-4g:
      role: "signal-status"
    signal-wifi:
      role: "signal-status"
    signal-ble:
      role: "signal-status"
    signal-gps:
      role: "signal-status"
    woodfish:
      surface: "{colors.device.surface.0}"
      ink: "{colors.brand.sandal.600}"
      tapGlow: "{colors.brand.amber.300}"
    row:
      parts: [icon, title, value, chevron]
    slider:
      rail: "{colors.device.surface.2}"
      knob: "{colors.brand.amber.600}"
    sync-btn:
      states: [busy, ok, pending, fail]
    modal-done:
      overlay: "#00000099"
      card: "{colors.device.surface.1}"
      actions: [restart, exit]
  mini:
    readingline:
      bg: "{colors.mini.surface.0}"
      currentChar: "{colors.mini.focus}"
      currentType: "{typography.miniapp.reading}"
      done: "{colors.mini.text.muted}"
      mode: "append-only flowing scripture"
    reading-archive:
      mode: "collapsible completed scripture segment"
    char-focus:
      type: "{typography.miniapp.reading}"
      color: "{colors.mini.focus}"
    statcard:
      card: "{colors.mini.surface.1}"
      value: "{typography.miniapp.stat}"
      label: "{typography.miniapp.caption}"
    devstatus-row:
      value: "{colors.mini.text.secondary}"
    state-banner:
      tones: [ok, pending, warn, danger]
    sync-action:
      states: [busy, pending, fail]
    setting-mirror:
      pending: "待设备应用"
      applied: "已生效"
    empty:
      icon: true
      text: "{colors.mini.text.muted}"
    modal-done:
      confetti: true
      actions: [restart, exit]
    confetti:
      color: "{colors.brand.amber.300}"
    bottom-nav:
      height: 56
      surface: "{colors.mini.surface.1}"
      active: "{colors.mini.focus}"
      inactive: "{colors.mini.text.muted}"
      items: [阅读, 记录, 设备, 设置]
    scripture-progress:
      track: "{colors.mini.surface.2}"
      fill: "{colors.brand.amber.600}"
      fields: [confirmed_chars, scripture_chars_total, percent]
---

# Electronic_Wooden_Fish — DESIGN.md

> 视觉身份权威。EXPERIENCE.md 行为权威；本文件与 EXPERIENCE.md 冲突时以两份 spines 为准；比任何 mock/wireframe 更高。当前 `style_candidates` 是待选方向，`selected_style` 在作者选择前保持空值；候选只改变视觉表达，不改变交互、数据或闭包。

## Brand & Style

一句话：**一盏烛下的木鱼，一段留白的经文。**

- **设备轨**：四周圆弧屏承载一只可辨识的木鱼；状态栏像器物铭刻，正文与统计为辅助信息；风格候选都保持低饱和、细节克制、木鱼抓人。
- **小程序轨**：以可持续阅读的心经纸面为核心；已确认正文连续保留，当前字与总进度是唯一高强调信息；四项导航保持轻量。
- 两轨在视觉上不需要一致（PRD 允许），但**令牌同源**：同一支琥珀、同一族字体，才能让「设备敲出、手机上看到」是同一件事。
- 语言：极简体中文；不堆词。反馈用语义短句与状态，不打断诵经节奏。

### Style Candidate Board

Pen 文件各自包含 10 个候选 sibling frame。候选共享同一信息结构、文案、状态和尺寸，只替换色彩、字体组合、材质暗示与一个静态签名细节；禁止霓虹、鲜艳多色、张扬动效、照片背景和营销装饰。`selected_style.device` 与 `selected_style.miniapp` 在作者选定前为 `null`。

设备候选：

| ID | 名称 | 视觉语言 | 签名细节 |
| --- | --- | --- | --- |
| `DEVICE-01` | 墨玉檀影 | OLED 近黑、檀木鱼身、温白字 | 鱼身外沿一圈短余韵环 |
| `DEVICE-02` | 漆器金缝 | 乌漆底、暗红棕、低饱和金 | 木鱼嘴槽一条金缝 |
| `DEVICE-03` | 松烟水墨 | 墨黑、纸白、松烟灰 | 7 字带左移留下短墨痕 |
| `DEVICE-04` | 檀木横纹 | 深木棕、温黑、暗金 | 鱼身两道横向木纹 |
| `DEVICE-05` | 烛影琥珀 | 近黑、局部烛焰琥珀 | 木鱼上方静态烛影弧 |
| `DEVICE-06` | 青黛静水 | 深青黛、温白、黄铜色 | 状态栏下方极细水线 |
| `DEVICE-07` | 朱砂印痕 | 暖黑、檀棕、极少朱砂 | 完成态才出现小印章 |
| `DEVICE-08` | 古籍页脊 | 深棕、纸白、书签金 | 当前字下方短书签线 |
| `DEVICE-09` | 温石微光 | 炭灰、暖灰、琥珀 | 进度沿浅刻槽增长 |
| `DEVICE-10` | 月白檀心 | 蓝黑、月白、檀木 | 鱼身后静态半月背光 |

小程序候选：

| ID | 名称 | 视觉语言 | 签名细节 |
| --- | --- | --- | --- |
| `MINI-01` | 宣纸留白·宋韵 | 宣纸、宋体、细横线 | 纸张纤维细纹 |
| `MINI-02` | 茶席雾光·侘寂 | 茶灰、黛青、软弧线 | 页面边缘墨晕 |
| `MINI-03` | 莲池月影·月白 | 月白、墨蓝、淡琥珀 | 月牙式进度轨 |
| `MINI-04` | 山寺晨雾·青灰 | 雾青、米白、低饱和渐层 | 渐隐山形线稿 |
| `MINI-05` | 沉香木案·册页 | 温棕、册页阴影、墨字 | 章节页签细线 |
| `MINI-06` | 篆刻经箧·印谱 | 网格、纸白、极少印红 | 经页编号印记 |
| `MINI-07` | 窗纸映光·宣白 | 半透明纸窗、柔光 | 当前字后静态光带 |
| `MINI-08` | 砂庭回纹·枯山水 | 暖灰纸、细砂纹 | 砂线进度聚拢 |
| `MINI-09` | 暮鼓檀音·墨棕 | 浅米与深檀双表面 | 底部暮线随进度增长 |
| `MINI-10` | 古籍折页·雅集 | 非对称留白、书签轨 | 右侧篇章索引 |

## Colors

色调令牌见 frontmatter `colors`。语义用法：

| 令牌 | 设备 | 小程序 |
| --- | --- | --- |
| 主底 `{colors.device.surface.0}` / `{colors.mini.surface.0}` | 全部主页底 | 阅读纸面 |
| 抬升/卡片 `{colors.device.surface.1}` / `{colors.mini.surface.1}` | 下滑设置页 | 统计卡/弹窗 |
| 主字 `{colors.device.text.primary}` / `{colors.mini.text.primary}` | 已诵字、标题 | 正文 |
| 强调 `{colors.brand.amber.400}` / `{colors.brand.amber.300}` | 当前字/进度/敲击光晕/待同步点 | 当前字/进度/完成 |
| 占位 `…muted` | 7字带空位（低对比，**不预览未来**） | 禁用、提示弱化 |
| 状态 | 已同步低饱和绿 / 待同步琥珀 / 故障陶红 / 低电暖黄（均低打扰） | 同类语义，更浅色系下加深墨轮廓 |

对比基线：设备正文与主底对比 ≥ 7:1（纸白对近黑）；强调琥珀不承载正文承载。`[ASSUMPTION]` 精确对比值在 Stage3 方向稿实测后微调，不晚于 Stage6 冻结。

## Typography

- 设备：正文/经文共用 **Noto Sans SC**（按需字重 400/500/600/700）；**只嵌入《心经》所需字形子集**（AD-4），不引入通用中文字库。
- 7字带字形居中；当前诵出字用 `typography.device.glyph`，已诵字 `typography.device.char`，空位 `typography.device.placeholder`（同为字形槽位、低对比）。
- 小程序：正文使用 `typography.miniapp.reading`、`typography.miniapp.body` 与 `typography.miniapp.caption`；正文行高 1.7 利于逐字滚动阅读。
- 数字用等宽语义（统计大数对齐），`[ASSUMPTION]` 设备数字字体沿用 Noto Sans SC 数字。

## Layout & Spacing

- 画布设备 **410×502**（CO5300 AMOLED），圆角 `{rounded.device.canvas}`（legbot 同款形态）；`{spacing.device.unit}=8`、屏幕边距 `{spacing.device.screen_edge}=16`、关键净空 `{spacing.device.critical_clearance}≥8`。
- 设备主页布局（自上而下）：状态栏 24 → 内容区 → 底部木鱼/动作。左右滑动切三主页；下滑进入设置（返回保持原页）。
- 小程序 `{spacing.miniapp.page_pad}=20px`；单栏内容流；四项底部导航始终可达。

## Elevation & Depth

- 设备 OLED：不依赖投影；用「表面色阶」分层（surface0→1→2）+ 完成遮罩用 60% 黑遮罩。
- 小程序：卡片用低对比 1px 分隔 + 极浅投影（视觉权重低）。

## Shapes

- 设备画布 410×502 圆角 110；内部控件圆角 12/16/999（pill）。
- 木鱼形：**单线手绘感**（檀木描边），敲击瞬间暖金光晕扩张后淡出（低打扰，无长驻呼吸）。

## Components

- `statusbar`：左电量/充电、中 4G、右同步点（常驻）。
- `woodfish`：主视觉 + 唯一可点敲区（点击= `device_touch`）。
- `charcell`：固定 7 槽；占位用低对比「空」而非未来字（反剧透 AD-19）。
- `progress-pill` / `today-count`：本轮进度与今日计数小标。
- `slider` / `row` / `sync-btn`：设置页音量/亮度/熄屏；立即同步行含进行中/成功/待同步/失败反馈。
- `modal-done`：完成遮罩 + 「从头开始 / 退出」。
- `readingline` / `char-focus`：阅读行动画，当前字琥珀放大聚焦、已诵字墨弱化；空/待数据用 muted。
- `statcard` / `devstatus-row` / `state-banner` / `sync-action` / `setting-mirror` / `empty` / `confetti` / `bottom-nav`：记录页、设备页、设置页与全局状态。

## Do's and Don'ts

- ✅ 金色只落在「这一字/这一步/这一瞬」；✅ 状态用轻点与短句；✅ 空位就是空位（低对比占位）。
- ❌ 不要彩色堆叠、常驻动效、把待同步伪装成已同步（AD-2）；❌ 设备不放小程序的阅读型长文；小程序不出现可点击木鱼（FR-F-002）。
