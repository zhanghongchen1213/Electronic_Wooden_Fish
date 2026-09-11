# 电子木鱼（EWF）交接：BMAD 当前阶段 + UI 设计任务接管

- **状态**：生效；双端风格已锁定，Stage 4 全帧 Pen/HTML 自动门禁通过并已保存。**设备轨 DEVICE-01 已由作者逐屏签收（2026-09-11）并冻结为 E3 视觉验收基线**；小程序轨 MINI-06 待签收
- **读者**：下一个接手 agent（尤其具备视觉/前端能力的 agent）与人类作者 Hongchenke
- **目的**：单文件自足——搞清 EWF 现在处于 BMAD 哪个阶段、正在执行什么任务、从哪继续、受哪些门禁约束；**无需回看其它文档即可从本节「接手者要做」开始干活**。本文档后续阶段变动时由执行者回写 `updated` 并追加变更记录。

---

## 1. BMAD 定位（我们现在在哪）

阶段序列：`CB 产品简报 → PRD → CU(UX) → CA 架构 → CE epics/stories → SP sprint → BD build → CR review`。

**已完成（产物均在仓库；标注为 draft 的文档除外，其余产物视为 final/生效）：**

| 阶段 | 菜单码 | 产物路径 | 状态 |
| --- | --- | --- | --- |
| 产品简报 | CB | `_bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/{brief.md,addendum.md}` | final（authority：产品方向/交互合同） |
| PRD | PRD | `…/prds/prd-Electronic_Wooden_Fish-2026-09-07/{prd.md,addendum.md}` | final（FR-E/B/F、UJ、SM、跨层字段） |
| 架构 | CA | `…/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` | final（AD-1~19、板级 GPIO 合同、Stack、Deferred） |
| Epic/Story | CE | `_bmad-output/planning-artifacts/epics.md` | final（9 epic / 53 story：E1–E4、S0–S4） |
| UX 契约 | CU | `…/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/{DESIGN.md,EXPERIENCE.md,UI_CONTRACT-device.md,UI_CONTRACT-miniapp.md,.memlog.md}` | **draft**（`DEVICE-01` 已签收并作为 E3 视觉验收基线；`MINI-06` 待签收；两轨均签收后统一定稿） |
| docs | — | `docs/`（含本交接）、`docs/embedded/`（legbot 迁移） | 本任务产物 |

**尚未进行：** SP sprint（菜单码 SP/SS）→ BD build（菜单码 BD，含 Developer 代理）。源码目录 `Embedded/`、`cloud/backend/`、`cloud/frontend/` **仍为空占位**（绿地，未进入实施）。

**一句话现状：** planning 全部完成、UX 契约仍为 draft；设备锁定 `DEVICE-01`（母版 `hbTEa`）**已逐屏签收**，小程序锁定 `MINI-06`（母版 `A358t`）待签收；设备 Stage 4 的 Pen/HTML 已按签收意见修订并重新生成（闭包 15/15、18 项测试通过），未进入 sprint/build。

---

## 2. 正在执行的任务（UI 设计流水线 Stage 0–6）

设备 OLED-LVGL 轨与微信小程序轨的 UI 走同一设计流水线（对齐 legbot 先例）：`UX 契约 → 轨道 UI_CONTRACT → pen 原型 → 静态 HTML 导出 → [设备 SquareLine→LVGL C | 小程序 uni-app 复刻] → 作者逐屏签收`。

