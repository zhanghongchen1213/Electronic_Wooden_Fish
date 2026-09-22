---
baseline_commit: fe98d7375f21b02f22ed2cbdb3824b1462940ace
---

# Story 4.1: 建立 canonical《心经》与版本校验

Status: done

## Story

As a 产品维护者，
I want 三端从同一个 canonical 文本源生成可消费汉字序列，
so that 设备、backend 和小程序不会因经文漂移而错位。

本 Story 覆盖 FR-C-002、AD-4、UX-DR4，是 Epic 4 的第一道门禁：在
backend 启动基线（4.3）、身份入口（4.4）、原子持久化（4.5）和快照恢复（4.6）
落地之前，先把「唯一经文」钉死为可构建、可校验、可追溯的单一落盘来源。
本 Story 只产出经文源、生成/校验工具和三端经文资源；**设备字形子集与
SquareLine 字体合同的建立属于 Story 3.1（Epic 3）**，backend/小程序在运行期
加载经文资源属于 4.3/6.x。本 Story 不实现游标推进、不实现同步、不接触
`Embedded/` 源码树。

## Acceptance Criteria

1. **Given** 项目准备构建或打包经文资源，**When** 生成 Embedded/backend/frontend 资源，
   **Then** 所有端使用同一落盘来源和 `scripture_version`，标点/空格/换行的消费
   语义可被校验。
2. **And** 任一端版本不一致时构建或运行检查失败，不自动映射、选经或导入。
3. **Given** canonical 文本被更新，**When** 执行版本变更检查，
   **Then** 变更包含可追溯的版本号、字符总数和影响说明。
4. **And** 不把设备 7 槽或小程序 17 槽误当成经文长度上限。

## Tasks / Subtasks

- [x] **1. 建立 canonical 单一落盘来源（AC: 1, 2）**
  - [x] 1.1 创建 `docs/contracts/canonical/heart-sutra.txt`：只含经文正文，
        UTF-8 **无 BOM**、LF、末尾单个换行；不得写入任何字段、计数、注释或
        书名号标题（`title` 只存在 manifest 中）。该文件是**唯一落盘来源**，
        Embedded/backend/frontend 均不得自留一份誊写副本。
  - [x] 1.2 从**可核验的公开经文来源**（如 CBETA 同等级资料）逐字誊写，
        禁止凭记忆手打；把版本/译本/出处与获取日期记入 manifest `provenance`。
        **若无法取得可核验来源，停止并报告阻塞**，不得先填占位文本
        （禁止 `placeholder`/`draft`/`TODO`）。
  - [x] 1.3 用冻结 UX 真源做前锚点校验：文本前 26 个字符必须等于
        `观自在菩萨，行深般若波罗蜜多时，照见五蕴皆空，度一切`
        （等价于 `Embedded/lvgl-design/ewf-device-ui-export.html` 经文页前两行
        13 槽行的顺序拼接），且第 27 字起必须接 `苦厄`。
  - [x] 1.4 创建 `docs/contracts/canonical/heart-sutra.manifest.json`，字段见
        Dev Notes「manifest 契约」。所有计数字段必须由工具从源文本派生，
        不得手工填入。
  - [x] 1.5 创建 `docs/contracts/canonical/README.md`：声明唯一来源、消费语义、
        生成/校验命令、**版本变更流程**，并附「版本变更记录」表（当前版本行
        必须与 manifest 一致）。

- [x] **2. 实现生成与校验工具（AC: 1, 2, 3）**
  - [x] 2.1 创建 `docs/contracts/canonical/scripture_tool.py`，纯 Python 3 标准库、
        无第三方依赖，提供两个子命令：`emit`（确定性地重写 manifest 与三端产物）
        与 `check`（只读校验，失败退出码非 0）。
  - [x] 2.2 `check` 必须覆盖：源文本 sha256 与 manifest 一致；三端产物的
        `scripture_version` 与 manifest 一致；三端产物的消费序列摘要与计数与
        manifest 一致；字符分类封闭（出现未声明的非消费字符即失败）；前锚点
        一致；当前版本的变更记录行存在且其摘要/计数与 manifest 一致。
  - [x] 2.3 `emit` 必须幂等：连续两次 `emit` 后 `git status` 无新增差异
        （产物字节稳定，不含时间戳等易变字段）。
  - [x] 2.4 失败信息用中文、可诊断、指明是「哪一端/哪个文件/哪个字段」不一致；
        `check` 不得修改任何文件。

- [x] **3. 产出三端经文资源（AC: 1, 2, 4）**
  - [x] 3.1 backend 资源：`cloud/backend/src/main/resources/canonical/heart-sutra.json`
        （由工具生成，供 4.3/5.x 加载；本 Story 不写 Java 代码）。
  - [x] 3.2 frontend 资源：`cloud/frontend/src/canonical/heart-sutra.generated.ts`
        （由工具生成）。`cloud/frontend/src/` 目前不存在，本 Story 只创建
        `src/canonical/` 这一条最小路径，完整 `src` 骨架仍由 6.1 建立。
  - [x] 3.3 Embedded 面向的生成头：`docs/contracts/canonical/generated/ewf_scripture_canonical.h`
        （由工具生成，带 `GENERATED — 禁止手改` 横幅）。**本 Story 不修改
        `Embedded/` 任何文件**：Epic 7 才是唯一允许同时修改 Embedded 与 cloud
        的 Epic，Epic 4 只负责生成并校验该头，Story 3.1 接入设备构建时再引用。
  - [x] 3.4 三端产物都必须暴露 `scripture_version`、消费序列与派生计数，
        且不得出现 Pencil Node ID、`data-pencil-id`、`TODO`、`draft`、
        `placeholder` 或任何作者说明。

- [x] **4. 接入构建/打包期检查（AC: 1, 2）**
  - [x] 4.1 在 `cloud/env-scripts/build-prod-backend.sh` 的 `mvn ... package`
        之前插入 `scripture_tool.py check`，使后端打包在没有 pom.xml 之外
        的情况下也保持 fail-closed 顺序（该脚本当前因缺 `cloud/backend/pom.xml`
        在更早处退出，4.3 提供 pom 后此门禁即生效）。
  - [x] 4.2 在 `docs/contracts/canonical/README.md` 中以**强制前置义务**记录：
        Embedded 与小程序构建/打包前必须执行同一条 `check`；并把接入点登记为
        Story 3.1（设备）与 Story 4.3/6.1（cloud）的落地责任。

