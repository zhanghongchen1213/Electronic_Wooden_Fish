# canonical《心经》

本目录承载 FR-C-002 与 AD-4 要求的**唯一落盘来源**：设备、backend、小程序三端一律
从 `heart-sutra.txt` 生成可消费汉字序列，并使用同一个 `scripture_version`。任一端版本
不一致时构建或校验必须失败，**不自动映射、不选经、不导入**。

## 唯一来源

| 文件 | 作用 |
| --- | --- |
| `heart-sutra.txt` | 唯一落盘来源，只含经文正文：UTF-8 无 BOM、LF、末尾单个换行 |
| `heart-sutra.manifest.json` | 派生元数据、校验锚点与上一版存档（`previous`），由 `emit` 生成，手改即校验失败 |
| `scripture_tool.py` | 生成（`emit`）与校验（`check`）工具，纯 Python 3 标准库 |
| `generated/ewf_scripture_canonical.h` | 面向设备构建的生成头（header-only：正文字节与每步偏移都在头内定义），Epic 7 / Story 3.1 接入时引用 |
| `tests/run_canonical_tests.py` | 门禁测试入口，含必须失败的负例 |

`heart-sutra.txt` 是**唯一**正文落盘位置。三端不得自留誊写副本；任何一端的经文资源
都必须可由 `emit` 从本文件确定性重建。

经题 `般若波罗蜜多心经` 只存在 manifest 的 `title` 字段，不写入 `heart-sutra.txt`，
也不进入消费序列。

### 正文来源与呈现口径

正文汉字逐字取自可核验公开来源：大正新脩大藏經 第8卷 No.251《般若波羅蜜多心經》
（唐·三藏法師玄奘譯），转录底本为《大正新修大藏經 第8卷·般若部四》第 864 页。
版本、译本、出处与获取日期记录在 manifest 的 `provenance`。

大正藏原文未附标点，标点属后世编辑决定。本 canonical 的标点口径依**冻结 UX 真源**
`Embedded/lvgl-design/ewf-device-ui-export.html`：该导出中经文只使用 `，`（7 处），
`。`、引号、叹号、顿号、分号、冒号均不出现，且经文页前两行恰好各 13 字符、第三行
以 `苦厄舍利` 起，与「每行 13 槽、标点占槽、标点随紧邻前一个汉字」一致。
因此 `non_consumable_chars` 为单元素集合 `["，"]`，字符分类封闭性最强。

正文汉字共 **260** 个，与冻结 UX 的设备进度分母 `260`（例如「心经进度 196 / 260 字」）
一致。设备木鱼页 7 槽、设备经文页每行 13 槽、小程序每行 17 槽都是**视口槽位**，
**不是**经文长度上限；工具不读取任何槽位常量。

## 消费语义

- **可消费字符**：BMP 的 CJK 统一表意文字 `U+4E00–U+9FFF` 与扩展 A `U+3400–U+4DBF`。
  一次有效敲击只推进一个可消费汉字。
- **非消费字符**：标点、空格、换行等一切非上述码点。它们不消耗敲击、不单独推进游标，
  随**紧邻的前一个汉字**在同一步内出现。源文本中出现的每个非消费字符都必须在
  manifest 的 `non_consumable_chars` 中显式列出；出现未声明的字符即校验失败。
- **步（step）**：`step[i] = 第 i 个可消费汉字 + 紧随其后的连续非消费字符`。这条定义
  同时决定了 7 槽 / 13 槽 / 17 槽中「标点计槽」的显示行为。
- **换行**：`heart-sutra.txt` 的换行只是文件级行结束，不是经文内容；正文内不得出现换行。
- **消费序列摘要**：`consumable_digest` = 可消费汉字按序拼接为 UTF-8 后取 sha256，
  是三端「同一版序列」的最低成本证据。
- **前锚点**：源文本前 26 字符固定为
  `观自在菩萨，行深般若波罗蜜多时，照见五蕴皆空，度一切`，第 27 字符起必须接 `苦厄`。

## 命令

```bash
# 生成：重写 manifest 与三端经文资源（确定性、幂等）
python3 docs/contracts/canonical/scripture_tool.py emit

# 校验：只读；任一端不一致即退出码非 0，且不修改任何文件
python3 docs/contracts/canonical/scripture_tool.py check

# 门禁测试：正向检查 + 必须失败的负例（负例在临时副本上执行）
python3 docs/contracts/canonical/tests/run_canonical_tests.py
```

`check` 覆盖：源文本 sha256 与 manifest 一致；三端产物的 `scripture_version`、消费序列
摘要与派生计数与 manifest 一致；生成头的正文字节载荷与每步字节偏移逐值一致；字符分类
封闭；前锚点一致；当前版本的变更记录行存在且其摘要与计数与 manifest 一致；变更记录中
同一份正文只对应一个版本号、版本号不重复且严格递增、初始版本行必须保留；
**紧邻当前版本之前的那一行**与 manifest 的 `previous` 存档逐值一致；步归属与源文本一致。