| Stage | 内容 | 状态 |
| --- | --- | --- |
| 0 | 脚手架：UX 运行目录、`lvgl-design/`、`miniapp-design/`、legbot 4 个工具收录 | ✅ 完成 |
| 1 | UX 契约 DESIGN.md + EXPERIENCE.md（双轨） | ✅ 风格与交互口径已锁定（draft，待视觉签收） |
| 2 | 双轨 UI_CONTRACT（可校验闭包表） | ✅ 完成(draft) |
| 3 | **10+10 风格候选 Pen/HTML（设备木鱼页 + 小程序心经阅读页）** | ✅ 完成；已选择 `DEVICE-01` / `MINI-06` |
| 4 | 全帧 Pen/HTML + 闭包校验脚本 | ✅ 设备轨 15/15 已签收并冻结为 E3 基线；🔄 小程序轨 14 帧待签收 |
| 5a | 设备 SquareLine → LVGL C 导出与接入 | ⏳ |
| 5b | 小程序 HTML → uni-app 复刻 | ⏳ |
| 6 | 收口：作者逐屏签收 + 回写 epics/架构 Deferred | ⏳ |

**本轮已收口的问题：** 两端不再只做换色；设备木鱼页具有可辨识木鱼和三环敲击反馈，最新字是唯一大字高亮；设备经文页为连续正文且最新字内联于流尾；小程序为连续阅读流、单一进度和随最新字移动的下划线；页面状态优先由组件变体表达。

---

## 3. 接手者要做（从 Stage 4 视觉签收开始，按顺序）

> 全局：中文输出；每到达一个**门禁**即暂停请作者签收；HTML 帧命名与闭包是硬校验，缺帧视为未完成。

**当前门禁**：Stage 4 的 Pen/HTML 视觉签收。**设备轨 `DEVICE-01` 已于 2026-09-11 由作者逐屏签收**，其 `ewf-device-ui.pen`/`.html` 冻结为 E3 视觉验收基线，规格写入 `DESIGN.md`/`EXPERIENCE.md`/`UI_CONTRACT-device.md`。**小程序轨 `MINI-06` 仍待签收**——两轨均签收前，UX 文档整体保持 `draft`、Stage 5 不启动；Stage 6 的对拍与文档定稿属于实施完成后的后续门禁。

### Stage 3 · 两轨 10+10 风格候选（pen → HTML）【已完成】
- **设备选择**：`DEVICE-01`「匠作写实·三分之四」，内部母版锚点 `hbTEa`。
- **小程序选择**：`MINI-06`「篆刻印谱」，内部母版锚点 `A358t`。
- 两轨各 10 个候选的闭包校验与文案卫生检查均已通过；候选列表继续保留为审阅记录，但不再作为 Stage 4 的并行视觉方向。

### Stage 4 · 选中风格全帧闭包（pen/HTML）【当前阶段】
- **当前产物**：`lvgl-design/ewf-device-ui.pen/.html` 与 `miniapp-design/ewf-miniapp-ui.pen/.html`；HTML 是后续复刻/生成的可解析事实源，两个 Pen 文件均已通过 Pencil 保存。
- **状态组织**：设备 Pen 采用 legbot 的“常驻主壳 + 按需设置屏”结构：`screen_shell` 横向承载木鱼/经文/统计三页，`W3247` 为 378×344 纵向设置 viewport。空带/填充/满带、木鱼/统计待校时、同步和信号状态均优先由组件补丁表达；仅充电暂停与完成锁定保留整屏状态。HTML 再合成契约要求的设备 15 帧与小程序 14 帧，并对组件派生帧标记 `data-ewf-screen-variant="false"`。
- **最新字规则**：设备木鱼页每帧只有最新已确认字可放大高亮，下一字出现后旧字回正文层级；经文页 `PawlG` 每行恒定 13 个字符槽（标点计槽），最新字紧跟前文；小程序最新字及其下划线绑定同一流尾字符。
- **敲击反馈**：设备木鱼页三道透明椭圆环 `idle → flash → idle`，金色闪光 160ms，连续输入只重启不叠加；母版只导出 idle 白描边，flash 由 `bindings` 在运行时切换描边色（`#e6bd69`），**不建金色环节点变体**；经文页不显示木鱼环。IDLE/FLASH 对照见母版区 `JZhSx`。
- **统计/设置**：`RsuSH` 只呈现 PRD 要求的今日与累计同构统计卡；`W3247` 首屏四行显示亮度、熄屏、音量、同步，第二页显示木鱼版本与木鱼 ID，亮度/熄屏复用 legbot `achTu`/`Pz5g1` 的分段胶囊语言。
- **自动门禁**：两端闭包差集为空，文案卫生通过；设备 3 环几何/160ms、连续经文历史、内联最新字，以及小程序单进度/最新字下划线均有自动校验。
- **bmad-ux 复核**：structure/prose review 与 consolidated validation report 已落在 `.../ux-Electronic_Wooden_Fish-2026-09-08/{review-structure.md,review-prose.md,review-rubric.md,validation-report.md,validation-report.html}`，无未解决 finding。
- **SquareLine 规划**：已新增 `lvgl-design/squareline_studio/ewf_project_manifest.json` 与目录 README，登记 1.6.1/8.3.11/8.4.0 版本边界、resident shell、按需 settings、组件变体和容量风险；未生成假 `.spj` 或字体位图。
- **门禁状态**：见本节开头的“当前门禁”；设备轨 `DEVICE-01` 已签收并冻结为 E3 视觉验收基线，小程序轨 `MINI-06` 待签收，四份 UX 文档整体继续保持 `draft`。

