<!-- 来源: legbot_watch · 源文件: docs/troubleshooting/LVGL运行时字体缺字问题根因与解决方案.md · legbot HEAD: 41a5ab8b9 · 迁移: 2026-09-08 · 判定: 搬(词表适配) · EWF 适配点: 工程名/文件名路径替换 -->

# LVGL 运行时字体缺字问题根因与解决方案

> 当前状态（2026-09-13）：设备 UX 真源为 `lvgl-design/ewf-device-ui.pen`。本仓已清理设计期 HTML、SquareLine 生成工程、字体合同和旧校验脚本；本文中的旧工具路径仅保留迁移背景，不可直接执行。固件实施阶段重新建立字体合同与校验工具。

## 1. 文档目的

本文记录定位并修复的 LVGL 中文缺字问题，目标是解释：

- 为什么触摸确认页会显示“滑块□□50%□□”；
- 为什么旧字体覆盖校验错误地返回通过；
- 为什么同类问题会在其他页面反复出现；
- 当前修复如何同时约束字体生成器、校验器和固件产物；
- 后续新增或修改运行时文案时必须执行哪些步骤。

本文适用于以下链路（词表已适配 EWF；HTML/SquareLine 工程位于 `lvgl-design/`，固件位于 `Embedded/`）：

```text
ewf-device-ui.html
    ↓
SquareLine ewf-device.spj / project_manifest.json
    ↓
状态投影 Embedded/components/ui/bindings/ui_manifest_projection.c
    ↓
运行时 Embedded/components/ui/bindings/ui_runtime_binding.c / Embedded/components/services/ui_service/ui_service.c
    ↓
字体合同 lvgl-design/squareline_studio/font_glyph_contract.json
    ↓
SquareLine 字体 .fcfg/.bin/.c
    ↓
固件 Embedded/components/ui/generated/fonts/*.c
```

## 2. 故障现象

触摸人工确认页面应显示：

```text
点击目标，滑块超过 50% 即可
```

设备实际显示时，“超”“过”“即”“可”被 LVGL 的缺字方框替代，表现近似：

```text
滑块□□ 50% □□
```

对应 Unicode 码点如下：

| 字符 | Unicode |
|---|---|
| 超 | `U+8D85` |
| 过 | `U+8FC7` |
| 即 | `U+5373` |
| 可 | `U+53EF` |

直接检查固件字体 `Embedded/components/ui/generated/fonts/ui_font_ns700_20.c`，修复前找不到以上四个码点；因此这是确定的字体字形缺失，不是编码损坏、坐标遮挡或 LVGL 标签裁剪。

## 3. 数据流与关键事实

### 3.1 运行时文案来源

触摸项问题文案由 `Embedded/components/services/ui_service/ui_service.c` 提供：

```c
return "点击目标，滑块超过 50% 即可";
```

运行时通过 `Embedded/components/ui/bindings/ui_runtime_binding.c` 写入逻辑标签：

```text
ui_selftest_manual_txt_manual_question
```

### 3.2 同一逻辑标签在不同状态使用不同字体

该标签不是始终使用一种字体：

| 状态投影 | 字体 |
|---|---|
| 触摸人工确认 | `ns700_20` |
| 其他人工确认变体/基准对象 | `ns700_23` |

`project_manifest.json` 的 `state_registry[].state_patch` 明确记录了各状态的 `lv_obj_set_style_text_font` 操作。

### 3.3 修复前的错误假设

修复前，字体生成器和字体校验器都只从最终 SquareLine 对象树建立：

```text
逻辑标签 → 字体
```

此时该标签只被识别为：

```text
ui_selftest_manual_txt_manual_question → ns700_23
```

虽然生成器随后遍历了各状态投影，但只把投影中的静态文本加入对应字体，并没有把投影字体补充到该逻辑标签的字体集合。

最终，`font_glyph_contract.json` 中登记的运行时文案只被加入 `ns700_23`，没有加入触摸页面真实使用的 `ns700_20`。

## 4. 根本原因

本问题由两个相互强化的缺陷组成。

### 4.1 字体生成器遗漏状态投影字体

文件：

```text
lvgl-design/squareline_studio/tools/generate_squareline_project.py
```

旧逻辑只根据 SquareLine 基准对象树解析运行时标签的字体。它没有把状态投影中的：

```text
target.logical_name
lv_obj_set_style_text_font
```

合并到 `fonts_by_label`。

因此，同一逻辑标签只要在某个变体中切换字号或字重，运行时文案就可能只进入基准字体，而没有进入设备当前真正使用的字体。

### 4.2 校验器复制了同一个错误假设

文件：

```text
lvgl-design/squareline_studio/tools/validate_font_coverage.py
```

旧校验器同样只按 SquareLine 基准对象树解析运行时标签字体，并强制认为：

```text
一个运行时标签只能唯一绑定一种字体
```

结果是：

1. 生成器把文案错误地加入 `ns700_23`；
2. 校验器也只检查 `ns700_23`；
3. `ns700_20` 实际缺字；
4. 校验器仍返回 `missing_glyphs: 0`。