- [x] **5. 测试与证据（AC: 1, 2, 3, 4）**
  - [x] 5.1 创建 `docs/contracts/canonical/tests/run_canonical_tests.py`：
        纯 Python、`assert` + 自定义失败计数、非 0 退出，**不得依赖 pytest**
        （本机无 pytest；范式参照 `Embedded/tests/run_host_tests.py`）。
  - [x] 5.2 负例必须覆盖 Dev Notes「必须失败的负例」全部 8 条，且负例在临时
        目录副本上执行，不得污染真实产物。
  - [x] 5.3 运行并记录：`check` 通过、负例全部按预期失败、`emit` 幂等。

## Dev Notes

### 真源与冲突裁决

按 `docs/README.md` 事实优先级读取，冲突时高者覆盖低者：

| 事项 | 真源 |
| --- | --- |
| Story 需求与 AC | `epics.md` §Epic 4 / Story 4.1（覆盖 FR-C-002、AD-4、UX-DR4） |
| 产品行为 | `prd.md` §6.1.2 FR-C-002、§4 术语表「canonical《心经》」、§6.4 跨层字段 |
| 跨层不变量 | `ARCHITECTURE-SPINE.md` AD-4、AD-19（另受 AD-16/AD-20 约束） |
| 消费/展示语义 | `UI_CONTRACT-device.md`（JINGWEN / scripture-history / reading-progress）、`UI_CONTRACT-miniapp.md`（readingline）、`DESIGN.md` §Typography |
| cloud 层规则 | `cloud/AGENTS.md` §1/§3/§5/§6/§7 |
| 编码与生成物规则 | `AGENTS.md` §3.3；`Embedded/AGENTS.md`（UTF-8 无 BOM）；`docs/embedded/style/C编码规范-Agent版.md` §文件头 + 红线「不手改生成物」 |

已核实的冲突/空白（本 Story 必须按下列裁决执行，不得再来回摇摆）：

1. **canonical 经文目前不存在于仓库任何位置。** 已检索全仓：仅 `epics.md`、
   `prd.md`、`ARCHITECTURE-SPINE.md`、`brief.md` 提到《心经》，以及
   `Embedded/lvgl-design/ewf-device-ui-export.html` / `.pen` 与
   `cloud/miniapp-design/ewf-miniapp-ui-export.html` / `.pen` 中的 UX 片段。
   → canonical 必须由本 Story 新建，`docs/contracts/canonical/` 是新目录。
2. **冻结 Pen/HTML 不是经文来源。** 它们只是 UX 真源，含经文片段与示例进度
   文案；不得把 Pen/HTML 当作 canonical 落盘来源，也不得从 Pen 反推整篇文本。
   Pen/HTML 只用作**前锚点校验**（见 1.3）。
3. **`Embedded/lvgl-design/squareline_studio/font_glyph_contract.json` 是 legbot
   遗留资产**（其 `runtime_sources` 指向 `助力开启`/`支付`/`模式` 等 legbot 内容），
   **不得复用或扩展**。EWF 的字体合同由 Story 3.1 重建，其字形子集输入即本
   Story 生成的消费序列。
4. **`docs/contracts/README.md` 目前只有占位说明**，声明 `sync-contract.md`
   尚未创建。本 Story 在 `docs/contracts/` 下新增 `canonical/` 子目录，
   **不创建、不预写 `sync-contract.md`**（那是 Story 4.2 的交付物）。
5. **JSON schema/文件粒度仍在架构 Deferred 中**（A-2，须经 sync-contract 收敛）。
   → 本 Story 只冻结**经文资源**的 schema（见下），不冻结同步包、持久化文件或
   WebSocket 帧的 schema。

### 上游故事与仓库情报

- **Epic 4 内无前置故事**（本 Story 是 4.x 的第 1 个），无需继承 Epic 4 的
  dev-story 复盘。Epic 1 的 Story 1.1（`1-1-建立定稿电源-启动与-bsp-自检.md`）
  当前为 `review`，其模式值得沿用：
  - 主机测试用**纯 Python + `assert` + 非 0 退出**，明确不依赖 pytest；
  - 「自检结果与硬件/软件证据口径」独立成节，区分**设计输入 / 软件观察 /
    待样机验证**——本 Story 的对应纪律是把「工具 `check` 通过」与「三端已能
    显示经文」严格分开（本 Story 不接入任何运行期）。
- **近期提交情报**：`47e4f18` 已落地 `cloud/AGENTS.md`、`application.yml`、
  `application-local.yml`、`env-scripts` 四个脚本与 `.env`/`.env.development`，
  即 AD-20 的路径契约已就位 → 本 Story 的 backend/frontend 产物路径与
  `build-prod-backend.sh` 插入点都已存在，不需要新建配置。
  `eeee145` 落地 BSP/CMake 基线；`Embedded/components/contract_checks/` 用
  `_Static_assert` 做编译期契约钉死的范式可参考，但**本 Story 不改 `Embedded/`**。
- **当前工作区**：`git status` 显示 Embedded 侧有大量未提交改动（含
  `Embedded/components/{BSP,app_state,control_gate,platform,services,ui}` 与
  `Embedded/CMakeLists.txt`、`sdkconfig*`）。这是上游故事的在制品，**本 Story
  必须完全不碰**，也不要因为看到未提交内容而回滚或清理。
- **公共设施现状**：`cloud/backend` 仅有 `src/main/resources/application*.yml`
  （无 `pom.xml`、无 Java 源码）；`cloud/frontend` 仅有 `.env`/`.env.development`。
  → 本 Story 只能产出**资源与生成物**，backend/小程序的运行期加载分别落在
  4.3 与 6.x。
- **本机环境事实**：`python3` = 3.12.13；`import pytest` 失败（未安装）。
  脚本只允许标准库。

### canonical 单一落盘来源（本 Story 冻结）

