# 电子木鱼（EWF）交接：BMAD 当前阶段 + UI 设计任务接管

- **状态**：生效；UX 视觉设计返工，Stage 3 风格候选重新开始（2026-09-09）
- **读者**：下一个接手 agent（尤其具备视觉/前端能力的 agent）与人类作者 Hongchenke
- **目的**：单文件自足——搞清 EWF 现在处于 BMAD 哪个阶段、正在执行什么任务、从哪继续、受哪些门禁约束；**无需回看其它文档即可从本节「接手者要做」开始干活**。本文档后续阶段变动时由执行者回写 `updated` 并追加变更记录。

---

## 1. BMAD 定位（我们现在在哪）

阶段序列：`CB 产品简报 → PRD → CU(UX) → CA 架构 → CE epics/stories → SP sprint → BD build → CR review`。

**已完成（产物均在仓库，除标 draft 外视为 final/生效）：**

| 阶段 | 菜单码 | 产物路径 | 状态 |
| --- | --- | --- | --- |
| 产品简报 | CB | `_bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/{brief.md,addendum.md}` | final（authority：产品方向/交互合同） |
| PRD | PRD | `…/prds/prd-Electronic_Wooden_Fish-2026-09-07/{prd.md,addendum.md}` | final（FR-E/B/F、UJ、SM、跨层字段） |
| 架构 | CA | `…/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` | final（AD-1~19、板级 GPIO 合同、Stack、Deferred） |
| Epic/Story | CE | `_bmad-output/planning-artifacts/epics.md` | final（9 epic / 53 story：E1–E4、S0–S4） |
| UX 契约 | CU | `…/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/{DESIGN.md,EXPERIENCE.md,UI_CONTRACT-device.md,UI_CONTRACT-miniapp.md,.memlog.md}` | **draft**（视觉返工中；style 候选签收后再定稿） |
| docs | — | `docs/`（含本交接）、`docs/embedded/`（legbot 迁移） | 本任务产物 |

**尚未进行：** SP sprint（菜单码 SP/SS）→ BD build（菜单码 BD，含 Developer 代理）。源码目录 `Embedded/`、`cloud/backend/`、`cloud/frontend/` **仍为空占位**（绿地，未进入实施）。

**一句话现状：** planning 全部完成、UX 契约 draft、上一版方向稿已回退；当前重新构建设备 10 种与小程序 10 种禅意风格候选，尚未进入 sprint/build。

---

## 2. 正在执行的任务（UI 设计流水线 Stage 0–6）

设备 OLED-LVGL 轨与微信小程序轨的 UI 走同一设计流水线（对齐 legbot 先例）：`UX 契约 → 轨道 UI_CONTRACT → pen 原型 → 静态 HTML 导出 → [设备 SquareLine→LVGL C | 小程序 uni-app 复刻] → 作者逐屏签收`。

| Stage | 内容 | 状态 |
| --- | --- | --- |
| 0 | 脚手架：UX 运行目录、`lvgl-design/`、`miniapp-design/`、legbot 4 个工具收录 | ✅ 完成 |
| 1 | UX 契约 DESIGN.md + EXPERIENCE.md（双轨；待视觉返工收口） | 🔄 返工中(draft) |
| 2 | 双轨 UI_CONTRACT（可校验闭包表） | ✅ 完成(draft) |
| 3 | **10+10 风格候选 Pen/HTML（设备木鱼页 + 小程序心经阅读页）** | 🔄 **返工中，未签收** |
| 4 | 全帧闭包 pen/HTML + 闭包校验脚本 | ⏳ |
| 5a | 设备 SquareLine → LVGL C 导出与接入 | ⏳ |
| 5b | 小程序 HTML → uni-app 复刻 | ⏳ |
| 6 | 收口：作者逐屏签收 + 回写 epics/架构 Deferred | ⏳ |

**返工原因：** 上一版仅改变颜色、设备状态栏未组件化、木鱼不可辨识、小程序正文不是连续阅读流，并含重复进度和作者化提示。当前以用户最新需求为唯一视觉返工输入。

---

## 3. 接手者要做（从 Stage 3 开始，按顺序）

> 全局：中文输出；每到达一个**门禁**即暂停请作者签收；HTML 帧命名与闭包是硬校验，缺帧视为未完成。

