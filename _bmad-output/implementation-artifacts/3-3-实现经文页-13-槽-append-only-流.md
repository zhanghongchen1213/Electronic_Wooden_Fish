# Story 3.3: 实现经文页 13 槽 append-only 流

story_key: 3-3-实现经文页-13-槽-append-only-流
baseline_commit: 0c9df2dde9dafb5d35d0c8ae3f09ec9de45ad7d2

Status: done

<!-- Note: Ultimate context engine analysis completed - comprehensive developer guide created. 本 Story 闭合 JINGWEN 页 scripture-history 13 槽 append-only、流尾 glyph-current、回看与新字锚回流尾；不实现统计页（3.4）、设置五态（3.5）、完成遮罩（3.6）。不依赖 cloud。不绘制电子木鱼/三环。 -->

## Story

As a 设备诵读者，
I want 在经文页回看已经敲出的内容并始终找到最新字，
so that 连续诵读不会被页面切换打断。

**覆盖需求：** FR-E-006、UX-DR9、AD-19、NFR2

本 Story 是 Epic 3 的**经文页阅读流**：在 Story 3.1 壳层 + Story 3.2 木鱼七字带/字体档基线上，把 JINGWEN 从标题占位升级为同一篇 **13 槽/行 append-only** `scripture-history` + 流尾 `glyph-current` + 心经进度，并实现回看与「新字锚回流尾」。  
**不**实现统计页今日/累计卡（3.4）、设置 sync-btn 五态（3.5）、完成遮罩与跨轮动作（3.6）。  
**必须复用** `progress_service` 的 `tap_round_cursor`/`tap_round_id` 快照、`ewf_scripture_canonical.h` 步偏移展开、`device_nav_service` 的 `active_page`/`screen_state`、3.2 的显示字符展开语义与 Serif 字体档、`ui_service`/`bindings`——禁止第二套经文誊写、第二篇历史流、在 `generated/` 写业务、在经文页绘制可点击木鱼或三环。

## Acceptance Criteria

### AC 1：13 槽 append-only 与唯一流尾 `glyph-current`（UX-DR9 / AD-19 / NFR2）

**Given** 经文页已打开（亮屏、`active_page==JINGWEN`）  
**When** 设备本地记录新的可消费汉字（`tap_round_cursor` 推进，含 `physical_pvdf` / 他页 `device_touch` / `automatic_tap` 经统一队列落盘后的游标变化）  
**Then** 按 canonical **步**展开显示流：`step[i] = 可消费汉字 + 紧随非消费字符`；显示字符按序 **append** 到**同一篇**流，每行固定 **13** 槽，标点占槽；任意时刻至多一个可见 `glyph-current`（大字 + `{colors.brand.amber.400}`）位于**流尾**；已有前文**不覆盖、不清空、不重排**  
**And** 页面**不**绘制可点击电子木鱼、**不**绘制 `tap-rings`；空位/未填行**绝不**预览未来经文；分母固定 `EWF_SCRIPTURE_CONSUMABLE_COUNT=260`，不得把 13 当成经文上限；高速连击只投影最新游标，不丢正式字符（NFR2）

### AC 2：回看与新字锚回流尾（UX-DR9 / UJ-5）

**Given** 用户向上回看，或回看期间产生新敲击  
**When** 滚动手势完成，或新字事件导致显示流增长  
**Then** 用户只能浏览已确认/本地已记录内容（显示流长度 = 已消费步展开后的字符数 ≤ `EWF_SCRIPTURE_TOTAL_CHARS=303`）；新字追加后视口**自动锚回流尾**，使最新字重新成为焦点  
**And** **不**生成第二篇历史流；滚动/锚点失败只打中文日志，**不**阻塞输入队列、持久化与同步；横向滑仍切主页

### AC 3：心经进度投影（FR-E-006）

