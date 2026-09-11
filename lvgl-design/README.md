# lvgl-design —— 设备 OLED（LVGL）UI 设计工作区

本目录承载电子木鱼**设备端 UI** 的「pen → HTML → SquareLine → LVGL C」流水线。流程、工程与驱动整体照搬自
`/Users/hongchenke/Documents/Github/legbot_watch`（只读蓝本）；**视觉按木鱼新定**（410×502 CO5300 AMOLED、三主页/7字带/完成遮罩等，见 UX 契约）。

## 流水线

```text
① UX 契约      _bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-*/DESIGN.md + EXPERIENCE.md
② 轨道契约      同目录 UI_CONTRACT-device.md（页面/状态/组件闭包、命名、SquareLine 导出边界）
③ 设计稿        ewf-device-direction.pen/.html（Stage 3：10 种候选）→ ewf-device-ui.pen/.html（选中后 Stage 4 全帧闭包）
④ SquareLine   squareline_studio/ewf_project_manifest.json →（Stage 5 按规划生成 `.spj`；SquareLine Studio 1.6.1 手工校调后 Export LVGL C）
⑤ C 产物        → Embedded/components/ui/generated/{ui.c,ui.h,screens/,components/,fonts/,filelist.txt}
                业务渲染/路由在 components/ui/bindings/；LVGL 仅 ui_task 独占调用（两层分离红线）
```

## 工具（照搬自 legbot，`squareline_studio/tools/`）

- `generate_squareline_project.py` —— HTML → `.spj/.sll/.slp`（同步字体）
- `postprocess_squareline_export.py` —— 规范化 SquareLine 导出、拆分工厂
- `validate_squareline_project.py` —— 工程校验（页面/Screen/字体闭包 vs HTML）
- `validate_font_coverage.py` —— 编译字体字形覆盖校验（禁止手改 .c 位图）

> 这些工具目前是 legbot 定向版本（含 watch 专属屏与字体子集）；Stage 4 已生成 EWF HTML 与设计期 `font_glyph_contract.json`，Stage 5 仍须按 EWF 页集和 Noto Serif SC 字形角色重定向后才能作为终验。

基于蓝本的 EWF Screen/Component/State 映射与容量审计见 [`squareline_studio/README.md`](/Users/hongchenke/Documents/Github/Electronic_Wooden_Fish/lvgl-design/squareline_studio/README.md) 和 [`ewf_project_manifest.json`](/Users/hongchenke/Documents/Github/Electronic_Wooden_Fish/lvgl-design/squareline_studio/ewf_project_manifest.json)。

## EWF Stage 3 风格候选

- Pencil 源：`ewf-device-direction.pen`（组件 master + `[UI][STYLES][DEVICE]` + 10 个 sibling frame）
- 离线 HTML：`ewf-device-direction.html`
- 生成命令：`python3 lvgl-design/export_ewf_style_board_html.py --input lvgl-design/ewf-device-style-board-source.html --output lvgl-design/ewf-device-direction.html --surface device --manifest lvgl-design/device-style-options.json`
- 候选校验：`python3 lvgl-design/validate_ewf_ui_closure.py --surface device --stage candidates --html lvgl-design/ewf-device-direction.html --manifest lvgl-design/device-style-options.json`
- 文案卫生：`python3 lvgl-design/ui_text_hygiene.py --surface device --html lvgl-design/ewf-device-direction.html --mode candidates`

每个候选固定 410×502，必须包含 `cmp_statusbar_gGgAm` instance、`scripture-progress`、`today-taps`、`total-taps` 和 `woodfish-anatomy`。木鱼主视觉由 reusable `cmp_woodfish_mark` 承载，使用 `assets/woodfish-reference.png` 的黑底白色斜槽轮廓；作者已选定 `DEVICE-01`，其余候选仅作审阅档案。

候选校验会忽略颜色后比较布局/排版/组件结构指纹，用于发现近重复候选；它只是自动预警，最终风格签收仍以逐屏渲染审阅为准。

候选中的 12/260、今日 128、累计 3,456 与 76% 电量仅为视觉样例；运行时分母必须来自 S0.3 `scripture_chars_total`，不写入 canonical 经文源。