`emit` 另外承担「正文 ↔ 版本号」的 fail-closed 绑定。判定只用两处事实：本文件「版本
变更记录」表（追加式台账）与本次从源文本派生的值——磁盘 manifest 的派生字段是 `emit`
的产物、可被手工改写，不作为自身校验的依据：

- 当前版本号已登记在表中：本次是重跑，正文必须仍是该版本登记的那一份。改正文而不递增
  版本号在此被拒（同一版本号不得指向两份正文）。
- 当前版本号尚未登记：本次是新发布的首次 `emit`，正文必须相对表中最新一行确有变化。
  不改正文而只递增版本号在此被拒。

因此手改 manifest 的 `source_sha256` 等派生字段无法绕过该绑定。

## 强制前置义务

**三端构建或打包前必须执行同一条 `check`。** 该检查是 fail-closed 的：不得在检查失败时
继续打包，也不得用默认文本或旧版本产物兜底。

| 端 | 接入点 | 落地责任 |
| --- | --- | --- |
| cloud 后端 | `cloud/env-scripts/build-prod-backend.sh`，位于 `mvn ... package` 之前 | 本目录已接入 |
| 设备（Embedded） | 设备固件构建流程，引用 `generated/ewf_scripture_canonical.h` | Story 3.1 |
| 小程序（frontend） | 小程序打包流程，引用 `cloud/frontend/src/canonical/heart-sutra.generated.ts` | Story 6.1 |

cloud 后端在 `cloud/backend/pom.xml` 就位（Story 4.3）前，`build-prod-backend.sh` 会在更早处
退出；该门禁位于 `mvn` 之前，pom 补齐后即生效。

生成头是 header-only 的自洽文件（正文字节与每步偏移都以 `static const` 头内定义）：
设备侧接入时只需把 `docs/contracts/canonical/generated/` 加入 include 路径后
`#include "ewf_scripture_canonical.h"`，不需要另建承载定义的 `.c`。

## 版本变更流程

1. 修改 `heart-sutra.txt`。
2. 递增 manifest 的 `scripture_version`（格式 `HS-<major>.<minor>.<patch>`）。任何影响
   消费序列或展示文本的改动都必须递增；纯说明性元数据措辞可不递增。
3. 运行 `emit` 重建 manifest 与三端产物（`emit` 保留已递增的版本号，并把上一版事实写入
   新 manifest 的 `previous` 存档）。`emit` 以本文件「版本变更记录」表为准做 fail-closed
   判定：版本号已登记在表中时，正文必须仍是该版本登记的那一份；尚未登记时，正文必须相对
   表中最新一行确有变化。故「不改正文只递增版本号」与「改正文不递增版本号」都会在此被拒。
4. 在本文件「版本变更记录」表**追加一行**：版本号、日期、字符总数、可消费汉字数、
   `source_sha256` 前 12 位、影响说明。
5. 运行 `check`。
6. 不改正文却变更版本号属违规：`emit` 拒绝重建（变更记录中最新一行与正文摘要一致而版本号
   已递增）。改正文却不递增版本号同样违规：`emit` 拒绝重建，`check` 亦因当前版本记录行
   的摘要与正文不符而失败。

### 变更记录的静态校验范围

变更记录只能追加：版本号不得重复、必须严格递增，初始版本 `HS-1.0.0` 行必须保留，
同一份正文不得登记在两个版本号下。

`check` 用 manifest 的 `previous` 存档钉住**紧邻当前版本之前的那一行**：该行的版本号、
字符总数、可消费汉字数、`source_sha256` 前 12 位逐值比对，任一单元格被改写即判定失败；
台账已有历史行而 `previous` 不是存档对象时同样失败。`previous` 由 `emit` 写入，记录上一
版发布时的 `{scripture_version, source_sha256, counts, consumable_digest}`。

**不在静态校验威胁模型内的部分**：本目录只保留上一版存档，不维护完整历史数组或哈希链，
因此**更早版本的历史行**没有可比对的真源，对它们的静态约束只有「版本号不重复、严格递增」
与「不同版本号不得指向同一份正文」。变更记录是人工维护的追加式台账，本目录不保留它的独立
副本，因此**手工改写变更记录行**同样无法被静态校验发现：例如改正文后同步改写当前版本行的
摘要与计数，改后的正文即可沿用原版本号通过判定。`check` 是构建与打包前的 fail-closed 门禁，
不是防篡改签名。

## 版本变更记录

| 版本 | 日期 | 字符总数 | 可消费汉字数 | `source_sha256` 前 12 位 | 影响说明 |
| --- | --- | --- | --- | --- | --- |
| HS-1.0.0 | 2026-09-22 | 303 | 260 | a265c93dbc60 | 首次冻结 canonical 源文本，建立唯一落盘来源、生成校验工具与三端经文资源 |