**Given** `watch_state` 快照含 `tap_round_cursor`  
**When** bindings 投影 JINGWEN  
**Then** `scripture-progress` 显示与木鱼页一致口径：`已诵 / 260`（或「心经进度 N / 260 字」）与 **0–100** 整型百分比（向下取整；`cursor==260` 强制 100）；可选投影 `round-index`「第 {tap_round_id} 次诵读」若快照可用  
**And** 本地未确认差值不得伪装成云端正式进度；同步短语继续走状态栏（3.1）

### AC 4：输入与手势边界（FR-E-007 / EXPERIENCE）

**Given** 用户在经文页  
**When** 触摸或实体敲击发生  
**Then** 经文页普通屏幕触摸**不**产生 `device_touch`/正式计数；实体 `physical_pvdf` 仍可推进统一队列（与 2.4/3.2 一致）；熄屏首触只唤醒  
**And** 在 `scripture-history` 视口内的垂直拖动驱动内容滚动（回看/跟尾）；**下滑进设置**仅当手势起点在 history 视口**外**（标题/状态栏带）时由既有 `device_nav` 识别——禁止把 history 内垂直滚动误开设置；滚动失败不改导航状态机

### AC 5：边界与卫生门禁

**Given** Story 3.1/3.2 壳层、七字带与字体合同已绿  
**When** 完成本 Story  
**Then** 主机可测：13 槽换行、标点占槽、前缀稳定、空流/满流、锚回流尾策略、不预览未来、无第二流；字体合同覆盖经文流动态标签与 `font_codes`；`missing_glyphs: 0`；用户可见文案无 TODO/draft/placeholder/Node ID/`data-pencil-id`  
**And** 不实现 `CHARGING_PAUSE` 输入暂停 gate；不改 `docs/contracts/canonical/**` 正文语义；所有 `lv_*` 仅 `ui_task`；不依赖 cloud

## Tasks / Subtasks

- [x] Task 1：冻结裁决并登记 deferred（AC: 1–5）
  - [x] 1.1 落地 Dev Notes「必须作出的裁决」A–I，在关键入口注释写明选择
  - [x] 1.2 明确本 Story **只消费** progress/nav 快照与 API，不重写累计/游标 owner；不改 3.2 木鱼触区语义
  - [x] 1.3 在 `_bmad-output/implementation-artifacts/deferred-work.md` 追加 `Deferred from: create-story of 3-3-...`（统计卡、设置五态、完成遮罩、真机滚动手感、history↔设置手势真机对拍等）
  - [x] 1.4 交接：3.4 统计；3.5 设置视觉；3.6 完成遮罩；Epic 7 真机对拍

- [x] Task 2：JINGWEN 对象树——history / 进度 / 可选 round-index（AC: 1, 3）
  - [x] 2.1 **先读**再改：`UI_CONTRACT-device.md` §4–§5、`DESIGN.md` `scripture-history`、`EXPERIENCE.md` UJ-5 /「经文页」敲击反馈、`ewf-device-ui.pen`/`ewf-device-ui-export.html` 中 JINGWEN/`PawlG`/`lo9C1`、现有 `ui_scr_shell.c`（JINGWEN 仅 title）、`ui_muyu_belt_policy.*`、`ui_service.c`
  - [x] 2.2 在 `ui_page_jingwen_root` 下增加：`scripture-history` 滚动容器（几何对齐 HTML：**362×286**，约 `left=24, top=102`）、内容区按行布局、流尾 `glyph-current`、`scripture-progress`（约 `top=410`）；保留既有 title「经文」与 statusbar；**禁止**挂 woodfish/tap-rings
  - [x] 2.3 generated 仅布局/空桩；行填充、焦点色、滚动锚点全部在 `bindings/`（建议 `ui_jingwen_stream_policy.c` + `ui_jingwen_projection.c`）
  - [x] 2.4 圆角/底色对齐 DEVICE-01（history 面 `#121212`、圆角约 18）；边距 16、净空 ≥8