可复现测试：`PYTHONPATH=lvgl-design python3 -m unittest discover -s lvgl-design/tests -p 'test_*.py'`

## EWF Stage 4 全帧（DEVICE-01）

- Pencil：`ewf-device-ui.pen`；`hbTEa` 是木鱼页母版。三主页位于 `screen_shell` 常驻横向 pager，设置页 `W3247` 是按需 Screen。`mDOlU` 字带与 `JmQTi` 进度是可复用母版，空带/填充/满带进入 `cmp_muyu_flow_states`（`ZauNI`），今日敲击 trusted/untrusted 进入 `cmp_today_taps_variants`（`Hlo77`），同步五态进入 `cmp_sync_btn_variants`（`fWCbZ`），状态栏状态进入 `cmp_statusbar_states`（`qLhoo`），三环 IDLE/FLASH 对照进入 `cmp_tap_rings_states`（`JZhSx`）；`cmp_statusbar_gGgAm` 只保留四个信号槽并用运行时补丁切换状态。充电暂停与完成遮罩因改变输入边界，保留为整屏状态，但其字带/进度仍引用共享母版。
- HTML 源导出：`ewf-device-ui-export.html`；交付 HTML：`ewf-device-ui.html`。组装命令：`python3 lvgl-design/assemble_ewf_full_html.py --input lvgl-design/ewf-device-ui-export.html --output lvgl-design/ewf-device-ui.html --surface device --style DEVICE-01`。
- 三环 `flash` 是**运行时描边色补丁**，不是节点变体：`n2eoh` 的 metadata 登记 `flashStroke:#e6bd69` / `durationMs:160`，母版只导出 idle 描边；木鱼页实例在 tap 事件把描边切到金色、160ms 后回 idle，连续输入只重启不叠加。IDLE/FLASH 对照见母版区 `cmp_tap_rings_states`（`JZhSx`）。
- 组装出的 HTML 用 `outline-color` 覆盖（与环节点的 `outline` 简写匹配）；idle 静态帧可 `:hover` / `:active` 预览金光，固件侧由 `data-ewf-motion="flash"` 表达。
- 木鱼页只有流尾最新字使用大字高亮；EMPTY 无可见大字，BASE/MID/FULL/DONE 的当前字分别位于最后已占用槽，右侧不得出现未来字。三环正好三层、160ms、两种有效输入共用。
- 分段进度使用金色已完成段：EMPTY/BASE/MID/FULL 分别为 0/1/3/6 段，完成态为 8 段；金色仍只属于敲击反馈、最新字与进度。
- 经文页 `scripture-history` 为单一 append-only 流；每行固定 13 个字符槽，标点占槽，`PawlG` 的最新字紧跟前文；垂直回看时新字到达自动回流尾。`lo9C1` 与 `fevaG` 是同一组件的 tail/review 变体，几何和字阶一致。
- 设置页首屏最多 4 行，第二页为木鱼版本/木鱼 ID；亮度与熄屏沿用 `mH17G`/`achTu`/`Pz5g1` 的分段胶囊，音量和同步为 EWF 自有组件。HTML 为闭包校验物化为 5 个设置状态帧，并标记 `data-ewf-screen-variant="false"`。
- 校验：`python3 lvgl-design/validate_ewf_ui_closure.py --surface device --stage full --style DEVICE-01 --html lvgl-design/ewf-device-ui.html`。

## 蓝本与照搬映射（只读源）

| 本仓 | legbot 蓝本 |
| --- | --- |
| `lvgl-design/ewf-device-ui.pen/.html` | `lvgl-design/watch-lvgl.pen` / `watch-lvgl.html`（HTML 形态范式） |
| `squareline_studio/*.spj` | `squareline_studio/watch-lvgl.spj` |
| `squareline_studio/tools/*.py` | 同名 `squareline_studio/tools/*.py` |
| `Embedded/components/ui/{generated,compat,bindings}`（E1/E3 实现期照搬） | `components/ui/...` |
| `Embedded/components/services/ui_service` | `components/services/ui_service/ui_service.c` |
| `Embedded/components/BSP/{CO5300,CST9217}` | 同名 BSP 封装 |
| `Embedded/main/idf_component.yml`（lvgl 8.4.0 等） | legbot 同文件版本锁定 |
