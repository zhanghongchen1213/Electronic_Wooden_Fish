---
name: Electronic_Wooden_Fish
description: "一敲一字的电子木鱼双端体验：设备诵经与小程序阅读。"
type: design
status: draft
created: 2026-09-08
updated: 2026-09-11
sources:
  - "{planning_artifacts}/briefs/brief-Electronic_Wooden_Fish-2026-09-08/brief.md"
  - "{planning_artifacts}/briefs/brief-Electronic_Wooden_Fish-2026-09-08/addendum.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/addendum.md"
  - "{planning_artifacts}/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
selected_style:
  device: DEVICE-01
  miniapp: MINI-06
style_candidates:
  device: [DEVICE-01, DEVICE-02, DEVICE-03, DEVICE-04, DEVICE-05, DEVICE-06, DEVICE-07, DEVICE-08, DEVICE-09, DEVICE-10]
  miniapp: [MINI-01, MINI-02, MINI-03, MINI-04, MINI-05, MINI-06, MINI-07, MINI-08, MINI-09, MINI-10]
style_lock:
  aesthetic_direction: "Dark OLED Luxury × tactile craft realism；签名细节为三道椭圆环的短促金光与流尾字焦点"
  device:
    name: "匠作写实·三分之四"
    source_master: hbTEa
    surface: "#050505"
    ink: "#fbfaf0"
    current: "#d9a441"
    tap_flash: "#e6bd69"
  miniapp:
    name: "篆刻印谱"
    source_master: A358t
    surface: "#f4f0e5"
    ink: "#2c2823"
    focus: "#a66b3a"
    seal: "#a64c3e"
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
      "0": "#050505"
      "1": "#121212"
      "2": "#252525"
    text:
      primary: "#fbfaf0"
      secondary: "#cfc4b0"
      muted: "#a6a29a"
    divider: "#FBFAF01A"
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
      "0": "#f4f0e5"
      "1": "#fffdf6"
      "2": "#d7d0c4"
    text:
      primary: "#2c2823"
      secondary: "#8b8177"
      muted: "#8b8177"
    divider: "#9E7B5722"
    focus: "#a66b3a"
    seal: "#a64c3e"
    seal-wash: "#a64c3e18"
    rule: "#9e7b5755"
typography:
  family:
    device: "Noto Serif SC"
    device_ui: "Noto Sans SC"
    miniapp: "Noto Serif SC"
    miniapp_ui: "Noto Sans SC"
  device:
    glyph: { family: "Noto Serif SC", size: 52, weight: 700, usage: "7字带与经文页最新字" }
    char: { family: "Noto Serif SC", size: 26, weight: 500, usage: "7字带已诵字" }
    placeholder: { family: "Noto Serif SC", size: 26, weight: 400, usage: "7字带空位低对比占位" }
    ui: { family: "Noto Sans SC", size: 16, weight: 400, usage: "状态、统计与设置" }
    value: { size: 34, weight: 600, usage: "统计大数" }
    title: { size: 22, weight: 500 }
    body: { size: 16, weight: 400 }
    label: { size: 13, weight: 400 }
  miniapp:
    lede: { family: "Noto Serif SC", size: 42, weight: 600, usage: "仅 LOGIN 品牌语" }
    reading: { family: "Noto Serif SC", size: 30, weight: 600, usage: "阅读流最新字（全 app 唯一放大字）" }
    stat-hero: { family: "Noto Serif SC", size: 56, weight: 700, usage: "仅 RECORDS 今日敲击" }
    stat: { family: "Noto Serif SC", size: 28, weight: 700, usage: "其余统计数字" }
    title: { family: "Noto Serif SC", size: 18, weight: 600, usage: "页标题与阅读流正文" }
    body: { family: "Noto Serif SC", size: 16, weight: 400, lineHeight: 1.7 }
    ui: { family: "Noto Sans SC", size: 13, weight: 500, usage: "行标签与行值、导航" }
    caption: { family: "Noto Sans SC", size: 12, weight: 400 }
    micro: { family: "Noto Sans SC", size: 11, weight: 500, usage: "区块标签、印章字、版记编号" }
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
      tapSources: [physical_pvdf, device_touch]
    tap-rings:
      count: 3
      shape: ellipse
      idleStroke: "{colors.device.text.primary}"
      flashStroke: "{colors.brand.amber.300}"
      durationMs: 160
      lifecycle: "idle→flash→idle"
      coalesce: "restart-on-event"
    scripture-history:
      mode: append-only
      axis: vertical
      defaultAnchor: tail
      onNewChar: "append-and-scroll-to-tail"
      futurePreview: false
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
      latestOnly: true
      underline: "always-under-latest"
    reading-archive:
      mode: "collapsible completed scripture segment"
    char-focus:
      type: "{typography.miniapp.reading}"
      color: "{colors.mini.focus}"
      underline: "{colors.mini.focus}"
      lifecycle: "persist-until-next-confirmed-char"
    statcard:
      layout: "boxless ledger rows — no fill card; separated by 1px rule"
      hero: "{typography.miniapp.stat-hero}"
      value: "{typography.miniapp.stat}"
      label: "{typography.miniapp.micro}"
      order: [today-hero, week-month-pair, total-right-aligned, streak]
    devstatus-row:
      layout: "boxless ledger row; label left / value right-aligned to axis 370"
      label: "{typography.miniapp.ui}"
      value: "{typography.miniapp.ui}"
    style-signature:
      role: "篆刻印谱签名层，六页共用、位置按页重组"
      parts: [grid, seal, seal-label, folio, rule, tick]
      grid: "48px 谱格（横线 x20→370，竖线 y150 起）"
      seal: "{colors.mini.seal} keyline + {colors.mini.seal-wash} fill"
      folio: "{typography.miniapp.micro} 纯数字版记编号"
      rule: "{colors.mini.rule} 1px 刻线"
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
      radius: "{rounded.miniapp.control}"
      active: "{colors.mini.focus}"
      inactive: "{colors.mini.text.muted}"
      items: [阅读, 记录, 设备, 设置]
    scripture-progress:
      track: "{colors.mini.surface.2}"
      fill: "{colors.brand.amber.600}"
      fields: [confirmed_chars, scripture_chars_total, percent]