### Stage 5a · 设备轨 → SquareLine → LVGL C【门禁：校验器全绿 + 帧可渲染】
- **动作**（工具见 §6；注意：这些工具目前面向 legbot，需按 EWF 页集重定向后再运行）：
  1. `python3 lvgl-design/squareline_studio/tools/generate_squareline_project.py --html lvgl-design/ewf-device-ui.html --project-dir lvgl-design/squareline_studio` 生成 `.spj/.sll/.slp`。
  2. 作者在 SquareLine Studio 1.6.1 打开 `.spj` 手工校调并 Export LVGL C（导出目标 LVGL 8.3.11，固件用 8.4.0，导出后在 8.4.0 重编译；**导出物落 `Embedded/components/ui/generated/`**：ui.c/ui.h/screens/components/fonts/filelist.txt）。
  3. 父 CMake 用 `generated/filelist.txt` 登记；`compat/lvgl/lvgl.h` 转发到 ESP-IDF lvgl；业务在 `bindings/`；接入 `ui_task`（结构照搬 legbot `components/ui` + `components/services/ui_service`，删 watch 专属屏）。
  4. 跑 `postprocess_squareline_export.py`、`validate_squareline_project.py`、`validate_font_coverage.py`；终验 `missing_glyphs:0` + 字体字节一致 + ESP-IDF 构建过。
- **DoD**：对照 FR-E-006/007 与 E3 story，三主页/设置/完成遮罩帧可渲染；作者签收。

### Stage 5b · 小程序轨 → uni-app 复刻【门禁：每页与 HTML 对拍】
- **动作**：按 `ewf-miniapp-ui.html` + `UI_CONTRACT-miniapp.md` 复刻到 `cloud/frontend/src/pages/{reading,records,device,settings}.vue`（+ LOGIN/OVERLAY）；scss 令牌取自 DESIGN `mini.*`/`brand.*`（映射表见 UI_CONTRACT-miniapp §1）；页面状态接 Pinia + mock（数据形状按 sync-contract frontend API **草案**，不依赖真实 backend）。
- **DoD**：每页与 HTML 布局/令牌/状态文案对拍一致；状态清单覆盖 UI_CONTRACT-miniapp §4。

### Stage 6 · 收口
- 作者两轨逐屏 demo 签收；把 UI 设计稿作为 E3/S3/S4 视觉验收基线记入 `epics.md`；把 ARCHITECTURE-SPINE Deferred「UX 层承接」翻转为「已承接」；DESIGN/EXPERIENCE/UI_CONTRACT 置 `status: final`。

---

## 4. 权威与约束（不可越界）