- [x] Task 3：13 槽 append-only 纯逻辑（AC: 1, NFR2）
  - [x] 3.1 新增零 ESP-IDF 依赖的流布局 policy：输入 `round_cursor`，输出行列表（每行 ≤13 槽 UTF-8）、总显示字符数、`glyph_current` 的行/列索引（空流时 -1）
  - [x] 3.2 **复用**与 3.2 相同的步→显示字符展开语义（`EWF_SCRIPTURE_STEP_OFFSETS` + `DISPLAY_UTF8`）；推荐抽取共享 `expand_consumed_display` 到可复用模块（如 `ui_scripture_display_expand.*`），或在 jingwen policy 内调用与 belt 等价的静态逻辑——**禁止**第二份心经正文
  - [x] 3.3 算法硬约束：`chars = expand([0, cursor))` → 按 13 槽切行（末行可不满）；前缀在 cursor 增大时只尾部增长；`cursor` 不变时输出字节级稳定；禁止用 7 槽尾窗算法冒充全文流
  - [x] 3.4 进度：直接复用 `ewf_ui_muyu_progress_percent` / 同文案规则（整型 %）；禁止发明小数进度或第二分母
  - [x] 3.5 主机测：空流、1 字、满行 13、第 14 字换行、标点步「萨，」占两槽、前缀稳定（增量 append）、cursor=260 行数/焦点、不出现未来字

- [x] Task 4：LVGL 投影与锚回流尾（AC: 1, 2）
  - [x] 4.1 `ui_jingwen_projection_apply`：只读 snapshot；仅在 `ui_task` 调用；游标不变可跳过重排（可选脏标记），游标增大必须 append 可见字符
  - [x] 4.2 已确认字：Serif char 档（约 24–26px / 500）+ muted/正文色（HTML 已确认行 `#a6a29a`）；`glyph-current`：Serif glyph **52/700** + amber.400（复用 `EWF_UI_MUYU_AMBER_400_HEX` 或抽共享常量）
  - [x] 4.3 锚点：新字到达后在 `ui_task` 内 `lv_obj_scroll_to_view(glyph_current)` 或 `lv_obj_scroll_to_y(..., bottom, LV_ANIM_OFF/短动画)`；默认锚尾；回看态被新字打断后强制回尾（DESIGN `onNewChar: append-and-scroll-to-tail`）
  - [x] 4.4 滚动容器：`LV_DIR_VER`；失败路径不得调用 progress/tap API；中文日志
  - [x] 4.5 接入 `ui_service` 刷新路径（与 `ui_muyu_projection_apply` 并列），保证他页敲击后切回 JINGWEN 也能看到最新流

- [x] Task 5：手势与导航共存（AC: 4）
  - [x] 5.1 裁决落地：history 视口内垂直手势 → LVGL 滚动；history 外下滑 → 既有 settings；左右滑 → 换页
  - [x] 5.2 若 CST/nav 全局手势与 LVGL 抢事件：在 JINGWEN + 起点落在 history 时抑制 `EWF_NAV_INPUT_SWIPE_UP/DOWN` 进设置/误切，或让 scroll 对象吃掉垂直位移；主机测钉死「history 内下拖不开设置」
  - [x] 5.3 经文页点击不调用 `tap_input_service_submit_device_touch`

- [x] Task 6：字体合同（AC: 1, 5）
  - [x] 6.1 确认 3.2 已交付 `serif700_52` / `serif500_26`（或 400/500 档）覆盖经文正文与焦点；若 HTML 24px 与 26px 观感差需新档，走 `.fcfg`+`generate_ewf_fonts.py --fonts-only --sync-generated-fonts`，**禁止**手改位图
  - [x] 6.2 更新 `font_glyph_contract.json`：`runtime_labels` / `font_codes` 覆盖经文行动态 `lv_label_set_text` 与焦点字体切换
  - [x] 6.3 跑通：
    ```bash
    PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
      Embedded/lvgl-design/squareline_studio/tools/test_font_runtime_projection_coverage.py
    PYTHONDONTWRITEBYTECODE=1 python3 \
      Embedded/lvgl-design/squareline_studio/tools/validate_font_coverage.py
    ```