这属于生成链和验证链共享同一错误模型造成的假阴性。

## 5. 为什么问题会反复出现

本项目为了减少常驻 LVGL 对象数量，会让多个页面变体复用同一个逻辑对象，再由状态投影修改位置、尺寸、文本和字体。

只要同时满足以下条件，就可能再次出现方框：

1. 多个状态复用同一个逻辑标签；
2. 不同状态为该标签配置了不同字体；
3. 运行时会再次调用 `lv_label_set_text()`；
4. 新文案字符没有出现在该状态的静态设计文本中；
5. 字体生成器只依据基准字体补充运行时字形。

因此，单独手工给某个字体补四个汉字只能解决当前症状，不能阻止下一条文案再次缺字。

## 6. 完整解决方案

### 6.1 生成器按逻辑标签汇总全部状态字体

`generate_squareline_project.py` 现在会在遍历每个状态投影时，将：

```text
patched_object.target.logical_name
patched_object.operations[].lv_obj_set_style_text_font
```

加入 `fonts_by_label`。

运行时文案如果没有显式指定唯一 `font_code`，会被加入该逻辑标签在所有状态中可能使用的字体，而不是只加入基准字体。

以本次问题为例：

```text
ui_selftest_manual_txt_manual_question
    → ns700_20
    → ns700_23
```

合同中的全部运行时问题文案现在同时进入这两种字体。

### 6.2 校验器独立读取状态投影字体集合

`validate_font_coverage.py` 新增对以下数据的解析：

```text
project_manifest.json
  └─ state_registry[]
       └─ state_patch.overrides
            ├─ target.logical_name
            └─ lv_obj_set_style_text_font
```

校验器会对每个运行时标签执行：

```text
运行时文案字符集合
    ⊆
该标签每一种实际投影字体的字形集合
```

只要任何一个状态字体缺字，校验立即失败。

### 6.3 重新生成字体资产

本次重新生成了：

```text
ui_font_ns700_20.fcfg
ui_font_ns700_20.bin
ui_font_ns700_20.c
ui_font_ns700_38.fcfg
ui_font_ns700_38.bin
ui_font_ns700_38.c
```

并同步更新：

```text
Embedded/components/ui/generated/fonts/ui_font_ns700_20.c
Embedded/components/ui/generated/fonts/ui_font_ns700_38.c
```

`ns700_38` 也被修复，是因为新的校验模型同时发现它存在相同的跨状态运行时字形缺口。

禁止直接手工编辑字体 C 数组。字体必须由 `.fcfg`、TTF 和生成脚本统一重建，否则 SquareLine 字体源、bin 和固件 C 源会再次失配。

### 6.4 增加回归测试

新增：

```text
lvgl-design/squareline_studio/tools/test_font_runtime_projection_coverage.py
```

测试固定以下合同：

1. `ui_selftest_manual_txt_manual_question` 必须识别到 `ns700_20` 和 `ns700_23`；
2. `ns700_20` 必须包含“点击目标，滑块超过 50% 即可”的全部字符；
3. 由 C 代码切换字号的模式标签必须显式登记 `ns700_22` 和 `ns700_24`；
4. 由 C 代码同时切换图标与字号的电量标签必须显式登记 `lucide_22` 和 `lucide_28`。

该测试在修复前稳定失败，字体重新生成后通过。

### 6.5 覆盖固件实际导出 C 和代码驱动的字体切换

全量复查发现，只检查 SquareLine 工程和状态投影仍然不够：

- SquareLine 原始工程中有 321 个标签；
- 固件实际编译的 `Embedded/components/ui/generated/screens` 与 `Embedded/components/ui/generated/components` 中有 264 个基础标签；
- `Embedded/components/ui/bindings/ui_manifest_projection.c` 中有 433 个状态投影标签；
- 运行时字形合同中有 62 个动态标签；
- 运行时受控函数中有 89 个非 ASCII 字符串字面量。

这些集合并不要求标签数量相等，因为状态变体会复用基础导出对象；但每一层的“文本 × 实际字体”都必须独立纳入字形闭合。

另外，`Embedded/components/ui/bindings/ui_runtime_binding.c` 存在不经过 `project_manifest.json` 的代码驱动字体切换：

| UI 元素 | 运行时字体集合 |
|---|---|
| 模式按钮文字 | `ns700_22`、`ns700_24` |
| 模式按钮图标 | `lucide_40`、`lucide_44` |
| 外骨骼关机文字 | `ns700_24`、`ns700_30` |
| 外骨骼关机图标 | `lucide_32`、`lucide_40` |
| 手环电量图标 | `lucide_22`、`lucide_28` |

这类路径必须在 `font_glyph_contract.json` 使用 `font_codes` 显式列出全部可能字体，不能依赖静态工程推断。

全量校验因此额外发现：

- `lucide_22` 缺少低电图标 `U+E056`；
- `lucide_28` 缺少正常电量图标 `U+E057`。

虽然当前代码按电量状态成对切换文字和字体，不一定立即显示方框，但它依赖更新顺序和样式状态，属于真实的潜在缺字。本次已将两枚字形同时加入两种电量图标字体，避免状态切换、对象复用或后续调整再次暴露问题。