**权威链（单向、最新覆盖）**：UX 契约 > 轨道 UI_CONTRACT > pen/HTML > 代码产物。spines 冲突时以 spines 为准。

**四条硬规则**：
1. **闭包校验**：HTML 帧 `[UI][PAGE|STATE|CMP|VAR]` 与 UI_CONTRACT 闭包差集为空 = 完成（缺帧=未完成）。
2. **令牌单一来源**：只用 DESIGN.md tokens；设备编译字体禁手改 .c/复制字号/只改固件侧，字体资产与 `generated/fonts` 字节一致；小程序 scss 与 tokens 一对一。
3. **两层分离（设备）**：`generated/` 只布局/字体/事件空桩；业务渲染/路由/typed intent 在 `bindings/`；LVGL 仅 `ui_task` 独占调用。
4. **作者签收门禁**：方向稿 / 全帧闭包 / LVGL 集成 / 小程序对拍 各门禁由 Hongchenke 逐屏签收。

**产品不变量（UI 不得突破）**：小程序**无点击木鱼、不产生敲击**（FR-F-002），只呈现 backend 已确认进度（AD-2）；设备本地镜像须带「同步中/待同步」标记（AD-2）；未校时显示「待校时」（AD-6）；7 字带空位低对比、**不预览未来经文**（AD-19）；心经进度显示已诵字数/总字数/百分比，今日敲击与累计敲击分开；小程序正文 append-only；后端持久化 JSON 零库（AD-16）；MVP 无 BLE/Wi-Fi/GPS 业务链路。最终页面禁止 AI 思维链、内部推理、Node ID、调试文案和作者提示。

---

## 5. 文件清单表

**当前可用（Stage 4 输入与产出）**
| 路径 | 作用 |
| --- | --- |
| `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/{DESIGN,EXPERIENCE,UI_CONTRACT-device,UI_CONTRACT-miniapp}.md` | 设计唯一权威 + 闭包真值 |
| `_bmad-output/planning-artifacts/epics.md`（Epic E3 / S3 / S4 各 Story） | 页面/状态需求来源与验收对照 |
| `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/{prd.md,addendum.md}` 与 `_bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/addendum.md` §5/§6 | 页面内容与默认值权威 |
| `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`（Stack/Deferred/AD-4/AD-2/AD-6） | 技术栈与不变量 |
| `/Users/hongchenke/Documents/Github/legbot_watch/{lvgl-design,docs}/` | 设备轨流水线/样式/踩坑只读蓝本 |
| `docs/embedded/**`、根 `AGENTS.md` | 设备侧迁移知识（含 LVGL 字体/两层分离规则） |
| `lvgl-design/{ewf-device-ui.pen,ewf-device-ui.html,ewf-device-ui-export.html,assemble_ewf_full_html.py,device-style-options.json}` | `DEVICE-01` 设备 Stage 4 真源与组装器 |
| `miniapp-design/{ewf-miniapp-ui.pen,ewf-miniapp-ui.html,ewf-miniapp-ui-export.html,miniapp-style-options.json}` | `MINI-06` 小程序 Stage 4 真源 |
| `lvgl-design/squareline_studio/font_glyph_contract.json` | 设备设计阶段静态/动态字形登记（Stage 5 继续落实） |
| `lvgl-design/validate_ewf_ui_closure.py`、`lvgl-design/ui_text_hygiene.py` | 当前闭包与文案卫生门禁 |

**后续生成（Stage 5，不代表当前已完成）**
| 路径 | 说明 |
| --- | --- |
| `lvgl-design/squareline_studio/*.spj…` | SquareLine 工程（Stage 5a） |
| `Embedded/components/ui/generated/**`、`bindings/**` | LVGL C 导出与业务接入 |
| `cloud/frontend/src/pages/**` | 小程序 uni-app 复刻 |

---

## 6. 工具与连接（Pencil MCP 已修复）

