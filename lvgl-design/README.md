# lvgl-design —— 设备 OLED（LVGL）UI 设计工作区

本目录承载电子木鱼**设备端 UI** 的「pen → HTML → SquareLine → LVGL C」流水线。流程、工程与驱动整体照搬自
`/Users/hongchenke/Documents/Github/legbot_watch`（只读蓝本）；**视觉按木鱼新定**（410×502 CO5300 AMOLED、三主页/7字带/完成遮罩等，见 UX 契约）。

## 流水线

```text
① UX 契约      _bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-*/DESIGN.md + EXPERIENCE.md
② 轨道契约      同目录 UI_CONTRACT-device.md（页面/状态/组件闭包、命名、SquareLine 导出边界）
③ 设计稿        ewf-device-direction.pen/.html（Stage 3：10 种候选）→ ewf-device-ui.pen/.html（选中后 Stage 4 全帧闭包）
④ SquareLine   squareline_studio/ewf-device.spj（由 generate_squareline_project.py 生成；SquareLine Studio 1.6.1 手工校调后 Export LVGL C）
⑤ C 产物        → Embedded/components/ui/generated/{ui.c,ui.h,screens/,components/,fonts/,filelist.txt}
                业务渲染/路由在 components/ui/bindings/；LVGL 仅 ui_task 独占调用（两层分离红线）
```

## 工具（照搬自 legbot，`squareline_studio/tools/`）

- `generate_squareline_project.py` —— HTML → `.spj/.sll/.slp`（同步字体）
- `postprocess_squareline_export.py` —— 规范化 SquareLine 导出、拆分工厂
- `validate_squareline_project.py` —— 工程校验（页面/Screen/字体闭包 vs HTML）
- `validate_font_coverage.py` —— 编译字体字形覆盖校验（禁止手改 .c 位图）

> 这些工具目前是 legbot 定向版本（含 watch 专属屏与字体子集）；在 Stage 4/5 生成本仓 HTML 与 `font_glyph_contract.json` 后需按 ewf 页集重定向与精简（记录于 `.memlog`）。

## EWF Stage 3 风格候选

- Pencil 源：`ewf-device-direction.pen`（组件 master + `[UI][STYLES][DEVICE]` + 10 个 sibling frame）
- 离线 HTML：`ewf-device-direction.html`
- 生成命令：`python3 lvgl-design/export_ewf_style_board_html.py --input lvgl-design/ewf-device-style-board-source.html --output lvgl-design/ewf-device-direction.html --surface device --manifest lvgl-design/device-style-options.json`
- 候选校验：`python3 lvgl-design/validate_ewf_ui_closure.py --surface device --stage candidates --html lvgl-design/ewf-device-direction.html --manifest lvgl-design/device-style-options.json`
- 文案卫生：`python3 lvgl-design/ui_text_hygiene.py --surface device --html lvgl-design/ewf-device-direction.html --mode candidates`

每个候选固定 410×502，必须包含 `cmp_statusbar_gGgAm` instance、`scripture-progress`、`today-taps`、`total-taps` 和 `woodfish-anatomy`。木鱼主视觉由 reusable `cmp_woodfish_mark` 承载，使用 `assets/woodfish-reference.png` 的黑底白色斜槽轮廓；候选可改变其构图、承托物、光影和进度表达，但不得把轮廓改成叶片或抽象 blob。`ewf-device-ui.*` 必须等作者选定 style 后再创建。HTML 不依赖 CDN、远程字体或脚本，生产输出清除 `data-pencil-id`。

候选校验会忽略颜色后比较布局/排版/组件结构指纹，用于发现近重复候选；它只是自动预警，最终风格签收仍以逐屏渲染审阅为准。

候选中的 12/260、今日 128、累计 3,456 与 76% 电量仅为视觉样例；运行时分母必须来自 S0.3 `scripture_chars_total`，不写入 canonical 经文源。

可复现测试：`PYTHONPATH=lvgl-design python3 -m unittest discover -s lvgl-design/tests -p 'test_*.py'`

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