## 7. 验证结果

> 注：本节数值均取自源工程（legbot_watch，2026-07-28）的参考构建快照并随文档迁移；EWF 固件尚未落地，复跑后应得到本仓自己的数值。判定只看形态与门槛：单测 OK、`missing_glyphs: 0`、`squareline_firmware_font_sources_identical: true`、Phase 3 `ok: true`、ESP-IDF 构建通过。

### 7.1 专项回归

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  lvgl-design/squareline_studio/tools/test_font_runtime_projection_coverage.py
```

结果：

```text
Ran 3 tests
OK
```

### 7.2 字体覆盖

```bash
PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_font_coverage.py
```

关键结果（数值为源参考值）：

```json
{
  "static_labels": 321,
  "generated_labels": 264,
  "projection_labels": 433,
  "runtime_labels": 62,
  "runtime_non_ascii_literals": 89,
  "fonts": 76,
  "required_font_glyph_pairs": 6232,
  "missing_glyphs": 0,
  "squareline_firmware_font_sources_identical": true
}
```

### 7.3 Phase 3 全链路

```bash
PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_phase3_integration.py
```

结果（数值为源参考值）：

```json
{
  "ok": true,
  "states": 30,
  "fonts": 76,
  "events": 64,
  "resolvers": 35
}
```

### 7.4 ESP-IDF v5.5.4 构建

在 EWF 固件工程根目录 `Embedded/` 下执行：

```bash
source /Users/hongchenke/.espressif-v5.5.4/v5.5.4/esp-idf/export.sh
idf.py build
```

结果（源参考构建输出；EWF 复跑后 `.bin` 名称与体积以本仓 Embedded 实际构建为准）：

```text
Project build complete.
<固件工程>.bin size: 0x324240    # 源参考值
app partition free: 0x5bdc0 bytes
```

为排除工作区中其他未完成代码对验证结果的影响，该结果使用干净 `HEAD` 快照叠加本次四套固件字体源后构建。应用分区仍剩约 367.4 KiB，约占最小应用分区的 10%（同为源参考值）。

## 8. 后续新增运行时文案的强制流程

修改任何由 `lv_label_set_text()`、`snprintf()` 或服务层模型动态提供的非 ASCII 文案时，必须执行以下流程。

### 步骤一：登记运行时文案

更新：

```text
lvgl-design/squareline_studio/font_glyph_contract.json
```

按逻辑标签登记所有可能文案。不要只登记当前页面截图中的字符串。

如果 C 代码会对同一标签调用 `lv_obj_set_style_text_font()` 切换字体，必须使用 `font_codes` 列出全部可能字体。禁止只登记设计稿中的默认字体。

### 步骤二：重新生成字体

```bash
python3 lvgl-design/squareline_studio/tools/generate_squareline_project.py \
  --html lvgl-design/ewf-device-ui.html \
  --project-dir lvgl-design/squareline_studio/ewf-device \
  --fonts-only \
  --sync-generated-fonts Embedded/components/ui/generated/fonts
```

路径以本仓 `lvgl-design/` 为准：`--html` 指向 `ewf-device-ui.html`，`--project-dir` 指向 `ewf-device.spj` 所在目录，`--sync-generated-fonts` 指向 `Embedded/components/ui/generated/fonts`，实际参数按本仓布局调整。

### 步骤三：运行专项与字体校验

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  lvgl-design/squareline_studio/tools/test_font_runtime_projection_coverage.py

PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_font_coverage.py
```

### 步骤四：运行集成校验

```bash
PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_phase3_integration.py
```

### 步骤五：编译固件

在 EWF 固件工程根目录 `Embedded/` 下执行：

```bash
source /Users/hongchenke/.espressif-v5.5.4/v5.5.4/esp-idf/export.sh
idf.py build
```

以上任一步失败都不得发布或刷写固件。

## 9. 快速排查方法

设备再次出现方框时，按以下顺序检查。

1. 确认方框对应的原始运行时文案；
2. 确认页面当前状态投影实际设置的字体，而不是只看 SquareLine 基准对象；
3. 检查 `Embedded/components/ui/bindings/ui_runtime_binding.c` 是否又对该标签切换了字体；
4. 在目标字体 C 文件（`Embedded/components/ui/generated/fonts/`）中搜索字符对应的 `U+XXXX` 注释；
5. 检查 `lvgl-design/squareline_studio/font_glyph_contract.json` 是否按逻辑标签登记文案及全部 `font_codes`；
6. 运行 `lvgl-design/squareline_studio/tools/validate_font_coverage.py`；
7. 重新生成字体，禁止手工复制其他字号字体或修改位图数组；
8. 重新执行 Phase 3 校验和 ESP-IDF 构建。

判断标准：

- 字体 C 文件存在目标 Unicode 码点；
- `.fcfg` 的 `symbols` 包含目标非 ASCII 字符；
- SquareLine 字体 C 与固件字体 C 字节一致；
- 字体覆盖结果为 `missing_glyphs: 0`；
- 固件完整构建通过。