- **Pencil MCP 桌面连接修复记录**：现象=`get_editor_state` 报 `failed to connect to running Pencil app: trae`。根因=本会话启动时（2026-09-08 16:59）加载了旧配置指向的 **trae 版** pencil server（`~/.pencil/mcp/trae/… --app trae`），而作者打开的桌面 Pen.app 是 desktop 宿主、`connectedAgents:[]`。处置：`~/.claude.json` 的 `mcpServers.pencil` 现指向 `/Applications/Pen.app/… --app desktop --agent claudeCodeCLI`（与打开的 Pen 一致）；旧的 trae server 进程已 kill。
- **接手时若仍连不上**：运行 `/mcp` 找到 `pencil` 并 **Reconnect**（或重启 Claude 会话），确认 Pen.app 顶部连接面板 `connectedAgents` 非空后再 `get_editor_state`。Pen 内打开 `legbot_watch/lvgl-design/watch-lvgl.pen` 可作设备画布参考（只读蓝本，勿改）。
- **SquareLine 工具**（`lvgl-design/squareline_studio/tools/`）：`generate_squareline_project.py`(html→工程/字体)、`postprocess_squareline_export.py`(规范化导出)、`validate_squareline_project.py`(工程/页/屏/字体闭包)、`validate_font_coverage.py`(字形覆盖)。**注意**：均照搬自 legbot，现为 **legbot 定向**（页面集/字体子集是 watch 的），须在生成本仓 `ewf-device-ui.html` 与 `font_glyph_contract.json` 后按 ewf 页集**重定向/精简**（见 `lvgl-design/README.md` 与 UX `.memlog.md`）；未重定向前不要直接拿它们当 EWF 终验。
- **EWF 方向稿校验器**：`lvgl-design/validate_ewf_ui_closure.py`（`--stage candidates` 检查每轨 10 个 style，`--stage full --style STYLE-ID` 检查选中 style 的 15/14 帧）；`ui_text_hygiene.py` 检查最终可见文案和 Node ID 泄漏。
- **HTML 形态范式**：参考 legbot `lvgl-design/watch-lvgl.html`（静态 Tailwind+内联 SVG lucide；每屏 410×502 圆角黑底块；节点 `data-pencil-name`；单一根容器横向铺排）。

---

## 7. 验收/门禁基线（UI_CONTRACT 状态闭包，作为 Stage 4 真值来源）

**设备轨（UI_CONTRACT-device §4，15 帧）**：`MUYU[BASE/EMPTY/MID/FULL/DONE_OVERLAY/CHARGING_PAUSE/UNTRUSTED_TIME]`、`JINGWEN[BASE]`、`TONGJI[BASE/UNTRUSTED_TIME]`、`SHEZHI[BASE/SYNC_BUSY/SYNC_OK/SYNC_PENDING/SYNC_FAIL]`、`SHELL statusbar[同步:OK/PENDING/BUSY/FAIL · 充电 · 低电 · 故障]`。

**小程序轨（UI_CONTRACT-miniapp §4，14 帧）**：`LOGIN[BASE/PERM_ERROR]`、`READING[LIVE/REPLAY/OFFLINE/EMPTY/DONE]`、`RECORDS[BASE/EMPTY]`、`DEVICE[BASE/FAIL_RETRY]`、`SETTINGS[BASE/PENDING]`。

**保留的产品边界**：设备保持圆弧屏、四信号状态组件、右侧电池、可辨识木鱼和心经进度，并采用 `DEVICE-01` 黑底白色海报式器物语言；小程序保持浅纸面、基线网格、印谱印记、连续确认正文、总进度和无木鱼输入，并采用 `MINI-06`。金色仅用于敲击反馈、最新字和进度，不重做页面底色。

---

## 8. 决策与口径同步记录

