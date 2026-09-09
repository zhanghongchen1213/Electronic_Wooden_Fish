---
stepsCompleted:
  - step-01-validate-prerequisites
  - step-02-design-epics
  - step-03-create-stories
  - step-04-final-validation
inputDocuments:
  - _bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md
  - _bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/addendum.md
  - _bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md
  - _bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/brief.md
  - _bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/addendum.md
---

# Electronic_Wooden_Fish - Epic Breakdown

## Overview

本文件把 PRD（含 FR/UJ/SM）与架构主干（AD 不变量、结构 seed、同步字段、板级合同）拆解为可被 Developer 代理逐条实现、带验收的史诗与用户故事。

**组织形态**：产品功能按 **嵌入式层 Embedded**（设备固件/硬件）与 **软件层 Software**（后端 `cloud/backend` + 小程序 `cloud/frontend`）两层组织（PRD §2、§5、§6）；epic 采用「两区 + 区内能力拆分」，区内按可交付能力划分，顶层按层分区。

**`layer` 标注取值**：`Embedded`（嵌入式层）· `Software·Backend` · `Software·Frontend`（软件层子层）· `Cross-layer`（跨层/端到端）。每个 epic 与 story 都必须带 `layer` 标注；FR 编号保留子层前缀（`FR-E-*` / `FR-B-*` / `FR-F-*`）以便追踪。

**权威与口径注记**：
- 后端持久化以架构主干 **AD-16（JSON 原子文件、零数据库，存 `cloud/backend/data/`）** 为唯一实现口径，**覆盖 PRD FR-B-009 早期「单文件 SQLite」决定**；PRD §2 分层图与 §6.1.9 已同步为 JSON 文件口径（见 PRD addendum §2 决策日志）。
- 本项目为个人原型：一台设备、一个固定 backend；MVP 固定《般若波罗蜜多心经》，不含 BLE/Wi-Fi/GPS 业务、多设备、OTA、选经、SQLite/PostgreSQL、CI/CD 与云运维体系。

## Requirements Inventory

### Functional Requirements

需求来源：PRD §5（嵌入式）、§6.1（后端）、§6.2（小程序）。描述沿用 PRD 行为口径；标注「→AD」处表示实现语义由架构主干不变量补强/覆盖。

#### 嵌入式层 Embedded（`FR-E-*`）

- **FR-E-001 — 电源、充电与复位**：单节锂电 + USB-C 充电 + PWR 硬件开关 + BOOT 与独立 RESET；重启后恢复已持久化的累计数与轮次；充电期间暂停敲击、经文推进与音频，但允许显示充电、电量与同步状态。（→AD-13）
- **FR-E-002 — PVDF 与低功耗唤醒**：PVDF 为唯一 MVP 实体输入；低功耗比较器输出唤醒，ESP32 ADC 二次确认；QMI8658C 上板但默认不启用，仅接 INT1，不参与 MVP 计数。（→AD-1）
- **FR-E-003 — 统一有效敲击队列**：`physical_pvdf` 与亮屏木鱼页 `device_touch` 进入**同一个**有效敲击队列；支持 1 秒内 1–20 次；充电中、完成遮罩中、故障锁定中直接忽略。（→AD-1/19）
- **FR-E-004 — 本地高水位与离线积压**：持久化 `local_total`、`acked_total`、`scripture_version`、`round_state`、`round_cursor` 与待同步标记；离线积压上限 1000；达到上限拒绝新输入并明确提示。（→AD-3/15）
- **FR-E-005 — 核心音频反馈**：ES8311 + NS4150B + 扬声器输出木鱼音；默认音量 50/100，0 等价静音；高速连击合并为可辨识节奏；音频失败不阻塞计数、持久化与同步；音量跨重启保留。（→AD-12）
- **FR-E-006 — 设备屏幕与三主页**：AMOLED 提供木鱼页、经文页、统计页与下滑设置页；屏幕四周保留圆弧，顶部左侧用可复用组件显示 4G/Wi-Fi/蓝牙/GPS 的 connected/no-signal/disabled 状态，右侧显示电池符号+百分比。木鱼页固定 7 字带：只呈现最近已诵内容、空位低对比度占位、**不预览未来经文**、新字右入旧字左移、标点随相邻字同一步出现但不消耗敲击；同时显示心经进度字数/百分比、今日敲击与累计敲击。经文页大号当前字 + 少量已确认前后文 + 心经进度；统计页仅今日与累计。未校时显示「待校时」；熄屏实体敲击短暂亮屏。7 字带容量不等于诵读字数上限，末字达到 100% 后完成锁定。（→AD-2/6/19）
- **FR-E-007 — 触摸与 PWR 导航**：CST9217 支持左右滑三主页、下滑设置、设置控件、完成弹窗与木鱼页点击；仅木鱼页点击产生 `device_touch`，其他页触摸不计数；熄屏首次触摸只唤醒；PWR 短按唤醒或按木鱼→经文→统计循环三页，长按不由固件模拟关机；设置页提供三档亮度（默认中）、5/15/30 秒自动熄屏（默认 15 秒）、0–100 音量（默认 50）与立即同步，均跨重启保留。（→AD-10/15）
- **FR-E-008 — Air780EGP 4G 活动窗口**：MVP 仅 Air780EGP 4G HTTPS JSON；设备在敲击、立即同步或待应用命令窗口内复用通信上下文；不配置 BLE/Wi-Fi 产品链路；GPS 保留控制能力但默认关闭；本 FR 上报环节不得单独突破 SM-3 端到端口径；弱网本地继续记录、恢复后补传。（→AD-6/14）
- **FR-E-009 — 状态与低打扰反馈**：通过 CW2015、RGB、AMOLED 与音频表达电量、4G、同步、待应用、低电量、有效敲击与故障状态；低打扰、不打断诵经节奏。（→AD-12/13）
- **FR-E-010 — USB-Serial-JTAG**：ESP32-S3 原生 USB-Serial-JTAG 完成烧录、CDC 日志与 JTAG 调试；不使用 CH340X，不启用 USB-OTG 业务。（→AD-11）

#### 软件层 · 后端 backend（`FR-B-*`）

- **FR-B-001 — 单身份单设备**：将唯一微信身份映射到一台固定设备；MVP 无多设备、解绑、换机迁移与公开账号治理。（→AD-16）
- **FR-B-002 — 高水位幂等同步**：接收 `local_total` 与设备状态，按设备身份比较 `acked_total` 与本次高水位；重复/更低提交不重复计数；返回最新确认高水位、差量与同步状态。（→AD-2/3）
- **FR-B-003 — 经文游标与轮次**：维护《心经》文本版本、`round_cursor`、`round_state` 与完成锁定；只按已确认差量推进正式游标；标点、空格、换行不消耗敲击；**backend 不以经文长度对差值盲推游标/完成锁定**，单轮推进只在设备本地发生、backend 以 `round_id`/游标边界校验并确认回传。（→AD-3/19）
- **FR-B-004 — 固定经文版本**：backend、嵌入式层、frontend 使用同一版固定《心经》可消费汉字序列与 `scripture_version`；版本不一致时停止推进并返回配置错误；不做自动映射。（→AD-4）
- **FR-B-005 — 完成、从头开始与退出**：末字确认后进入完成锁定；「从头开始」创建新轮次并重置游标；「退出」保留完成状态与历史统计；锁定期间任何输入不计数。（→AD-3/19）
- **FR-B-006 — 状态推送与断线补齐**：通过 WebSocket 推送差量、当前字、游标、轮次、完成状态、同步状态与设备状态；frontend 断线后按**快照水位 S** 补齐，不重复已展示字符；WebSocket 与查询接口在同一后端状态下结果一致。（→AD-17）
- **FR-B-007 — 命令修订与待设备应用**：保存音量、亮度、熄屏、立即同步与篇章命令的最新 `command_revision`；设备离线时命令进入待设备应用；设备下一次 HTTPS 活动取得后按修订号应用并回传已应用修订；**旧命令不得覆盖新命令；`command_revision` 语义 = 设备已应用命令的单调高水位，不依赖单次 ACK 到达**。（→AD-5）
- **FR-B-008 — 历史统计与可信时间**：提供今日、近 7 日、近 30 日、累计与连续天数；离线差量按 backend 确认日期归档；连续天数以当天存在已确认敲击计算；设备无可信时间时不伪造当天统计；日界按 backend 配置时区切分。（→AD-6）
- **FR-B-009 — JSON 文件持久化（零数据库）**：以单实例单进程运行，用 JSON 文件原子落盘（临时文件 + fsync + rename）保存高水位、游标、轮次、完成状态、命令、设备状态与统计；数据落 `cloud/backend/data/`，重启全恢复；**不引入任何数据库**。（→AD-16，覆盖早期 SQLite 口径）

#### 软件层 · 小程序 frontend（`FR-F-*`）

- **FR-F-001 — 微信登录与固定设备入口**：登录后直接进入唯一设备；无绑定、迁移与多设备治理 UI。（→AD-16/18）
- **FR-F-002 — 经文呈现页**：小程序不显示电子木鱼、不产生正式敲击；连续正文 append backend 已确认内容，已有前缀不覆盖、不清空，当前字聚焦，顺序不越权、不预览未来，并显示心经总进度字数/百分比。（→AD-2/17/19）
- **FR-F-003 — 在线逐字动画**：每个 backend 已确认汉字按顺序追加一次；已有正文保持可见；动画落后时动态加速，不跳过、合并或重复字符；渲染环节不单独突破 SM-3 端到端 1 秒口径。（→AD-17/19）
- **FR-F-004 — 离线积压回放**：重开后按 backend 差量顺序回放；回放期间新事件排队，结束后无缝接入实时动画。（→AD-17）
- **FR-F-005 — 完成反馈**：收到 backend 完成确认后显示礼花与「从头开始 / 退出」；待确认不提前声称完成；弹窗期间不展示新推进。（→AD-17/19）
- **FR-F-006 — 记录页**：展示今日、近 7 日、近 30 日、累计与连续天数；全部来自 backend 已确认数据，本地待同步数据不混入正式统计。（→AD-6/17）
- **FR-F-007 — 设备页**：展示电量/充电、4G、最后同步、待同步数量、待设备应用与失败状态；提供刷新状态、立即同步与重试入口。（→AD-5/17）
- **FR-F-008 — 设置镜像**：可修改音量、亮度、熄屏时长与篇章命令；设备离线时显示待设备应用而非已生效；收到设备已应用确认后刷新最终状态。（→AD-5）
- **FR-F-009 — 断线恢复**：WebSocket 断开时切换到查询/回放；重连后只补齐未展示的 backend 差量（序号 > 快照水位 S），不重复已展示字符。（→AD-17）
- **FR-F-010 — 空态与失败态**：首次登录、无历史、离线、同步失败、队列已满、低电量、待校时、待设备应用、篇章完成与权限错误均有明确 UI；本地待同步数据不得伪装成正式进度。（→AD-2/17）

