---
name: Electronic_Wooden_Fish
description: "一敲一字的电子木鱼双端体验：设备诵经与小程序阅读。"
type: design
status: draft
created: 2026-09-08
updated: 2026-09-09
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

- **设备轨**：四周圆弧屏承载一只高识别度的电子木鱼剪影/器物图像；风格候选可以在图标海报、漆器、版画、浮雕、窗棂、月白等方向大胆分化。
- **小程序轨**：以可持续阅读的心经正文为核心；已确认正文连续保留，当前字与总进度是唯一高强调信息；候选可以在长卷、册页、窗纸、印谱、月相、夜读等方向展开更丰富的阅读叙事，纸面、夜读、光影和图形密度均可重新定义。
- 两轨在视觉上不需要一致（PRD 允许），但**令牌同源**：同一支琥珀、同一族字体，才能让「设备敲出、手机上看到」是同一件事。
- 语言：极简体中文；不堆词。反馈用语义短句与状态，不打断诵经节奏。

### Style Candidate Board

Pen 文件各自包含 10 个候选 sibling frame。候选共享产品行为语义、必要组件和画布几何，但视觉探索不设固定色彩、材质或构图模板；可以改变构图、字体层级、材质/物件、进度形态、光影、图形密度和静态/响应签名。不得把只换色的版本当作独立 style。候选板上的 `axes` 与结构指纹只用于发现近重复和辅助审阅，最终以渲染结果、可读性、产品主题、硬件几何和作者选择为准。`selected_style.device` 与 `selected_style.miniapp` 在作者选定前为 `null`。

设备候选：

| ID | 名称 | 视觉语言 | 签名细节 |
| --- | --- | --- | --- |
| `DEVICE-01` | 匠作写实·三分之四 | 黑底白色木鱼剪影、海报比例、巨大主物件 | 同心余韵环与高识别度斜槽 |
| `DEVICE-02` | 漆器祭台·正面器物 | 漆牌、低台、正面器物层级 | 竖向木槌导轨 |
| `DEVICE-03` | 木雕剖面·中轴图 | 竖排字带、中轴线、剖面图式 | 进度刻槽与中轴标尺 |
| `DEVICE-04` | 手作案台·俯视 | 木案横纹、斜向动作轴、俯视陈设 | 木案 grain 与斜向木槌轴 |
| `DEVICE-05` | 山门窗棂·圆窗取景 | 圆窗、十字窗棂、环形信息 | 窗环承载进度 |
| `DEVICE-06` | 松烟木刻·版画 | 黑白反相、木刻留白、负形嘴槽 | 刻痕与版画边界 |
| `DEVICE-07` | 石窟浮雕·侧光 | 石案浮雕、深浅层、侧光 | 铭刻面与侧光边缘 |
| `DEVICE-08` | 香篆烟线·禅房器物 | 静态烟线、器物中心、留白 | 香篆线围绕木鱼 |
| `DEVICE-09` | 铜框陈设·木鱼与铜铃 | 双层铜框、器物陈列、刻度 | 铜框与底部刻度进度 |
| `DEVICE-10` | 月白留白·三维轮廓 | 月白圆盘、负空间、大比例剪影 | 圆盘背光与宽留白 |

小程序候选：

| ID | 名称 | 视觉语言 | 签名细节 |
| --- | --- | --- | --- |
| `MINI-01` | 宋版长卷 | 窄纵向宣纸阅读栏、宋体、页码细线 | 窄经页栏与页码细线 |
| `MINI-02` | 茶席侘寂 | 居中窄栏、手工弧线、茶灰纸面 | 手工弧线与茶席侧轴 |
| `MINI-03` | 莲池月影 | 月相进度徽记、月下琥珀当前字 | 月相圆环包围当前字 |
| `MINI-04` | 山寺晨雾 | 山形线稿、雾面层次、纵向阅读轴 | 纵向阅读轴与山形线稿 |
| `MINI-05` | 沉香册页 | 册页边框、章节页签、页角状态 | 册页边框与章节页签 |
| `MINI-06` | 篆刻印谱 | 基线网格、印记编号、连续正文 | 基线网格与印谱编号 |
| `MINI-07` | 窗纸映光 | 纸窗分栏、当前字静态光带 | 纸窗双栏与静态光带 |
| `MINI-08` | 砂庭回纹 | 砂线进度、低密度枯山水纹理 | 低密度砂线与回纹 |
| `MINI-09` | 暮鼓檀音 | 浅米/深檀双表面、鼓点式进度刻度 | 上下双表面与鼓点刻度 |
| `MINI-10` | 古籍折页 | 非对称留白、右侧篇章索引、书签进度轨 | 非对称折页与右侧书签轨 |

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