- [x] Task 7：主机测、合同断言与构建（AC: 全）
  - [x] 7.1 主机测（纯 C/Python）：13 槽换行、标点占槽、前缀稳定、锚点策略状态机（review→append→tail）、history 内下滑不进设置（policy 级）、无未来字
  - [x] 7.2 `contract_checks`：行宽=13、几何常量、禁止 JINGWEN 挂 woodfish 符号（能静态扫的扫）、复用进度分母 260
  - [x] 7.3 `python3 Embedded/tests/run_host_tests.py` 全绿；canonical tests 仍绿
  - [x] 7.4 环境允许时 `idf.py build`（ESP-IDF v5.5.4 / esp32s3）并记入 `docs/embedded/build_records/`；无样机标注 `hardware_pending`：滚动手感、锚尾动画、history↔设置手势、长文 24 行滚动性能

## Dev Notes

### 真源与冲突裁决

| 主题 | 唯一依据与本 Story 裁决 |
| --- | --- |
| Story 需求 | `epics.md` Epic 3 / Story 3.3 |
| 视觉/行为 | `DESIGN.md` + `EXPERIENCE.md` UJ-5 + `UI_CONTRACT-device.md`；Pen/HTML：`Embedded/lvgl-design/ewf-device-ui*` |
| 经文/步语义 | `docs/contracts/canonical/`（HS-1.0.0）；标点占槽；13=行宽不是上限；全文显示槽上限 303 |
| 架构 | AD-19 消费语义；AD-10 `ui_task`；AD-9 只读快照；AD-4 单一经文源；AD-1 统一队列 |
| 木鱼/三环 | **经文页不绘制**；计数触区仍仅 MUYU（3.2） |
| 充电 | PRD final：不实现 `CHARGING_PAUSE` 输入暂停 |
| 进度小数 | HTML 示例「16.2%」为设计示意；实现与 3.2 一致用**整型** 0–100 |
| 下滑冲突 | EXPERIENCE「向下滑回流尾」与「下滑进设置」并存 → 见裁决 H |

### 必须作出的裁决（实现采用）

| ID | 裁决 |
| --- | --- |
| A | 填充单位 = 显示字符流中的**单字符槽**（含标点占槽），与 3.2 裁决 A 一致；一步「萨，」可占两槽并可能跨行 |
| B | 布局 = 已消费显示流的**全文按 13 槽换行**（append-only），**不是** 7 槽尾窗；右侧/流尾=最新=`glyph-current` |
| C | 数据只读 `tap_round_cursor`（+ canonical）；UI **绝不**本地 `++cursor`；禁止第二历史缓冲区（不得另存一份「归档流」对象树） |
| D | JINGWEN **无** woodfish、**无** tap-rings；`automatic_tap`/`physical_pvdf` 只推进流与进度 |
| E | 新字 → `append` + **强制锚回流尾**（即使此前在回看）；回看只改变 scroll offset，不改变数据 |
| F | 进度复用 3.2 公式与分母 260；文案产品词；可选 `round-index` 用 `tap_round_id` |
| G | 字体：正文 Serif ~24–26/500；焦点 52/700 amber.400；缺档才扩合同，优先复用 3.2 已生成档 |
| H | **手势：** history 视口内垂直拖动 = 内容滚动；history **外**下滑 = 打开设置；左右滑 = 换页。钉死主机测，真机手感可 `hardware_pending` |
| I | `CHARGING_PAUSE`/统计卡/设置五态/完成遮罩：**不实现** |

### 当前代码实况（UPDATE — 必读）