> 说明：PRD §6 明确当前不存在 backend 与 frontend 共同承担义务的软件层横切 FR；跨子层协作通过分属两端的 story + 同一契约面互相引用表达，不发明 `FR-S-*`。

### NonFunctional Requirements / Success Metrics

需求来源：PRD §10、§11、§2 层间责任。**验收方式**：个人原型 MVP 阶段不冻结对外承诺的统计门限；SM 与 FR 中的「明显 / 大量 / 可辨识」等主观量词，由创作者在目标外壳样机上逐条演示签收判定。

- **SM-1 — 可靠输入**：单次与 1 秒 20 次连续输入不出现明显漏记、重记或乱序。（绑定 E2/S0）
- **SM-2 — 动画连续性**：木鱼页 7 字带与小程序的阅读行动画不丢正式字符、不重复播放。（绑定 E3/S3）
- **SM-3 — 端到端实时性**：稳定 4G 网络下，95% 有效敲击在其发生 1 秒内被小程序展示。端到端口径：设备敲击 → backend 确认 → frontend 渲染；任何环节不得单独突破该口径。（绑定 E4/S1/S2/S3/S0）
- **SM-4 — 离线恢复**：设备离线累计、重启、重复提交与网络恢复后，高水位、游标、轮次与统计最终一致。（绑定 E2/E4/S1/S0）
- **SM-5 — 持久化**：backend 重启后高水位、游标、配置与历史统计不丢失（AD-16 JSON 原子文件）。（绑定 S1）
- **SM-6 — 个人使用**：创作者完成原型后愿意持续使用（列为首轮样机后的回看项，非工程内门禁）。（绑定全体体验）
- **SM-7 — 一天续航**：约 30 分钟/天实际敲击、其余时间待机的使用模型下，设备至少支撑完整一天；最终以样机功耗实测为准。（绑定 E4/E1 功耗）
- **SM-C1**：不以同时常开 BLE/Wi-Fi/4G、堆叠音效或增加功能数量换取「在线感」，避免牺牲续航与核心诵经节奏。（贯穿约束）

### Additional Requirements

需求来源：架构主干 ARCHITECTURE-SPINE（AD-1~19、结构 seed、同步字段、板级合同、Deferred 前置）+ PRD §11/§8 + 09-08 简报附录（参考）。

#### 跨层不变量（AD-1~19 一览，逐条影响实现；权威规则以架构主干为准）

| AD | 一句话规则 | 主要绑定 |
| --- | --- | --- |
| AD-1 | 统一正式输入源 `physical_pvdf`/`device_touch`，无第二计数路径；除统一队列外任何代码不得推进正式累计 | E2、S1 |
| AD-2 | backend 是已确认进度/游标/轮次/完成/统计/命令的唯一权威；小程序只呈现已确认内容；设备可展示本地镜像但须带非正式同步标记 | S1、S3、S4 |
| AD-3 | 幂等高水位同步、无逐事件重放；轮次只由设备本地推进、backend 校验确认并回传轮次状态 | E2、E4、S1、S0 |
| AD-4 | 固定经文版本、单一文本源、构建期一致、版本不一致即停 | E2/E3、S1、S3、S0 |
| AD-5 | 命令以单调「已应用修订」高水位收敛；离线进入待设备应用 | E4、S1、S2、S4 |
| AD-6 | 可信时间门禁：日统计以 backend 配置时区为权威；设备无可信日期显示「待校时」 | E3、E4、S1、S4 |
| AD-7 | 服务任务事件驱动固件范式：ISR 只投递事件，服务间不得同步直调 | E1~E4 |
| AD-8 | BSP 独占板级资源、统一封装硬件驱动；新芯片目录登记进 CMake | E1 |
| AD-9 | `device_state` 是嵌入式产品事实源；服务只读不可变快照 | E2/E3/E4 |
| AD-10 | 共享资源单一仲裁（共享 I²C、Air780EGP UART、LVGL 单一 UI 任务） | E3、E4 |
| AD-11 | USB-Serial-JTAG 为唯一调试通道；GPIO43/44 让给 Air780EGP UART | E1、E4 |
| AD-12 | 反馈/外设失败不阻塞核心链路；高速连击合并音效；动画队列不丢正式字符 | E2/E3/E4 |
| AD-13 | 电源边界：分轨供电、低电优先持久化、充电暂停输入 | E1、E2、E4 |
| AD-14 | 唯一产品联网链路是 Air780EGP 4G HTTPS；BLE/Wi-Fi/GPS 不入业务 | E4 |
| AD-15 | 设置跨重启保留；计数以本地高水位持久化；设备不保存绝对敲击时间 | E2/E3、S0 |
| AD-16 | backend 沿用 miaowu REST 骨架；持久化 = JSON 原子文件、零数据库（存 `cloud/backend/data/`）；接口 `/api/v1` + `{code,message,data}` 信封；覆盖 FR-B-009 早期 SQLite 决定 | S1 |
| AD-17 | frontend 只消费已确认差量：WebSocket 推送 + 快照水位 S 补齐；帧格式/心跳下沉 sync-contract | S1/S2/S3/S4、S0 |
| AD-18 | 前端技术基线：uni-app（Vue 3 + TS + Vite + Pinia）仅微信小程序；单一 `VITE_API_BASE_URL` 与统一 `api/request` | S3/S4 |
| AD-19 | 经文消费/展示语义：一敲一字、标点随附不耗敲击、不预览未来；跨轮次以 `round_id` 区分 | E2/E3、S1、S3、S0 |

#### 前置契约与工程项（影响 epic/story 落地顺序）

- **sync-contract.md 冻结为前置**（架构 Deferred，耦合 AD-3/19）：backend 与 Embedded 各自的模块 spec/实现**拆分前**，须先经 `docs/contracts/sync-contract.md` 冻结：同步字段与状态词表、`round_id` 跨轮归属、未确认完成的「从头开始」跨轮竞态、确认回传字段、快照水位 S/消息序号、JSON schema 与文件粒度（`[ASSUMPTION A-2]`）、WebSocket 帧/心跳/重连参数。→ 落 S0，先于 E2/E4/S1/S3 实现。
- **固定《心经》canonical 单源**：单一落盘来源、三端构建/打包期嵌入并校验 `scripture_version`；设备内置字形仅覆盖该部经文。（AD-4）→ S0 + S1 + S3 + E3
- **跨层同步字段清单**：`device_id` · `local_total` · `acked_total` · `scripture_version` · `round_id` · `round_state` · `round_cursor` · `command_revision` · `battery_percent` · `network_mode` · `audio_config_version` · `firmware_version`；事件来源 `physical_pvdf`、`device_touch`；同步状态词表固定（本地已记录/同步中/已同步/待同步/同步失败；待设备应用为命令独立维度）。（PRD §7 + spine §Structural Seed）
- **板级 GPIO 合同**（spine 默认合同，冲突须先改 spine 再调整）：BOOT IO0 / PWR IO46 / RESET EN / PVDF ADC IO9 / PVDF 唤醒 IO11 / 共享 I²C IO1·IO2（CW2015·CST9217·ES8311·QMI8658C）/ CST9217 IO38·IO39 / QMI8658C INT1 IO41（INT2 不接、不占 IO45）/ CO5300 IO4·40·5·6·7·12·42·47 / ES8311 IO13·14·17·18·21 / NS4150B PA_EN IO48 / Air780EGP UART IO43·44、DTR IO10、RST IO15、NET_STATUS IO16、GNSS_VCC IO8 / RGB DATA IO3 / USB D−·D+ IO19·IO20。启动绑带：下载要求 GPIO0=0、GPIO46=0。
- **结构目录 seed**：`Embedded/`（ESP-IDF：components/BSP + platform + services + app_state + main）、`cloud/backend/`（Spring Boot 单进程 jar + `data/` JSON 状态文件）、`cloud/frontend/`（uni-app 微信小程序）；本仓库三个目录已存在但为空，属绿地实现起点。
- **技术栈基线**：ESP-IDF ≥5.5.4,<5.6.0 + LVGL 8.4（沿用 legbot）；Spring Boot 3.3.7 + Java 17 + Maven（沿用 miaowu `backend/pom.xml`）；uni-app Vue 3 + TS + Vite + Pinia（沿用 miaowu `frontend/package.json`）；本地脚本沿用 miaowu `env-scripts`。版本出处/EOL 与升级时机见 spine Deferred，不在此重复承诺。
- **复用与不沿用**：复用 `legbot_watch`（CO5300/CST9217/CW2015/QMI8658C/ES8311/NS4150B BSP、共享 I²C、LVGL 主页/状态栏/下滑设置/亮屏时序）与 `main_control`（Air780EGP UART/AT、DTR 休眠唤醒、PDP、HTTPS JSON、退避、GPS 开关）；不照搬其引脚常量、ML307R/BLE/外骨骼业务、QMI8658C INT2→IO45 合同、四主页与产品 payload。
- **工程验证门禁**（PRD §11 + spine）：GPIO0/45/46 绑带与 USB GPIO19/20 无冲突；I²C 扫描四器件地址；PVDF 单次与 1 秒 20 次计数与完成遮罩忽略；重复 HTTPS/离线恢复/backend 重启/小程序回放与 WebSocket 重连不重复不错序；Air780EGP 发射峰值不掉压；无可信日期不伪造今日统计；命令离线显示待设备应用。
- **非目标（不进入任何 story）**：BLE/Wi-Fi 业务链路、GPS 业务、自动 OTA、多设备/换机迁移、公开账号/社交/排行榜/提醒/付费、多平台前端、选经/导入、SQLite/PostgreSQL、量产认证/供应链/云运维。（PRD §8/§9）

#### 设备端 UX 参考（来自 09-08 简报附录 §5，作实现参考，非独立 UX-DR）

- 通用导航：主页左右滑动（木鱼→经文→统计），下滑进设置、返回保持原页；顶部状态栏常驻电量/充电、4G、同步/待同步；PWR 短按熄屏唤醒/亮屏按三页循环，长按不软件关机。
- 木鱼页：电子木鱼为唯一可计数触摸区；7 字形位置，**只展示已诵内容、未填充位低对比度占位、不预览未来经文**；标点随附同一步插入且正式计数只 +1；动画积压时缩短位移时间、清空后恢复节奏；同时显示心经进度字数/百分比、今日敲击与累计敲击，同步未确认用同步中/待同步标记。
- 经文页：大号当前字 + 少量已确认前后文 + 心经总进度；MVP 无选经入口。统计页：今日敲击 + 累计敲击。
- 设置页默认值：三档亮度默认中、5/15/30 秒熄屏默认 15、音量 0–100 默认 50、立即同步（进行中/成功/待同步/失败）。
- 输入/反馈：熄屏实体敲击直接计数并短亮屏；熄屏首触只唤醒；USB-C 充电暂停实体/触摸输入、经文推进与音频；RGB 只表达敲击确认/待同步/低电量/故障，低打扰。
- 完成：末字确认后设备与小程序显示完成反馈与「从头开始 / 退出」；遮罩期间所有输入忽略；从头开始新建轮次、退出保留完成态与历史。