```
docs/contracts/canonical/
  heart-sutra.txt                       # 唯一落盘来源（经文正文）
  heart-sutra.manifest.json             # 派生元数据与校验锚点
  README.md                             # 消费语义、命令、版本变更流程 + 记录表
  scripture_tool.py                     # emit / check
  generated/ewf_scripture_canonical.h   # Embedded 面向的生成头（不落 Embedded/）
  tests/run_canonical_tests.py          # 纯 Python 门禁测试
```

选 `docs/contracts/canonical/` 的理由：`docs/contracts/` 已是跨层契约的家
（`docs/README.md`、`cloud/AGENTS.md` §1）；`docs/hardware/电子木鱼-硬件网络清单.json`
已有「机器可读产物放 docs」的先例。工具与源数据同目录，便于一次性审查。

### 消费语义（可校验的判定规则）

以 AD-19 与 FR-C-003 为准，本 Story 把它固化成可执行判定，避免各端各自解释：

- **可消费字符**：Unicode 汉字，限定在 BMP 的 CJK 统一表意文字 `U+4E00–U+9FFF`
  与扩展 A `U+3400–U+4DBF`。一次有效敲击**只推进一个**可消费汉字。
- **非消费字符**：标点、空格、换行等一切非上述码点。它们**不消耗敲击、不单独
  推进游标**，随**紧邻的前一个汉字在同一步内出现**。
- **步（step）定义**：`step[i] = 第 i 个可消费汉字 + 紧随其后的连续非消费字符`。
  这条定义同时决定了设备 7 槽/13 槽与小程序 17 槽中「标点计槽」的显示行为，
  因此必须以测试固定。
- **非消费字符集合必须显式封闭**：源文本中出现的每个非消费字符都要在 manifest
  `non_consumable_chars` 中列出。出现未声明的字符（例如 Latin 字母、`《》`、
  半角句点）即判定失败，**不得自动映射或规范化**（AC 2「不自动映射」）。
- **换行**：`heart-sutra.txt` 内的换行只是文件级行结束，不是经文内容；若正文
  需要换行/空格作为排版分隔，必须作为非消费字符显式计入并声明。
- **消费序列摘要**：`consumable_digest` = 把可消费汉字按序拼接为 UTF-8 后取
  sha256。它是三端「同一版序列」的最低成本证据。

### manifest 契约（本 Story 冻结）

`docs/contracts/canonical/heart-sutra.manifest.json`：

| 字段 | 含义 |
| --- | --- |
| `schema_version` | 本 schema 的版本，初始 `1` |
| `title` | `般若波罗蜜多心经`（**只在此处**，不写入 `heart-sutra.txt`） |
| `scripture_version` | 不透明字符串，格式固定 `HS-<major>.<minor>.<patch>`，初始 `HS-1.0.0` |
| `source_file` | `heart-sutra.txt` |
| `source_sha256` | 源文本字节的 sha256 |
| `consumable_digest` | 可消费汉字序列的 sha256 |
| `counts.total_chars` | 源文本字符总数（含非消费字符，不含文件末尾换行） |
| `counts.consumable_han` | 可消费汉字数 |
| `counts.non_consumable` | 非消费字符数 |
| `non_consumable_chars` | 源文本中实际出现的非消费字符去重列表 |
| `first_consumable` | `观`（前锚点首字） |
| `anchor_26` | `观自在菩萨，行深般若波罗蜜多时，照见五蕴皆空，度一切` |
| `provenance` | `{edition, source, retrieved}`，记录译本/出处/获取日期 |

`counts.*`、`consumable_digest`、`source_sha256` 一律由工具派生；手改即校验失败。

### 三端生成产物

三端产物都必须携带：`scripture_version`、可消费序列、`consumable_digest`、
`counts.consumable_han`。Embedded 侧额外需要「游标 → 字」的定位能力，因此生成头
采用「全文 + 每步起点的字节偏移」形式，避免在 C 侧复制判定逻辑：

```c
/* GENERATED — 由 docs/contracts/canonical/scripture_tool.py 生成，禁止手改。 */
#define EWF_SCRIPTURE_VERSION            "HS-1.0.0"
#define EWF_SCRIPTURE_CONSUMABLE_COUNT   /* 派生 */
#define EWF_SCRIPTURE_STEP_COUNT         /* == CONSUMABLE_COUNT */
extern const char     EWF_SCRIPTURE_DISPLAY_UTF8[];   /* 源文本 UTF-8 */
extern const uint16_t EWF_SCRIPTURE_STEP_OFFSETS[];   /* 每步在 DISPLAY_UTF8 中的字节偏移 */
```

- 该头文件是**生成物**，遵循 `C编码规范-Agent版.md` 红线「不手改生成物」；
  必须带 `GENERATED` 横幅和生成命令。
- 本 Story 不把它放进 `Embedded/`，也不改 `Embedded/tests/run_host_tests.py`
  （Epic 边界见下）。Story 3.1 接入设备构建时再添加 include 路径。
- backend/frontend 产物同理只承载经文资源，不含同步字段、不含游标状态机。

### 版本变更流程（AC 3 的可执行形式）

1. 修改 `heart-sutra.txt`。
2. 递增 `scripture_version`（任何影响消费序列或展示文本的改动都必须递增；
   纯说明性元数据如 `provenance` 措辞、补记日期可不递增）。
3. 运行 `emit` 重建 manifest 与三端产物。
4. 在 `README.md` 的「版本变更记录」表**追加一行**：版本号、日期、字符总数、
   可消费汉字数、`source_sha256` 前 12 位、影响说明（例如「修正第 N 步
   标点归属，`round_cursor` 语义不变」）。
5. 运行 `check`；当前版本没有对应记录行、或记录行摘要/计数与 manifest 不一致
   即判定失败。
6. 不改正文的版本号变更属于违规：`check` 会因记录行摘要不匹配而失败。

### 必须失败的负例（AC 1/2/3）