### Stage 3 · 两轨 10+10 风格候选（pen → HTML）【门禁：候选板签收】
- **输入**：`DESIGN.md`、`EXPERIENCE.md`、两份 UI_CONTRACT、PRD/简报和 `frontend-design-pro` 规则。上一版 Pen/HTML 不作为视觉真源。
- **动作**：清空旧 Pen 画布，在每份 Pen 中建立 `[UI][STYLES]` 风格板和 10 个真实 sibling frame；每个候选至少改变构图、字体层级、材质/物件处理、进度表达、光影/签名细节中的三项。
- **设备必备**：可复用 `gGgAm` statusbar、四信号组件、右侧电池、7 字窗口、心经进度字数+百分比、今日敲击、累计敲击；木鱼主视觉复用 `cmp_woodfish_mark` 与参考轮廓资产，页面保留器物/触区语义。
- **小程序必备**：连续 append-only 心经正文、单一进度组件、字数+百分比+进度条、四项导航；不出现木鱼、孤立标点、重复进度或解释性节点。
- **产物**：`lvgl-design/ewf-device-direction.pen/.html`、`miniapp-design/ewf-miniapp-direction.pen/.html`，另附两份 10 风格并排审阅 HTML。
- **DoD**：候选板通过 hygiene/几何/组件/风格差异校验；作者分别选择 1 个风格。

### Stage 4 · 选中风格全帧闭包（pen/HTML）【门禁：闭包校验通过 + 作者抽查】
- **前置**：只有在作者为两端各选择一个 style 后才能开始。按两份 UI_CONTRACT 逐帧补齐设备 15 帧与小程序 14 帧；选中 style 是唯一视觉母版。
- **动作**：写/复用校验脚本：解析 HTML 全部 `data-pencil-name`，与契约闭包做差集；**差集为空才算完成**（可为 python 单文件，放 `lvgl-design/` 或 `tools/`，README 注明）。
- **产物**：`lvgl-design/ewf-device-ui.pen/.html`、`miniapp-design/ewf-miniapp-ui.pen/.html`（HTML = 后续复刻/生成的**可解析事实源**）。
- **DoD**：闭包差集为空；无重复进度、作者化文案或孤立标点；作者抽查逐屏通过。

### Stage 5a · 设备轨 → SquareLine → LVGL C【门禁：校验器全绿 + 帧可渲染】
- **动作**（工具见 §6，注意工具目前是 legbot 定向、需按 ewf 页集重定向后再跑）：
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

**输入（读）**
| 路径 | 作用 |
| --- | --- |
| `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/{DESIGN,EXPERIENCE,UI_CONTRACT-device,UI_CONTRACT-miniapp}.md` | 设计唯一权威 + 闭包真值 |
| `_bmad-output/planning-artifacts/epics.md`（Epic E3 / S3 / S4 各 Story） | 页面/状态需求来源与验收对照 |
| `_bmad-output/planning-artifacts/prds/…/prd.md`（FR-E-006/007、FR-F-002~008/010）与 `briefs/…09-08/addendum.md` §5/§6 | 页面内容与默认值权威 |
| `_bmad-output/planning-artifacts/architecture/…/ARCHITECTURE-SPINE.md`（Stack/Deferred/AD-4/AD-2/AD-6） | 技术栈与不变量 |
| `/Users/hongchenke/Documents/Github/legbot_watch/lvgl-design/` + `docs/` | 设备轨流水线/样式/踩坑只读蓝本 |
| `docs/embedded/**`、根 `AGENTS.md` | 设备侧迁移知识（含 LVGL 字体/两层分离规则） |

**产出（写）**
| 路径 | 说明 |
| --- | --- |
| `lvgl-design/ewf-device-direction.pen/.html` → `ewf-device-ui.pen/.html` | 设备轨设计稿（Stage3→4） |
| `miniapp-design/ewf-miniapp-direction.pen/.html` → `ewf-miniapp-ui.pen/.html` | 小程序轨设计稿 |
| `lvgl-design/squareline_studio/*.spj…` | SquareLine 工程（Stage5a） |
| `Embedded/components/ui/generated/**`、`bindings/**` | LVGL C 导出与业务接入 |
| `cloud/frontend/src/pages/**` | 小程序复刻 |

---

## 6. 工具与连接（Pencil MCP 已修复）