### UX Design Requirements

本项目的 UX 契约位于 `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/`，由 `DESIGN.md`（视觉）与 `EXPERIENCE.md`（行为）及两份 `UI_CONTRACT` 共同维护。当前仍处于 Stage 3 候选返工，视觉 style 尚未由作者签收；设备/小程序只能在签收后进入全帧与实现，不另发明新的 UX-DR 清单。

### FR Coverage Map

每个 FR 归主 epic（单一所有权）。S0 只绑定以下 FR 的**契约接口面**（先行定稿字段/状态/修订语义，不占实现性所有权），实现性验收归各自主 epic。

```
FR-E-001: Epic E1 - 电源/充电/复位与重启恢复（power 生命周期 API）
FR-E-002: Epic E2 - PVDF 唤醒 + ADC 确认（后置 tuning story）
FR-E-003: Epic E2 - 统一有效敲击队列（physical_pvdf + device_touch）
FR-E-004: Epic E2 - 本地高水位 local_total/acked_total + 离线积压
FR-E-005: Epic E3 - 核心音频反馈（ES8311+NS4150B）
FR-E-006: Epic E3 - AMOLED 三主页/7字带（含经文页契约、反剧透占位、待校时）
FR-E-007: Epic E3 - 触摸导航/电子木鱼点击/PWR 循环/设置持久化
FR-E-008: Epic E4 - Air780EGP 4G 活动窗口同步
FR-E-009: Epic E3 - 状态与低打扰反馈（CW2015/RGB/电量/故障）
FR-E-010: Epic E1 - 原生 USB-Serial-JTAG 调试通道

FR-B-001: Epic S1 - 单身份单设备映射
FR-B-002: Epic S1 - 高水位幂等同步（无逐事件重放）
FR-B-003: Epic S1 - 经文游标/轮次（backend 校验确认、不盲推）
FR-B-004: Epic S1 - 固定经文版本一致性（不一致即停）
FR-B-005: Epic S1 - 完成/从头开始/退出
FR-B-006: Epic S2 - WebSocket 推送 + 快照水位 S 断线补齐
FR-B-007: Epic S2 - 命令修订待设备应用（已应用修订高水位翻转）
FR-B-008: Epic S1 - 历史统计与可信时间归档
FR-B-009: Epic S1 - JSON 文件持久化（AD-16 零库，覆盖 SQLite 口径）

FR-F-001: Epic S3 - 微信登录直达唯一设备
FR-F-002: Epic S3 - 经文阅读行呈现（无木鱼入口、不预览未来）
FR-F-003: Epic S3 - 在线逐字动画（不跳/合/重）
FR-F-004: Epic S3 - 离线积压回放接实时
FR-F-005: Epic S3 - 完成反馈（礼花 + 从头开始/退出）
FR-F-006: Epic S4 - 记录页统计
FR-F-007: Epic S4 - 设备页状态/刷新/重试/立即同步
FR-F-008: Epic S4 - 设置镜像（待设备应用/已生效刷新）
FR-F-009: Epic S4 - 断线恢复（快照水位补齐）
FR-F-010: Epic S4 - 空态与失败态
```

**SM 验收归属（防指标孤儿）**：SM-1→E2；SM-2→E3＋S3；**SM-3（端到端 1 秒）两段式**→① S2 联调（设备模拟器）完成软件段通道预算**预演**（非终验）；② E4/S3 后真机 E2E 演示签收 story（样机门禁，**终验**）；SM-4→S1（模拟器预演）+ E4/S3 真机联调；SM-5→S1；SM-6→首轮样机回看；SM-7→E4 功耗 story（样机实测）。

## Epic List

> **组织**：嵌入式层 Embedded（E1–E4）与软件层 Software（S0–S4）两区。**读取顺序 ≠ 构建顺序**。
>
> **建议交付顺序（P6）**：先 S0（Foundation，冻结同步 + frontend API 表面 + canonical 经文产物 + 设备模拟器）→ 之后两条 lane **并行**：Embedded lane **E1 → E2 → E3 → E4**（同一固件仓**顺序**交付，只增补、不重写前序已实现功能 —— P7 纪律）；Software lane **S1 → S2**（S3/S4 可随 S1 的 API 契约先行 build-on-mock）。
>
> **跨层验收两段式（P5，防桩测假绿）**：① **预演**——S1/S2 以 S0 **设备模拟器**、S3/S4 以冻结 **API 契约** mock 做独立验收（软件段 SM-3 通道预算预演，非终验）；② **真机签收**——样机门禁下 E4/S3 的真机 E2E 演示 story 完成 SM-3/SM-4 终验。
>
> 设备模拟器按 sync-contract 发 tap 流/高水位同步包（P2）；frontend 消费的 REST/WS API 形状在 S0 定稿（P1），供 S3/S4 造 mock。

### Epic E1 · 设备平台底座与调试通道就绪（Embedded）

- **layer**：`Embedded`
- **目标（用户结果）**：样机能上电、烧录、读日志、JTAG 调试；充电/硬开关/复位行为正确；重启后累计与轮次恢复。是一切后续能力的物理与工程前提（Enabler）。
- **FR 覆盖**：`FR-E-010`、`FR-E-001`
- **绑定 AD**：AD-8（BSP 独占板级资源）、AD-11（USB-Serial-JTAG 唯一调试通道）、AD-13（分轨供电/电源边界）
- **实现要点**：首 story = Embedded 工程骨架（components/BSP + platform + services + app_state + main，CMake 纳管，AD-8）；板级 GPIO 合同 bring-up（IO 绑带、I²C 扫描、分轨供电）；**E1 只出「电源状态事件」**（充电中/低电/关机），运行期输入门控与低电优先落盘分别由 E2/E4 消费，不重复实现。
- **首个 Story**：Embedded 工程骨架 + USB-Serial-JTAG 冒烟（能烧录、能看日志、能 JTAG）。

#### Story E1.1 · 建立 Embedded 工程骨架（BSP 独占 + 分层目录）

- **layer**：`Embedded` ｜ **绑定**：FR-E-010（前提）、AD-8
- **As a** 设备固件开发者，**I want** 一份可编译、可空跑的 ESP-IDF 分层工程骨架，**So that** 后续 BSP/服务/界面 story 都能在统一结构和 CMake 纳管下落地。

**Acceptance Criteria:**
- Given 仓库为空，When 按结构 seed 初始化 `Embedded/`（`components/BSP` + `platform` + `services` + `app_state` + `main`），Then `idf.py -DIDF_TARGET=esp32s3 build` 成功且能烧录启动进入主循环。
- Given 骨架含 BSP 组件模板与 app_state 空快照，When 任一服务注册，Then 能经 typed event 收发，且 BSP 内新增芯片目录未登记进 CMake 时构建失败。
- Given 工程含统一日志约定（模块 TAG 用 ASCII、正文中文），When 启动，Then 各模块日志按 TAG 清晰可辨。

#### Story E1.2 · 原生 USB-Serial-JTAG 烧录/日志/JTAG 通道

- **layer**：`Embedded` ｜ **绑定**：FR-E-010、AD-11
- **As a** 设备固件开发者，**I want** 用 ESP32-S3 原生 USB-Serial-JTAG 完成烧录、CDC 日志与 JTAG 调试，**So that** 无需 CH340X/OTG 即可可靠迭代固件。

**Acceptance Criteria:**
- Given 样机通过 GPIO19/20 USB 连接 PC，When `idf.py flash monitor`，Then PC 识别 USB CDC，日志持续输出，烧录成功。
- Given 需要断点调试，When 用 OpenOCD 连接内置 JTAG，Then 可设断点/单步/读寄存器。
- Given USB 处于连接态，When 触发调试会话，Then 自动 light sleep 被禁用（防串口失联）；工程不含 CH340X 与 USB-OTG 业务代码。

#### Story E1.3 · 电源/充电/复位板级语义与电源状态事件

- **layer**：`Embedded` ｜ **绑定**：FR-E-001、AD-13
- **As a** 诵经者，**I want** 设备电源行为符合硬件合同（PWR/BOOT/RESET、充电识别、长按关机归板级），**So that** 不会因固件误关机或复位异常而丢节奏。

**Acceptance Criteria:**
- Given 固件运行中，When 读到 PWR(IO46)/BOOT(IO0)/EN 输入，Then 固件只读 PWR、不模拟软件关机；长按 PWR 不触发固件关机（由板级电源电路处理）。
- Given USB-C 插入充电，When 充电状态变化，Then 固件发布 `charging` 电源状态事件供上层消费，且不打断已持久化状态。
- Given 需要进入下载模式，When 操作 BOOT+RESET，Then 能完成下载；启动绑带 GPIO0/45/46 采样不被 PWR/QMI8658C 输出推错电平。

#### Story E1.4 · 板级总线与 GPIO 合同 bring-up 冒烟

- **layer**：`Embedded` ｜ **绑定**：AD-8、AD-10、工程验证门禁
- **As a** 设备固件开发者，**I want** 上电对共享 I²C 与 GPIO 布局做一次冒烟扫描，**So that** 在写驱动前确认板级合同与目标物料无冲突。

**Acceptance Criteria:**
- Given 样机上电，When 执行共享 I²C(IO1/IO2) 扫描，Then 列出 CW2015/CST9217/ES8311/QMI8658C 实际地址，无重复冲突。
- Given 板级合同已登记，When 比对 GPIO 分配表，Then 无引脚复用冲突（Air780EGP UART IO43/44、USB IO19/20、比较器 IO11 等互不侵占）。
- Given 某器件地址与合同不符，When 冒烟扫描完成，Then 以日志标记偏差，并提示按「先更新架构主干再调整」流程处理。

#### Story E1.5 · 本地持久化基元与启动恢复

- **layer**：`Embedded` ｜ **绑定**：FR-E-001（恢复前提）、AD-15
- **As a** 诵经者，**I want** 固件具备可靠的本地持久化基元与启动恢复能力，**So that** 重启/断电后进度不丢，后续计数/轮次/设置可跨重启保留。