- **AD-16 覆盖 FR-B-009**：后端持久化 = JSON 原子文件、零数据库（存 `cloud/backend/data/`）；PRD §2/§6.1.9 与 addendum 已同步为 JSON 口径（epics coverage 亦按此）。
- **读取顺序 ≠ 构建顺序**：S0（sync-contract 冻结，epics Story S0.1）先落地后，Embedded lane 与 Software lane 可并行；跨层验收两段式（模拟器预演 + 真机签收）。
- **双轨视觉**：两轨「可不同但同源」，令牌单一来源；设备视觉新定、工程照搬 legbot（P7 纪律：Embedded 同仓顺序交付、只增补不重写）。
- **设计完成约束**：闭包校验 + 令牌/字体单一来源 + 两层分离 + 作者逐屏签收（详见 §4）。
- 未提交 git 改动集中在 `_bmad-output/planning-artifacts/{epics.md,ux-designs}`、`lvgl-design/`、`miniapp-design/`、`docs/`、`AGENTS.md`。

## 9. 2026-09-09 续作变更记录

- 上一版方向稿已回退，不再作为视觉真源。
- 本轮已重新保存两份真正重构的 Pen 候选板，并导出离线审阅 HTML；两轨各 10 个候选的闭包校验与文案卫生检查均已通过，HTML 已清除 `data-pencil-id` 与外部资源。候选已改为不同构图、阅读轴、材质/物件与进度表现，并由结构指纹校验拒绝只换色版本。
- Hongchenke 已锁定设备 `DEVICE-01` / 母版 `hbTEa` 与小程序 `MINI-06` / 母版 `A358t`；四份 UX 文档、两份 style options 与双轨 README 已同步口径。
- 设备 Pen 已按反馈修正：非最新字符不再保留大字体高亮；经文页最新字内联于已确认正文流尾；木鱼页三环采用 160ms 金色闪光组件变体；空带/填充/满带和待校时均下沉为组件状态；设置页保留唯一页面，亮度与自动熄屏保持一致，仅同步组件拥有状态变体。小程序阅读页保持连续正文、单一进度，并将最新字与下划线绑定。
- 根据 legbot_watch 与 SquareLine 配置复核，设备 Pen 已进一步重构：三主页进入常驻横向 shell；`RsuSH` 重做为今日/累计统计卡；`W3247` 按 `mH17G` 的两页、四行可视设置列表重做，并加入木鱼版本/ID；`PawlG`、`lo9C1`、`fevaG` 统一为 10 槽/行的经文组件。
- Stage 4 两端 HTML 已生成：设备 15 帧、小程序 14 帧；full closure、文案 hygiene 和单元测试已通过。自动通过不等于完成。
- 当前门禁：等待 Hongchenke 逐屏视觉签收；签收前 UX 状态保持 `draft`，不得进入 Stage 5。设备字体合同已登记设计阶段的字形（当前不能作为固件终验），SquareLine 工具仍需在 Stage 5 按 EWF 页集与字体角色重定向后才能完成固件终验。

## 10. 2026-09-10 快速收口记录

- 设备 Pen 的 `mDOlU`（7 字带）与 `JmQTi`（心经进度）已提升为可复用母版；`ZTLH6` 充电页与 `VkR3m` 完成页仅覆写实例内容/状态，未再复制屏幕专属字带或进度树。
- `ZauNI` 状态板已校正为：`EMPTY` 无可见大字、`BASE` 最新字槽 3、`MID` 最新字槽 7、`FULL` 最新字槽 4；`DONE_OVERLAY` 末字独占槽 7，移除普通槽重叠。
- `RsuSH`、`W3247`、`Hlo77`、`PawlG/lo9C1/fevaG` 的组件关系和变体样式已同步至 `ewf_project_manifest.json`、`DESIGN.md`、`UI_CONTRACT-device.md`、`lvgl-design/README.md` 与 SquareLine README。
- 最新导出与门禁证据：设备 full 15/15、小程序 full 14/14、两轨候选各 10/10；双端文案卫生无错误，18 项单元测试通过，JSON 与 `git diff --check` 通过。`ewf-device-ui.pen` 已由 Pencil 保存。
- 仍不提前生成 SquareLine `.spj` 或字体位图；Stage 5 需先按 EWF 页集重定向 legbot 工具并完成真实授权/固件 round-trip。当前唯一未完成门禁仍是作者逐屏视觉签收。