1. 改 `heart-sutra.txt` 一个汉字、不更新 manifest → 失败（`source_sha256`/摘要不符）。
2. manifest 的 `scripture_version` 改成三端产物不含的版本 → 失败（版本不一致）。
3. 把某标点移到另一个汉字之后（改变步归属）→ 失败（`consumable_digest` 或 counts 不符）。
4. 源文本插入未声明的非消费字符（`A` 或 `《》）→ 失败（分类封闭）。
5. 删除当前版本的变更记录行 → 失败（AC 3）。
6. 篡改 `anchor_26` 或源文本前 26 字 → 失败（前锚点）。
7. 手工改动任一三端产物的消费序列 → 失败（产物与 manifest 不一致）。
8. `counts.consumable_han` 被手工写成 `7`/`13`/`17` → 失败（AC 4：槽数不是经文长度上限）。

### 红线与禁止事项

- **Epic 边界**：Epic 7 是唯一允许同时修改 Embedded 与 cloud 的 Epic。本 Story
  在 Epic 4（cloud），**不得修改 `Embedded/` 任何文件**，不得修改
  `Embedded/tests/run_host_tests.py`。
- **不做**：选经、导入、异体字自动映射、多文本源、通用中文字库、经文版本协商、
  运行期回退到默认文本。版本不一致一律**停止并报告配置错误**（FR-C-002）。
- **不臆造**：正文必须来自可核验来源；不得凭记忆写出「差不多」的经文。
  无法核验即报告阻塞。
- **槽数不是长度**：设备木鱼页字带 7 槽、设备经文页每行 13 槽、小程序每行 17 槽
  都是**视口槽位**，不是经文长度上限（AC 4）。工具不得读取任何槽位常量。
- **UI 分母**：设备 `reading-progress` 的进度分母取**运行时** `scripture_chars_total`
  （`UI_CONTRACT-device.md` §reading-progress）；生成产物不得把构建期常量当作该
  分母的替代来源，也不得把 `scripture_chars_total` 写进经文源。
- **文案卫生**：生成产物与交接文档不得出现 AI 思维链、Node ID、`data-pencil-id`、
  `TODO`、`draft`、`placeholder`、作者提示（`AGENTS.md` §3.7、`cloud/AGENTS.md` §5）。
- **编码**：全部文本文件 UTF-8 无 BOM、LF、末尾单换行（`Embedded/AGENTS.md`）。
  发现 `纭/锛/涓` 或 `FF FE` 立即停止并重写。

### 测试与完成判定

- 门禁入口：`python3 docs/contracts/canonical/scripture_tool.py check`（退出码 0）与
  `python3 docs/contracts/canonical/tests/run_canonical_tests.py`（退出码 0）。
- **不得依赖 pytest**（本机 `python3 -c "import pytest"` 失败）；用 `assert` +
  失败计数 + 非 0 退出的项目既有范式。
- 通过标准：`check` 通过；8 条负例全部按预期失败；`emit` 幂等（二次运行无 diff）；
  三端产物的 `scripture_version` 完全一致；`git diff --check` 干净。
- 不得把「工具跑通」写成「设备/小程序已能显示经文」——本 Story 未接入任何运行期。

## Project Structure Notes

- 新增目录 `docs/contracts/canonical/`：跨层契约目录下的新叶子目录，用于承载
  FR-C-002 要求的**唯一落盘来源**与其生成/校验工具。理由与先例见 Dev Notes。
  这是本 Story 唯一的结构性新增，与 `ARCHITECTURE-SPINE.md` §Structural Seed 的
  目录树不冲突（该 seed 非穷举）。
- 新增目录 `cloud/frontend/src/canonical/`：`cloud/frontend/` 目前只有 `.env` 与
  `.env.development`，尚无源码树。本 Story 只创建 `src/canonical/` 最小路径，
  **不预建** pages/api/stores 等骨架（属 6.1）。若 6.1 最终采用不同 src 根，
  只需移动该生成产物，生成与校验入口不变。
- 新增 `cloud/backend/src/main/resources/canonical/`：`src/main/resources/` 已存在
  （`application.yml`、`application-local.yml`），本 Story 只在其下新增经文资源
  子目录，不新增 Java 源码。
- 修改 `cloud/env-scripts/build-prod-backend.sh`：仅在 `mvn package` 前插入一条
  fail-closed 检查，不改动其根目录解析、JDK/Maven 校验、产物与 `.sha256` 逻辑
  （AD-20 固定契约）。
- 明确不触碰：`Embedded/**`（含 `Embedded/tests/`、`Embedded/lvgl-design/`）、
  `docs/contracts/sync-contract.md`（不存在，属 4.2）、`cloud/backend/pom.xml`
  （属 4.3）。

### References

- [Source: `_bmad-output/planning-artifacts/epics.md`#Epic 4 / Story 4.1] 需求、AC 与覆盖需求标记（FR-C-002、AD-4、UX-DR4）
- [Source: `_bmad-output/planning-artifacts/epics.md`#Epic 7] 「Epic 7 是唯一允许同时修改 Embedded 与 cloud 的 Epic」
- [Source: `_bmad-output/planning-artifacts/epics.md`#Requirements Inventory / AD-4] 「canonical 经文：单一文本源与 `scripture_version` 在三端构建期生成并校验；版本不一致即停」
- [Source: `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`#6.1.2 FR-C-002] 单一落盘来源、三端构建/打包期嵌入、不一致即停
- [Source: `prd.md`#4 术语表] 「canonical《心经》：三端共享的唯一可消费汉字序列及其 `scripture_version`」
- [Source: `prd.md`#6.1.3 FR-C-003] 标点/空格/换行随相邻汉字出现，不单独消费敲击
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md`#AD-4] 固定经文版本、构建期一致、不一致即停、设备字形只覆盖本篇
- [Source: `ARCHITECTURE-SPINE.md`#AD-19] 一敲一字、标点随附、不预览未来、`round_id` 跨轮
- [Source: `ARCHITECTURE-SPINE.md`#AD-16 / #AD-20] 零库 JSON 原子文件、backend 配置与交付脚本固定路径（本 Story 只遵守，不修改其契约）
- [Source: `ARCHITECTURE-SPINE.md`#Deferred] JSON schema/文件粒度须经 `sync-contract.md` 收敛（本 Story 只冻结经文资源 schema）
- [Source: `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/UI_CONTRACT-device.md`#JINGWEN / #scripture-history] 每行 13 槽、标点计槽、不预览未来
- [Source: `UI_CONTRACT-device.md`#reading-progress] 「分母取运行时 `scripture_chars_total`，不写入 canonical 经文源，也不引入新指标」
- [Source: `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/UI_CONTRACT-miniapp.md`#readingline] 每行 17 槽、标点计槽、只追加 backend 已确认字
- [Source: `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/DESIGN.md`#Typography] UX-DR4：设备只嵌入《心经》所需字形子集，不引入通用中文字库
- [Source: `Embedded/lvgl-design/ewf-device-ui-export.html`] 冻结 Pen 导出的经文页前两行 13 槽行（`观自在菩萨，行深般若波罗蜜` / `多时，照见五蕴皆空，度一切`）与第三行起 `苦厄`，作为前锚点证据
- [Source: `cloud/AGENTS.md`#1 / #3] cloud 权威顺序、canonical 单一来源与版本一致性、JSON 零库、`/api/v1` 信封
- [Source: `cloud/AGENTS.md`#5] 页面可见文案只能是产品文案、状态文案和 canonical《心经》
- [Source: `cloud/AGENTS.md`#6] cloud Epic 4–6 必须可用 mock 独立验证；先更新契约样例与测试再改实现
- [Source: `AGENTS.md`#3.3] 读/写仓库文本默认 UTF-8 无 BOM；种子乱码即停
- [Source: `AGENTS.md`#3.7] 最终页面文案与设计产物硬禁令（Node ID / `data-pencil-id` / `TODO` / `draft` / `placeholder`）
- [Source: `Embedded/AGENTS.md`] UTF-8 无 BOM、LF、末尾单换行；禁止污染文本进入生成物
- [Source: `docs/embedded/style/C编码规范-Agent版.md`] §文件头 Doxygen 模板；红线「不手改生成物」
- [Source: `docs/contracts/README.md`] `sync-contract.md` 尚未创建；跨层语义先冻结再落到各层
- [Source: `docs/README.md`] 事实优先级与「分层文档不得自行定义经文游标或同步状态」
- [Source: `Embedded/tests/run_host_tests.py`] 项目主机测试范式：纯 Python、`assert`、无 pytest、无第三方依赖、非 0 退出
- [Source: `cloud/env-scripts/build-prod-backend.sh`] JDK 17 / Maven 3.9.x 校验与 `mvn -B -U clean package -DskipTests` 的插入点

## Dev Agent Record

### Agent Model Used

Claude Code（bmad-dev-story，2026-09-22；create-story 上下文于同日生成）

### Debug Log References

- 门禁测试入口：`python3 docs/contracts/canonical/tests/run_canonical_tests.py` → 10/10 通过（退出码 0）
- 版本一致性门禁：`python3 docs/contracts/canonical/scripture_tool.py check` → 退出码 0
- 既有设备侧主机测试回归：`python3 Embedded/tests/run_host_tests.py` → 11/11 源码合同 + 9 个主机用例通过（退出码 0，未改动 `Embedded/`）
- 负例实测：8 条负例分别在临时目录副本上复现，失败信息均指向预期字段（`source_sha256`、`scripture_version`、`step 序列`、`non_consumable_chars`、`anchor_26`、变更记录行、产物消费序列、`counts.consumable_han`）

### Completion Notes List

- **可核验来源**：正文汉字逐字取自大正新脩大藏經 第8卷 No.251《般若波羅蜜多心經》（唐·三藏法師玄奘譯），转录底本为《大正新修大藏經 第8卷·般若部四》第 864 页；版本/译本/出处/获取日期已记入 manifest `provenance`。未凭记忆书写正文，转换过程为「保留来源汉字、按冻结 UX 口径归并标点」的机械规则。
- **前锚点校验通过**：源文本前 26 字符等于冻结 UX 真源锚点 `观自在菩萨，行深般若波罗蜜多时，照见五蕴皆空，度一切`，第 27 字符起接 `苦厄`。
- **标点口径依据**（本 Story 的关键裁决）：设备冻结 UX 导出 `Embedded/lvgl-design/ewf-device-ui-export.html` 中经文只使用 `，`（7 处），`。`、引号、叹号、顿号、分号、冒号均不出现；且经文页第 1、2 行各恰好 13 字符（`观自在菩萨，行深般若波罗蜜` / `多时，照见五蕴皆空，度一切`），第 3 行以 `苦厄舍利` 起。第 3 行证明 `苦厄` 之后紧接 `舍利` 而非 `。舍`，故 canonical 不含句末句号。最终 `non_consumable_chars` 为单元素集合 `["，"]`，字符分类封闭性最强，并与「每行 13 槽、标点占槽」的槽位证据逐字符一致。
- **长度交叉核验**：正文可消费汉字 **260** 个（经题不计入消费序列），与设备冻结 UX 的进度分母 `260`（「心经进度 196 / 260 字」）一致；含经题的传统字数 268 亦由同一来源复算得到。
- **派生纪律**：manifest 的 `counts.*`、`source_sha256`、`consumable_digest`、`non_consumable_chars` 全部由 `emit` 从源文本派生，无手工填入字段；`emit` 保留已递增的 `scripture_version` 与 `provenance`，供版本变更流程使用。
- **`emit` 幂等**：真实仓库连续两次 `emit` 后三端产物与 manifest 字节完全一致，`git status` 无新增差异（产物不含时间戳等易变字段）。
- **构建期门禁已接入**：`cloud/env-scripts/build-prod-backend.sh` 在 `mvn ... package` 之前执行 `check`。已用干净副本（退出码 0、继续打包）与篡改副本（退出码 1、停止打包）双向验证 fail-closed 行为。该脚本在本机因缺少 `cloud/backend/pom.xml` 会在更早的门禁退出（属 Story 4.3），门禁位置在 `mvn` 之前，pom 补齐后即生效。
- **未接入任何运行期**：本 Story 只产出经文源、生成/校验工具与三端经文资源；不实现游标推进、不实现同步、不新增 backend/小程序运行期代码，也不读取任何槽位常量。
- **未修改 `Embedded/` 任何文件**（Epic 边界，Epic 7 才是唯一允许同时修改 Embedded 与 cloud 的 Epic）。生成头落盘于 `docs/contracts/canonical/generated/`，不在 ESP-IDF 组件路径内，不参与固件构建，故未执行 ESP-IDF 构建。
- **文案卫生**：生成产物与交接文档经扫描，不含 Node ID、`data-pencil-id`、`TODO`、`draft`、`placeholder` 或作者提示。
- **编码**：全部新增/修改文本文件为 UTF-8 无 BOM、LF、末尾单换行；`git diff --check` 干净。

### File List

- `docs/contracts/canonical/heart-sutra.txt`（新增）
- `docs/contracts/canonical/heart-sutra.manifest.json`（新增）
- `docs/contracts/canonical/README.md`（新增）
- `docs/contracts/canonical/scripture_tool.py`（新增）
- `docs/contracts/canonical/generated/ewf_scripture_canonical.h`（新增）
- `docs/contracts/canonical/tests/run_canonical_tests.py`（新增）
- `cloud/backend/src/main/resources/canonical/heart-sutra.json`（新增）
- `cloud/frontend/src/canonical/heart-sutra.generated.ts`（新增）
- `cloud/env-scripts/build-prod-backend.sh`（修改）

### Change Log

- 2026-09-22：实现 Story 4.1 全部任务与子任务 —— 建立 canonical《心经》单一落盘来源、`emit`/`check` 工具、
  三端经文资源、构建期门禁与 10 项门禁测试（含 8 条必须失败的负例）；状态推进至 `review`。

### Review Findings

本轮为 attempt-3 的 `deep` / `full` / `autofix` 审查（4 层全部 active 且 mandatory，全部成功，无 failed layer）。目标之一是独立复核前两轮遗留的 2 条 medium 是否真实闭合：

- **前轮 blocker 1（生成头不是合法 C、两个符号只有声明无定义）已真实闭合。** 本轮改为 header-only 自洽头：`EWF_SCRIPTURE_DISPLAY_UTF8[909]` 与 `EWF_SCRIPTURE_STEP_OFFSETS[260]` 均头内 `static const` 定义。独立复核（未复用套件代码）：`cc -fsyntax-only -x c <头>` 退出码 0；探针 TU 仅 include 该头即可链接并运行（`bytes=909 steps=260 consumable=260 total=303`，首偏移 0、末偏移 906）；自写脚本从 `heart-sutra.txt` 独立复算，909 个字节载荷逐字节相同、260 个步起点逐值相同，`source_sha256`/`consumable_digest`/三项计数与前 26 字锚点全部复算一致。
- **前轮 blocker 2 的两个诚实路径已闭合，但修复本身自相矛盾，引出新的 HIGH（见第 1 条）。** 已实测闭合的两条：① 只递增版本号（正文不动）并追加同摘要记录行 → `check` 退出 1（两个版本号指向同一份正文）；② 改正文而不递增版本号 → `emit` 退出 1 且 `check` 退出 1。上一轮所述「文档化流程即可绕过」的两条路径不再成立。

- [x] [Review][Decision] `emit` 的「正文 ↔ 版本号」守卫不可满足，文档化的版本变更流程无法执行（attempt-3 新引入；attempt-4 复核已闭合） [`docs/contracts/canonical/scripture_tool.py:144`、`:286-306`、`:312-313`] — `build_manifest()` 第 144 行把新 manifest 的版本号取自上一版 manifest（`version = previous.get("scripture_version") or INITIAL_SCRIPTURE_VERSION`），而 `emit()` 第 312-313 行先 `build_manifest` 再 `check_version_binding`；因此 `check_version_binding` 第 298 行的放行分支 `if previous_version != manifest["scripture_version"]: return` 在上一版带版本号时恒为假（死代码），第 300 行的 `raise` 对**任何**正文变化都会触发。独立复现（真实产物副本沙箱）：把 `heart-sutra.txt` 的 `罣` 改成 `挂`、按 `README.md:97-99` 把 manifest 的 `scripture_version` 递增为 `HS-1.0.1`、再按 `README.md:100` 运行 `emit` → 退出码 1，输出「正文已变化…但仍为 'HS-1.0.1'」——诊断信息与刚做过的递增动作自相矛盾。后果：`README.md` §版本变更流程 第 1-3 步（本 Story 交付的 AC 3 可执行形式）无法执行；仅剩两条出口且都破坏绑定：(a) 删除 manifest 后 `emit` 退出 0 并把版本号静默回落为 `HS-1.0.0`（实测；随后 `check` 会因记录行摘要不符而失败），(b) 手改 manifest 的 `source_sha256` 为新值后 `emit` 退出 0、`check` 亦退出 0（实测，见第 2 条），而该字段正是守卫唯一的存档依据。判 unresolved：修复须先裁定「版本号 ↔ 正文」的存档载体（今日除 manifest 自身外无任何持久存档；记录行只存 12 位摘要），可选方向包括改由 README 记录行充当存档、或另立只追加的版本档案、或撤回该守卫仅由 `check` 承担，三者互斥且都改动 `emit` 的输入契约，需人裁定，故不在 autofix 下自动改写。【attempt-4 复核：已闭合】判定真源已从「可手改的 manifest 派生字段」换成 README 追加式台账 + 本次从源文本派生的值；独立沙箱（未复用套件代码）走完 §版本变更流程第 1-5 步：改正文（`罣`→`挂`）→ 递增 `scripture_version` 为 `HS-1.0.1` → `emit` 退出 0（`previous` 正确写为 `{HS-1.0.0, a265c93dbc60…}`）→ 追加记录行 → `check` 退出 0。上述出口 (b) 已实测封堵（改正文 + 手改 manifest `source_sha256`：`emit` 退出 1、`check` 退出 1）；出口 (a) 仍能让 `emit` 退出 0，但 `check` 退出 1，整体 fail-closed。
- [x] [Review][Decision] 版本变更记录的历史行可被静默改写，`README.md` 的承诺强于实现（attempt-3 未解决；attempt-4 复核已闭合） [`docs/contracts/canonical/scripture_tool.py:345-380`、`docs/contracts/canonical/README.md:104-107`] — `check_version_history()` 只做三件事：版本号单元格是否重复、两行的（字符总数 / 可消费汉字数 / `source_sha256` 前 12 位）三元组是否相同、`HS-1.0.0` 版本号单元格是否存在；历史行的**数值单元格从不与任何存档比对**。两个独立复现：① 正文不动、manifest 与三端产物版本改为 `HS-1.0.1`、把 `HS-1.0.0` 行的摘要前 12 位改写成 `ffffffffffff`、再追加正确的 `HS-1.0.1` 行 → `check` 退出 0 并报「一致」；② 移动一个逗号、保持 `HS-1.0.0`、同步 manifest 的 `source_sha256`、就地改 `HS-1.0.0` 行的摘要 → `check` 退出 0，即同一版本号指向两份正文。`README.md:105-107` 承诺「变更记录中两个版本号指向同一份正文、或初始版本行被改写/删除，即判定失败…历史行被改写或删除同样失败」，其中「删除」与「改写版本号单元格」确实失败，但「改写数值单元格」不失败，承诺强于实现。与第 1 条同源：两条都需先手改 manifest 的派生字段才可达（诚实的前端流程已被第 1 条的守卫拦住），故按「手工伪造生成物」层记录。判 unresolved：修复须在「补一条历史行数值不得改写/须与存档互不相同的校验以兑现承诺」与「把 README 措辞收敛为如实描述现有校验」之间选择，二者互斥，需人裁定。附注：dev receipt 称该残留需「同时篡改 manifest + 三端产物 + README 记录行」，实测只需改 manifest 的 `source_sha256` 加 README 记录行（三端产物由 `emit` 重建），措辞高估了篡改门槛。【attempt-4 复核：已闭合】改用 manifest 的 `previous` 单步存档钉住紧邻当前版本之前的那一行（版本号 / 字符总数 / 可消费汉字数 / `source_sha256` 前 12 位逐值比对），独立复现三种单元格变体改写、删除该行、重排记录行，`check` 均退出非 0；`README.md`「变更记录的静态校验范围」已如实交出可验证边界（更早版本行无真源、手工改写变更记录行无法被发现）。本轮另发现「改正文后仅改写变更记录行即沿用原版本号通过 `emit`/`check`」这一残留，按 `check` 非防篡改签名的既定定位写入同一节，措辞不再弱于实现。
- [x] [Review][Patch] 负例 3 的注释对其失败集合的描述不成立 [docs/contracts/canonical/tests/run_canonical_tests.py:223] — 已修复：原注释称「同步刷新 manifest 的源摘要，使失败只能来自步归属/消费序列，而不是源摘要」，实测该场景的失败信息还包含三端 `source_sha256` 不一致（3 条）与 `anchor_26` 不一致（1 条），「只能来自步归属」不成立；注释已改写为如实描述（该断言固定的仍是步归属，因为 `step`/`步` 文本只会由步归属比对产出，用例保护力不变）。修复后 `python3 docs/contracts/canonical/tests/run_canonical_tests.py` 退出码 0（17/17）。
- [x] [Review][Defer] 打包期 canonical 门禁没有自动验证，且在当前仓库状态下不可达 [`cloud/env-scripts/build-prod-backend.sh:104-117`] — deferred: `run_canonical_tests.py` 全文只以 `subprocess` 驱动 `scripture_tool.py`，不含任何 shell 脚本调用；全仓检索 `build-prod-backend` 只命中文档与 `upload-backend-jars.sh` 的提示文案；仓内无 CI 目录。删掉或改弱该门禁块不会让任何断言失败（`bash -n` 只是语法解析）。该门禁当前亦不可达：脚本在 `resolve_project_root`/pom 检查处（第 62/80 行）因缺 `cloud/backend/pom.xml` 提前退出（pom 由 Story 4.3 提供）。把打包脚本纳入自动验证需为其搭 java/mvn stub 沙箱，属新增范围，宜与 4.3 一并落地。
- [x] [Review][Defer] 同样打包并启动 `cloud/backend` 的本地入口 `start-local-test-backend.sh` 未接入同一条 fail-closed 门禁 [`cloud/env-scripts/start-local-test-backend.sh:149`、`:166`] — deferred: 该入口以 `mvn -DskipTests clean package` 产出同一个 `saas.jar`（内嵌本 Story 新增的 `canonical/heart-sutra.json`）并以 `java -jar` 启动，与已被门禁覆盖的生产脚本共享同一后端目录、同一 JDK/Maven 闸门；任何测试都未引用该脚本，故其不采用不会被任何断言标记。spec Task 4.1 只登记 `build-prod-backend.sh`，本地便捷入口是否也要 fail-closed 属产品决策，且该脚本同样在 pom 就位前跑不通。
- [x] [Review][Defer] 标点口径的冻结 UX 证据边界窄于 README 的表述 [`docs/contracts/canonical/heart-sutra.txt`、`docs/contracts/canonical/README.md:29-33`] — deferred: canonical 含 43 个 `，`，而 `Embedded/lvgl-design/ewf-device-ui-export.html` 全文恰 7 个 `，`（全部落在前两行 13 槽锚点内），`cloud/miniapp-design/ewf-miniapp-ui-export.html` 不含经文文本；manifest 所引来源页实测带标点并使用 `，；。！？` 与引号，故「其余 36 处标点的归并规则」是编辑决定，README 只声明了导出侧证据，未声明映射规则。汉字侧已逐字核对（非臆造），属需在 README/provenance 补充声明的透明度缺口（标点占槽、直接影响设备第 3 行起的 13 槽内容）。
- [x] [Review][Defer] 生成头 header-only 的 `static const` 形态使每个 include 的编译单元各持一份载荷副本 [`docs/contracts/canonical/generated/ewf_scripture_canonical.h`、`docs/contracts/canonical/scripture_tool.py:252,257`] — deferred: spec Dev Notes 的 C 头草图用的是 `extern` 声明 + 唯一实现，实现改为头内 `static const`（为满足「生成头必须可独立通过 `cc -fsyntax-only` 且符号可链接」而选）。实测 `static` 内部链接使每个 TU 各持一份（两份 TU 取地址不同；单 TU 目标文件 921 字节 rodata，含 909 B 正文字节 + 520 B 偏移表 + 字符串字面量）。正文与偏移表合计约 2.2 KB/TU 的重复量对设备可接受，但按 spec 草图应有单一定义点，且 `docs/embedded/style/C编码规范-Agent版.md` 要求头文件不定义对象。设备侧接入点是 Story 3.1（Epic 边界禁止本 Story 往 `Embedded/` 放承载 `.c`），故把「维持 header-only」与「另生成 `.c` 承载定义」的取舍留给 3.1 裁定；`README.md:14,90` 已显式记录 header-only 决策。
- [x] [Review][Defer] 交付目录内含未忽略的 `__pycache__/*.pyc`，会随新目录一并入库 [`docs/contracts/canonical/__pycache__/scripture_tool.cpython-312.pyc`] — deferred: `git check-ignore` 确认根 `.gitignore` 无 `__pycache__/` 规则（仅 `Embedded/.gitignore` 有），该文件不在本 Story 的 diff 与 File List 内，属编译缓存残留；修复应为仓库级忽略规则（`.gitignore` 属工作区基建，不在本 Story diff 范围）。

**Rejected**

- `false` — 「故事文档中的测试计数与实际交付套件不一致」（acceptance-auditor）：实测套件定义 17 个 `test_*`、输出 `17/17 通过`，而故事 Dev Agent Record 写「10/10」、Review Findings 写「13/13」，确为过期数字；但修复即编辑被审查的 spec，按 triage 规则一律驳回。
- `false` — 「`source_sha256` 的口径与实现不一致」（verification-gap）：实测文件字节（910 B）复算得 `833078ec3b79…`，去掉末尾换行后的正文（909 B）复算得 `a265c93dbc60…`（即 manifest 与三端产物登记的值）；但「源文本字节的 sha256」这一表述只存在于 spec 的 manifest 契约表（spec:206），交付的 `README.md` 并未定义该字段的口径，修正即编辑 spec，按规则驳回。工具口径与 spec 自身「换行只是文件级行结束、不是经文内容」的规则自洽。
- `false` — 「spec 负例 3 声称的失败机制与实测不符」（acceptance-auditor）：实测该变异下 `consumable_digest` 与 `counts.*` 确不改变，失败来自 `anchor_26`、三端 `source_sha256` 与步序列；但修复即编辑被审查的 spec，按规则驳回；其中可由本 Story 修改的部分（测试文件注释的描述失真）已作为上方 Patch 处理。
- `low` — manifest 的 `title` 未钉死在常量（edge-case-hunter）：`check_manifest_against_source()` 钉住了 `first_consumable` 与 `anchor_26`，但 `title` 只与自身比对，需同时手改 manifest 与前端/后端产物才能让错经题通过 `check`；属「手工伪造生成物」，日常流程不可达，修复需新增守卫。
- `low` — 生成的 TS/C 字符串字面量未转义（edge-case-hunter）：当前 canonical 只含汉字与 `，`，需先改正文才可达，而正文变更受 `emit` 守卫约束；修复需新增转义守卫。
- `low` — 偏移表按 `uint16_t` 输出但无上限校验（edge-case-hunter）：需源文本 UTF-8 超过 65535 字节（约 2.1 万汉字）才可达，而设备字形只覆盖本篇《心经》（260 字）；修复需新增上限守卫。
- `low` — `scripture_version` 的 `HS-<major>.<minor>.<patch>` 格式未校验（edge-case-hunter）：校验只作用于 README 记录行的版本号单元格，需先手工把 manifest 版本号改成畸形值并伪造记录行才可达；修复需新增正则守卫。
- `low` — `emit` 成功信息的句读歧义（本轮编排者独立复核发现，未由审查层提交）：`emit` 打印「非消费字符 {' '.join(...)}。」，单元素集合输出为「非消费字符 ，。」，末尾句号与集合元素视觉上难以区分；仅影响诊断可读性，修复为直接改写措辞。

> 审查结论：前轮 blocker 1 已真实闭合；blocker 2 的诚实路径已闭合，但新引入的「版本绑定守卫不可满足」使 AC 3 的版本变更流程不可执行，属已复现的 HIGH 且修复形态需人裁定，故本次不推进 `done`，状态保持 `review`。审查后 `python3 docs/contracts/canonical/tests/run_canonical_tests.py` 退出码 0（17/17），`scripture_tool.py check` 退出码 0，`emit` 幂等（真实产物副本上两轮 emit 字节未变）。active/mandatory 4 层：`blind-hunter`、`edge-case-hunter`、`verification-gap`、`acceptance-auditor`，全部成功，无 failed layer。

### attempt-4 审查（deep / full / autofix）

机器 receipt：`.autopilot/4/4-1-建立-canonical-心经-与版本校验/attempt-4/code-review.json`。

- **上两条 Decision 的闭合复核**：见上方两条的「attempt-4 复核」注记，均由独立沙箱复现确认，不再需要人裁定。
- **新发现（HIGH，已 autofix）**：`emit` 在「版本号已递增、记录行尚未追加」这一窗口内不幂等 —— `build_previous_view` 把磁盘 manifest 误当上一版，把本版本事实错记为上一版并覆盖真正的上一版存档。实测：该窗口内两次 `emit` 后 manifest 字节由 `91c26053…` 变为 `29995315…`，`previous.source_sha256` 由 `a265c93dbc60…` 被改写为新正文摘要；追加记录行后 `check` 退出 1，此后重复 `emit` 无法恢复（工具侧无出路）。修复：判定改用「磁盘 manifest 的 `source_sha256` 是否等于本次派生值」，并新增回归测试 `test_positive_emit_repeatable_before_ledger_row`（修复前失败、修复后通过）。
- **新发现（LOW，已 autofix）**：`README.md`「不在静态校验威胁模型内的部分」把手改的最小集合写成「同时改写 manifest 与变更记录两处」，实测仅改写当前版本记录行的数值单元格即可奏效；措辞已收敛为如实描述。
- **裁定（不构成 finding）**：manifest 新增 `previous` 字段而 `schema_version` 保持 `1` 是正确的 —— 本 schema 由本 Story 首次建立，从未发布过不含该字段的 v1，且 `canonical/heart-sutra.json` 尚无任何运行期消费者，`emit` 与 `check` 对 `schema_version` 的期望自洽（`SCHEMA_VERSION` 常量唯一）。
- **回归证据**：patch 后 `python3 docs/contracts/canonical/tests/run_canonical_tests.py` 退出码 0（24/24）、`scripture_tool.py check` 退出码 0；真实产物（manifest 与三端资源）在沙箱 `emit` 后与仓库逐字节一致，补丁不改动任何交付产物。
- **结论**：4 层（`blind-hunter`、`edge-case-hunter`、`verification-gap`、`acceptance-auditor`）全部成功，无 failed layer，无 unresolved HIGH/MEDIUM，状态推进 `done`。