**Acceptance Criteria:**
- Given 固件含持久化基元（NVS/分区、typed key 读写、启动时装载「最近一次持久化快照」），When 写入一测试键后硬断电重启，Then 值被完整恢复。
- Given 恢复完成，When 对比启动时刻，Then 以单一 `device_state` 快照方式装载，无部分写损坏（写入采用原子/校验策略）。
- Given 尚无任何计数写入（E2 前空态），When 启动，Then 进程正常运行并持有初始轮次/游标占位，不因键缺失崩溃。

### Epic E2 · 拿起就敲：统一敲击与本地计数闭环（Embedded）

- **layer**：`Embedded`
- **目标（用户结果）**：无论实体 PVDF 还是屏幕电子木鱼点击，都进入同一有效敲击队列、一敲一字；1 秒 20 次不丢不乱；掉电/重启/离线继续可敲，本地高水位与积压上限正确。
- **FR 覆盖**：`FR-E-002`、`FR-E-003`、`FR-E-004`
- **绑定 AD**：AD-1（统一输入源/无第二计数路径）、AD-3（本地高水位）、AD-15（跨重启持久化/不存绝对时间）、AD-19（推进语义）
- **实现要点**：**故事顺序 = 注入 seam 先行、UI 无关计数、PVDF 后置**——首 story 先建**与 UI 无关的输入事件注入 seam**（P3：`device_touch`/`physical_pvdf` 统一入口，供测试注入与 E3 真触摸接入同一条路），再让统一队列 + `local_total` 本地持久化闭环在样机桌面跑通；`physical_pvdf` 唤醒/ADC 确认、20cps 与外壳演示按工程门禁拆为后置 tuning story，风险前移。输入门控（充电中/完成遮罩中/故障锁定中忽略）消费 E1 的电源状态事件与完成态。
- **首个 Story**：输入事件注入 seam + `device_touch` 统一队列 + `local_total` 持久化（桌面可验证，UI 无关）。

#### Story E2.1 · 输入事件注入 seam（UI 无关的统一事件源）

- **layer**：`Embedded` ｜ **绑定**：FR-E-002/003 前提、AD-1
- **As a** 诵经者，**I want** 实体与触摸输入先归一为同一事件源接口，**So that** 计数逻辑可在木鱼页 UI（E3）存在前独立开发与验证。

**Acceptance Criteria:**
- Given 输入 seam 提供 `physical_pvdf` 与 `device_touch` 两个 typed 事件入口，When 从测试注入任一来源事件，Then 进入同一个有效敲击队列，后续计数/经文/持久化路径完全一致。
- Given seam 处于 UI 无关态，When 未接真实触摸/传感器，Then 队列仍可被测试驱动（无 UI 依赖）。
- Given 除该队列外存在任何代码路径，When 试图推进正式累计，Then 被拒绝（AD-1：无第二计数路径）。

#### Story E2.2 · device_touch 统一队列 + local_total 本地持久化

- **layer**：`Embedded` ｜ **绑定**：FR-E-003、FR-E-004、AD-1/15/19
- **As a** 诵经者，**I want** 一次点击推进一次本地累计并落盘，**So that** 桌面样机上即可验证「一敲 +1、断电不丢」的计数正确性。

**Acceptance Criteria:**
- Given 木鱼页点击事件由 seam 注入，When 触发一次有效敲击，Then `local_total` +1 并同步持久化；同一事件不重复计数。
- Given 连续注入 1 秒 20 次，When 计数完成，Then 20 次全部计入，无明显漏/重（计数层先验证；音频/UI 表现由 E3 承担）。
- Given 计数落盘后，When 硬断电重启，Then `local_total` 恢复，与重启前一致。

#### Story E2.3 · 输入门控：充电/故障锁定忽略 + 熄屏首触不计数

- **layer**：`Embedded` ｜ **绑定**：FR-E-003、FR-E-007、AD-1
- **As a** 诵经者，**I want** 在充电中、故障锁定中不误计数、熄屏首次触摸只唤醒，**So that** 不会产生伪敲击。

**Acceptance Criteria:**
- Given 处于充电状态（E1 `charging` 事件），When 注入实体/触摸事件，Then 直接忽略，不推进累计、不推进经文。
- Given 处于故障锁定或上层「输入抑制」标记（预留完成遮罩复用的同一接口），When 注入输入，Then 被忽略。
- Given 屏幕熄灭，When 第一次触摸到达，Then 仅唤醒不计数；亮屏后下一次木鱼页点击才计为 `device_touch`。

#### Story E2.4 · 本地高水位与离线积压上限

- **layer**：`Embedded` ｜ **绑定**：FR-E-004、AD-3/15
- **As a** 诵经者，**I want** 设备维护 `local_total` 与已确认 `acked_total` 双高水位并标记待同步，**So that** 离线期间可继续敲，恢复后只补传差值。

**Acceptance Criteria:**
- Given 设备离线敲击，When 累计增长，Then 持久化待同步标记与差值（离线积压上限 1000）。
- Given 积压已达 1000，When 再来一次有效输入，Then 拒绝新输入并给出明确提示，不越界累积。
- Given 本地已记录，When 尚无可信网络时间，Then 不伪造任何绝对时间戳归档（时间相关留 backend，AD-6）。

#### Story E2.5 · physical_pvdf 唤醒 + ADC 二次确认

- **layer**：`Embedded` ｜ **绑定**：FR-E-002、AD-1
- **As a** 诵经者，**I want** 实体 PVDF 敲击经比较器唤醒、ADC 二次确认后才成为正式输入，**So that** 环境振动不会误触发、真敲击不丢。

**Acceptance Criteria:**
- Given PVDF 产生候选信号，When 比较器输出(IO11)唤醒系统，Then ESP32 醒后由 ADC(IO9) 二次确认，确认通过才投递 `physical_pvdf` 事件。
- Given 二次确认期间 ADC 输入越界/超阈值，When 判定非有效敲击，Then 丢弃且不推进计数。
- Given QMI8658C 上板，When MVP 计数流程运行，Then 其不参与敲击判定（仅 INT1 预留未来扩展）。

#### Story E2.6 · 20 次/秒与外壳误触的样机演示签收门禁

- **layer**：`Embedded` ｜ **绑定**：FR-E-002/003 验收、SM-1、工程验证门禁
- **As a** 诵经者，**I want** 在目标外壳与安装结构下做最终输入可靠性签收，**So that** 「一敲一字、1 秒 20 次不丢不乱、携带不误触发」在真实形态下成立。

**Acceptance Criteria:**
- Given 目标外壳样机，When 连续多组 1 秒 20 次实体敲击，Then 计数无肉眼可见漏/重/乱序（以作者演示签收为准）。
- Given 携带/取放/普通环境振动，When 无主动敲击，Then 误触发率在可接受范围（演示签收判定）。
- Given 20cps 输入可靠性在真机不达标，When 按残余风险收敛，Then 以明确节流上限替代并冻结对应 MVP 边界（不静默扩范围）。

#### Story E2.7 · 本地经文游标与轮次引擎（一敲一字推进）

- **layer**：`Embedded` ｜ **绑定**：FR-E-003/FR-E-006 推进语义、FR-B-003 设备侧前提、AD-3/4/15/19
- **As a** 诵经者，**I want** 每次有效敲击在设备本地推进下一个可消费汉字并维护轮次游标，**So that** 断网时「一敲一字」也成立，轮次完成可被检测并在联网后上报。

**Acceptance Criteria:**
- Given 一次有效敲击进入统一队列，When 引擎消费，Then 推进到 canonical 序列的下一个可消费汉字；标点/空格/换行随相邻字同一步出现、不消耗敲击，正式计数只 +1（AD-19）。
- Given 引擎使本轮末位可消费汉字被诵出，When 完成判定，Then `round_state` 置完成并进入本地完成锁定（后续输入忽略），供 E3 遮罩展示与 E4 上报。
- Given 经文来自 S0 单一落盘 canonical 产物，When 构建嵌入，Then 校验 `scripture_version` 与契约一致，不一致则停止推进并报配置错误（AD-4）。
- Given 任一游标/轮次变化，When 持久化 `round_id`/`round_state`/`round_cursor`，Then 断电重启后恢复并继续，无跨轮分裂（AD-15）。

### Epic E3 · 设备诵经体验：屏幕/触摸/声音/灯与设置（Embedded）

- **layer**：`Embedded`
- **目标（用户结果）**：三主页 + 7 字木鱼动画（不预览未来/占位低对比/标点随附）+ 木鱼音 + RGB + 下滑设置页；充电暂停、完成遮罩、熄屏首触、PWR 循环等交互规则正确，设置跨重启保留。
- **FR 覆盖**：`FR-E-005`、`FR-E-006`、`FR-E-007`、`FR-E-009`
- **绑定 AD**：AD-2（本地镜像须带非正式同步标记）、AD-6（无可信日期显「待校时」）、AD-9（device_state 事实源）、AD-10（LVGL 单一 UI 任务）、AD-12（反馈失败不阻塞）、AD-15、AD-19
- **实现要点**：单 UI 任务内多 story 顺序落地；设备 UX 先完成 10 种禅意风格候选与作者选择门禁，再把选中风格复制到全帧闭包。木鱼页使用“心经进度字数/百分比 + 今日敲击 + 累计敲击 + 7 字窗口”，经文页使用已确认前后文与总进度；设置默认值（音量 50/中亮度/15s 熄屏）写死入 UI 首帧；固件构建期嵌入 S0 canonical 经文并校验 `scripture_version` 一致（P4，配合 FR-B-004/AD-4）。
- **首个 Story**：三主页框架 + 顶部状态栏 + 下滑设置（LVGL，屏幕冒烟）。

#### Story E3.1 · LVGL 三主页框架 + 顶部状态栏 + 下滑设置骨架

- **layer**：`Embedded` ｜ **绑定**：FR-E-006、FR-E-007（导航）、AD-10
- **As a** 诵经者，**I want** 屏幕具备三主页 + 常驻状态栏 + 下滑设置的整体框架，**So that** 各页面内容能在统一 UI 任务下逐步填入。

**Acceptance Criteria:**
- Given 屏幕点亮，When 初始化 LVGL，Then 木鱼页/经文页/统计页三主页可左右滑动，下滑进入设置页，返回后保持原主页。
- Given 页面切换，When 任意主页可见，Then 顶部左侧常驻显示 4G/Wi-Fi/蓝牙/GPS 四个可复用信号组件（connected/no-signal/disabled），顶部右侧显示电池符号+百分比及充电状态；图标状态不改变 MVP 只有 4G 业务链路的边界。
- Given 多页面同时存在，When 发生刷新/触摸，Then UI 更新只发生在单一 LVGL 任务边界，无并发绘制冲突（AD-10）。
- Given `gGgAm` statusbar 组件被复用，When 任意设备页面加载，Then 组件子节点均位于圆弧屏安全区内，不发生顶部裁切或左右溢出。