| 模块 | 现状 | 本 Story 动作 |
| --- | --- | --- |
| `ui_scr_shell.c` | JINGWEN 仅 title「经文」+ statusbar | 补 history 滚动容器、行/焦点、进度 |
| `ui_muyu_belt_policy.c` | 已实现步展开 + 7 槽尾窗 + 进度 | **复用展开与进度**；勿改成 13 槽尾窗 |
| `ui_muyu_projection.c` / `ui_service.c` | 只投影 MUYU | 并列接入 `ui_jingwen_projection_*` |
| `device_nav_*` | 左右滑换页、下滑开设置 | 与 history 滚动共存（裁决 H）；尽量少改 nav，优先 UI 侧吞垂直事件 |
| `progress_service` / `watch_state` | `tap_round_cursor`/`tap_round_id` 已闭环 | 只读快照 |
| `ewf_scripture_canonical.h` | TOTAL_CHARS=303，CONSUMABLE=260 | 唯一经文源 |
| 字体 | 3.2 已有 serif700_52 / serif500_26 / serif400_26 | 优先复用；合同扩展 runtime 标签 |
| 3.1/3.2 deferred | 明确 13 槽交给 3.3 | 本 Story 闭合软件侧 |

### 目录与文件结构要求

```
Embedded/components/ui/
  bindings/
    ui_scripture_display_expand.*   # 可选：从 belt 抽出的共享展开
    ui_jingwen_stream_policy.*      # 纯逻辑 13 槽换行（推荐）
    ui_jingwen_projection.*         # LVGL 投影 + 锚尾（推荐）
    ui_muyu_belt_policy.*           # 保持 7 槽；可改为调用共享展开
    ui_muyu_constants.h             # 可抽共享色/进度分母，或 jingwen 旁路复用
  generated/screens/ui_scr_shell.*  # 补 JINGWEN 对象，无业务
Embedded/lvgl-design/squareline_studio/
  font_glyph_contract.json          # 扩展经文流动态标签
docs/contracts/canonical/generated/ewf_scripture_canonical.h  # 只读
Embedded/tests/
  test_ui_jingwen_stream_policy.c   # 新建主机测
```

### 不属于本 Story 的边界

- 统计页今日/累计/环形进度（3.4）
- 设置页四行控件与 sync-btn 五态（3.5）
- 完成遮罩「从头开始/退出」与跨轮 `action_id`（3.6）
- 小程序 17 槽阅读流（已由 6.3；设备不得抄 17）
- Air780 真机、端到端对拍（Epic 7）
- 充电暂停 gate、软件关机、KILL、BLE/Wi-Fi/GPS 业务
- 升级 LVGL 9、手改字体位图、第二份经文文件、第二篇历史流 UI

### 架构合规要点

- **AD-19：** 一敲一字；标点随附占槽；不预览未来；7/13/17 都是视口常数，不写入 canonical 工具。
- **AD-10：** 全部 `lv_*` / scroll API 仅 `ui_task`。
- **AD-9：** UI 只读 `watch_state_snapshot`；禁止读 progress 私有静态。
- **AD-12 / NFR2：** 渲染/滚动失败不得阻塞计数与落盘。
- **AD-4：** 只 `#include "ewf_scripture_canonical.h"`。

### Testing Requirements

- 流：空/1/13/14（换行）、标点占槽、前缀稳定、单 `glyph-current` 在流尾、cursor=260
- 锚点：review offset → append → 回到尾
- 手势 policy：history 内垂直 ≠ 开设置；非 JINGWEN 不投影经文交互
- 字体：`missing_glyphs: 0`；动态经文标签进合同
- 主机：`run_host_tests.py` 全绿；canonical 全绿
- 分级：host/编译 ≠ 真机滚动/锚尾观感；后者 `hardware_pending`

### Previous Story Intelligence