component_id_namespace:
  device:
    stat-card: "今日/累计同构统计卡"
    device-identity: "木鱼版本/木鱼ID"
  miniapp:
    statcard: "记录页今日/近7日/近30日/累计/连续卡"
---

# Electronic_Wooden_Fish — DESIGN.md

> 本文件是视觉权威，EXPERIENCE.md 是行为权威；两者均高于 mock/wireframe，冲突时按各自职责解释。`DEVICE-01` 与 `MINI-06` 已锁定为当前视觉母版；其余候选仅保留作审阅记录，不进入 Stage 4 全帧稿。

## Brand & Style

一句话：**一盏烛下的木鱼，一段留白的经文。**

- **设备轨（锁定 DEVICE-01）**：四周圆弧屏承载黑底白色海报式电子木鱼；木鱼周围三道透明椭圆环是唯一敲击光效，金色只在有效敲击的短暂反馈中出现。
- **小程序轨（锁定 MINI-06）**：浅纸面、基线网格和印谱印记承载一篇连续心经；正文、最新字和总进度构成唯一阅读层级，不添加外层展示背景。
- 两轨在视觉上不需要一致（PRD 允许），但**令牌同源**：同一支琥珀、同一族字体，才能让「设备敲出、手机上看到」是同一件事。
- 语言：极简体中文；不堆词。反馈用语义短句与状态，不打断诵经节奏。

### Style Candidate Board

Pen 文件保留 10 个候选 sibling frame 作为审阅档案。候选共享产品行为语义、必要组件和画布几何，但只有已锁定母版进入 Stage 4；候选名称、结构指纹和审阅标签不进入产品页面。

### Locked Style

- **设备 `DEVICE-01` / master `hbTEa`**：保留现有三分之四海报构图、黑色屏面（`#050505`）、白色木鱼轮廓（`#fbfaf0`）、Noto Serif SC 字形层级和透明椭圆环。环的 idle 为低对比描边，flash 为 `{colors.brand.amber.300}`，160ms 后恢复 idle。
- **小程序 `MINI-06` / master `A358t`**：保留现有浅纸面（`#f4f0e5`）、低密度基线网格、印谱印记、Noto Serif SC 正文与 Noto Sans SC 状态/导航。最新字使用 `{colors.mini.focus}` 放大，2px 下划线始终贴在最新字下方。
- `hbTEa` 与 `A358t` 只作为 Pen 内部复制锚点；派生页面使用各自唯一节点 ID，交付 HTML 不输出 Node ID 或 `data-pencil-id`。

### Candidate Archive

以下候选表只保留审阅与审计索引，不参与 Stage 4 的视觉实现。

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

对比基线：设备正文与主底对比 ≥ 7:1（纸白对近黑）；小程序正文与纸面满足可读对比；强调色不承载长段正文。选中母版的色值在 Stage 4 固定，不再因候选板调色。

## Typography