## 11. 2026-09-11 设备 Pen 修订与 HTML 重生成

- **Pencil MCP 连接**：`~/.claude.json` 的 `mcpServers.pencil` 原指向 cursor 版 server（`--app cursor`），与桌面 Pen.app（`--app desktop`，socket `pencil-desktop.sock`）不匹配。已改为 `/Applications/Pen.app/Contents/Resources/app.asar.unpacked/out/mcp-server-darwin-arm64 --app desktop --agent claudeCodeCLI`；该配置为全局唯一定义点，无项目级覆盖，后续会话直接可用。备份 `~/.claude.json.bak-20260911`。
- **设置页 `W3247` 修复**：`YxGuI`（亮度选择器）与 `oLRuW`（熄屏选择器）两个 reusable 母版曾被置于 `y=2000` 画布外，导致 `UG52G`/`TJWQe` 两行的三选项不可见；已归位到行内 `x=198,y=16`。`cQ5wd`（sync-btn 母版）由 `x=390,y=2000` 回到 page-1 第 4 行 `x=0,y=264`；误占第 4 行的 `j6zSeO`（device-identity 母版）移回文档顶层母版区。
- **今日敲击间距**：母版 `Z8rcP` 的 label 框 86px 宽而文字实际约 59px，value 又右对齐，造成约 65px 空隙；现收为 label 宽 63、value `x=70` 且左对齐，与「累计敲击 3,456 次」的 11px 基准一致。`hbTEa`/`ZTLH6`/`VkR3m` 及变体区同步生效。
- **统计页 `RsuSH` 重构**：删除违反契约的近 7 日 / 近 30 日 / 连续诵读三张卡（PRD §145、epics FR-E-006、UI_CONTRACT ×3 均规定设备仅今日与累计），并删除解释性文案「累计值跨诵读周期保留」。今日/累计保留双卡，新增 `reading-progress`（`FKhdz`）：环形进度 + `N / 260 字` + 百分比 + 第 N 次诵读。
- **经文页槽位**：每行由 10 槽改为 **13 槽**（`fontSize=24 × 13 = 312 ≤ 框宽 320`），`lo9C1`/`fevaG`/`PawlG` 三组件一致；`UI_CONTRACT-device.md`、`EXPERIENCE.md`、`DESIGN.md`、两份 README 与 `ewf_project_manifest.json` 的 `line_chars` 已同步。
- **组件去重与清理**：`Z8rcP` 确认为 today-taps 唯一母版（页面均为 ref 实例）；`Hlo77` 中内容与 MUYU 完全重复的两个 TONGJI 变体已删；零引用的 `T300Z`/`R8iG9`/`kz1aY` 删除（`n2eoh` 保留，3 处引用）。
- **状态栏状态陈列**：新增 `cmp_statusbar_states`（`qLhoo`），以 ref + descendants 表达 12 态——信号连接的 connected/`signal-zero`/`wifi-off`/全禁用，同步的 ok/busy/pending/fail，电量的 full/medium/low/charging。补齐 `DESIGN.md` 已定义但 pen 缺失的变量 `ewf-device-battery-low`、`ewf-device-danger`。
- **三环闪光修复**：`n2eoh` 的 idle 描边为白色低透明（`#fbfaf066`/`044`/`033`），flash 规格本已登记在 metadata（`flashStroke:#e6bd69`、`durationMs:160`）与 manifest。HTML 侧此前**完全无效**：CSS 写 `stroke` 而环节点用 `outline`（div 上 `stroke` 不生效），且无任何触发。已改为 `outline-color` 覆盖并加 `:hover`/`:active` 预览，`data-ewf-motion="flash"` 保留给固件语义。母版区新增 `cmp_tap_rings_states`（`JZhSx`）作 IDLE/FLASH 对照。**不做节点变体**：flash 不改变布局与输入边界，按契约由 `bindings` 驱动。
- **HTML 重生成**：`ewf-device-ui-export.html` 由 Pen 重新导出，`assemble_ewf_full_html.py` 重跑生成 `ewf-device-ui.html`（15 帧）。
- **组装器修复**：`replace_node` 原先用非贪婪正则匹配到第一个内层闭合标签，当 `today-taps` 由单节点变为 ref 实例（label + value 嵌套）后，会把其后同级内容整段丢弃（EMPTY 帧由 33KB 掉到 16KB）。现改用嵌套感知的 `element_span` 定位；EMPTY/UNTRUSTED 的今日敲击改为覆写 value 子节点而非整体文本。
- **门禁**：闭包校验 15/15 通过（errors 0、warnings 0）；文案卫生 `--mode candidates` 通过；18 项单元测试通过；`font_glyph_contract.json` 的设计快照与运行时标签随结构同步更新。
- **遗留口径**：`ui_text_hygiene.py --mode production` 报 `design_annotation_attribute`（要求交付 HTML 不含 `data-pencil-name`），但 assemble 刻意保留该属性作闭包标记（AGENTS 只要求清除 `data-pencil-id`）。**新旧 HTML 同样触发**，属既有口径冲突，未在本次改动中变更。
- 当前门禁仍为作者逐屏视觉签收；四份 UX 文档保持 `draft`。