- 设备：候选优先使用受控中文字体（Noto Sans SC、Noto Serif SC 或 Source Han Serif SC），按候选选择字重/字阶；**只嵌入《心经》所需字形子集**（AD-4），不引入通用中文字库。
- 7字带字形居中；当前诵出字用 `typography.device.glyph`，已诵字 `typography.device.char`，空位 `typography.device.placeholder`（同为字形槽位、低对比）。
- 小程序：正文使用受控中文字体并在候选间改变字阶、字面宽度或阅读轴；正文使用 `typography.miniapp.reading`、`typography.miniapp.body` 与 `typography.miniapp.caption`，行高 1.7 利于逐字滚动阅读。
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
- 木鱼形：主识别轮廓以 `lvgl-design/assets/woodfish-reference.png` 为唯一视觉基准——黑底、白色双叶鱼身与连续斜向嘴槽；页面可在轮廓外围叠加眼点、木纹、底座或独立木槌来表达器物感，但不得改成与参考图无关的叶片/抽象 blob，也不得用照片替代。敲击瞬间暖金光晕 100–180ms 淡出（低打扰，无长驻呼吸）。

## Components

- `statusbar`：左侧四项信号、右侧电池符号+百分比；同步短语作为轻量状态。
- `connectivity-rail` / `signal-status`：4G、Wi-Fi、蓝牙、GPS 各有 connected/no-signal/disabled；图标形状与短文案同时变化。Wi-Fi/蓝牙/GPS 的视觉状态不代表 MVP 业务链路。
- `battery-status`：电池符号、百分比、充电标记和低电量标记同一组对齐。
- `woodfish`：胖鱼身、鱼头、水平嘴槽、眼点、尾鳍/底座、两道木纹和独立木槌必须同时可辨；主视觉 + 唯一可点敲区（点击= `device_touch`）。
- `charcell`：固定 7 槽；占位用低对比「空」而非未来字（反剧透 AD-19）。
- `scripture-progress` / `round-index`：心经已诵字数、总可消费字数、百分比和当前诵读状态；7 字带容量不是进度分母。
- `today-taps` / `total-taps`：今日敲击与跨诵读周期累计敲击并列呈现。
- `slider` / `row` / `sync-btn`：设置页音量/亮度/熄屏；立即同步行含进行中/成功/待同步/失败反馈。
- `modal-done`：完成遮罩 + 「从头开始 / 退出」。
- `readingline` / `char-focus` / `reading-archive`：持续累积正文，当前尾字琥珀聚焦，已诵字墨色弱化，完成篇章可折叠归档。
- `scripture-progress`：总进度轨道与百分比文本同时呈现。
- `statcard` / `devstatus-row` / `state-banner` / `sync-action` / `setting-mirror` / `empty` / `confetti` / `bottom-nav`：记录页、设备页、设置页与全局状态。

## Do's and Don'ts

- ✅ 金色只落在「这一字/这一步/这一瞬」；✅ 状态用轻点与短句；✅ 空位就是空位（低对比占位）。
- ✅ 每个候选 style 应以真实渲染差异呈现自己的构图、字阶、材质/物件与进度叙事；候选名称只用于风格板标注，不进入产品 UI 文案。结构指纹是近重复预警，不是视觉签收替代物。
- ❌ 不要无语义的彩色/装饰堆叠、常驻动效、把待同步伪装成已同步（AD-2）；❌ 设备不放小程序的阅读型长文；小程序不出现可点击木鱼（FR-F-002）。
- ❌ 最终页面不得出现 AI 思维链、作者说明、Node ID、调试标签或重复进度；解释性内容只存在于 UX 文档和审计日志。
