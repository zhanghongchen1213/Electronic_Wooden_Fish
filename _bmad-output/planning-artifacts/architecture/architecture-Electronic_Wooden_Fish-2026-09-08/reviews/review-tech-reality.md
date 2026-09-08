---
title: 技术现状核验评审 — Electronic_Wooden_Fish ARCHITECTURE-SPINE
lens: tech-reality-check
date: 2026-09-08
subject: _bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md
method: 本地基线文件证据（miaowu / legbot_watch / main_control）+ web 检索交叉核验
---

# 技术现状核验评审

评审口径：核验 spine 承诺的每一项技术决策是「经 web 或本地现实核对」，而非凭训练数据断言。每条结论分三态：**实核通过**（本地+web 双重可证）、**通过但需补出处**（事实正确但文档未内嵌可复核的出处/日期）、**存疑/需修订**（事实或表述与 2026-09-08 现实不符）。

总评一句：**所有版本锁档均有本地文件可证且 web 复核为真，无「凭训练数据断言」的硬错误；真正缺口是 spine 正文未内嵌外部生命周期断言的出处 URL 与核验日期、Deferred 中后端升级目标（3.5.x）已过时、以及删除根 architecture.md 前未移植硬件 primary-source 链接。**

---

## 一、逐条核验

### 1. ESP-IDF ≥5.5.4,<5.6.0 及其生命周期表述 —— 实核通过（正文缺出处）