- 设备：DEVICE-01 使用 Noto Serif SC 表现木鱼页/经文页字形，Noto Sans SC 表现状态、统计与设置；**只嵌入《心经》所需字形子集**（AD-4），不引入通用中文字库。
- 7字带字形居中；当前诵出字用 `typography.device.glyph` 并保持高亮直到下一有效字，已诵字用 `typography.device.char`，空位用 `typography.device.placeholder`。
- 小程序：MINI-06 使用 Noto Serif SC 表现连续正文与最新字，Noto Sans SC 表现导航/状态/进度；正文行高 1.7，最新字放大且下划线不随时间消失。
- 数字用等宽语义（统计大数对齐），`[ASSUMPTION]` 设备数字字体沿用 Noto Sans SC 数字。

## Layout & Spacing

- 画布设备 **410×502**（CO5300 AMOLED），圆角 `{rounded.device.canvas}`（legbot 同款形态）；`{spacing.device.unit}=8`、屏幕边距 `{spacing.device.screen_edge}=16`、关键净空 `{spacing.device.critical_clearance}≥8`。
- 设备主页布局（自上而下）：状态栏 24 → 内容区 → 底部木鱼/动作。三页作为 `screen_shell` 的横向常驻 pager（位置 0/410/820）；下滑进入按需设置屏，返回保持原页。全帧只保留圆弧屏根帧，不加展示板或装饰背景。
- 设置屏沿用 legbot 的 378×344 纵向 viewport；每行 378×80、间隔 8，首屏最多 4 行，木鱼版本与木鱼 ID 在第二滚动页。
- 小程序 `{spacing.miniapp.page_pad}=20px`；单栏内容流；四项底部导航始终可达。全帧只保留 A358t 纸面根帧，不加外层卡片或渐变背景。

## Elevation & Depth

- 设备 OLED：不依赖投影；用「表面色阶」分层（surface0→1→2）+ 完成遮罩用 60% 黑遮罩。
- 小程序：卡片用低对比 1px 分隔 + 极浅投影（视觉权重低）。

## Shapes

- 设备画布 410×502 圆角 110；内部控件圆角 12/16/999（pill）。
- 木鱼形：主识别轮廓以 `lvgl-design/assets/woodfish-reference.png` 为唯一视觉基准——黑底、白色双叶鱼身与连续斜向嘴槽。`tap-rings` 由三道透明椭圆描边组成，只有有效敲击时切换为暖金并在 160ms 内恢复，无长驻呼吸或额外背景。

## Components

### 设备 Pencil 母版与变体映射

| 组件层 | 母版/状态板 | 变体样式与使用边界 |
| --- | --- | --- |
| 常驻组件 | `VphYz` statusbar + `qLhoo`、`n2gMHJ` woodfish-anatomy、`n2eoh` tap-rings + `JZhSx` | 每个页面只实例化一次；信号/电量/同步由运行时补丁切换；三环仅在木鱼页 flash（描边切金 160ms）。`qLhoo`/`JZhSx` 是母版区状态对照板，只用于审阅，不进入页面渲染 |
| 字带组件 | `mDOlU` charcell + `ZauNI` | `EMPTY` 七个普通占位；`BASE` 最新字槽 3；`MID` 最新字槽 7；`FULL` 最新字槽 4；任何状态都只有一个可见 `glyph-current` |
| 进度组件 | `JmQTi` scripture-progress + `ZauNI` | 0/12/84/196/260 只改变已完成段与文案，不生成新 Screen；充电/完成页继续实例化同一母版 |
| 统计组件 | `ZtT4f` stat-card、`Z8rcP` today-taps + `Hlo77`、`FKhdz` reading-progress | 今日/累计同构卡；trusted/untrusted 只改变值色与状态词，不改变几何；本次诵读用环形进度，分母取运行时 `scripture_chars_total` |
| 设置组件 | `YxGuI` brightness、`oLRuW` timeout、`cQ5wd` sync-btn、`j6zSeO` device-identity | 亮度/熄屏/音量骨架固定；同步五态只覆写 action；版本/ID 共用一行母版 |
| 经文组件 | `lo9C1` tail、`fevaG` review、页面实例 `PawlG` | 同一 362×286 几何、每行 13 槽；tail 才提升流尾字，review 不制造第二套排版 |