- **3.2（直接前置）**：`ui_muyu_belt_policy` 已钉死「显示字符槽 + 标点占槽 + 只读 cursor」；Serif 52/26 档与琥珀色常量已落地；code-review 修了进度百分比、三环 origin/timer、字带几何——本 Story **不要回退** MUYU 行为；经文页是**全文流**不是尾窗。
- **3.1**：`ui_task`/`ui_service`、三主页 pager、statusbar、字体合同基线；JINGWEN 仅占位 title。
- **2.4**：左右滑/下滑设置/熄屏首触；经文页触摸不计数；本 Story 必须与下滑设置手势共存（裁决 H）。
- **2.1/2.2**：统一队列与 `round_cursor` owner 在 progress；UI 只投影。
- **2.6/2.7**：`automatic_tap` 与 fault gate——流跟随游标；故障锁定时不伪造未来字。
- **4.1 / 6.3**：canonical 260/303；小程序 17 槽投影可作算法参考，但设备必须 13，且数据源是本地 cursor 不是 cloud 确认差量。
- **deferred-work**：3.1/3.2 已把 13 槽明示交给本 Story。

### Git Intelligence

- 基线 `0c9df2d`：Epic 2 服务层；UI Epic 3 工作区已有 3.1/3.2 木鱼页实现（可能仍未全部 commit）。
- 模式延续：纯 policy 主机测 + bindings 投影 + `hardware_pending`；共享展开函数优于复制粘贴。

### Latest Tech Information

- LVGL **8.4.0**（固件锁）：经文滚动用 `lv_obj_set_scroll_dir(..., LV_DIR_VER)`、`lv_obj_scroll_to_y` / `lv_obj_scroll_to_view`；读偏移 `lv_obj_get_scroll_y` / `lv_obj_get_scroll_bottom`。文档：https://docs.lvgl.io/8.4/overview/scroll.html
- 锚尾推荐：`lv_obj_update_layout` 后对焦点子对象 `lv_obj_scroll_to_view(child, LV_ANIM_OFF)`，或 `scroll_to_y(panel, get_scroll_y+get_scroll_bottom, …)`；连击时避免长动画堆积（NFR2）
- 不升 LVGL 9；SquareLine 1.6.1 导出后仍在 8.4.0 重编译
- ESP-IDF **v5.5.4** / ESP32-S3；新增 API 必须查该版本文档
- 颜色：焦点 `#d9a441`（amber.400）；已确认行可用 `#a6a29a`（与 HTML 一致）

### Project Structure Notes