## 12. 2026-09-11 设备轨签收与规范落定

- **签收**：作者确认设备侧 `ewf-device-ui.pen` 满足需求，设备轨 `DEVICE-01` 完成逐屏视觉签收。`lvgl-design/ewf-device-ui.pen` 与 `ewf-device-ui.html` 冻结为 **E3 视觉验收基线**；E3 各 story 的实现须与其对拍。
- **落定范围**（规格与交互逻辑写入文档，pen 是唯一视觉真源）：
  - `DESIGN.md`：设备母版/变体映射表登记 `FKhdz` reading-progress、`qLhoo` cmp_statusbar_states、`JZhSx` cmp_tap_rings_states；`tap-rings` 明确 flash 为运行时描边补丁、不建节点变体；`battery-status` 补满/中/低/充电色定义。
  - `EXPERIENCE.md`：三环 flash 标注为描边色运行时补丁（母版只导出 idle 白描边）；新增「纯色/图标类瞬时反馈不进入状态建模」条目并集中列出设备侧五个状态对照板。
  - `UI_CONTRACT-device.md`：§4.1 修正 `Hlo77`→`cmp_today_taps_variants`、`fWCbZ`→`cmp_sync_btn_variants`，`T300Z` 状态板替换为 `JZhSx`，新增 `qLhoo`；§5 组件清单补 `reading-progress` 与状态板引用，`stat-card` 几何订正为 378×64；§6 导出边界补 flash 与状态补丁的 bindings 落点。
  - `epics.md`：UX 段更新为设备轨已签收并作为 E3 视觉验收基线，小程序轨待签收。
  - `ARCHITECTURE-SPINE.md`：Deferred 表「设备 UI 默认值…与统计页展示范围」翻转为**已承接（2026-09-11）**。
- **状态标记**：`DESIGN.md` / `EXPERIENCE.md` / `UI_CONTRACT-device.md` / `UI_CONTRACT-miniapp.md` 的 `status` 仍为 `draft` —— 两份 UX 文档与两份契约按双轨整体定稿，须待小程序轨 `MINI-06` 也签收后再统一置 `final`；`updated` 字段已更新为 2026-09-11。
- **下一步**：完善并签收 `miniapp-design/ewf-miniapp-ui.pen`（`MINI-06`），随后回写 Stage 6 收口。