- **Pencil MCP 桌面连接修复记录**：现象=`get_editor_state` 报 `failed to connect to running Pencil app: trae`。根因=本会话启动时（2026-09-08 16:59）加载了旧配置指向的 **trae 版** pencil server（`~/.pencil/mcp/trae/… --app trae`），而作者打开的桌面 Pen.app 是 desktop 宿主、`connectedAgents:[]`。处置：`~/.claude.json` 的 `mcpServers.pencil` 现指向 `/Applications/Pen.app/… --app desktop --agent claudeCodeCLI`（与打开的 Pen 一致）；旧的 trae server 进程已 kill。
- **接手时若仍连不上**：运行 `/mcp` 找到 `pencil` 并 **Reconnect**（或重启 Claude 会话），确认 Pen.app 顶部连接面板 `connectedAgents` 非空后再 `get_editor_state`。Pen 内打开 `legbot_watch/lvgl-design/watch-lvgl.pen` 可作设备画布参考（只读蓝本，勿改）。
- **SquareLine 工具**（`lvgl-design/squareline_studio/tools/`）：`generate_squareline_project.py`(html→工程/字体)、`postprocess_squareline_export.py`(规范化导出)、`validate_squareline_project.py`(工程/页/屏/字体闭包)、`validate_font_coverage.py`(字形覆盖)。**注意**：均照搬自 legbot，现为 **legbot 定向**（页面集/字体子集是 watch 的），须在生成本仓 `ewf-device-ui.html` 与 `font_glyph_contract.json` 后按 ewf 页集**重定向/精简**（见 `lvgl-design/README.md` 与 UX `.memlog.md`）；未重定向前不要直接拿它们当 EWF 终验。
- **EWF 方向稿校验器**：`lvgl-design/validate_ewf_ui_closure.py`（`--stage candidates` 检查每轨 10 个 style，`--stage full --style STYLE-ID` 检查选中 style 的 15/14 帧）；`ui_text_hygiene.py` 检查最终可见文案和 Node ID 泄漏。
- **HTML 形态范式**：参考 legbot `lvgl-design/watch-lvgl.html`（静态 Tailwind+内联 SVG lucide；每屏 410×502 圆角黑底块；节点 `data-pencil-name`；单一根容器横向铺排）。

---

## 7. 验收/门禁基线（EXPERIENCE 状态闭包，作为 Stage4 真值来源）

**设备轨（UI_CONTRACT-device §4，15 帧）**：`MUYU[BASE/EMPTY/MID/FULL/DONE_OVERLAY/CHARGING_PAUSE/UNTRUSTED_TIME]`、`JINGWEN[BASE]`、`TONGJI[BASE/UNTRUSTED_TIME]`、`SHEZHI[BASE/SYNC_BUSY/SYNC_OK/SYNC_PENDING/SYNC_FAIL]`、`SHELL statusbar[同步:OK/PENDING/BUSY/FAIL · 充电 · 低电 · 故障]`。

**小程序轨（UI_CONTRACT-miniapp §4，14 帧）**：`LOGIN[BASE/PERM_ERROR]`、`READING[LIVE/REPLAY/OFFLINE/EMPTY/DONE]`、`RECORDS[BASE/EMPTY]`、`DEVICE[BASE/FAIL_RETRY]`、`SETTINGS[BASE/PENDING]`。

**保留的产品边界**：设备必须保持圆弧屏、四信号状态组件、右侧电池、可辨识木鱼和心经进度；小程序必须保持浅米阅读基底、连续确认正文、总进度和无木鱼输入。具体视觉风格等待 10+10 候选签收。

---

## 8. 决策与口径同步记录

- **AD-16 覆盖 FR-B-009**：后端持久化 = JSON 原子文件、零数据库（存 `cloud/backend/data/`）；PRD §2/§6.1.9 与 addendum 已同步为 JSON 口径（epics coverage 亦按此）。
- **读取顺序 ≠ 构建顺序**：S0（sync-contract 冻结，epics Story S0.1）先落地后，Embedded lane 与 Software lane 可并行；跨层验收两段式（模拟器预演 + 真机签收）。
- **双轨视觉**：两轨「可不同但同源」，令牌单一来源；设备视觉新定、工程照搬 legbot（P7 纪律：Embedded 同仓顺序交付、只增补不重写）。
- **设计完成约束**：闭包校验 + 令牌/字体单一来源 + 两层分离 + 作者逐屏签收（详见 §4）。
- 未提交 git 改动集中在 `_bmad-output/planning-artifacts/{epics.md,ux-designs}`、`lvgl-design/`、`miniapp-design/`、`docs/`、`AGENTS.md`。

## 9. 2026-09-09 续作变更记录

- 上一版方向稿已回退，不再作为视觉真源。
- Stage 3 重新开始：先产出设备 10 种、小程序 10 种真正不同的禅意 style；候选板通过 hygiene/几何/组件校验后再由 Hongchenke 各选 1 种。
- 本轮已重新保存两份真正重构的 Pen 候选板，并导出离线审阅 HTML；设备 10/10、小程序 10/10 候选闭包与文案 hygiene 均通过，HTML 已清除 `data-pencil-id` 与外部资源。候选已改为不同构图、阅读轴、材质/物件与进度表现，并由结构指纹校验拒绝只换色版本。
- 当前门禁：等待 Hongchenke 从两份 10 风格候选板中各选择 1 种；选定前不得创建 `ewf-device-ui.*` / `ewf-miniapp-ui.*` 全帧稿，也不得进入 Stage 5 复刻。