#### Story E3.2 · 木鱼页 7 字滚动带（引擎驱动 + 反剧透占位）

- **layer**：`Embedded` ｜ **绑定**：FR-E-006、AD-2/19
- **As a** 诵经者，**I want** 木鱼页以固定 7 字带呈现最近已诵内容并显示心经总进度，**So that** 我能连续诵读且不把 7 字窗口误解为字数上限。

**Acceptance Criteria:**
- Given 木鱼页可见，When 引擎（E2.7）推进一字，Then 新字自右侧进入、旧字左移、最左退出，7 位固定；未填充位以低对比度占位，绝不预览未来文字。
- Given 相邻字之间出现标点，When 推进发生，Then 标点随同一步自动出现，不消耗敲击（正式计数只 +1）。
- Given 高速连击积压，When 动画追不上，Then 缩短位移时间追平、不丢正式字符，清空后恢复正常节奏（SM-2 设备侧）。
- Given 本地有未确认差值或尚未同步，When 木鱼页展示心经进度/今日敲击/累计敲击，Then 以「同步中/待同步」等非正式状态显式标记（AD-2），本地镜像不冒充已确认；7 字带满位后继续左移，不触发完成。

#### Story E3.3 · 电子木鱼点击接入 seam + 触摸导航/PWR 循环

- **layer**：`Embedded` ｜ **绑定**：FR-E-007、AD-1/10
- **As a** 诵经者，**I want** 木鱼页的电子木鱼点击成为真 `device_touch` 正式输入，其它页触摸不计数，PWR 负责唤醒与循环，**So that** 触摸语义与实体完全一致且不误触。

**Acceptance Criteria:**
- Given 亮屏且位于木鱼页，When 点击电子木鱼区域，Then 经 E2.1 seam 投递 `device_touch`，计入累计/经文/音频/同步，与实体敲击同路径（AD-1）。
- Given 位于经文页/统计页/设置页，When 任意触摸，Then 不产生任何正式敲击。
- Given 屏幕熄灭，When 首次触摸，Then 仅唤醒不计数（E2.3 门控）；PWR 短按在亮屏时按木鱼→经文→统计顺序循环，持续按住不触发固件关机。

#### Story E3.4 · 经文页内容契约（大号当前字 + 已确认前后文 + 心经进度）

- **layer**：`Embedded` ｜ **绑定**：FR-E-006（经文页）、AD-19、09-08 附录 §5.3
- **As a** 诵经者，**I want** 经文页以大号当前字配少量已确认前后文与百分比进度呈现，**So that** 诵经时能看清当前心经进度而不被打断。

**Acceptance Criteria:**
- Given 经文页可见，When 引擎游标变化，Then 大号显示当前可消费字，前/后展示少量已诵/已临近内容（不预览未诵未来字）。
- Given 页面加载，When 渲染，Then 显示 `round_consumed / scripture_chars_total · percent%`；MVP 不出现选经/导入入口。
- Given 经文数据，When 嵌入，Then 来自 S0 canonical 产物且构建期校验 `scripture_version` 一致（AD-4）。

#### Story E3.5 · 统计页：今日/累计 + 「待校时」

- **layer**：`Embedded` ｜ **绑定**：FR-E-006/009、AD-6
- **As a** 诵经者，**I want** 设备统计页并列显示今日敲击与累计敲击，未校时时显「待校时」，**So that** 今日与跨诵读周期的累计口径清楚。

**Acceptance Criteria:**
- Given 统计页可见，When 渲染，Then 并列显示“今日敲击 N 次”和“累计敲击 M 次”（近 7/30 日与连续天数不在设备端，归小程序记录页）；累计跨所有 `round_id` 保留。
- Given 设备尚无可信时间（硬断电未校时），When 显示今日，Then 呈现「待校时」，不把未知日期的敲击错归当天（AD-6）。
- Given 本地存在未同步差值，When 显示今日计数，Then 带非正式同步标记，不冒充 backend 已确认统计。

#### Story E3.6 · 核心音频反馈（ES8311 + NS4150B）

- **layer**：`Embedded` ｜ **绑定**：FR-E-005、AD-12/10
- **As a** 诵经者，**I want** 每次有效敲击伴随可辨识的木鱼音、高速时自动合并，**So that** 声音强化诵经连续感而不叠加成噪声。

**Acceptance Criteria:**
- Given 一次有效敲击，When 触发反馈，Then 输出一次短木鱼音；音量默认 50/100，0 等价静音。
- Given 1 秒 20 次连击，When 音频播放，Then 按短间隔重触发或合并为可辨识节奏，不允许 20 个完整样本无控制叠加（FR-E-005/SM-1 听感演示签收）。
- Given 音频设备初始化/播放失败，When 敲击继续，Then 计数、持久化、同步不受阻塞；静音/暂停/故障时 NS4150B PA 回到禁用（AD-12/13）。

#### Story E3.7 · 设置页 + 设置跨重启持久化 + 立即同步

- **layer**：`Embedded` ｜ **绑定**：FR-E-007、AD-10/15
- **As a** 诵经者，**I want** 在设置页调整亮度/熄屏/音量并发起立即同步，**So that** 显示与声音符合个人偏好且重启不丢失。

**Acceptance Criteria:**
- Given 设置页可见，When 调整亮度（低/中/高，默认中档）、自动熄屏（5/15/30 秒，默认 15）、音量（0–100，默认 50），Then 对应 UI/音频立即生效并持久化，重启后保留（AD-15）。
- Given 用户触发「立即同步」，When 投递同步请求事件，Then 页面以同步状态事件呈现进行中/待同步/失败（不虚构成功）；在 4G 传输（E4）就绪前如实显示待同步/未连接，成功反馈随 E4 落地（无前向依赖）。
- Given 同步/设置动作与本地诵经并发，When 互不阻塞，Then 设置保存不打断计数与动画节奏。

#### Story E3.8 · RGB 与电量低打扰状态反馈（CW2015）

- **layer**：`Embedded` ｜ **绑定**：FR-E-009、AD-12/13
- **As a** 诵经者，**I want** RGB 与电量通道以克制方式表达敲击/同步/电量/故障，**So that** 状态可知但不打断诵经。

**Acceptance Criteria:**
- Given 状态变化，When 触发 RGB，Then 用克制的颜色与占空比表达：敲击确认、待同步、低电量、故障（不与主屏争夺注意力）。
- Given 电量计 CW2015 就绪，When 轮询电量，Then 状态栏右侧以电池符号+百分比展示电量并在充电/低电量时明确低打扰提示。
- Given 故障发生，When 状态机进入故障态，Then 有明确低打扰呈现并保持输入锁定语义（E2.3）。

#### Story E3.9 · 完成遮罩与全交互规则（从头开始/退出）

- **layer**：`Embedded` ｜ **绑定**：FR-E-003/005/007、FR-B-005（设备侧）、AD-2/19
- **As a** 诵经者，**I want** 《心经》完成后设备进入完成遮罩并可选择从头开始或退出，**So that** 完成态明确、历史保留、新轮次干净开始。

**Acceptance Criteria:**
- Given 引擎（E2.7）判定当前诵读完成，When 显示完成遮罩，Then 进度保持 100%/N，遮罩期间所有实体/触摸输入被忽略（复用 E2.3 输入抑制），不增加本地或云端计数。
- Given 用户选择「从头开始」，When 确认，Then 创建新轮次（`round_id` 递增）、游标回到首字，历史统计保留（未确认完成的跨轮竞态语义按 S0 sync-contract 处理）。
- Given 用户选择「退出」，When 确认，Then 保留完成状态与历史，离开当前诵经呈现。
- Given 充电中/首触/PWR 等边界，When 复核交互，Then 均不与完成遮罩语义冲突。

### Epic E4 · 设备 4G 同步与低功耗续航（Embedded）

- **layer**：`Embedded`
- **目标（用户结果）**：敲击后设备在活动窗口把高水位同步到 backend，响应里带回已应用命令与确认轮次；弱网/断网本地继续、恢复补传；Air780EGP 发射峰值不掉压，满足一天续航；BLE/Wi-Fi/GPS 不入业务。
- **FR 覆盖**：`FR-E-008`
- **绑定 AD**：AD-3（幂等上报）、AD-6（可信时间取信）、AD-13（低电优先落盘）、AD-14（唯一 4G HTTPS 链路）
- **实现要点**：低电优先落盘在发送前保证累计持久化；活动窗口复用 PDP/HTTPS 上下文；**Cross-layer 联调 story 收尾**：真实设备→backend 上报/命令应用的 1 秒预算与命令待应用翻转验证（配合 S0 契约）；含**真机 E2E 演示签收 story**（样机门禁，SM-3/SM-4 终验，P5）。
- **首个 Story**：Air780EGP AT/网络注册/PDP/HTTPS 单事务窗口（沿用 main_control 经验）。

#### Story E4.1 · Air780EGP 4G 活动窗口（AT/PDP/HTTPS 单事务）

- **layer**：`Embedded` ｜ **绑定**：FR-E-008、AD-14
- **As a** 诵经者，**I want** 设备只在敲击/立即同步/待应用命令触发的活动窗口内联网，**So that** 不维持长连接、省电且不依赖 BLE/Wi-Fi。

**Acceptance Criteria:**
- Given 触发条件发生（有效敲击、立即同步、有待应用命令），When 进入活动窗口，Then 通过 Air780EGP AT 完成网络注册/PDP 并复用 HTTPS JSON 上下文上报，结束后回低功耗，不维持长连接。
- Given 活动窗口未触发，When 系统空闲，Then 不发起任何网络注册/连接（BLE/Wi-Fi 固件不配置为产品通道，GPS 保持默认关闭）。
- Given 无实体敲击也无同步需求，When 长时间待机，Then 不因联网产生不必要功耗（SM-7 前提）。

#### Story E4.2 · 高水位幂等上报包与命令/确认回传应用

- **layer**：`Embedded` ｜ **绑定**：FR-E-008、FR-B-007（设备侧）、AD-3/5/6
- **As a** 诵经者，**I want** 活动窗口按 sync-contract 上报高水位与轮次并消费响应，**So that** backend 幂等对账、命令与可信时间回到设备。

**Acceptance Criteria:**
- Given 一次上报，When 组装 payload，Then 含 `local_total`/`acked_total`、`round_id`/`round_state`/`round_cursor`、已应用命令修订 `command_revision`、电量/网络/固件状态（按 S0 sync-contract 字段），不传绝对敲击时间（AD-6/15）。
- Given 收到 backend 响应，When 解析，Then 更新本地已确认 `acked_total`/差量，并回传确认后的轮次状态收敛本地镜像；同时应用响应中的待应用命令（按修订号），并取用可信时间校时。
- Given 重复或乱序的上报，When backend no-op，Then 本地不重复计数、不越界回退（幂等，配合 S1）。