- UI：`Embedded/components/ui`；设计真源：`Embedded/lvgl-design`
- 冲突：根路径 `lvgl-design/` → 映射 `Embedded/lvgl-design/`（3.1 裁决 A）
- 禁止新建第二套 event bus、在 `generated/` 写 progress/NVS、把 7/13/17 写入 canonical 正文工具
- 小程序 17 槽与设备 13 槽不得混用常量

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` §Epic 3 / Story 3.3、UX-DR9、FR-E-006、NFR2]
- [Source: `_bmad-output/planning-artifacts/ux-designs/.../UI_CONTRACT-device.md` §3–§5 JINGWEN / scripture-history]
- [Source: `_bmad-output/planning-artifacts/ux-designs/.../DESIGN.md` components.device.scripture-history]
- [Source: `_bmad-output/planning-artifacts/ux-designs/.../EXPERIENCE.md` UJ-5、经文页敲击与滚动]
- [Source: `_bmad-output/planning-artifacts/architecture/.../ARCHITECTURE-SPINE.md` AD-19/10/9/4/1/12]
- [Source: `docs/contracts/canonical/README.md` 步语义、13 槽视口、303/260]
- [Source: `docs/contracts/canonical/generated/ewf_scripture_canonical.h`]
- [Source: `Embedded/lvgl-design/ewf-device-ui-export.html` JINGWEN scripture-history 362×286]
- [Source: `_bmad-output/implementation-artifacts/3-2-实现木鱼页七字带与三环反馈.md`]
- [Source: `_bmad-output/implementation-artifacts/3-1-建立设备-ui-骨架与字体合同.md`]
- [Source: `_bmad-output/implementation-artifacts/2-4-实现触摸-pwr-导航与设备设置持久化.md`]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md`]
- [Source: `Embedded/components/ui/bindings/ui_muyu_belt_policy.c`]
- [Source: `Embedded/components/ui/generated/screens/ui_scr_shell.c`]
- [Source: LVGL 8.4 scroll — https://docs.lvgl.io/8.4/overview/scroll.html]

## Dev Agent Record

### Agent Model Used

Composer (Auto / Epic Autopilot)

### Implementation Plan

- 裁决 A–I 写入 `ui_jingwen_constants.h` / geometry 注释；只读 cursor/nav。
- 从 belt 抽出共享展开；jingwen 全文 13 槽切行；MUYU 保持 7 槽尾窗。
- LVGL 投影在 bindings；generated 仅布局空桩。
- history 视口垂直手势抑制 SWIPE_UP/DOWN 进设置。

### Debug Log References

- host: `python3 Embedded/tests/run_host_tests.py` → 0
- font UT/validate → missing_glyphs:0
- canonical → 24/24
- idf build attempt-01 → `/Users/hongchenke/Documents/Github/Electronic_Wooden_Fish/docs/embedded/build_records/20260924-122221/build-01.log` exit 0

### Completion Notes List

- 2026-09-24：create-story 完成——上下文引擎分析 epics/UX 契约/architecture/canonical 13 槽与 303 显示上限/3.2 展开与字体复用/JINGWEN 壳层占位/2.4 下滑设置与回看手势冲突裁决；确认全文 append-only（非 7 槽尾窗）、无木鱼无三环、新字强制锚尾；Status=ready-for-dev。Ultimate context engine analysis completed - comprehensive developer guide created。
- 2026-09-24：dev-story 完成——共享 `ui_scripture_display_expand`；`ui_jingwen_stream_policy` 13 槽全文流；`ui_jingwen_projection` 投影+锚尾；`ui_jingwen_gesture_policy` + tap_input 抑制 history 内垂直开设置；壳层 history/进度/round-index；字体合同扩「次第诵读」并 `missing_glyphs:0`；主机测/canonical/idf build 全绿；`hardware_pending` 真机手感记入 quality_debt。Status=review。
- 2026-09-24：code-review deep/autofix 完成——脏标记/行可见性/行高56/锚尾日志/tap_input 垂直抑制主机测已修；history 滚动通路与 ui_service 投影编排测 deferred；Status=done。

### File List

- Embedded/components/contract_checks/contract_checks.c
- Embedded/components/services/tap_input_service/tap_input_service.c
- Embedded/components/services/ui_service/ui_service.c
- Embedded/components/ui/CMakeLists.txt
- Embedded/components/ui/bindings/ui_jingwen_constants.h
- Embedded/components/ui/bindings/ui_jingwen_gesture_policy.c
- Embedded/components/ui/bindings/ui_jingwen_gesture_policy.h
- Embedded/components/ui/bindings/ui_jingwen_projection.c
- Embedded/components/ui/bindings/ui_jingwen_projection.h
- Embedded/components/ui/bindings/ui_jingwen_stream_policy.c
- Embedded/components/ui/bindings/ui_jingwen_stream_policy.h
- Embedded/components/ui/bindings/ui_muyu_belt_policy.c
- Embedded/components/ui/bindings/ui_scripture_display_expand.c
- Embedded/components/ui/bindings/ui_scripture_display_expand.h
- Embedded/components/ui/ewf_ui_geometry.h
- Embedded/components/ui/generated/fonts/ui_font_ns600_16.c
- Embedded/components/ui/generated/fonts/ui_font_ns700_22.c
- Embedded/components/ui/generated/fonts/ui_font_serif400_26.c
- Embedded/components/ui/generated/fonts/ui_font_serif500_26.c
- Embedded/components/ui/generated/fonts/ui_font_serif700_28.c
- Embedded/components/ui/generated/fonts/ui_font_serif700_52.c
- Embedded/components/ui/generated/screens/ui_scr_shell.c
- Embedded/components/ui/generated/screens/ui_scr_shell.h
- Embedded/lvgl-design/squareline_studio/assets/Fonts/NotoSerifSC-400.ttf
- Embedded/lvgl-design/squareline_studio/assets/Fonts/NotoSerifSC-500.ttf
- Embedded/lvgl-design/squareline_studio/assets/Fonts/NotoSerifSC-700.ttf
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_ns600_16.c
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_ns600_16.fcfg
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_ns700_22.c
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_ns700_22.fcfg
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif400_26.c
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif400_26.fcfg
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif500_26.c
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif500_26.fcfg
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif700_28.c
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif700_28.fcfg
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif700_52.c
- Embedded/lvgl-design/squareline_studio/assets/Fonts/ui_font_serif700_52.fcfg
- Embedded/lvgl-design/squareline_studio/font_glyph_contract.json
- Embedded/lvgl-design/squareline_studio/tools/generate_ewf_fonts.py
- Embedded/lvgl-design/squareline_studio/tools/validate_font_coverage.py
- Embedded/tests/run_host_tests.py
- Embedded/tests/test_ui_jingwen_stream_policy.c
- Embedded/tests/test_ui_shell_contract.py
- Embedded/tests/test_tap_input_runtime.c
- _bmad-output/implementation-artifacts/deferred-work.md

### Review Findings

- [x] [Review][Patch] 脏标记忽略 `tap_round_id` 导致 round-index 滞后 [`ui_jingwen_projection.c`] — autofix：脏检查同时比较 cursor 与 round_id
- [x] [Review][Patch] 游标回退时多余行盒只清文本不隐藏 [`ui_jingwen_projection.c`] — autofix：`sync_row_visibility` 隐藏超额行
- [x] [Review][Patch] 清空槽位未复位 confirmed 样式，amber 可能残留 [`ui_jingwen_projection.c`] — autofix：清槽/清行均 `style_confirmed`
- [x] [Review][Patch] destroy 后 `s_visible_rows` 未复位导致跳过重建 [`ui_jingwen_projection.c`] — autofix：句柄为空时重置计数
- [x] [Review][Patch] 行高 34 裁切 52px glyph-current [`ui_jingwen_constants.h`] — autofix：`EWF_UI_JINGWEN_ROW_HEIGHT=56`
- [x] [Review][Patch] 锚尾失败无中文日志 [`ui_jingwen_projection.c`] — autofix：history 为空时 `ESP_LOGW`
- [x] [Review][Patch] tap_input history 垂直抑制缺调用点主机测 [`test_tap_input_runtime.c`] — autofix：补 JINGWEN history 内/外垂直滑用例
- [x] [Review][Defer] history 内垂直回看无 LVGL 触控/滚动通路 [`tap_input_service.c` / projection] — deferred: 触控由 tap_input 独占且无 `lv_indev`；suppress 只吞导航不转发 `lv_obj_scroll_by`；需 ui_task 滚动消息或 indev 合流（超出无歧义 patch）
- [x] [Review][Defer] `ui_service`→`ui_jingwen_projection_apply` 缺零 LVGL 主机编排测 — deferred: 仓库无 LVGL host 投影装具；流布局已由 `test_ui_jingwen_stream_policy` 钉死

#### Rejected

- `false`：`reviewing` 形参空置 / 无 scroll 状态机 — 裁决 E 规定新字一律强制锚尾，回看只改 scroll offset；与 AC2 锚尾语义一致，非缺陷。
- `low`：创建失败后继续推进 `s_visible_rows` 的极端 OOM 路径 — 已顺手加 NULL 日志与 break/continue；更深重试策略不值额外复杂度。

## Change Log

- 2026-09-24：create-story 生成 ready-for-dev 上下文；sprint-status：`3-3-实现经文页-13-槽-append-only-流` → ready-for-dev。
- 2026-09-24：dev-story 实现经文页 13 槽 append-only 流；sprint-status → review。
- 2026-09-24：code-review（deep/autofix）修复脏标记/行可见性/行高/锚尾日志/tap_input 垂直抑制主机测；history 滚动通路与 ui_service 投影编排测 deferred；sprint-status → done。