- `statusbar`：左侧四项信号、右侧电池符号+百分比；同步短语作为轻量状态。
- `connectivity-rail` / `signal-status`：4G、Wi-Fi、蓝牙、GPS 各有 connected/no-signal/disabled；每个位置保留一个图标槽，由运行时颜色/图标补丁切换，不复制隐藏节点。Wi-Fi/蓝牙/GPS 的视觉状态不代表 MVP 业务链路。全部状态组合（信号四态、同步四态、电量满/中/低/充电）在母版区 `qLhoo` 对照板中穷举，页面本体不因此增加节点。
- `battery-status`：电池符号、百分比、充电标记和低电量标记同一组对齐；满/中/低/充电分别用 `{colors.device.battery.fill}` 与 `{colors.device.battery.low}`。
- `woodfish`：木鱼页的交互组合组件，拥有唯一可计数触区（点击=`device_touch`），内部组合 `woodfish-anatomy` 与 `tap-rings`。
- `woodfish-anatomy`：`woodfish` 的非交互视觉子组件；胖鱼身、鱼头、水平嘴槽、眼点、尾鳍/底座、两道木纹和独立木槌必须同时可辨，不单独拥有点击语义。
- `tap-rings`：三道同心椭圆描边；`physical_pvdf` 与 `device_touch` 在木鱼页共用 `idle→flash→idle` 反馈，flash 持续 160ms，连续输入只重启不叠加。母版只导出 idle 描边（白 `#fbfaf066` / `#fbfaf044` / `#fbfaf033`），flash 由 `bindings` 在运行时把描边切到 `{colors.brand.amber.300}` = `#e6bd69` —— **不建立金色环节点变体**，否则会与固件常量形成双真源；母版区的 `JZhSx` 仅作 IDLE/FLASH 视觉对照。HTML 侧同一语义由 `data-ewf-motion="flash"` 表达。
- `charcell`：固定 7 槽；占位用低对比点位而非未来字（反剧透 AD-19），只有一个 `glyph-current` 可使用大字焦点。
- `scripture-history`：经文页为同一篇 append-only 流；每行固定 13 个字符槽，标点占槽但不计敲击；最新字在流尾使用 `glyph-current`，旧字永久保留并可垂直回看，新字到达时自动回到流尾。
- `scripture-progress` / `round-index`：心经已诵字数、总可消费字数、百分比和当前诵读状态；7 字带容量不是进度分母。
- `today-taps` / `total-taps`：今日敲击与跨诵读周期累计敲击并列呈现；统计页使用两个同构 `stat-card`。
- `stat-card`：左侧琥珀刻线、标签、数字和单位；设备只展示今日与累计，不承载近 7/30 日或连续天数。
- `reading-progress`：统计页本次诵读区块；环形进度（已诵占比）+ `已诵 / 总字数` + 百分比 + 第 N 次诵读。它是统计页第三块内容，与今日/累计两张 `stat-card` 同处一屏，不引入新指标。
- `settings-list` / `slider` / `row` / `sync-btn`：设置页两页纵向列表；亮度与熄屏使用分段胶囊，音量使用 0–100 滑块，立即同步含进行中/成功/待同步/失败反馈。
- `device-identity`：木鱼版本与木鱼 ID 共用同一行组件，不复制排版。
- `modal-done`：完成遮罩 + 「从头开始 / 退出」。
- `readingline` / `char-focus` / `reading-archive`：持续累积正文，最新尾字放大并用焦点色高亮，2px 下划线始终跟随最新字，已诵字墨色弱化，完成篇章可折叠归档。
- `statcard` / `devstatus-row` / `state-banner` / `sync-action` / `setting-mirror` / `empty` / `confetti` / `bottom-nav`：记录页、设备页、设置页与全局状态。

组件 ID 以表面为命名空间：`device/stat-card` 是设备今日/累计双卡，`mini/statcard` 是小程序五项记录卡，两者不是同一实现；其余同名组件才表示跨端语义一致。Pencil 节点 ID 仅作审计锚点，逻辑 CompId 才是下游绑定名。

## Do's and Don'ts

- ✅ 金色只落在「这一字/这一步/这一瞬」；✅ 三环反馈短促且可恢复；✅ 状态用轻点与短句；✅ 空位就是空位（低对比占位）。
- ✅ 相同屏幕骨架优先复用组件状态；只有信息架构、布局或输入边界真实变化时才建立整屏变体。设置页同步状态只变化 `sync-btn`，亮度/自动熄屏/音量骨架不复制设计。
- ✅ 每个候选 style 应以真实渲染差异呈现自己的构图、字阶、材质/物件与进度叙事；候选名称只用于风格板标注，不进入产品 UI 文案。结构指纹是近重复预警，不是视觉签收替代物。
- ❌ 不要无语义的彩色/装饰堆叠、常驻动效、把待同步伪装成已同步（AD-2）；❌ 设备不放小程序的阅读型长文；小程序不出现可点击木鱼（FR-F-002）。
- ❌ 最终页面不得出现 AI 思维链、作者说明、Node ID、调试标签或重复进度；解释性内容只存在于 UX 文档和审计日志。