#### Story E4.3 · 弱网/断网本地继续与恢复补传

- **layer**：`Embedded` ｜ **绑定**：FR-E-008、FR-E-004、AD-3/14
- **As a** 诵经者，**I want** 弱网或断网时继续本地敲击，网络恢复后在活动窗口按差值补传，**So that** 离线不丢、恢复不重。

**Acceptance Criteria:**
- Given 4G 不可用或小程序关闭，When 敲击持续，Then 本地照常累计/显示/音频，状态栏显示待同步，无 BLE/Wi-Fi 回退通道。
- Given 网络恢复，When 进入补传活动窗口，Then 只上传高于已确认的累计高水位与最新轮次，由 backend 按差值推进一次（不依赖逐事件重放）。
- Given 补传后仍待同步，When 观察状态，Then 直到 backend 确认后才清除待同步标记。

#### Story E4.4 · 低电优先落盘 + 4G 功耗窗口（一天续航）

- **layer**：`Embedded` ｜ **绑定**：FR-E-001/009（边界）、AD-13、SM-7
- **As a** 诵经者，**I want** 低电时先保证累计落盘、4G 发射不引发掉压，**So that** 断电/低电不丢记录、样机满足一天续航。

**Acceptance Criteria:**
- Given 电量进入低电，When 需要落盘或发网，Then 先保证累计与轮次持久化完成，再执行其它（不主动断网做危险降级）。
- Given Air780EGP 发射峰值，When 电池轨供电，Then 不发生掉压重启（工程门禁实测；若掉压则缩短活动窗口或调整电源/模组选型，收敛 MVP 边界）。
- Given 约 30 分钟/天敲击 + 其余待机的使用模型，When 实测功耗，Then 设备至少支撑完整一天（以样机功耗实测为准，SM-7）。

#### Story E4.5 · 真机 E2E 演示签收（SM-3/SM-4 终验）

- **layer**：`Cross-layer` ｜ **绑定**：SM-3/SM-4 终验、FR-E-008、FR-B-007、AD-3/5/6
- **As a** 诵经者，**I want** 在真机上完成端到端演示签收，**So that** SM-3/SM-4 与命令待应用翻转以真实闭环确认（非模拟器预演）。

**Acceptance Criteria:**
- Given 真机 + backend + 小程序在线，When 连续敲击，Then 稳定网络下 ≥95% 有效敲击在其发生 1 秒内被小程序展示（SM-3 终验，端到端口径，不达标则记录各段耗时并按残余风险收敛「在线实时」承诺）。
- Given 断网累计 + 重启 + 重复提交 + 网络恢复，When 观察收敛，Then 高水位/游标/轮次/统计最终一致（SM-4）。
- Given 小程序下发设置后设备离线/休眠，When 下一次活动窗口设备取得命令，Then 设备按修订号应用并回报已应用修订，backend 将「待设备应用」翻转为「已生效」、小程序刷新最终状态（AD-5）。
- Given 硬断电后未校时即敲击，When 恢复联网校时，Then 设备端「今日」正确显示「待校时」直到取得可信时间，不伪造当天统计（AD-6）。

### Epic S0 · 跨层同步契约与共同基线冻结（Foundation，前置 · Cross-layer）

- **layer**：`Cross-layer`
- **目标（用户结果）**：零用户价值，但为「设备↔backend↔小程序」提供共同不变量基线，含三类冻结物：① **同步数据契约**——同步字段/状态词表、`round_id`、`command_revision`=已应用高水位、快照水位 S/消息序号、JSON schema 与文件粒度（ASSUMPTION A-2）、WS 帧/心跳；② **frontend-facing API 表面（P1）**——REST 路径、`{code,message,data}` 信封与错误码表、登录→单设备映射、WS 帧格式；③ **canonical 经文产物（P4）**——《心经》可消费字符序列 + `scripture_version` 单一落盘源。另产出**设备模拟器（P2）**。
- **FR 覆盖**：契约接口面 `FR-B-002~007`、`FR-E-004`（实现性 FR 归主 epic）
- **绑定 AD**：AD-1/2/3/4/5/6/19；产出 `docs/contracts/sync-contract.md`
- **实现要点**：第一个落地物；冻结后 Embedded 线与 Software 线即可并行。跨轮竞态（未确认完成的从头开始）、确认回传字段、双文件原子写顺序均在此收口；API 表面与经文产物同时作为 S3/S4 mock 与三端嵌入校验的基准确认。
- **关键 Stories（示意）**：① 同步字段/状态词表/`round_id`/`command_revision` 定稿；② frontend-facing REST/WS API 契约与错误码表定稿；③ canonical《心经》字符序列 + `scripture_version` 产物生成；④ 设备模拟器（按 sync-contract 发 tap 流/高水位包）。
- **首个 Story**：同步字段/状态词表/`round_id`/`command_revision` 语义定稿入 sync-contract.md。

#### Story S0.1 · 同步数据契约冻结（sync-contract.md）

- **layer**：`Cross-layer` ｜ **绑定**：AD-1/2/3/5/6、FR-B-002~007 / FR-E-004 接口面
- **As a** 各端实现者，**I want** 一份冻结的跨层同步数据契约，**So that** backend/固件/小程序各自实现不自相矛盾、不互相发明协议。

**Acceptance Criteria:**
- Given 契约文档 `docs/contracts/sync-contract.md` 建立，When 编写，Then 定义：同步字段清单与类型/权威归属、同步状态词表（本地已记录/同步中/已同步/待同步/同步失败；待设备应用为命令独立维度）、事件来源 `physical_pvdf`/`device_touch`、`round_id` 跨轮归属语义、`command_revision`=「设备已应用命令的单调高水位」、确认回传字段（含确认后轮次状态与可信时间）、快照水位 S 与单调消息序号。
- Given 契约涉及 JSON 持久化，When 定稿，Then 明确 schema、文件粒度与**双文件原子写顺序**规则（先推进后归档/先归档后推进的崩溃安全），防止重复归档或丢批（ASSUMPTION A-2）。
- Given 契约涉及 WebSocket，When 定稿，Then 定义帧格式/心跳/断线重连参数。
- Given 契约冻结，When 三端模块实现开始，Then 以本文件为唯一实现依据，任何改动须先更新本契约再调整（Deferred 关闭）。

#### Story S0.2 · frontend-facing API 表面契约定稿（P1）

- **layer**：`Cross-layer` ｜ **绑定**：AD-16/17/18、FR-F-001~010 消费面
- **As a** 前端开发代理，**I want** backend 暴露的 REST/WebSocket API 形状在 S0 定稿，**So that** S3/S4 可据此造 mock 独立开发，不依赖 S1 时序。

**Acceptance Criteria:**
- Given API 契约文档，When 编写，Then 定义 REST 路径（登录→设备、状态查询、统计、命令等）、`{code,message,data}` 信封与错误码表、请求/响应示例。
- Given 登录映射，When 定稿，Then 明确微信登录→唯一设备身份映射契约（openid/JWT 语义，MVP 单身份单设备）。
- Given 实时通道，When 定稿，Then 定义 WS 帧格式/序号/心跳（与 S0.1 一致）及断线补齐基准 = 查询响应内冻结的快照水位 S。
- Given 契约可用，When S3/S4 mock 构建，Then 仅依据本文件即可搭建，无未定义接口依赖。

#### Story S0.3 · canonical《心经》字符序列产物 + 版本戳（P4）

- **layer**：`Cross-layer` ｜ **绑定**：AD-4、FR-B-004、FR-E-006 / FR-F-002/003 文本面
- **As a** 三端实现者，**I want** 《心经》可消费字符序列以单一落盘产物 + `scripture_version` 交付，**So that** 无各端誊写漂移、构建期可校验一致。

**Acceptance Criteria:**
- Given canonical 文本源，When 生成，Then 产出统一「可消费汉字序列」产物（含标点随附/换行语义，不含任何预览文本）并打 `scripture_version` 版本戳。
- Given 产物落盘，When 三端嵌入，Then 固件（E3.2/3.4）、backend（S1）、小程序（S3）均从此源生成/嵌入并校验版本一致；任一不一致时停止推进并返回配置错误（AD-4）。
- Given MVP 固定此经，When 变更经文，Then 走版本升级流程，不做运行期自动映射、不选经。

#### Story S0.4 · 设备模拟器工具（P2）

- **layer**：`Cross-layer` ｜ **绑定**：AD-3/5/6、SM-3/4 预演
- **As a** 后端与前端开发代理，**I want** 一台按 sync-contract 行事的「设备模拟器」，**So that** 软件线在硬件到货前即可做端到端预演与桩验收。

**Acceptance Criteria:**
- Given 模拟器工具就绪，When 运行，Then 能按契约发出 tap 流/高水位同步包（含轮次游标、已应用命令修订、电量等字段），可配节奏（单次/慢速/1 秒 20 次/离线积压突发）。
- Given 模拟器行为，When 面向 backend，Then 可模拟离线后补传、重复/乱序提交、命令待应用回报，覆盖 S1/S2 幂等与翻转验收场景。
- Given 预演目的，When 驱动 S2 联调，Then 完成软件段 SM-3 通道预算**预演**（明确非终验，终验归 E4.5 真机签收）。

### Epic S1 · 云端权威：backend 幂等/游标/统计/JSON 持久化（Software·Backend）

- **layer**：`Software·Backend`
- **目标（用户结果）**：任何设备上报只按高水位幂等推进一次、游标/轮次/完成语义正确、历史统计按可信时间归档、backend 重启后 JSON 文件全恢复；无 SQLite/PostgreSQL 依赖。
- **FR 覆盖**：`FR-B-001`、`FR-B-002`、`FR-B-003`、`FR-B-004`、`FR-B-005`、`FR-B-008`、`FR-B-009`
- **绑定 AD**：AD-2（唯一权威）、AD-3、AD-4、AD-6、AD-16（JSON 零库 + `/api/v1` 信封）
- **实现要点**：首 story = backend 工程骨架（Maven + Spring Boot 3.3.7 + `/api/v1` + `{code,message,data}` 信封 + `data/` 目录 + health）；JSON 原子写（tmp+fsync+rename）；**统一以 S0 设备模拟器作为测试桩**做幂等/游标/完成验收（P2）；微信登录→单设备身份沿用 miaowu WechatMiniClient/JWT 模式；增加「构建期嵌入 canonical 经文并校验 `scripture_version`」story（引用 S0，P4）。
- **首个 Story**：backend 骨架 + JSON 零库存储基元 + 信封/health。

#### Story S1.1 · backend 工程骨架 + API 信封 + JSON 原子写基元

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-009（基元）、AD-16
- **As a** 后端开发代理，**I want** 一个基于 miaowu 骨架、可本地起停的 backend 工程，**So that** 后续接口与持久化 story 有统一基线。

