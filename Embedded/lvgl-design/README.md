# lvgl-design —— 设备侧 UX 真源

本目录承载冻结的 Pencil 设计源与 Story 3.1 重建后的 SquareLine/字体工具：

- `ewf-device-ui.pen`：DEVICE-01 设备侧 UX 唯一视觉真源。
- `ewf-device-ui-export.html`：作者导出的同名 HTML。
- `squareline_studio/`：EWF 工程 `ewf-device`（SquareLine 1.6.1 → LVGL 8.3.11 导出形态；固件锁 LVGL 8.4.0）。
- `assets/woodfish-reference.png`：木鱼参考图（`cmp_woodfish_mark`）。

设备画布固定为 410×502，圆角 110，边距 16，状态栏 24；三主页由 `screen_shell` 常驻 pager 承载（内容宽 1230），设置页 `SHEZHI` 按需加载。

## 字体合同（禁止手改 `.c` 位图）

```bash
# 若本机有 Noto Serif SC 可变字体，可放到 /tmp/NotoSerifSC-VF.ttf
PYTHONDONTWRITEBYTECODE=1 python3 \
  Embedded/lvgl-design/squareline_studio/tools/generate_ewf_fonts.py \
  --fonts-only \
  --sync-generated-fonts Embedded/components/ui/generated/fonts

PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  Embedded/lvgl-design/squareline_studio/tools/test_font_runtime_projection_coverage.py

PYTHONDONTWRITEBYTECODE=1 python3 \
  Embedded/lvgl-design/squareline_studio/tools/validate_font_coverage.py

PYTHONDONTWRITEBYTECODE=1 python3 \
  Embedded/lvgl-design/squareline_studio/tools/validate_squareline_project.py \
  --html Embedded/lvgl-design/ewf-device-ui-export.html \
  --project-dir Embedded/lvgl-design/squareline_studio
```

验收：`missing_glyphs: 0` 且 `squareline_firmware_font_sources_identical: true`。

## 下游实现

`Embedded/components/ui/generated/` 仅布局/字体/事件空桩；业务投影在 `bindings/`；`ui_task`（`ui_service`）独占 `lv_*`。