- **Spine 承诺**：锁档 ≥5.5.4,<5.6.0；`v5.5 现处维护期、最新 5.5.5，v6.0 为现行 major`。
- **本地证据**：`legbot_watch/docs/hardware/ESPWatch-S3-4G_硬件软件二次开发文档.md` 记录「正式开发统一使用 ESP-IDF `5.5.x`，当前推荐 `5.5.4`」，并载明 5.5.0 仅是厂商示例 lock 历史、安装使用官方离线安装 5.5.4。spine 与 legbot 基线逐字对齐。
- **Web 证据（2026-09-08 检索）**：ESP-IDF v6.0 于 2026-03 发布为现行 stable major（v6.0.1 04-26、v6.0.2 06-28）；`release/5.5` 自 **2026-07 进入 maintenance**；5.5 线实际补丁序列 5.5.3(02-2026)/5.5.4(03-2026)/**5.5.5(07-2026)**；维护窗口延伸约至 2028-01。spine 表述与 web 现状**一致且精确**。
- **处置**：✅ 实核通过。两处提示：
  1. legbot 的 5.5.4 证据来自「Windows 离线安装器」文档，本机（macOS）的 toolchain 装配方式未被 spine 捕获——档位只锁了版本区间，未锁「怎么装」。可在 finalize 时补一行装配方式。
  2. 5.5 线在维护期内仍可能出 5.5.6+，区间 `<5.6.0` 已容纳，无需改动；但正文应将「维护期/最新补丁」的 web 出处与核验日期（2026-09-08）内嵌，否则该句会随时间无声腐烂（见 §二）。

### 2. Spring Boot 3.3.7 + 其 OSS EOL 表述 —— 事实正确；但 Deferred 升级目标已过时（存疑点）

- **Spine 承诺**：backend 沿用 miaowu pom 基线 Spring Boot 3.3.7 + Java 17；`3.3.x OSS 已于 2025-06 EOL`（Deferred A-4）；升级「受支持线（3.5.x/4.x）」。
- **本地证据**：`miaowu/backend/pom.xml` 父版本 = `spring-boot-starter-parent 3.3.7`、`java.version=17` —— 与 spine 逐字吻合。**关键补充发现**：spine AD-16 同时把「零库 JSON 原子写」做法指向 `legbot_watch/cloud`；而 `legbot_watch/cloud/backend/pom.xml` 是 **Spring Boot 3.5.16 + Java 17、无任何数据库驱动依赖**，其 `src/.../domain/DeviceStore.java` 用写锁 + `writeAtomically(...)` 落 `devices-local.json` —— 即「零库 JSON 持久化」在兄弟仓库是**真实存在且在 Spring Boot 3.5 线上跑的**。
- **Web 证据（2026-09-08 检索）**：Spring Boot 3.3 OSS EOL = **2025-06-30**（spine 表述正确；末补丁 3.3.13）；**3.5 OSS 已于 2026-06-30 EOL**；当前 OSS 受支持线只剩 **4.0（EOL 2026-12-31）与 4.1（当前 stable，4.1.1 @2026-08-20，EOL 2027-07-31）**。
- **存疑点**：Deferred A-4 写的升级目标「受支持线（3.5.x/4.x）」——**3.5.x 在 spine 落款日已是 OSS-EOL**。若按字面执行，会升到一个同样 EOL 的线。正确目标应为 **4.0.x/4.1.x**。
- **另一判断**：既然持久化已改为零库 JSON（偏离 miaowu 的 PostgreSQL 业务模型）、且零库做法的现实参照就是 `legbot_watch/cloud`（Spring Boot 3.5.16），那么 backend 框架锚点仍挂在 miaowu 3.3.7 上、与持久化参照分属两个仓库、两代 Spring Boot，**是同一决策内部的自相矛盾锚点**。更自洽的做法：backend 框架/持久化整体锚 `legbot_watch/cloud` 基线（3.5.16 + Java 17 + JSON 零库），仅沿用 miaowu 的 REST 信封/微信 JWT 模式；或保留 miaowu 3.3.7 但把「为什么不用 3.5/4.0」写成显式决策。
- **处置**：⚠️ 部分修订。3.3.7 锁档与「2025-06 EOL」表述 ✅ 实核通过（个人单设备、安全窗口非关注点，成立）；但 **Deferred 升级目标文字需从「3.5.x/4.x」改为「4.0.x/4.1.x」**，并建议把框架锚点与零库持久化参照统一到同一仓库/同一代框架上。

### 3. LVGL 8.4 —— 实核通过（legacy 锁档，上游已冻结）

- **Spine 承诺**：LVGL 8.4（沿用 legbot 锁档，不升 LVGL9）。
- **本地证据**：`legbot_watch/dependencies.lock` 锁定 `lvgl/lvgl 8.4.0`；`managed_components/lvgl__lvgl/lvgl.h` 宏 `LVGL_VERSION_MAJOR=8 / MINOR=4 / PATCH=0`；配套 `espressif/esp_lvgl_port` 依赖约束 `lvgl ^8` —— 组件级锁档自洽。注：`legbot_watch/components/ui/generated/*`（SquareLine Studio 生成）文件头注释为「LVGL version: 8.3.11」，是**生成工具的标注**，实际编译依赖为 8.4.0；二者兼容，非冲突，但值得知道 UI 生成侧用的是 8.3 语义。
- **Web 证据（2026-09-08 检索）**：LVGL 现行 major 为 **v9.5（2026-02-18 发布，支持期至 2027-02-18）**；v8.4 发布 2024-03-19、**支持期 2025-03-20 已结束**（v8 起每 minor 支持 1 年）。即 8.4 是**已冻结、不再有任何上游补丁**的旧线。
- **处置**：✅ 实核通过（作为 legbot 复用锁档，决策正确且现实可证）。提示：spine 宜明确写一句「LVGL 8.4 上游支持已于 2025-03 结束、无安全/修复补丁；个人离线 UI 可接受；升级 LVGL 9 = 需重做 legbot 生成 UI + 换 esp_lvgl_port」，把「不升」从默认省略变成显式知情决策。

### 4. uni-app：Vue 3 + TypeScript + Vite + Pinia，仅微信小程序 —— 实核通过

- **Spine 承诺**：沿用 miaowu frontend 的 uni-app（Vue 3 + TS + Vite + Pinia）骨架与 HBuilderX 调试，仅构建微信小程序。
- **本地证据**：`miaowu/frontend/package.json`：`@dcloudio/uni-app 3.0.0-4060620250520001`（2025-05-20 快照）、`@dcloudio/uni-mp-weixin`、`@dcloudio/vite-plugin-uni`、`vue ^3.5.13`、`typescript ~5.7.3`、`vite ^5.4.11`、`pinia ^2.3.1`、`vue-tsc`；scripts 含 `dev:mp-weixin` / `build:mp-weixin` 与 `type-check`。spine 描述与 package.json 逐项吻合。
- **Web 证据（2026-09-08 检索）**：2026 年经典版 uni-app（Vue3+Vite，编译为微信小程序原生 WXML/WXSS/JS）仍是「微信小程序 + Vue3 + TS」的**官方推荐且适用**栈；**uni-app x**（UTS/uvue 原生渲染）是与经典版**互不兼容的另一体系**，面向高性能 App/鸿蒙，本项目的微信小程序目标不需要它。选型方向正确。
- **处置**：✅ 实核通过（锁档本地可证）。提示：uni-app CLI 编译器锁在 2025-05-20 快照 + Vite 5.4（当前 Vite 已是 6/7 代）；经典 uni-app 的编译器/插件版本是**框架钉死**的、不可自由升 Vite，属正常锁档，但重装 node_modules 时应沿用 package-lock 以免编译器漂移；若日后换用更新的 HBuilderX 需回归验证。

### 5. Java 17 —— 实核通过（含 2026-09 免费支持悬崖提示）

- **Spine 承诺**：backend Java 17（沿用 miaowu pom）。
- **本地证据**：`miaowu/backend/pom.xml` 与 `legbot_watch/cloud/backend/pom.xml` 均为 `java.version=17` —— 兄弟仓库在 2026-09 仍以 17 为基线，锚点自洽。
- **Web 证据（2026-09-08 检索）**：Oracle JDK 17 Premier Support **2026-09-30 结束**（即 spine 落款日后约 3 周），Oracle 免费 NFTC 更新 2024-09 已截止；但免费发行版延续：Eclipse Temurin 17 至 ~2027-10、Amazon Corretto 17 至 2029-10。Spring Boot 4.1 最低要求仍是 Java 17，无被迫升级。
- **处置**：✅ 实核通过（个人原型 + 免费发行版场景安全）。提示：spine 或后端实现应确认**用哪个 JDK 发行版**（Temurin/Corretto 而非 Oracle NFTC），以避开 2026-09 免费支持悬崖的歧义。

### 6. JSON 零库持久化（AD-16 / A-2）—— 实核通过，且暴露「第二锚点」

- **本地证据**：`legbot_watch/cloud/backend` 无 DB 依赖，`data/devices-local.json` 存在，`DeviceStore.java` 以读写锁 + `writeAtomically(DeviceDataFile)` 落盘 —— spine 所述「零库 + 原子写(tmp+rename) + 单实例串行 + 重启可恢复」**在 legbot cloud 是真实实现**，可作参照物。✅
- **处置**：见 §一.2 的锚点统一建议。

---

## 二、「沿用本地基线 + [ASSUMPTION]」是否足以构成 reality-checked？

分两层回答：

- **对「锁档版本号」：足够。** miaowu 的 pom/package.json、legbot 的 dependencies.lock/lvgl.h/docs 都是具体、稳定、任何未来 builder 都能就地复核的文件；比训练记忆强得多。这部分不需要更强证据。spine 的 `[ASSUMPTION]` + Stack 表「经 web/本地复核」标注在锁档维度是诚实的。
- **对外部「生命周期断言」（EOL 日期、维护期、现行 major、最新补丁）：不足够。** 这些是**有时效、会腐烂**的外部事实，spine 正文只以陈述句呈现、**未内嵌 URL 与核验日期**；出处只在 `.memlog.md` 里以「2026-09-08 web 复核」一笔带过。本次核验证明这些断言**恰好正确**，但文档缺少让未来 builder **复核这些断言的机制**——若无人重查，`3.3 EOL`、`v6.0 现行`、`5.5.5 最新` 会在一年后变错而 spine 不自知。Spine 顶部自称「seed——草拟时经 web/本地复核」，在无内嵌出处的情况下属于**轻度过度宣称**。

**建议（低成本高收益）**：在 spine 增加一个 ≤10 行的「版本核验出处」小节，或把 Deferred 表每行 `[ASSUMPTION]` 附一行 `出处: <URL>（核验于 2026-09-08）`。出处锚定建议：

| 断言 | 建议锚定出处 |
| --- | --- |
| ESP-IDF v5.5 maintenance / v6.0 现行 / 补丁序列 | Espressif release notes：https://release-notes.espressif.com/ ；ROADMAP https://github.com/espressif/esp-idf/blob/master/ROADMAP.md |
| Spring Boot 各线 OSS EOL | https://endoflife.date/spring-boot |
| LVGL 版本支持政策 | https://lvgl.io/docs/open/introduction/policies |
| Java 发行版 EOL | https://endoflife.date/java |

另建议在「工程验证门禁」或 finalize 条件里加一条显式门禁：**finalize 前重跑本表核验一次并盖章日期**，使版本时效成为「门禁」而非「假设」。

---

## 三、删除根 architecture.md 后硬件事实可核验性意见

- **memlog 既定**：spine 定稿后删除根 `architecture.md`；其硬件/同步细节先并入 spine seed/AD 防信息丢失；PRD §0 等活跃引用改指新 spine。本评审不质疑「删」的意图（消除双权威冲突是合理的——根文档至今仍是 `status: final` 的「单一事实源」且 §8.3 写着 SQLite，与 spine AD-16 冲突）。
- **已具备的可复核保障（足够的部分）**：
  1. spine 内嵌 GPIO 板级合同表，并注明「来源：09-08 附录 §3.1」，而该附录（briefs/.../addendum.md §3.1）**仍存活**且内容一致；附录还自带冲突规则——「若目标模组数据手册或原理图审计证明冲突，必须先更新架构和决策日志再调整」。GPIO 事实有**双写 + 冲突门禁**，可复核。
  2. spine 保留了「工程验证门禁」（自述从旧根文档保真迁移），把烧录/总线/电源/联网/重启可恢复逐条列成验收项，是 future builder 可对照执行的复核闸口。
  3. 硬件细分项（上拉阻值、FPC 供电时序、电池/稳压料号、比较器料号）在 spine Deferred 中被明确为「只可在原理图/数据手册/样机验证后回填，不作产品承诺」——即把不可核验项挡在承诺之外。
- **缺口（会导致可核验性下降的部分）**：根 `architecture.md` §4.1/§4.2 是**唯一**携带硬件 primary-source 链接的地方（Espressif USB-Serial-JTAG 指南、内置 JTAG 配置指南、ESP32-S3 数据手册 PDF）。spine 迁移了**事实与门禁，却没有迁移这些出处 URL**。删除根文档后，「为什么这些 GPIO/为什么 USB-Serial-JTAG 可行/以哪份数据手册为准」的引用链会被切断；future builder 只能靠 legbot 文档间接反推，无法直接拿到权威出处。对纯功能事实影响小，对**电气/启动绑带这类必须查手册确认的事实**影响明显。
- **结论**：删除可接受，**但有一个前置条件**——在定稿删除前，把根文档 §4 的官方文档/数据手册 URL 清单移植进 spine（新开一个 ≤8 行的「硬件主要出处」小节，列出 Espressif 两篇指南、ESP32-S3 数据手册、及 CO5300/CST9217/CW2015/ES8311/NS4150B/Air780EGP 的供应商手册占位）。否则硬件事实虽由 spine+附录承载、可复核**冲突**，却不可复核**依据**。若不想改动 spine，退路是把根文档降级保留为 `docs/hardware-basis.md` 而不删除（成本最低）。

---

## 四、评审结论清单（处置一览）

| # | 承诺 | 处置 | 备注 |
| --- | --- | --- | --- |
| 1 | ESP-IDF ≥5.5.4,<5.6.0 / v5.5 维护期 / 5.5.5 / v6.0 现行 | ✅ 实核通过 | 正文补 URL+日期；5.5 线维护至约 2028-01 |
| 2 | Spring Boot 3.3.7 / 3.3 OSS EOL 2025-06 | ✅ 事实正确 | 个人原型成立；⚠️ Deferred「升级 3.5.x/4.x」须改「4.0.x/4.1.x」（3.5 已于 2026-06 EOL） |
| 2b | backend 框架锚 miaowu、零库持久化参照 legbot cloud | ⚠️ 锚点不一致 | legbot cloud = SB 3.5.16 + Java 17 + JSON 零库；建议整体锚 legbot cloud 或把不锚写成显式决策 |
| 3 | LVGL 8.4 | ✅ 实核通过（legacy 锁档） | 上游支持 2025-03 已止；把「不升」写成知情决策 |
| 4 | uni-app Vue3+TS+Vite+Pinia 仅微信小程序 | ✅ 实核通过 | 2026 经典版仍适用；uni-app x 是另一体系、不需要 |
| 5 | Java 17 | ✅ 实核通过 | 2026-09 Oracle Premier 截止；确认用 Temurin/Corretto 等免费发行版即可 |
| 6 | JSON 零库原子写 | ✅ 实核通过 | legbot cloud DeviceStore 真实存在 |
| 7 | `[ASSUMPTION]`+沿用本地基线 是否够 | ⚠️ 锁档够、生命周期断言不够 | 正文内嵌出处 URL + 核验日期；finalize 加重核门禁 |
| 8 | 删根 architecture.md 后硬件可复核 | ⚠️ 有条件通过 | 移植 §4 primary-source URL 清单后再删，或降级保留为 hardware-basis.md |

## 五、核验引用的主要 web 出处

- ESP-IDF release/support：https://release-notes.espressif.com/ ；https://www.espressif.com/en/news/ESP_IDF_6.0 ；https://github.com/espressif/esp-idf/blob/master/ROADMAP.md
- Spring Boot EOL：https://endoflife.date/spring-boot ；https://www.danvega.dev/blog/spring-boot-end-of-life
- LVGL 版本/政策：https://lvgl.io/docs/open/introduction/policies ；https://lvgl.io/docs/open/9.5/introduction/repo.html
- Java EOL：https://endoflife.date/java ；https://endoflife.ai/article-java-eol-by-vendor
- uni-app / uni-app x：https://github.com/dcloudio/uni-app ；https://deepwiki.com/dcloudio/uni-app