**Acceptance Criteria:**
- Given 空 `cloud/backend/`，When 初始化，Then 建立 Maven + Spring Boot 3.3.7（锚 miaowu `pom.xml`）工程，接口统一 `/api/v1` + `{code,message,data}` 信封（code=0 成功），提供 health 端点与 `data/` 状态目录。
- Given 本地运行，When 使用 env-scripts，Then 可按 profile=local 一键起停/构建。
- Given JSON 写路径，When 实现存储基元，Then 采用临时文件 + fsync + rename 原子写，任何中断不产生半写文件。
- Given 工程形态，When 观察，Then 单实例单进程、零数据库依赖（不引入 SQLite/PostgreSQL）。

#### Story S1.2 · 状态域 JSON 持久化与重启恢复

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-009、AD-16、SM-5
- **As a** 诵经者，**I want** backend 的高水位/游标/轮次/完成/命令/统计落 JSON 文件且重启全恢复，**So that** 服务重启不丢任何已确认状态。

**Acceptance Criteria:**
- Given 后端状态域划分，When 写入，Then 覆盖：ack 高水位、经文游标/轮次、完成状态、设备命令、日统计，均原子落盘至 `cloud/backend/data/`。
- Given 进程重启，When 重新加载，Then 高水位、游标、轮次、配置与历史统计全部恢复（SM-5）。
- Given 写盘与崩溃并发，When 复现中断，Then 不产生损坏文件（原子 rename + 恢复上次完好版本）。
- Given 无外部数据库，When 检查部署依赖，Then 只有本进程与文件系统，无其它服务依赖。

#### Story S1.3 · 单身份单设备映射

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-001、AD-16
- **As a** 作者（唯一微信身份），**I want** 登录即绑定我的固定设备，**So that** 无需任何账号/多设备治理即可使用。

**Acceptance Criteria:**
- Given 微信登录请求，When 换取 openid/JWT（沿用 miaowu WechatMiniClient/JWT 模式），Then 映射到固定 `device_id`，同一身份始终同一设备。
- Given 不同身份，When 上报，Then 高水位/游标/统计互相隔离，不串用。
- Given MVP 边界，When 检查入口，Then 无多设备/解绑/换机迁移/公开账号治理 API 或 UI。

#### Story S1.4 · 高水位幂等同步

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-002、AD-2/3
- **As a** 诵经者，**I want** 设备上报只按累计高水位幂等推进一次，**So that** 重复/乱序提交不会重复计数。

**Acceptance Criteria:**
- Given 设备同步请求（`local_total`/`acked_total`/轮次/状态/已应用命令修订），When 处理，Then 按 `device_id` 比较 `acked_total` 与本次高水位，只推进高于已确认的差值一次；同值或更低为 no-op。
- Given 重复提交，When 多次收到相同或更低高水位，Then 计数不再增加，并返回最新确认高水位、差量与同步状态供设备/前端对账。
- Given 响应，When 回传，Then 含确认后 `round_state`/`round_cursor`、待应用命令与可信时间（配合 E4.2）。

#### Story S1.5 · 经文游标与轮次校验（不盲推）

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-003、AD-3/19
- **As a** 诵经者，**I want** backend 只确认设备本地推进的轮次游标而不按经文长度盲推，**So that** 跳字/重字/乱序与两端轮次分裂都不会发生。

**Acceptance Criteria:**
- Given 设备上报携带 `round_id`/游标，When 校验，Then 仅当与 backend 记录的轮次/高水位一致时确认推进；**不以差值除以经文长度盲推**游标或完成。
- Given 重复或乱序提交，When 处理，Then 不造成跳字、重字、游标回退或跨轮错位。
- Given 标点/空格/换行语义，When 计数推进，Then 它们不消耗敲击（计数以设备差值为准，AD-19 一致）。
- Given 版本一致（S1.8 保障），When 文本不符，Then 停止推进并返回配置错误。

#### Story S1.6 · 完成 / 从头开始 / 退出

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-005、AD-3/19
- **As a** 诵经者，**I want** backend 在末字确认后进入完成锁定，且从头开始/退出语义正确，**So that** 完成态与历史统计不被破坏。

**Acceptance Criteria:**
- Given 本轮末字被确认，When 推进完成，Then backend 置完成锁定；锁定期间该轮任何新输入不计数。
- Given 「从头开始」，When 处理，Then 创建新轮次（`round_id` 递增）并重置游标至首字，历史统计保留（跨轮竞态按 S0.1 sync-contract 对账）。
- Given 「退出」，When 处理，Then 保留完成状态与历史，不影响后续统计。
- Given 未确认完成的本地重置竞态，When 对账，Then 按 sync-contract 定义行为，不产生两端轮次分裂（配合 S0.1/AD-3）。

#### Story S1.7 · 历史统计与可信时间归档

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-008、AD-6
- **As a** 诵经者，**I want** 今日/近 7 日/近 30 日/累计/连续天数均由 backend 按确认时间归档，**So that** 统计权威一致且不伪造。

**Acceptance Criteria:**
- Given 已确认敲击，When 归档，Then 离线差量按 backend **确认日期**归档到对应日桶，连续天数以当天存在已确认敲击计算，日界按 backend 配置时区切分。
- Given 设备无可信日期，When 上报同步，Then backend 不以设备端日期伪造当日统计（设备端仍显「待校时」）。
- Given 统计接口，When 前端查询，Then 返回今日/近 7/30/累计/连续天数，全部来自已确认数据，与本地待同步数据隔离。

#### Story S1.8 · backend 嵌入 canonical 经文并校验版本

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-004、AD-4
- **As a** 后端开发代理，**I want** backend 在构建期嵌入 canonical《心经》产物并校验 `scripture_version`，**So that** 三端文本一致、不一致即停。

**Acceptance Criteria:**
- Given S0.3 canonical 产物存在，When backend 构建，Then 嵌入该单源字符序列并携带 `scripture_version`。
- Given 版本校验，When 与契约不一致，Then 返回配置错误并停止推进，不做自动映射。
- Given 运行期，When 设备上报的 `scripture_version` 不符，Then 拒绝推进并报配置错误（配合 S1.5）。

### Epic S2 · 实时推送与命令链（backend 侧 · Software·Backend）

- **layer**：`Software·Backend`（命令前端镜像在 S4）
- **目标（用户结果）**：小程序经 WebSocket 收到已确认差量并按快照水位 S 补齐；音量/亮度/熄屏命令按「已应用修订」收敛——离线命令不覆盖新命令、进入待设备应用，设备回报已应用后翻转。
- **FR 覆盖**：`FR-B-006`、`FR-B-007`
- **绑定 AD**：AD-5（命令修订高水位）、AD-17（WebSocket + 快照水位 S）
- **实现要点**：帧格式/心跳/序号按 S0 sync-contract 落地；命令链不建前端 story（归 S4），用 S0 设备模拟器验证「待设备应用→已生效」翻转；**末端含 SM-3 软件段预算预演 story**（设备模拟器驱动，明确为预演、非终验；真机终验归 E4/S3 真机签收 story，P5）。
- **首个 Story**：WebSocket 通道 + 快照水位 S 断线补齐语义。

#### Story S2.1 · WebSocket 已确认差量推送

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-006、AD-17
- **As a** 诵经者，**I want** backend 在确认差量后经 WebSocket 实时推给小程序，**So that** 手机端能逐字跟进诵经而不轮询。

**Acceptance Criteria:**
- Given 设备同步被确认出新差量，When 广播，Then 向已连接的小程序推送该差量消息（携带单调序号，与 S0.1 契约一致）。
- Given 消息序号，When 前端消费，Then 只消费序号 > 前端快照水位 S 的消息，保证不重复。
- Given 推送与查询，When 对同一后端状态校验，Then WebSocket 推送内容与查询接口结果一致（FR-B-006）。

#### Story S2.2 · 断线补齐（快照水位 S）

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-006、AD-17
- **As a** 诵经者，**I want** 小程序断线重连后只补齐未展示的 backend 差量，**So that** 不重复已展示字符、不出现空洞或重叠。

**Acceptance Criteria:**
- Given 小程序请求补齐，When 调用查询接口，Then backend 返回在当前状态冻结的快照水位 S 与对应差量（断线补齐基准 = 快照水位 S，而非前端「已展示计数」或连接记忆）。
- Given 存在序号空洞，When 重连补齐，Then 前端先查询补齐空洞再续播，不跳不重（配合 S3/S4 前端实现）。
- Given 前后端共存，When 校验，Then 查询与 WebSocket 结果在相同后端状态下一致。

#### Story S2.3 · 命令修订与「待设备应用」收敛

- **layer**：`Software·Backend` ｜ **绑定**：FR-B-007、AD-5
- **As a** 诵经者，**I want** 音量/亮度/熄屏/立即同步等命令按单调修订号收敛，**So that** 离线命令不乱序、旧命令不覆盖新命令、状态如实。

**Acceptance Criteria:**
- Given 前端下发设置，When backend 保存，Then 只保留并下发最新修订号 `command_revision`（语义=设备已应用高水位），并持久化。
- Given 设备离线，When 命令进入待设备应用，Then 前端显示「待设备应用」而非已生效。
- Given 设备在下次活动窗口随同步包回报已应用修订，When backend 校验，Then 以「已应用 ≥ 已下发」幂等将待设备应用翻转为已生效，不依赖单次 ACK 到达。
- Given 重复下发，When 幂等处理，Then 不重复、不乱序、旧命令不覆盖新命令。

#### Story S2.4 · SM-3 软件段预算预演（设备模拟器驱动）

- **layer**：`Cross-layer` ｜ **绑定**：SM-3（预演）、AD-17
- **As a** 诵经者，**I want** 在硬件到货前用设备模拟器预演「确认→推送→前端展示」的软件通道预算，**So that** 尽早发现后端/推送/渲染延迟瓶颈。

**Acceptance Criteria:**
- Given 设备模拟器（S0.4）以 20 次/秒 tap 流驱动，When 观察端到端软件链（backend 确认 → WebSocket → 小程序渲染），Then 记录各段耗时与累计分布，量化通道预算（明确为**预演**，非 SM-3 终验）。
- Given 预演出现瓶颈，When 定位，Then 提出并实施 backend/推送侧优化，复查分布是否回落预算内。
- Given 预演完成，When 交付，Then 生成报告供真机签收（E4.5）时对照，不把模拟结果当正式指标通过。

### Epic S3 · 小程序诵经呈现与完成反馈（Software·Frontend）

- **layer**：`Software·Frontend`
- **目标（用户结果）**：微信登录直达唯一设备；阅读行逐字动画呈现 backend 已确认经文（不跳/不重/不预览未来）；离线积压回放接实时；心经完成显示礼花与「从头开始/退出」。
- **FR 覆盖**：`FR-F-001`~`FR-F-005`
- **绑定 AD**：AD-2（只呈现已确认）、AD-17（快照水位消费）、AD-18（uni-app 基线）、AD-19
- **实现要点**：首 story = frontend 工程骨架（uni-app Vue3+TS+Vite+Pinia + 单一 `VITE_API_BASE_URL` + 统一 `api/request` + 登录路由），**S4 复用本骨架**；**mock 依据 = S0 的 frontend-facing API 契约**（P1），不依赖 S1 时序。阅读页先完成 10 种禅意 style 候选与作者选择，再以选中风格承接全帧闭包；正文采用 append-only 阅读流，增加 `scripture-progress` 总进度百分比与 `reading-archive` 已完成篇章保留；动画落后动态加速不丢字符；增加「构建期嵌入 canonical 经文并校验 `scripture_version`」story（引用 S0，P4）。
- **首个 Story**：frontend 骨架 + 登录直入 + 经文页静态呈现。

#### Story S3.1 · frontend 骨架 + 微信登录直达唯一设备

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-001、AD-18
- **As a** 诵经者，**I want** 打开小程序即微信登录并直达我的设备，**So that** 无任何绑定/迁移流程即可看进度。

**Acceptance Criteria:**
- Given 空 `cloud/frontend/`，When 初始化，Then 建立 uni-app（Vue 3 + TS + Vite + Pinia）工程，仅构建微信小程序，单一 `VITE_API_BASE_URL` + 统一 `api/request`（信封 + 鉴权头）。
- Given 首次进入，When 微信登录，Then 直达唯一设备主页（按 S0.2 API 契约对接，mock 阶段不依赖真实 S1 时序）。
- Given MVP 边界，When 检查 UI，Then 无绑定/迁移/多设备治理界面（FR-F-001）。

#### Story S3.2 · 经文呈现页（持续正文 + 当前字聚焦 + 总进度）

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-002、AD-2/19
- **As a** 诵经者，**I want** 小程序以持续累积的心经正文、当前字聚焦和总进度百分比呈现已确认内容，**So that** 我能自然阅读已诵内容且不产生正式敲击。

**Acceptance Criteria:**
- Given 小程序主界面，When 渲染，Then 无电子木鱼/点击敲击入口，不产生任何正式输入事件。
- Given 已确认差量，When 展示，Then 新字 append 到当前诵读正文并聚焦尾字，已有内容不覆盖、不清空，只呈现 backend 已确认字符，顺序不越权、不预览未来（AD-19）。
- Given `confirmed_chars=0`，When 渲染，Then 显示“等待设备诵读”；首个确认字到达后成为正文第一字。
- Given 当前诵读达到 `scripture_chars_total`，When 渲染，Then 显示 100% 和完成状态，全文保留并等待完成弹窗操作。
- Given 选定的 10 种 style 候选，When 对拍视觉稿，Then 当前实现只能使用作者选中的 style；候选之间至少有三个非颜色维度差异。
- Given 最终模板，When 运行文案卫生检查，Then 不包含 Node ID、AI 思维链、作者说明、重复进度或孤立标点。

#### Story S3.3 · 在线逐字动画（不跳/不合并/不重复）

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-003、AD-17/19、SM-3（渲染侧）
- **As a** 诵经者，**I want** 每个已确认汉字按顺序追加一次、落后时动态加速，**So that** 快速连敲也能流畅跟诵而不丢字。

**Acceptance Criteria:**
- Given 实时差量到达（序号 > 快照水位 S），When 逐字展示，Then 每个汉字 append 到当前诵读正文一次，不跳过、不合并、不重复，前缀内容保持不变（FR-F-003）。
- Given 动画落后积压，When 队列增长，Then 动态加速（缩短间隔）追平，清空后恢复节奏，不丢正式字符。
- Given 渲染侧测量，When 与 SM-3 口径对齐，Then 渲染环节不单独突破端到端 1 秒预算（SM-3 渲染侧约束）。

#### Story S3.4 · 离线积压回放 + 无缝接实时

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-004、AD-17
- **As a** 诵经者，**I want** 重新打开小程序时先按 backend 差量顺序回放离线积压，回放中新增事件排队，结束后无缝接实时，**So that** 不漏字不重字不乱序。

**Acceptance Criteria:**
- Given 存在离线积压差量，When 打开小程序，Then 按 backend 差量顺序追加回放（不跳不重），已有正文保留，回放期间新到达事件进入队列。
- Given 回放结束，When 队列接入，Then 无缝切换为实时动画，无重复展示已回放字符。
- Given 断线重连，When 切换查询/回放，Then 只补齐未展示差量（序号 > 快照水位 S），不重复已展示字符（FR-F-009 语义）。

#### Story S3.5 · 完成反馈（礼花 + 从头开始/退出）

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-005、AD-17/19
- **As a** 诵经者，**I want** 仅在收到 backend 完成确认后显示礼花与操作，**So that** 完成状态可信、不提前声称完成。

**Acceptance Criteria:**
- Given backend 完成确认到达，When 前端收到，Then 保留 100% 全文并显示礼花与「从头开始/退出」弹窗（FR-F-005）。
- Given 尚未收到确认，When 当前诵读本地看似结束，Then 不提前声称完成、不显示礼花。
- Given 弹窗展示期间，When 有新差量到达，Then 不展示新推进（弹窗期间冻结新字呈现）。

#### Story S3.6 · 前端嵌入 canonical 经文并校验版本

- **layer**：`Software·Frontend` ｜ **绑定**：FR-B-004（frontend 面）、AD-4
- **As a** 诵经者，**I want** 小程序构建期嵌入 S0.3 canonical《心经》并校验 `scripture_version`，**So that** 三端文本一致、不一致即停。

**Acceptance Criteria:**
- Given S0.3 canonical 产物，When 前端构建，Then 嵌入该单源字符序列并携带 `scripture_version`。
- Given 与契约/backend 版本不一致，When 发现，Then 停止推进并呈现配置错误提示，不做自动映射。
- Given 顺序呈现，When 逐字播放，Then 字符序列与 canonical 产物一致，不出现誊写漂移。

### Epic S4 · 小程序记录/设备状态与设置操作（Software·Frontend）

- **layer**：`Software·Frontend`
- **目标（用户结果）**：回看今日/近 7/30 日/累计/连续天数；查看电量/4G/最后同步/待同步/失败并可刷新重试；下发设置，设备离线时显示「待设备应用」而非已生效，设备确认后刷新。
- **FR 覆盖**：`FR-F-006`~`FR-F-010`
- **绑定 AD**：AD-5、AD-6、AD-17
- **实现要点**：复用 S3 的 frontend 骨架与请求封装；命令闭环验收用模拟 backend 桩（翻转待设备应用→已生效），真实回环在 S2 的 Cross-layer 联调验证；本地待同步数据永不混入正式统计（空态/失败态齐全）。
- **首个 Story**：记录页 + 设备页骨架（统计来自 backend 已确认数据）。

#### Story S4.1 · 记录页统计（backend 已确认数据）

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-006、AD-6/17
- **As a** 诵经者，**I want** 记录页展示今日敲击/近 7/30 日/累计敲击/连续天数，**So that** 我能区分当前篇章进度与长期累计。

**Acceptance Criteria:**
- Given 记录页，When 查询统计，Then 展示今日敲击、近 7 日、近 30 日、累计敲击与连续天数，单位清楚且全部来自 backend 已确认数据（按 S0.2 API 契约）。
- Given 本地存在待同步数据，When 渲染统计，Then 待同步数据不混入正式统计（AD-2）。
- Given 查询失败/为空，When 展示，Then 呈现对应失败/空态而非错误数值。

#### Story S4.2 · 设备页状态与刷新/重试

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-007、AD-5/17
- **As a** 诵经者，**I want** 设备页显示电量/4G/最后同步/待同步数量/待设备应用/失败状态并可刷新重试，**So that** 我随时了解设备与云端是否一致。

**Acceptance Criteria:**
- Given 设备页，When 加载，Then 展示电量/充电、4G、最后同步、待同步数量、待设备应用与失败状态（数据来自 backend 确认/设备状态）。
- Given 状态过时，When 点刷新，Then 重新拉取并更新。
- Given 存在失败/待同步，When 点「立即同步」或重试，Then 触发设备活动窗口，刷新后状态收敛（配合 E4）。
- Given 设备离线，When 命令下发，Then 显示待设备应用而非已生效。

#### Story S4.3 · 设置镜像（音量/亮度/熄屏/命令下发）

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-008、AD-5
- **As a** 诵经者，**I want** 在小程序里修改音量/亮度/熄屏并下发，**So that** 无需在设备上也能调整偏好，且状态如实。

**Acceptance Criteria:**
- Given 用户在设置镜像修改任一设置，When 提交，Then 经 backend 按 `command_revision` 持久化并下发（设备离线则进入待设备应用）。
- Given 设备未回报已应用，When 显示，Then 显示「待设备应用」而非已生效（不冒充已应用）。
- Given 收到设备已应用修订回报（AD-5 翻转），When 刷新，Then 显示最终生效状态。
- Given 两端同时修改，When 收敛，Then 以单调修订号后写者生效、旧命令不覆盖新命令。

#### Story S4.4 · 断线恢复与查询补齐

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-009、AD-17
- **As a** 诵经者，**I want** WebSocket 断开时自动切查询/回放、重连后只补齐未展示差量，**So that** 实时诵经体验在网络抖动时不重复不空洞。

**Acceptance Criteria:**
- Given WebSocket 断开，When 发生，Then 前端切换为查询/回放模式并给出同步状态提示。
- Given 重连成功，When 恢复，Then 只补齐未展示差量（序号 > 快照水位 S），不重复已展示字符，随后接回实时。
- Given 断线与实时事件并发，When 处理，Then 不丢字、不重字、不乱序。

#### Story S4.5 · 空态与失败态全覆盖

- **layer**：`Software·Frontend` ｜ **绑定**：FR-F-010、AD-2/17
- **As a** 诵经者，**I want** 首登/无历史/离线/同步失败/队列已满/低电量/待校时/待设备应用/完成/权限错误都有明确状态，**So that** 我不会误解数据或操作失败。

**Acceptance Criteria:**
- Given 上述任一状态，When 对应界面呈现，Then 显示明确空态/失败态文案与可用操作（首登引导、无历史、离线、同步失败、队列已满、低电量、待校时、待设备应用、篇章完成、权限错误）。
- Given 本地有待同步数据，When 任意界面，Then 绝不以正式进度/统计伪装呈现（AD-2）。
- Given 数据加载失败，When 重试可用，Then 提供重试且失败态不被当作成功。
