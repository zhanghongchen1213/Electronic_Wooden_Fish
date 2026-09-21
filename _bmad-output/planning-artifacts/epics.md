---
stepsCompleted:
  - step-01-validate-prerequisites
  - step-02-design-epics
  - step-03-create-stories
  - step-04-final-validation
inputDocuments:
  - "_bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/brief.md"
  - "_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
  - "_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
  - "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/DESIGN.md"
  - "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/EXPERIENCE.md"
  - "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/UI_CONTRACT-device.md"
  - "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/UI_CONTRACT-miniapp.md"
---

# Electronic_Wooden_Fish - Epic Breakdown

## Overview

本文档基于产品简报、PRD、架构主干和两份 UX spine/UI contract，整理电子木鱼 MVP 的需求库存、Epic 分轨和可执行 Stories。Epic/Story 设计遵循 PRD 的 Embedded/cloud 并行边界，并以 mock/stub 保证各 Epic 可独立推进。

## Requirements Inventory

### Functional Requirements

- **FR-E-001：电源、供电与复位。** 支持单节锂电、USB-C 外部供电/充电、LTC2954 `PWR_INT` PWR 输入、BOOT0 下载控制和 EN 复位；USB-C 外部供电、电池供电、外部供电并充电三种条件下输入、经文、音频、显示、持久化和同步均可用；已持久化状态重启后恢复；运行态 BOOT0 短按只切换自动模式，不改变下载流程。
- **FR-E-002：PVDF 与低功耗唤醒。** PVDF 是唯一 MVP 实体传感器；比较器负责唤醒，ESP32 ADC 二次确认；QMI8658A 仅作未来预留，不参与 MVP 计数；单次/连续敲击应产生候选信号，普通振动不应误触发，ADC 不越界。
- **FR-E-003：统一有效敲击队列。** `physical_pvdf`、`device_touch`、`automatic_tap` 进入同一有效敲击队列，支持 1 秒 1–20 次输入；完成遮罩和故障锁定期间忽略输入；三类来源共享累计、经文、音频、RGB、持久化和同步结果。
- **FR-E-004：本地高水位与离线积压。** 持久化 `local_total`、`acked_total`、`round_id`、`scripture_version`、`round_state`、`round_cursor`、`pending_completion`、待同步事件段及 `action_id`；查询/回放侧保存 `snapshot_seq`、`replay_cursor`；自动模式只存在内存，离线积压上限 1000 次；网络关闭、小程序关闭或重启后继续记录，重复同步不重复计数，达到上限时统一拒绝并提示。
- **FR-E-005：核心音频反馈。** 使用 ES8311、NS4150B 与扬声器输出木鱼音，默认音量 50/100，音量 0 静音；高速连击限速或合并声音；音频失败不得阻塞计数、持久化和同步；音量跨重启保留。
- **FR-E-006：设备屏幕与三主页。** CO5300 AMOLED 提供木鱼页、经文页、统计页和下滑设置页；常驻状态栏展示 4G/Wi-Fi/蓝牙/GPS 被动状态、电量百分比和同步状态；木鱼页固定 7 字带、心经进度字数/百分比、今日/累计敲击；经文页展示 append-only 经文流；统计页仅展示今日与累计；设备不依赖或伪造充电状态；未校时显示“待校时”；熄屏实体敲击短暂亮屏；达到末字后保持 100% 并进入完成锁定。
- **FR-E-007：触摸与 PWR 导航。** 支持三主页左右滑动、下滑设置、设置控件、完成弹窗和木鱼点击；只有木鱼页木鱼区域产生 `device_touch`；熄屏首次触摸只唤醒；PWR 短按唤醒/循环三页；BOOT0 短按切换自动模式；长按关机由板级电源负责；亮度低/中/高、熄屏 5/15/30 秒、音量 0–100、立即同步设置跨重启保留。
- **FR-E-008：Air780EGP 4G 活动窗口。** MVP 只使用 Air780EGP HTTPS JSON；敲击、设备触摸、自动敲击、立即同步和命令应用时复用通信上下文；不配置 BLE/Wi-Fi 业务链路，GPS 默认关闭；弱网时本地继续记录、恢复后补传；发射峰值不得引发掉压重启；设备上报环节不得单独突破 95%/1 秒端到端目标。
- **FR-E-009：状态与低打扰反馈。** 通过 CW2015、RGB、AMOLED、音频表达电量、4G、同步、待设备应用、低电量、有效敲击、自动模式和故障；状态需可辨识、低打扰、不中断诵经节奏；不表达主控无法确认的充电状态。
- **FR-E-010：USB-Serial-JTAG。** 使用 ESP32-S3 原生 USB-Serial-JTAG 完成烧录、CDC 日志和 JTAG 调试；不使用 CH340X 或 USB-OTG 业务；PC 可识别并完成烧录/日志。
- **FR-E-011：BOOT0 自动模式。** 运行态完整短按/释放 BOOT0 经过防抖后进入或退出自动模式；以确认时刻为周期锚点，每 3 秒生成一个 `automatic_tap`；事件沿用真实敲击全链路，不跨重启持久化；长按、抖动、启动采样和下载流程忽略；退出后不生成新事件，已入队事件按统一顺序完成；完成/故障/队列满 gate 不得绕过；连续 10 周期单周期误差不超过 ±100ms。
- **FR-C-001：单身份单设备。** 唯一微信身份映射到一台固定设备；不提供多设备、解绑、换机迁移或公开账号治理；不同身份不共享高水位与统计。
- **FR-C-002：固定经文版本。** Embedded、backend、frontend 使用同一版固定《般若波罗蜜多心经》可消费汉字序列和 `scripture_version`；canonical 文本只有一个落盘来源，构建/打包期校验一致；不提供选经、导入或自动映射，版本不一致即停止推进并报告配置错误。
- **FR-C-003：经文游标与轮次。** 设备本地推进 `round_id`/`round_cursor`，cloud 校验并确认轮次、游标和完成状态；标点、空格、换行随相邻汉字出现，不单独消耗敲击；重复/乱序提交不得跳字、重字或回退游标。
- **FR-C-004：完成、从头开始与退出。** 末字确认后进入完成锁定；“从头开始”创建新轮次并回到首字；“退出”保留完成状态和历史统计；完成锁定期间任何输入不计数；新轮次不清除旧篇章和累计。
- **FR-C-005：历史统计与可信时间。** 提供今日、近 7 日、近 30 日、累计和连续天数；离线差量按 cloud 确认日期、配置时区归档；连续天数按已确认敲击计算；无可信日期时设备显示“待校时”，cloud 不伪造当天统计。
- **FR-C-006：状态查询与恢复基准。** 为设备和小程序提供带单调序号的状态快照、差量和恢复基准；小程序断线后以查询响应冻结的快照水位补齐；序号空洞先查询补齐，不能以自身展示计数推断权威状态。
- **FR-C-007：高水位幂等同步。** 接收 `local_total`、`acked_total`、轮次和设备状态，按设备身份比较累计高水位，只确认高于已确认值的差量；相同/更低高水位为 no-op；返回最新确认高水位、差量和轮次状态；冲突返回明确业务错误。
- **FR-C-008：命令修订与待设备应用。** 保存音量、亮度、熄屏时长、立即同步和篇章命令的最新 `command_revision`；设备离线时显示“待设备应用”；下一次 HTTPS 活动取得并应用最新修订，回传已应用修订；旧修订不得覆盖新修订；重复获取/回传幂等。
- **FR-C-009：实时推送与断线补齐。** 通过 WebSocket 推送已确认差量、当前字、游标、轮次、完成状态、同步状态和设备状态；断线使用查询/回放补齐，恢复后继续消费新序号；WebSocket 与查询结果一致。
- **FR-C-010：JSON 文件持久化。** cloud 单实例单进程运行，使用临时文件、`fsync`、`rename` 原子落盘，在 `cloud/backend/data/` 保存身份、高水位、游标、轮次、命令、设备状态和统计；不引入任何数据库；重启恢复全部状态，写入中断不得留下半写状态。
- **FR-C-011：统一接口、鉴权和错误语义。** 对外接口统一 `/api/v1` 与 `{code,message,data}` 信封，成功 `code=0`；业务错误 HTTP 200+业务码，协议错误使用对应 HTTP 状态；微信登录、令牌刷新、设备身份校验遵循单身份单设备；并发 401 只刷新一次，失败唤醒等待请求并回登录入口，敏感会话不入日志。
- **FR-C-012：登录直达唯一设备。** 小程序登录后直接进入唯一设备，不出现绑定/迁移/多设备选择页；首次登录、重复登录和权限失败均有对应状态与明确重试动作。
- **FR-C-013：心经持续阅读流。** 小程序主界面按冻结 Pen/HTML 呈现 append-only《心经》纸面；只追加 cloud 已确认汉字，已有前缀不覆盖/清空；每行 17 槽，标点计槽；当前字只聚焦已确认内容；不出现可点击电子木鱼、不预览未来经文、只保留一组进度。
- **FR-C-014：在线逐字动画。** 每个已确认汉字按序追加并展示一次，已有内容持续可见；队列积压时可动态加速但不得跳过、合并或重复；最新字使用冻结焦点样式和下划线；队列清空后恢复基线节奏。
- **FR-C-015：离线积压回放。** 小程序重开或断线后按 cloud 差量顺序回放；回放期间的新事件排队；回放结束后无缝接入实时；不跳字、不重字、不乱序，新事件不插入旧差量。
- **FR-C-016：完成反馈。** 仅在收到 cloud 完成确认后保留全文和 100% 进度，显示短促礼花及“从头开始/退出”弹窗；待确认时不提前声称完成；弹窗期间不展示新推进；从头开始创建新 `round_id`，退出保留完成篇章和历史统计。
- **FR-C-017：记录、设备和设置页面。** 记录页展示今日/近 7 日/近 30 日/累计/连续；设备页展示电量、4G、最后同步、待同步、待设备应用和失败，不展示主控无法确认的充电状态；设置页镜像音量/亮度/熄屏并下发命令；提交后显示“待设备应用”，收到应用修订后才显示“已生效”；刷新、立即同步和重试可重复。
- **FR-C-018：空态、失败态和权限态。** 覆盖首次登录、无历史、离线、同步失败、队列已满、低电量、待校时、待设备应用、篇章完成和权限错误；每种状态有产品短句和可执行动作；本地待同步数据不得混入正式进度、记录和统计；页面不得出现 AI 思维链、Node ID、TODO、draft 或 placeholder。
- **FR-C-019：视觉真源对拍。** 小程序以 `MINI-06` Pen 和作者导出 HTML 为视觉真源；登录、阅读、记录、设备、设置和完成弹窗的布局、令牌、字体、状态文案和导航触区逐屏对拍；页面边界和卫生检查通过，不从历史候选稿复制。

### NonFunctional Requirements

- **NFR1 / SM-1 可靠输入：** 单次和 1 秒 20 次连续输入不得出现明显漏记、重记或乱序；主观量词在目标外壳样机上演示签收。
- **NFR2 / SM-2 动画连续性：** 设备 7 字带和小程序阅读行不得丢失正式字符或重复播放。
- **NFR3 / SM-3 端到端实时性：** 稳定 4G 下，95% 有效敲击应在发生后 1 秒内被小程序展示，口径为设备敲击→backend 确认→frontend 渲染。
- **NFR4 / SM-4 离线恢复：** 离线累计、设备重启、重复提交、网络恢复后，高水位、游标、轮次和统计最终一致。
- **NFR5 / SM-5 持久化：** backend 重启后高水位、游标、配置和历史统计不丢失。
- **NFR6 / SM-6 个人可持续使用：** 原型完成后作者愿意持续使用；工程期不直接判定，列为首轮样机后的回看指标。
- **NFR7 / SM-7 一天续航：** 约 30 分钟/天实际敲击、其余时间待机的模型下至少支撑完整一天，最终以样机实测为准。
- **NFR8 / SM-8 自动模式一致性：** BOOT0 自动模式按 3 秒周期生成事件，其 UI、经文、音频、累计、持久化、同步和历史结果与真实敲击一致；退出后不再生成新事件。
- **NFR9 / SM-C1 产品聚焦与功耗边界：** 不以常开 BLE/Wi-Fi/4G、堆叠音效或扩展功能换取在线感；保持续航、低打扰和核心诵经节奏。

### Additional Requirements

- **AD-1 单一正式输入链路：** 物理 PVDF、设备触摸和 PRD 定义的 `automatic_tap` 必须归一到一个有效敲击队列；除该队列外不得推进正式累计，原始波形不保存/不上报。架构旧文案将正式输入写成两类，需由同步契约按 PRD 最终枚举收敛。
- **AD-2 backend 权威：** cloud backend 是已确认 `acked_total`、游标/轮次/完成、历史统计和命令状态的唯一权威；小程序只呈现已确认进度，设备本地未确认值必须标记为待同步/同步中。
- **AD-3 幂等高水位：** 活动窗口内 HTTPS 上报累计高水位和轮次；backend 对高于确认值的差量只确认一次，同值/更低为 no-op，不按经文长度盲推游标；离线积压上限 1000，达到上限统一拒绝并提示。
- **AD-4 canonical 经文：** 《心经》单一文本源与 `scripture_version` 在三端构建期生成并校验；版本不一致即停，不做自动映射、选经或导入；设备字形只覆盖本篇经文及必要 UI 字符子集。
- **AD-5 命令修订：** 音量/亮度/熄屏等命令使用单调 `command_revision`；backend 仅保留最新修订，设备在活动窗口回传已应用高水位；未应用显示“待设备应用”，达到修订后显示“已生效”。
- **AD-6 可信时间：** backend 按配置时区以确认时间归档日统计；设备不保存绝对敲击时间；未取得可信网络/HTTPS 时间前设备今日显示“待校时”，不伪造当天统计。
- **AD-7 事件驱动固件：** 长期能力实现为 FreeRTOS 服务任务，使用 typed event/queue；ISR 只投递事件；可合并事件可丢最新，命令型同步/持久化/命令应用不得静默丢失，需 busy/error 与中文日志。
- **AD-8 BSP 独占资源：** 只有 BSP 定义 GPIO、总线、电源使能和初始化顺序；硬件驱动经组件管理器引入后由自有 BSP 封装稳定 API、自检入口和错误码；新增芯片目录登记 CMake。
- **AD-9 `device_state` 事实源：** 服务通过 typed update 发布，UI、音频、4G、同步、设置和持久化只读不可变快照，不读其他服务内部变量。
- **AD-10 共享资源仲裁：** I²C 通过单一总线仲裁，Air780EGP AT 使用单一 UART 事务所有权，LVGL 仅由单一 `ui_task` 更新；不得并发抢占共享资源。
- **AD-11 USB 调试基线：** ESP32-S3 原生 USB-Serial-JTAG（GPIO19/20）负责烧录、CDC 日志和 JTAG；不使用 CH340X/USB-OTG；USB 连接时禁用自动 light sleep 以防日志失联。
- **AD-12 故障隔离与合并反馈：** 音频/触屏/渲染失败不得阻塞计数、落盘或同步；高速连击保留全部计数，音频合并成可辨识节奏，动画队列追平而不丢正式字符。
- **AD-13 电源边界：** 按硬件基线落实分轨、Air780EGP 发射峰值保护、低电先落盘、PWR 长按由板级电源负责；PRD final 明确所有供电条件均可用，需在实现前消解架构/UX 旧“充电暂停”语义。
- **AD-14 唯一联网链路：** MVP 只启用 Air780EGP 4G HTTPS 活动窗口，不维持常开长连接；BLE/Wi-Fi 仅为硬件保留状态，GPS 默认关闭；不得引入模组独立业务云。
- **AD-15 本地持久化：** 跨重启保留高水位、轮次、游标、经文版本、设置和待同步标记；不保存绝对敲击时间；低电先保证累计落盘。
- **AD-16 backend 技术基线：** Java 17 + Spring Boot 3.3.7 + Maven 基线，沿用 REST/JWT/微信登录模式；单实例单进程，`cloud/backend/data/` 使用临时文件+fsync+rename 原子 JSON，禁止任何数据库及第二套平行 backend；生产/本地配置位置固定。
- **AD-17 frontend 同步基线：** 只消费 backend 确认差量；WebSocket 推送配合查询冻结快照水位补齐，序号空洞先查询；断线回放期间新事件排队；完成反馈仅由 backend 确认触发。
- **AD-18 frontend 技术基线：** uni-app（Vue 3 + TypeScript + Vite + Pinia）仅微信小程序；所有请求经 `VITE_API_BASE_URL` 和统一 `api/request`，本地状态不得成为权威；生产 `.env`、本地 `.env.development`，默认 API 含 `/api/v1`。
- **AD-19 经文消费语义：** 每次有效敲击只消费一个 canonical 可消费汉字，标点/空格/换行随附；设备 7 槽、小程序 17 槽均不预览未来；跨轮次用 `round_id`，完成后从头开始保留历史，退出保留完成态。
- **AD-20 环境与交付脚本：** backend 配置固定为 `application.yml`/`application-local.yml`，local profile、9218 端口、`saas.jar` 与 `.sha256` 产物固定；脚本不启动公网隧道；构建使用通用 JDK17/Maven 3.9.x；上传前校验 Jar 与 SHA-256，并上传到约定目录；frontend AppID 与 backend 配置对齐。
- **硬件事实源与验证门禁：** GPIO、电源、FPC、BOM、器件连接和启动绑带只从 `docs/hardware/` 读取；Epic 1/2/7 必须保留 ERC/PCB/样机和 USB、总线、PVDF、4G、功耗证据，不把设计输入或编译成功误写成整机验证完成。
- **同步契约前置：** `docs/contracts/sync-contract.md` 当前只有 README 占位；在 Epic 4/2 的模块 spec 前冻结字段、帧格式、重连、`automatic_tap`、`round_id`、`pending_completion`、`action_id`、`snapshot_seq`、`replay_cursor` 与跨轮竞态。
- **Deferred 与风险：** JSON schema/文件粒度、WebSocket 心跳参数、IDF/LVGL/Spring Boot 升级、生产域名/AppID、明文密钥、云端托管/健康检查、产物签名、低功耗参数、硬件细分和音频/动画定标均不得在 Story 中伪装成已验证事实；需按 architecture Deferred 条件进入后续门禁。
- **非目标边界：** 不实现 BLE/Wi-Fi/GPS 业务、自动 OTA、选经/导入、多设备/换机、社交/排行榜/提醒/付费、量产认证与云运维体系；QMI8658A 不参与 MVP 敲击。

### UX Design Requirements

- **UX-DR1 视觉真源与双轨令牌：** 设备使用冻结 DEVICE-01/Pen 母版，微信小程序使用冻结 MINI-06/Pen 母版；两轨可不同风格但共享琥珀和字体语义，历史候选稿不参与实现。
- **UX-DR2 设备画布与布局：** 实现 410×502 CO5300 画布、圆角 110、16px 边距、≥8px 关键净空、24px 状态栏；三主页常驻横向 pager，设置页独立按需加载，禁止外层展示板/装饰背景。
- **UX-DR3 小程序画布与布局：** 实现 390×844 对拍基线、62px 顶部安全区、20px 页面边距、56px 底部导航及底部安全间距；实际微信设备只等比例缩放，不改变阅读流语义。
- **UX-DR4 字体合同：** 设备木鱼/经文使用 Noto Serif SC，状态/统计/设置使用 Noto Sans SC；小程序正文使用 Noto Serif SC、导航/状态使用 Noto Sans SC；设备只嵌入 canonical《心经》和 UI 所需子集，生成字体与 SquareLine 资产字节一致，禁止手改位图。
- **UX-DR5 设备状态栏：** 常驻呈现 4G/Wi-Fi/蓝牙/GPS connected/no-signal/disabled 单槽状态、电池百分比/符号和同步短语；状态通过图标+文字/颜色双编码，图标可见不等于业务链路启用。
- **UX-DR6 木鱼交互边界：** 木鱼页电子木鱼是唯一可计数触区（≥96×96）；`woodfish-anatomy` 只负责器物识别，不独立接收输入；经文/统计/设置普通触摸只导航或滚动。
- **UX-DR7 三环反馈：** `physical_pvdf` 与 `device_touch` 在木鱼页触发恰好三道椭圆环 `idle→flash→idle`，flash 160ms，切换由 bindings 运行时补丁完成，连续输入只重启光效，不建立金色节点变体。
- **UX-DR8 设备 7 字带：** 固定 7 槽，新字右入、旧字左移，空位低对比占位；仅一个 `glyph-current` 可使用大字焦点；不预览未来经文，标点随相邻字出现且不消耗敲击。
- **UX-DR9 设备经文流：** 经文页为单篇 append-only 流，每行 13 槽，标点占槽；流尾最新字大字高亮，已确认前文可垂直回看；回看期间新字追加后自动锚回流尾；不生成第二历史副本。
- **UX-DR10 设备进度与统计：** 设备显示 `已诵/总字数`、百分比、当前诵读状态、今日敲击与跨轮次累计；统计页不承载近 7/30 日或连续天数；到 100% 后保持 N/N。
- **UX-DR11 设备设置页：** 首屏最多四行，第二页展示木鱼版本/ID；亮度低/中/高、熄屏 5/15/30 秒、音量 0–100、立即同步控件均复用固定骨架；同步只切换 busy/ok/pending/fail 组件状态，不复制整屏。
- **UX-DR12 设备完成遮罩：** 末字确认后使用完成遮罩冻结输入，提供“从头开始/退出”；不另建顶层 OVERLAY，复用木鱼页骨架和 `modal-done` 组件。
- **UX-DR13 设备状态闭包：** 对拍覆盖 MUYU、JINGWEN、TONGJI、SHEZHI、SHELL 的 15 个顶层状态/变体，含空带、填充、满带、未校时、同步五态、低电、故障、完成和信号状态；组件状态优先于整屏变体。
- **UX-DR14 generated/bindings 边界：** SquareLine/LVGL generated 仅放布局、字体、事件空桩；业务路由、状态投影、三环 flash、经文滚动、状态栏补丁全部由 bindings 在独占 `ui_task` 中驱动。
- **UX-DR15 小程序阅读流：** 每行固定 17 槽，正文 18px，最新字 28px 焦点色并带持续 2px 下划线；每个 backend 已确认字符 append 一次，前缀永不覆盖/清空，未来经文不预览。
- **UX-DR16 小程序页面闭包：** 实现 LOGIN、READING、RECORDS、DEVICE、SETTINGS、OVERLAY 六类页面和 14 个状态；登录直达唯一设备；四项底部导航始终可达，每项触区不小于 44×44。
- **UX-DR17 小程序离线回放：** REPLAY 状态显示“回放中·新事件排队”，先按 backend 差量顺序回放，期间新事件排队，回放结束无缝接实时；断线切查询/回放，不用本地展示计数作基准。
- **UX-DR18 小程序完成与归档：** 仅 backend 确认后显示 100%、短促礼花和“从头开始/退出”；完成篇章全文保留，“从头开始”在下方开启新 `round_id` 区块，旧篇可折叠。
- **UX-DR19 小程序记录与设备状态：** 记录页显示今日/近 7 日/近 30 日/累计/连续；设备页显示电量、4G、最后同步、待同步、待设备应用和失败，并提供刷新/立即同步/重试；不伪造充电状态。
- **UX-DR20 小程序设置镜像：** 音量 0–100 滑杆、亮度低/中/高分段、熄屏 5/15/30 秒分段；提交后显示“待设备应用”，只有收到对应应用修订才显示“已生效”。
- **UX-DR21 空态/失败态/权限态：** 覆盖首登、无历史、离线、同步失败、队列已满、低电量、待校时、待设备应用、完成和权限错误；每个状态同时给出“是什么+能做什么”，不把空态当成零值成功。
- **UX-DR22 视觉令牌与层次：** 设备使用 surface0/1/2 分层、低打扰状态色与 60% 完成遮罩；小程序使用纸面根帧、1px 刻线和极浅投影，正文/底对比满足可读性，强调色不承载长段正文。
- **UX-DR23 动效与可访问性：** 正文/底对比目标 ≥7:1；同步状态不能只靠颜色；提供减少动效偏好，关闭插值但保留一次状态确认；三环 160ms、礼花短促且无常驻动画；小程序关键操作提供可读文本。
- **UX-DR24 Pen/HTML 对拍与卫生检查：** 逐屏比对布局、令牌、字体、状态文案、导航触区和页面边界；最终页面禁止 AI 思维链、作者说明、Node ID、`data-pencil-id`、TODO、draft、placeholder、调试标签和重复进度。
- **UX-DR25 交互与文案基线：** 使用极简中文、状态短句、少打扰语气；错误/空态说明状态和可执行动作；不使用营销词和感叹号；设备/小程序同步词表保持一致。
- **UX-DR26 纯运行时状态补丁：** 信号、电量、同步点颜色/图标和三环 flash 属运行时补丁，不因单色/数值/同步文案复制整屏；Pencil 状态板只为审阅，对外渲染保持 canonical 结构。

### FR Coverage Map

- FR-E-001：Epic 1 - 设备可靠上电、供电条件、PWR/BOOT0/EN 与重启恢复
- FR-E-002：Epic 1 - PVDF 比较器唤醒、ADC 二次确认与误触发边界
- FR-E-003：Epic 2 - 三类输入统一进入有效敲击队列
- FR-E-004：Epic 2 - 本地高水位、离线积压、重启恢复与队列上限
- FR-E-005：Epic 2 - 木鱼音频、默认音量、高速合并与故障隔离
- FR-E-006：Epic 3 - 设备三主页、状态栏、7 字带、经文流、统计与完成态
- FR-E-007：Epic 2 - 触摸/PWR/BOOT0 导航和设置持久化；Epic 3 - 页面与交互投影
- FR-E-008：Epic 2 - Air780EGP 活动窗口、弱网本地记录与恢复补传；Epic 7 - 真机端到端验证
- FR-E-009：Epic 2 - 电量、网络、同步、自动模式和故障反馈；Epic 3 - 设备 UI 状态闭包
- FR-E-010：Epic 1 - 原生 USB-Serial-JTAG 烧录、CDC 日志与 JTAG 调试
- FR-E-011：Epic 2 - BOOT0 自动模式定时器、去抖、退出与持久化边界；Epic 3 - 自动模式 UI 投影；Epic 7 - 跨层一致性
- FR-C-001：Epic 4 - 单身份单设备映射与非目标治理边界
- FR-C-002：Epic 4 - canonical《心经》、scripture_version 与三端一致性
- FR-C-003：Epic 4 - 轮次/游标权威模型；Epic 5 - 轮次校验与幂等运营
- FR-C-004：Epic 5 - 完成锁定、从头开始、退出与历史保留
- FR-C-005：Epic 5 - 可信时间、确认日期归档与历史统计
- FR-C-006：Epic 4 - 快照、差量和恢复基准；Epic 5 - 断线补齐服务行为
- FR-C-007：Epic 5 - 高水位幂等同步与冲突错误；Epic 7 - 真实设备联调
- FR-C-008：Epic 5 - command_revision、待设备应用与已生效收敛；Epic 7 - 真机应用回传
- FR-C-009：Epic 5 - WebSocket 推送、序号和断线补齐；Epic 7 - 端到端重连验证
- FR-C-010：Epic 4 - JSON 零库原子持久化基线；Epic 5 - 业务状态落盘恢复
- FR-C-011：Epic 4 - /api/v1 信封、微信登录、JWT 与错误语义；Epic 5 - 业务接口验收
- FR-C-012：Epic 4 - 登录直达唯一设备；Epic 6 - 登录页与权限状态呈现
- FR-C-013：Epic 6 - 17 槽 append-only 阅读流与确认进度；Epic 7 - 真机字符收敛
- FR-C-014：Epic 6 - 在线逐字动画和最新字焦点；Epic 7 - 端到端实时呈现
- FR-C-015：Epic 5 - 差量回放语义；Epic 6 - 回放 UI/队列；Epic 7 - 真实断线恢复
- FR-C-016：Epic 5 - cloud 完成确认与轮次动作；Epic 6 - 完成弹窗/礼花；Epic 7 - 跨端完成验收
- FR-C-017：Epic 6 - 记录、设备和设置页面及命令镜像
- FR-C-018：Epic 6 - 空态、失败态、低电、待校时、权限和队列已满状态
- FR-C-019：Epic 6 - MINI-06 Pen/HTML 逐屏对拍与卫生门禁

NFR 主责分布：Epic 1 覆盖 NFR1/4/7 的硬件前置；Epic 2 覆盖 NFR1/3/4/7/8/9；Epic 3 覆盖 NFR2/8/9；Epic 4 覆盖 NFR4/5；Epic 5 覆盖 NFR2/3/4/5；Epic 6 覆盖 NFR2/3/4/6/9；Epic 7 对 NFR1/2/3/4/5/7/8 做最终联调验收。

## Epic List

### Epic 1: 设备可靠上电、唤醒与诊断

用户能够在定稿硬件上稳定开机、用实体敲击唤醒，并通过原生 USB 完成烧录和诊断。该 Epic 只使用板级自测和驱动替身，不依赖 cloud。
**FRs covered:** FR-E-001, FR-E-002, FR-E-010

### Epic 2: 无网也能连续敲击并保存进度

用户可以通过实体木鱼、屏幕木鱼或自动模式连续诵读；即使网络中断或设备重启，累计、轮次和设置仍可恢复，并获得音频、RGB、状态及 4G 活动窗口反馈。该 Epic 使用本地状态与 HTTPS mock，不等待真实 backend。
**FRs covered:** FR-E-003, FR-E-004, FR-E-005, FR-E-007, FR-E-008, FR-E-009, FR-E-011

### Epic 3: 在设备屏幕完成“一敲一字”诵读

用户能通过木鱼页、经文页、统计页和设置页理解当前字、进度、历史、同步及自动模式状态，并完成新轮次操作。该 Epic 使用本地状态注入和 UI self-test，不依赖 cloud。
**FRs covered:** FR-E-006, FR-E-009, FR-E-011

### Epic 4: 登录后获得唯一、可恢复的云端诵读基准

用户登录后直达唯一设备；系统拥有统一 canonical《心经》、轮次、快照、接口信封、身份和原子持久化基线。该 Epic 使用设备上报 mock，不等待固件。
**FRs covered:** FR-C-001, FR-C-002, FR-C-003, FR-C-006, FR-C-010, FR-C-011, FR-C-012

### Epic 5: 弱网或离线后仍能恢复进度、历史与设置

用户的高水位、轮次、统计和设备设置可在重复提交、断线、重启和离线修改后幂等收敛，并获得完成与回放语义。该 Epic 使用 mock device 和 mock event stream。
**FRs covered:** FR-C-003, FR-C-004, FR-C-005, FR-C-006, FR-C-007, FR-C-008, FR-C-009, FR-C-010, FR-C-011, FR-C-015, FR-C-016

### Epic 6: 在微信小程序持续阅读并管理设备

用户能在小程序登录、阅读已确认经文、查看记录和设备状态、修改设置，并处理空态、失败态、回放和完成状态。该 Epic 使用 mock API、WebSocket 和固定快照。
**FRs covered:** FR-C-012, FR-C-013, FR-C-014, FR-C-015, FR-C-016, FR-C-017, FR-C-018, FR-C-019

### Epic 7: 完成真机端到端诵读闭环

用户从三类设备输入到小程序逐字呈现的整个链路，在弱网、重连、命令应用、自动模式和完成跨轮场景下保持一致，并形成样机验收证据。该 Epic 依赖 Epic 1～6 的契约和独立实现，是唯一允许同时修改 Embedded 与 cloud 的 Epic。
**FRs covered:** FR-E-008, FR-E-011, FR-C-007, FR-C-008, FR-C-009, FR-C-013, FR-C-014, FR-C-015, FR-C-016

## Epic 1: 设备可靠上电、唤醒与诊断

用户能够在定稿硬件上稳定开机、用实体敲击唤醒，并通过原生 USB 完成烧录和诊断。该 Epic 只使用板级自测和驱动替身，不依赖 cloud。

### Story 1.1: 建立定稿电源、启动与 BSP 自检

As a 设备开发者，
I want 按硬件事实源完成电源、启动绑带、PWR、BOOT0、EN 和基础 BSP 初始化，
So that 设备能在各供电条件下安全启动并为后续输入验证提供稳定底座。

**覆盖需求：** FR-E-001、AD-8、AD-13

**Acceptance Criteria:**

**Given** `docs/hardware/` 中的 GPIO、电源和启动绑带基线已载入
**When** 固件执行 BSP 初始化和启动自检
**Then** PWR 只作为 LTC2954 `PWR_INT` 只读输入，BOOT0/EN 角色与硬件基线一致，固件不驱动 KILL、不模拟软件关机
**And** USB-C 外部供电、电池供电和外部供电并充电三种条件下均不因产品逻辑暂停输入、显示、音频或同步路径

**Given** 设备处于下载流程或运行态
**When** 分别执行“BOOT0 保持低电平后长按 PWR 上电”和运行态 BOOT0 短按
**Then** 前者进入下载采样，后者只产生运行态自动模式切换，不改变下一次下载路径
**And** 自检输出包含可追溯的引脚、供电和复位结果，不把设计输入误报为样机通过

### Story 1.2: 闭合 PVDF 唤醒与 ADC 二次确认

As a 设备使用者，
I want 实体敲击能可靠唤醒并被确认，
So that 普通取放和环境振动不会伪造诵读输入。

**覆盖需求：** FR-E-002、AD-1

**Acceptance Criteria:**

**Given** PVDF、比较器和 ADC 已按硬件网络清单连接
**When** 单次敲击、连续敲击和普通携带振动分别触发输入
**Then** 比较器事件只负责唤醒，ADC 二次确认后才产生候选有效输入，ADC 采样不越界
**And** QMI8658A 不参与 MVP 主计数，也不保存或上报原始波形

**Given** 输入确认模块运行自测
**When** 使用可复现的敲击/振动样本执行测试
**Then** 输出候选信号稳定性、误触发和阈值结果，供目标外壳演示签收
**And** 任何失败只报告候选输入失败，不直接增加正式累计

### Story 1.3: 启用 USB-Serial-JTAG 诊断路径

As a 固件开发者，
I want 使用 ESP32-S3 原生 USB-Serial-JTAG 完成烧录、CDC 日志和 JTAG 调试，
So that 设备不需要额外 USB 转换芯片且诊断链路稳定。

**覆盖需求：** FR-E-010、AD-11

**Acceptance Criteria:**

**Given** 设备通过 GPIO19/20 连接 USB
**When** 在主机上执行烧录、打开 CDC 日志并启动 JTAG 调试
**Then** 三种操作均可成功，日志模块 TAG 为稳定 ASCII 且正文为中文
**And** 工程不启用 CH340X 或 USB-OTG 业务

**Given** USB 调试连接保持期间
**When** 系统进入空闲/低功耗路径
**Then** 自动 light sleep 被按项目锚点禁用，CDC 不随机失联
**And** 诊断脚本保存烧录和日志回执，便于 Epic 2/3 自测复用

## Epic 2: 无网也能连续敲击并保存进度

用户可以通过实体木鱼、屏幕木鱼或自动模式连续诵读；即使网络中断或设备重启，累计、轮次和设置仍可恢复，并获得音频、RGB、状态及 4G 活动窗口反馈。该 Epic 使用本地状态与 HTTPS mock，不等待真实 backend。

### Story 2.1: 统一有效敲击队列与输入闸门

As a 设备使用者，
I want 实体敲击、木鱼点击和自动敲击走同一条输入链路，
So that 每次正式输入的计数、经文、反馈和同步语义一致。

**覆盖需求：** FR-E-003、AD-1、AD-7

**Acceptance Criteria:**

**Given** `physical_pvdf`、`device_touch` 或 `automatic_tap` 事件到达
**When** 输入服务处理事件
**Then** 三类事件进入同一个 typed queue，并且一次有效事件最多推进一个可消费汉字、累计只增加一次
**And** 除该队列外没有其它路径可以推进正式累计，事件来源写入可诊断记录

**Given** 设备处于完成遮罩、故障锁定或离线积压已满状态
**When** 任一输入到达
**Then** 事件按统一 gate 被忽略或拒绝并给出明确状态，不产生新字符或计数
**And** USB-C 外部供电、电池供电和外部供电并充电不会额外引入拒绝输入分支

### Story 2.2: 实现本地高水位事务持久化与离线积压

As a 设备使用者，
I want 设备在离线、重启和重复同步后仍保留正确进度，
So that 短时断网不会丢失或重复我的诵读。

**覆盖需求：** FR-E-004、AD-3、AD-15、NFR4、NFR5

**Acceptance Criteria:**

**Given** 一次有效敲击已通过输入 gate
**When** 设备提交本地状态更新
**Then** `local_total`、`acked_total`、`round_id`、`scripture_version`、`round_state`、`round_cursor`、`pending_completion` 和相关 `action_id` 作为同一持久化事务落盘
**And** 自动模式只保存在运行内存，重启后默认为关闭

**Given** 网络不可用、应用未打开或设备重启
**When** 设备继续接收敲击并随后恢复
**Then** 本地高水位和轮次可恢复，离线积压按 `local_total - acked_total` 计算，重复同步不会重复计数
**And** 积压达到 1000 次时三类输入统一拒绝并显示队列已满，不静默丢数据

### Story 2.3: 提供木鱼音频与低打扰反馈

As a 设备使用者，
I want 每次有效敲击获得可辨识的木鱼音、灯效和状态反馈，
So that 诵读节奏连续且不会被外设故障打断。

**覆盖需求：** FR-E-005、FR-E-009、AD-12

**Acceptance Criteria:**

**Given** 有效敲击进入队列且音频/LED 外设可用
**When** 事件完成确认
**Then** ES8311/NS4150B 输出木鱼音，RGB 和状态更新与同一事件关联，默认音量为 50/100
**And** 音量 0 等价静音，音量设置跨重启保留

**Given** 1 秒内连续发生 20 次输入或音频外设返回错误
**When** 反馈服务处理事件
**Then** 计数、持久化和同步全部继续，声音按短间隔限速/合并成可辨识节奏，不能叠加成噪声
**And** 屏幕、音频或 RGB 任一失败都产生低打扰故障状态，不阻塞核心队列

### Story 2.4: 实现触摸、PWR 导航与设备设置持久化

As a 设备使用者，
I want 用屏幕和 PWR 在主页、设置和唤醒状态之间切换，
So that 不打开手机也能完成基本操作。

**覆盖需求：** FR-E-007、AD-10、AD-15

**Acceptance Criteria:**

**Given** 设备亮屏或熄屏
**When** 用户左右滑动、下滑、短按 PWR 或触摸屏幕
**Then** 三主页循环、设置入口和熄屏唤醒行为符合契约；熄屏首触只唤醒，非木鱼页普通触摸不计数
**And** 木鱼页唯一计数触区产生 `device_touch`，完成遮罩/故障 gate 不被绕过

**Given** 用户修改亮度、熄屏时长、音量或点击立即同步
**When** 设置保存或动作完成
**Then** 值写入本地设置，默认值为中亮度、15 秒、音量 50，立即同步进入可观察状态
**And** 设备重启后设置保持，PWR 长按关机仍由板级电源负责

### Story 2.5: 实现 Air780EGP 活动窗口客户端与 HTTPS mock

As a 设备使用者，
I want 设备在需要时联网并在弱网下自动恢复，
So that 网络不会改变本地诵读的可靠性。

**覆盖需求：** FR-E-008、AD-14、NFR3

**Acceptance Criteria:**

**Given** 敲击、立即同步或待应用命令触发活动窗口
**When** Air780EGP 客户端执行 AT/HTTPS JSON 事务
**Then** 复用通信上下文上传高水位、轮次、设备状态和已应用命令修订，完成后可回到低功耗
**And** BLE/Wi-Fi 不被配置为业务回退，GPS 默认关闭，不连接模组独立业务云

**Given** 注册失败、超时、弱网或 HTTPS mock 返回错误
**When** 客户端执行退避与恢复
**Then** 本地继续记录并显示待同步/同步失败，网络恢复后补传，发射峰值不导致掉压重启
**And** mock 能覆盖成功、重复提交、低于高水位和冲突响应

### Story 2.6: 实现 BOOT0 自动模式

As a 设备使用者，
I want 短按 BOOT0 临时开启或关闭每 3 秒一次的自动敲击，
So that 可以用固定节奏演示完整诵读而不改变下载流程。

**覆盖需求：** FR-E-011、AD-7、AD-19、NFR8

**Acceptance Criteria:**

**Given** 设备处于开机运行态
**When** 用户完成一次防抖后的 BOOT0 短按/释放
**Then** 自动模式只切换一次，以释放确认时刻为周期锚点，每 3 秒生成一个 `automatic_tap`
**And** 连续 10 个周期的单周期误差不超过 ±100ms，长按、抖动、启动采样和下载流程电平不产生切换

**Given** 自动模式运行中、队列已满、完成或故障 gate 生效
**When** 周期到期或用户再次短按
**Then** 退出后不生成新事件，已入队事件按统一队列完成；gate 不被绕过，完成时停止定时器
**And** 模式不跨重启持久化，任意供电条件下均可使用

### Story 2.7: 保护低电和故障下的核心链路

As a 设备使用者，
I want 低电、显示故障或外设异常时仍不丢失计数，
So that 设备的关键诵读状态优先得到保护。

**覆盖需求：** FR-E-009、AD-13、NFR7、NFR9

**Acceptance Criteria:**

**Given** 电量低、4G 发射峰值或持久化即将执行
**When** 系统调度多个动作
**Then** 先完成累计和轮次落盘，再执行非关键反馈或通信，不使用软件重启/空循环模拟关机
**And** 低电状态通过低打扰文案/颜色/图标表达，不伪造充电检测结果

**Given** 设备按约 30 分钟/天敲击、其余时间待机的模型运行
**When** 执行活动窗口、显示和音频功耗测量
**Then** 记录电流、掉压、重启和待机证据，形成一天续航验收输入
**And** 失败时只按 PRD 风险策略收敛活动窗口、电池或默认亮度，不扩展 MVP 范围

## Epic 3: 在设备屏幕完成“一敲一字”诵读

用户能通过木鱼页、经文页、统计页和设置页理解当前字、进度、历史、同步及自动模式状态，并完成新轮次操作。该 Epic 使用本地状态注入和 UI self-test，不依赖 cloud。

### Story 3.1: 建立设备 UI 骨架与字体合同

As a 设备使用者，
I want 设备界面在固定画布上保持稳定的页面骨架和中文字形，
So that 每个状态都能被可靠阅读。

**覆盖需求：** FR-E-006、UX-DR1、UX-DR2、UX-DR4、UX-DR5、UX-DR14、AD-10

**Acceptance Criteria:**

**Given** DEVICE-01 Pen 和 UI_CONTRACT-device 已冻结
**When** 建立 LVGL 8.4/SquareLine 工程
**Then** 使用 410×502、圆角 110、16px 边距、≥8px 净空、24px 状态栏和三主页常驻 pager，设置页按需加载
**And** generated 只包含布局/字体/事件空桩，业务状态、路由和运行时补丁位于 bindings，所有 LVGL 调用由 `ui_task` 独占

**Given** 设备字体构建执行
**When** 生成 Noto Serif SC/Noto Sans SC 资源
**Then** 只包含《心经》可消费字形和必要 UI 字符，SquareLine 资产与 generated 字体字节一致
**And** 缺字检查覆盖静态标签、动态标签、状态投影和字体切换，禁止手工编辑位图

### Story 3.2: 实现木鱼页七字带与三环反馈

As a 设备诵读者，
I want 木鱼页在每次敲击时显示下一个字并给出短促光效，
So that 物理动作和视觉节奏保持一致。

**覆盖需求：** FR-E-006、UX-DR6、UX-DR7、UX-DR8、AD-19、NFR2

**Acceptance Criteria:**

**Given** 木鱼页处于可输入状态
**When** 收到 `physical_pvdf`、`device_touch` 或本地已确认状态
**Then** 新可消费汉字从右侧进入固定 7 槽，旧字左移，唯一 `glyph-current` 使用大字焦点，空位不预览未来字
**And** 标点随相邻汉字出现且不消耗敲击，进度显示已诵/总字数和百分比

**Given** 有效事件触发木鱼页反馈
**When** bindings 更新 `tap-rings`
**Then** 三道椭圆描边在 160ms 内 `idle→flash→idle`，flash 使用运行时琥珀色补丁，连续事件只重启光效不叠加节点
**And** 完成、故障或熄屏首触不触发正式光效

### Story 3.3: 实现经文页 13 槽 append-only 流

As a 设备诵读者，
I want 在经文页回看已经敲出的内容并始终找到最新字，
So that 连续诵读不会被页面切换打断。

**覆盖需求：** FR-E-006、UX-DR9、AD-19、NFR2

**Acceptance Criteria:**

**Given** 经文页已打开
**When** 设备本地记录新的可消费汉字
**Then** 字符追加到同一篇流尾，每行固定 13 槽，标点占槽，最新字以大字高亮，已有前文不覆盖
**And** 页面不绘制可点击木鱼，不预览未来经文

**Given** 用户向上回看或回看期间产生新敲击
**When** 滚动手势或新字事件完成
**Then** 用户只能浏览已确认/本地已记录内容；新字追加后视口自动锚回流尾
**And** 不生成第二篇历史流，滚动失败不阻塞输入与持久化

### Story 3.4: 实现设备统计页与诵读进度

As a 设备诵读者，
I want 在统计页看到今日、累计和当前诵读进度，
So that 我能理解当前轮次和长期积累。

**覆盖需求：** FR-E-006、FR-E-009、UX-DR10、AD-6

**Acceptance Criteria:**

**Given** 设备存在本地高水位或已同步状态
**When** 打开统计页
**Then** 显示今日敲击、跨 `round_id` 累计、已诵/总字数、百分比和当前诵读序号/状态
**And** 统计页不显示近 7 日、近 30 日或连续天数，这些指标只在小程序记录页出现

**Given** 设备尚未取得可信时间或当前轮次到达末字
**When** 统计值投影到页面
**Then** 今日区域显示“待校时”或确认后的值，百分比保持 0–100 且完成后保持 N/N
**And** 本地未确认差值使用“同步中/待同步”标记，不混入云端正式统计

### Story 3.5: 实现设置页与同步状态闭包

As a 设备使用者，
I want 在设置页修改音量、亮度、熄屏和立即同步并看见结果，
So that 我能控制设备而不误解命令是否生效。

**覆盖需求：** FR-E-007、FR-E-009、UX-DR11、UX-DR13、UX-DR26、AD-5

**Acceptance Criteria:**

**Given** 用户进入下滑设置页
**When** 查看或修改控件
**Then** 首屏最多 4 行，亮度为低/中/高、熄屏为 5/15/30 秒、音量为 0–100，第二页显示木鱼版本和木鱼 ID
**And** 结构复用同一骨架，默认值为中亮度、15 秒和音量 50

**Given** 用户点击立即同步或同步请求处于不同阶段
**When** `sync-btn` 状态改变
**Then** 仅切换 busy/ok/pending/fail 文案和状态色，不复制整屏，并与累计同步状态词表一致
**And** PRD final 的三种供电条件均保持功能可用；旧 `CHARGING_PAUSE` 变体在契约消解前不得实现为输入暂停 gate

### Story 3.6: 实现完成遮罩与跨轮操作

As a 设备诵读者，
I want 完成《心经》后能明确选择重新开始或退出，
So that 当前篇章和历史积累都不会被误操作覆盖。

**覆盖需求：** FR-E-006、FR-E-007、UX-DR12、AD-19

**Acceptance Criteria:**

**Given** 本地轮次到达末字并进入完成锁定
**When** UI 投影完成状态
**Then** 木鱼页显示完成遮罩，冻结所有输入，提供“从头开始”和“退出”两个动作
**And** 遮罩不创建第二套页面骨架，复用 `modal-done` 和现有状态栏/进度组件

**Given** 用户选择从头开始或退出
**When** 动作通过 `action_id` 执行
**Then** 从头开始创建新 `round_id` 并回到首字，退出保留完成态、旧篇章和累计统计，重复动作幂等
**And** 自动模式在完成时停止，新轮次必须再次显式启用

## Epic 4: 登录后获得唯一、可恢复的云端诵读基准

用户登录后直达唯一设备；系统拥有统一 canonical《心经》、轮次、快照、接口信封、身份和原子持久化基线。该 Epic 使用设备上报 mock，不等待固件。

### Story 4.1: 建立 canonical《心经》与版本校验

As a 产品维护者，
I want 三端从同一个 canonical 文本源生成可消费汉字序列，
So that 设备、backend 和小程序不会因经文漂移而错位。

**覆盖需求：** FR-C-002、AD-4、UX-DR4

**Acceptance Criteria:**

**Given** 项目准备构建或打包经文资源
**When** 生成 Embedded/backend/frontend 资源
**Then** 所有端使用同一落盘来源和 `scripture_version`，标点/空格/换行的消费语义可被校验
**And** 任一端版本不一致时构建或运行检查失败，不自动映射、选经或导入

**Given** canonical 文本被更新
**When** 执行版本变更检查
**Then** 变更包含可追溯的版本号、字符总数和影响说明
**And** 不把设备 7 槽或小程序 17 槽误当成经文长度上限

### Story 4.2: 冻结跨层同步契约

As a Embedded/cloud 开发者，
I want 把字段、事件、序号和跨轮边界写入统一同步契约，
So that 并行实现可以使用同一协议而不各自猜测。

**覆盖需求：** FR-C-003、FR-C-006、AD-2、AD-3、AD-5、AD-6、AD-19

**Acceptance Criteria:**

**Given** `docs/contracts/sync-contract.md` 当前只有 README 占位
**When** 创建契约正文
**Then** 定义 `device_id`、高水位、轮次、状态、命令修订、设备状态、`snapshot_seq`、`replay_cursor` 和 `action_id`
**And** 明确 `physical_pvdf`、`device_touch`、`automatic_tap` 三类来源及累计/轮次/完成语义

**Given** 设备离线、完成未确认、设备重置、WebSocket 空洞或命令重复
**When** 契约描述恢复和错误路径
**Then** 明确按 `round_id` 分段、`pending_completion`、冲突恢复、快照水位续订、幂等重放和拒绝条件
**And** WebSocket 帧、心跳和重连参数留有可测试的版本化字段，不在各层重复定义

### Story 4.3: 建立 backend 启动、接口信封与环境基线

As a backend 开发者，
I want 在固定目录启动 EWF backend 并提供统一接口信封，
So that设备 mock 和小程序 mock 可以独立联调。

**覆盖需求：** FR-C-010、FR-C-011、AD-16、AD-20

**Acceptance Criteria:**

**Given** `cloud/backend` 是绿地目录
**When** 使用 Java 17、Spring Boot 3.3.7 基线和 Maven 启动本地 profile
**Then** 服务读取 `application.yml`/`application-local.yml`，监听 9218，所有公开接口位于 `/api/v1`
**And** 成功返回 `{code:0,message,data}`，业务错误保持 HTTP 200+业务码，协议错误使用对应 HTTP 状态

**Given** 本地、生产和 mock 客户端分别调用接口
**When** 检查配置和响应
**Then** 不读取 miaowu 的支付/数据库/OBS 配置，不启动第二套 backend，敏感会话信息不写入日志
**And** `cloud/frontend` 的 `VITE_API_BASE_URL` 与 backend 路径约定可以直接对接

### Story 4.4: 实现微信登录与单设备身份

As a 唯一设备用户，
I want 登录后直接得到我的固定设备上下文，
So that 我不需要处理绑定、迁移或多设备选择。

**覆盖需求：** FR-C-001、FR-C-011、FR-C-012

**Acceptance Criteria:**

**Given** 微信 code、AppID 和设备身份映射已配置
**When** 首次登录、重复登录或令牌刷新
**Then** backend 按 WechatMiniClient/JWT 模式返回唯一设备身份，重复登录得到同一映射
**And** 不提供绑定、解绑、换机迁移或设备选择入口

**Given** code 无效、权限失败、身份不匹配或并发请求同时过期
**When** backend/client 处理鉴权
**Then** 返回可执行的权限/令牌错误，客户端只允许一个刷新请求并唤醒所有等待请求
**And** 不同身份之间隔离高水位、轮次和统计

### Story 4.5: 建立 JSON 原子持久化基线

As a 设备用户，
I want backend 重启后保留我的云端状态，
So that服务进程重启不会清空诵读记录。

**覆盖需求：** FR-C-010、AD-16、NFR5

**Acceptance Criteria:**

**Given** backend 需要保存身份、高水位、轮次、命令、设备状态和统计
**When** 写入 `cloud/backend/data/`
**Then** 使用临时文件、`fsync` 和 `rename` 原子替换，不引入 SQLite、PostgreSQL 或其他数据库
**And** 单实例单进程下每个文件的 schema/粒度有版本和迁移说明

**Given** 写入中断、进程重启或 JSON 内容损坏
**When** backend 启动恢复
**Then** 不读取半写文件，不静默回退到错误高水位，返回可诊断恢复错误
**And** 正常重启后所有已确认状态、配置和历史统计恢复一致

### Story 4.6: 提供状态快照与恢复基准

As a 小程序或设备客户端，
I want 获取带快照水位的权威状态和差量，
So that断线后能从确定位置继续而不是猜计数。

**覆盖需求：** FR-C-006、AD-2、AD-17

**Acceptance Criteria:**

**Given** backend 已有已确认状态和单调序号
**When** 客户端调用状态查询
**Then** 响应同时包含权威快照、差量、`snapshot_seq`、轮次和命令状态
**And** 查询响应冻结的水位可作为 WebSocket 续订和回放的唯一基准

**Given** 客户端提交已展示计数或请求存在序号空洞
**When** backend 校验恢复请求
**Then** 不接受客户端自报展示计数作为权威，空洞先返回查询补齐要求
**And** 旧轮次事件不能写入当前轮次

## Epic 5: 弱网或离线后仍能恢复进度、历史与设置

用户的高水位、轮次、统计和设备设置可在重复提交、断线、重启和离线修改后幂等收敛，并获得完成与回放语义。该 Epic 使用 mock device 和 mock event stream。

### Story 5.1: 实现高水位幂等同步

As a 设备用户，
I want 同一批离线数据重复提交也只被确认一次，
So that 网络重试不会制造重复敲击。

**覆盖需求：** FR-C-007、AD-3、NFR4

**Acceptance Criteria:**

**Given** mock device 提交 `local_total`、`acked_total`、轮次、游标和状态
**When** backend 处理高水位
**Then** 只对高于已确认值的差量确认一次，同值或更低值为 no-op，并返回最新确认高水位和差量
**And** 响应包含确认后的轮次状态、待应用命令和可对账字段

**Given** 设备身份、轮次边界或基准高水位冲突
**When** backend 校验提交
**Then** 返回明确业务错误和可恢复冲突状态，不盲推游标、不静默回退、不无限重试
**And** 同步行为可用 mock 测试覆盖重复、乱序、降低高水位和跨轮提交

### Story 5.2: 实现轮次、完成与跨轮动作

As a 设备用户，
I want完成《心经》后从头开始而不覆盖旧篇章，
So that 每次诵读和历史都能被区分。

**覆盖需求：** FR-C-003、FR-C-004、FR-C-016、AD-19

**Acceptance Criteria:**

**Given** backend 收到末字对应的设备轮次和游标
**When** 边界校验成功
**Then** 只在确认后将轮次置为完成，保存 `pending_completion` 语义并返回完成状态
**And** 完成前不允许新轮次覆盖未确认轮次

**Given** 客户端以 `action_id` 请求从头开始或退出
**When** 请求重复、乱序或重试
**Then** 从头开始只创建一个新 `round_id`，退出保留完成态和历史，重复请求返回同一结果
**And** 完成锁定期间任何新的正式输入都不推进游标

### Story 5.3: 实现可信时间和历史统计

As a 设备用户，
I want 今日、近 7 日、近 30 日、累计和连续天数基于已确认数据计算，
So that离线补传不会被错误归档。

**覆盖需求：** FR-C-005、AD-6、NFR4

**Acceptance Criteria:**

**Given** backend 收到在线或离线差量
**When** 按配置时区确认并归档
**Then** 今日/周期/累计/连续统计只使用已确认敲击，并按确认日期归档离线差量
**And** 设备未取得可信时间时不生成虚假的当天记录

**Given** 查询统计时没有记录或存在待同步数据
**When** frontend 请求记录
**Then** 返回明确空态/待同步状态，不把本地未确认差值算入正式统计
**And** 统计结果在重启和重复查询后保持稳定

### Story 5.4: 实现命令修订和待设备应用收敛

As a 设备用户，
I want 离线修改设置后最终只应用最新版本，
So that设备页不会把旧设置误报为已生效。

**覆盖需求：** FR-C-008、AD-5

**Acceptance Criteria:**

**Given** 用户连续修改音量、亮度、熄屏或篇章动作
**When** backend 保存命令
**Then** 只保留最新 `command_revision`，命令携带可重放 `action_id`，旧修订不能覆盖新修订
**And** frontend 在设备未确认前显示“待设备应用”

**Given** 设备在活动窗口回传已应用修订
**When** `applied_revision >= current_revision`
**Then** backend 幂等地将状态收敛为“已生效”，不依赖单次 ACK 到达
**And** 重复获取/回传命令不会改变最终值

### Story 5.5: 实现 WebSocket 差量推送

As a 小程序用户，
I want在线时按确认顺序收到逐字差量和设备状态，
So that阅读流可以及时更新而不跳字。

**覆盖需求：** FR-C-009、AD-17、NFR2、NFR3

**Acceptance Criteria:**

**Given** backend 确认了新的高水位或设备状态
**When** WebSocket 连接正常
**Then** 推送带单调序号的差量、当前字、游标、轮次、完成、同步和设备状态
**And** WebSocket 内容与同一 `snapshot_seq` 的查询结果一致

**Given** 连接断开、重连或客户端序号落后
**When** 客户端请求恢复
**Then** backend 依据查询冻结水位补齐未展示差量，再从水位+1 继续推送
**And** `seq <= last_applied_seq` 的消息可丢弃，空洞不得直接播放

### Story 5.6: 实现错误语义、重试与安全边界

As a 小程序用户，
I want同步失败和协议错误有清晰、可重试的结果，
So that我不会把失败误解成已成功。

**覆盖需求：** FR-C-011、AD-16、AD-18

**Acceptance Criteria:**

**Given** 请求发生参数错误、身份错误、业务冲突、上游超时或服务器异常
**When** GlobalExceptionHandler 处理异常
**Then** 按统一信封返回业务码/HTTP 状态，业务错误不伪装成成功数据
**And** 失败响应包含可执行的重试或重新登录语义，不暴露 session_key/JWT 明文

**Given** frontend 同时收到多个 401
**When** 令牌刷新成功或失败
**Then** 只发起一次刷新，所有等待请求被唤醒；刷新后每个请求最多重试一次，仍失败则只跳转一次登录入口
**And** 日志和持久化不记录敏感会话信息

### Story 5.7: 验证业务状态重启与原子恢复

As a 设备用户，
I want同步、完成、命令和统计在 backend 重启后仍然一致，
So that服务维护不会破坏诵读。

**覆盖需求：** FR-C-010、FR-C-011、NFR4、NFR5

**Acceptance Criteria:**

**Given** backend 在高水位、轮次、命令和统计更新过程中被停止
**When** 进程重新启动
**Then** JSON 恢复后 `local/acked` 对账、`round_id`/游标/完成、命令修订和统计不互相矛盾
**And** 半写临时文件不会覆盖最后一个完整快照

**Given** 重启后 mock device 重复发送最后一次请求
**When** backend 再次处理
**Then** 请求保持幂等，不重复增加差量或创建新轮次
**And** 恢复测试保存输入、输出和文件校验作为验收证据

## Epic 6: 在微信小程序持续阅读并管理设备

用户能在小程序登录、阅读已确认经文、查看记录和设备状态、修改设置，并处理空态、失败态、回放和完成状态。该 Epic 使用 mock API、WebSocket 和固定快照。

### Story 6.1: 建立 MINI-06 小程序壳层、令牌和导航

As a 小程序用户，
I want 页面在冻结纸面和底部导航上保持一致，
So that我能在阅读、记录、设备和设置之间稳定切换。

**覆盖需求：** UX-DR1、UX-DR3、UX-DR16、UX-DR22、AD-18

**Acceptance Criteria:**

**Given** MINI-06 Pen/UI contract 已冻结
**When** 建立 uni-app Vue 3 + TypeScript + Vite + Pinia 壳层
**Then** 使用 390×844、62px 安全区、20px 页边距、56px 底部导航和纸面根帧，不添加外层展示背景
**And** 生产 `.env`、开发 `.env.development` 和统一 `VITE_API_BASE_URL` 路径可被构建检查

**Given** 用户在任一一级页面
**When** 点击底部导航
**Then** `阅读/记录/设备/设置` 四项始终可达、当前项有焦点色、每个触区不小于 44×44
**And** 页面状态存放在 Pinia/本地存储但不被当作 cloud 权威

### Story 6.2: 实现登录直达与权限状态

As a 小程序用户，
I want 首次打开时完成登录并直接进入唯一设备，
So that不需要额外绑定步骤。

**覆盖需求：** FR-C-012、UX-DR16、UX-DR21

**Acceptance Criteria:**

**Given** 用户首次打开、重复打开或授权失败
**When** 执行微信登录流程
**Then** 成功后直达阅读页/唯一设备，不出现绑定、迁移或设备选择页
**And** 权限失败显示一句话原因和可执行的重新授权/重试动作

**Given** 登录请求返回后端业务错误或令牌过期
**When** 页面恢复
**Then** 使用统一 API 信封解释错误并避免重复跳转
**And** 页面文案不包含实现说明、调试标签或内部节点标识

### Story 6.3: 实现在线心经持续阅读流

As a 小程序用户，
I want 只看到 backend 已确认并按顺序追加的经文，
So that手机阅读与设备诵读保持一致。

**覆盖需求：** FR-C-013、FR-C-014、UX-DR15、AD-17、AD-19、NFR2

**Acceptance Criteria:**

**Given** 收到确认差量或当前快照
**When** 阅读页渲染新字
**Then** 正文按序 append 到固定 17 槽行，每个字符只消费一次，已有前缀不覆盖、不清空，标点占槽
**And** 最新字为 28px 焦点色并带持续 2px 下划线，正文槽为 18px，只保留一组总进度

**Given** 差量队列积压或为空
**When** 动画调度执行
**Then** 可缩短插值追平但不得跳过、合并或重复字符，队列清空后恢复基线节奏
**And** 小程序没有可点击电子木鱼，也不预览未来经文

### Story 6.4: 实现断线查询与离线回放

As a 小程序用户，
I want 重新打开或断网后完整回放已确认进度，
So that恢复连接不会跳字或重复。

**覆盖需求：** FR-C-015、FR-C-006、UX-DR17、AD-17、NFR4

**Acceptance Criteria:**

**Given** WebSocket 断开、应用重开或本地保存 `replay_cursor`
**When** 页面进入 REPLAY/OFFLINE 状态
**Then** 先使用查询冻结快照水位补齐差量，显示“回放中·新事件排队”或断线 banner
**And** 回放期间新事件排队，不插入旧差量，回放结束后无缝切换实时

**Given** 出现序号空洞、重复消息或旧轮次事件
**When** 消费队列
**Then** 先查询补齐，丢弃已消费序号，拒绝把旧轮次写入当前正文
**And** 本地展示计数不能作为权威恢复基准

### Story 6.5: 实现完成态、礼花和篇章归档

As a 小程序用户，
I want 在 cloud 确认完成后查看全文并选择下一步，
So that新旧篇章都能保留。

**覆盖需求：** FR-C-016、UX-DR18、AD-19

**Acceptance Criteria:**

**Given** backend 返回当前轮次完成确认
**When** 阅读页收到完成状态
**Then** 保留全文和 100% 进度，显示短促礼花及独立完成弹窗，不提前在待确认状态显示完成
**And** 弹窗期间不追加新正式字符

**Given** 用户点击从头开始或退出
**When** 通过 `action_id` 调用动作
**Then** 从头开始在下方建立新 `round_id` 区块，旧篇章可折叠；退出保留完成态和历史统计
**And** 重复点击不会创建多个轮次

### Story 6.6: 实现记录页统计

As a 小程序用户，
I want 查看今日、周期、累计和连续诵读统计，
So that我能回看长期使用情况。

**覆盖需求：** FR-C-017、UX-DR19、UX-DR22、AD-6

**Acceptance Criteria:**

**Given** backend 返回已确认统计或无历史
**When** 打开记录页
**Then** 显示今日、近 7 日、近 30 日、累计和连续天数，单位和空态明确
**And** 统计只来自 backend 已确认数据，本地待同步差量不进入任何数字

**Given** 统计请求加载、失败或刷新
**When** 用户下拉刷新或重试
**Then** 页面分别呈现 loading/empty/fail/refreshed 状态，失败可再次操作
**And** 使用无盒化账本行、冻结令牌和低对比分隔，不用装饰背景伪造数据层级

### Story 6.7: 实现设备状态页与同步动作

As a 小程序用户，
I want 查看设备电量、4G、最后同步和待处理状态，
So that我能判断设备是否需要操作。

**覆盖需求：** FR-C-017、FR-C-018、UX-DR19、UX-DR21、AD-2

**Acceptance Criteria:**

**Given** backend 返回设备快照
**When** 打开设备页
**Then** 显示电量、4G、最后同步、待同步数、待设备应用和失败状态，状态同时使用图标与文字
**And** 不展示主控无法确认的充电状态，不把待同步伪装为已同步

**Given** 用户点击刷新、立即同步或失败重试
**When** 请求处于 busy/pending/fail/ok
**Then** `sync-action` 展示真实阶段，动作可重复且不会并发制造重复请求
**And** 空态/失败态说明“是什么+能做什么”，不显示伪造数字

### Story 6.8: 实现设置镜像与待应用状态

As a 小程序用户，
I want 修改设备音量、亮度和熄屏时长并知道何时生效，
So that离线命令不会被误认为已应用。

**覆盖需求：** FR-C-017、FR-C-008、UX-DR20、AD-5

**Acceptance Criteria:**

**Given** 用户进入设置页
**When** 操作音量、亮度或熄屏控件
**Then** 使用 0–100 滑杆、低/中/高分段和 5/15/30 秒分段，选中态和当前值同时可见
**And** 默认值为音量 50、亮度中、熄屏 15 秒，控件符合 MINI-06 对拍基线

**Given** 命令已提交但设备尚未回传应用修订
**When** 页面刷新或收到状态推送
**Then** 显示“待设备应用”，只有 `applied_revision` 达到目标才显示“已生效”
**And** 旧修订、重复提交和离线失败不会覆盖最新选择

### Story 6.9: 完成空态、失败态、权限态和视觉卫生门禁

As a 小程序用户，
I want 在所有非正常状态下仍得到清晰、可操作的页面，
So that错误不会破坏阅读体验。

**覆盖需求：** FR-C-018、FR-C-019、UX-DR21、UX-DR23、UX-DR24、UX-DR25、UX-DR26、NFR9

**Acceptance Criteria:**

**Given** 首登、无历史、离线、同步失败、队列已满、低电量、待校时、待设备应用、完成或权限错误出现
**When** 页面进入对应状态
**Then** 六类页面/14 个状态均有可见、可操作且不依赖颜色单独传达的状态表达
**And** 小程序正文/底对比满足目标 ≥7:1，关键动作提供可读文本，减少动效时保留一次状态确认

**Given** 运行逐屏对拍和卫生扫描
**When** 检查 Pen/HTML、布局、令牌、字体、导航触区和页面文案
**Then** 通过 MINI-06 14 状态对拍，页面不含 AI 思维链、作者说明、Node ID、`data-pencil-id`、TODO、draft、placeholder、调试标签或重复进度
**And** 只有冻结 Pen/HTML 作为视觉真源，历史候选稿和临时导出物不进入实现

## Epic 7: 完成真机端到端诵读闭环

用户从三类设备输入到小程序逐字呈现的整个链路，在弱网、重连、命令应用、自动模式和完成跨轮场景下保持一致，并形成样机验收证据。该 Epic 依赖 Epic 1～6 的契约和独立实现，是唯一允许同时修改 Embedded 与 cloud 的 Epic。

### Story 7.1: 对接 Air780EGP 真机 HTTPS 活动窗口

As a 设备用户，
I want 真机输入能在活动窗口内被 backend 确认，
So that离线优先链路最终能够连成闭环。

**覆盖需求：** FR-E-008、FR-C-007、FR-C-008、AD-14、NFR3、NFR4

**Acceptance Criteria:**

**Given** Epic 1/2/4/5 的接口契约、设备身份和 HTTPS 配置已冻结
**When** 真机发生实体、触摸或自动敲击
**Then** Air780EGP 通过 UART AT/HTTPS JSON 上报高水位、轮次、状态和已应用修订，backend 返回确认和待应用命令
**And** 连接只在活动窗口复用，不启用 BLE/Wi-Fi 回退或常开长连接

**Given** 网络中断、超时、低电或模组发射峰值
**When** 真机执行恢复流程
**Then** 本地累计优先落盘，状态显示待同步/失败，网络恢复后只补传缺口且不掉压重启
**And** 保存 AT、HTTP、退避、耗时、电流和重启证据

### Story 7.2: 验证命令修订从小程序到设备生效

As a 小程序用户，
I want 设置修改最终在设备上应用并回显，
So that两端的设置状态能够收敛。

**覆盖需求：** FR-C-008、UX-DR20、AD-5、NFR4

**Acceptance Criteria:**

**Given** 小程序提交新的 `command_revision`
**When** 设备下一次 HTTPS 活动取得命令
**Then** 设备按修订号原子应用音量/亮度/熄屏设置，并在同一或后续包回传已应用高水位
**And** 设备重启后设置和已应用修订保持一致

**Given** 旧命令、重复响应或活动窗口失败
**When** backend/frontend 处理状态
**Then** 旧修订不能覆盖新修订，页面保持“待设备应用”，收到满足条件的修订后才变为“已生效”
**And** 命令 action 重试不会重复改变设备状态

### Story 7.3: 验证 WebSocket 与快照回放闭环

As a 小程序用户，
I want 真机确认结果在断线重连后继续逐字呈现，
So that正式字符不会丢失、重复或乱序。

**覆盖需求：** FR-C-009、FR-C-015、UX-DR17、AD-17、NFR2、NFR4

**Acceptance Criteria:**

**Given** 真机上报已被 backend 确认并产生 `snapshot_seq`
**When** WebSocket 在线推送或中途断开
**Then** frontend 按快照水位接收并消费差量，断线期间新事件排队，重连后先查询补齐再续订
**And** 已展示序号不重复播放，空洞和旧轮次事件不会写入当前篇章

**Given** 设备离线积压后恢复
**When** backend 返回有序差量
**Then** 小程序先回放旧差量，再自然接入实时新增；设备本地状态与 cloud 确认状态通过同步词表可见地区分
**And** 形成网络包、快照、replay_cursor 和页面字符的对账记录

### Story 7.4: 验证高峰输入、动画连续性和端到端实时性

As a 设备使用者，
I want 连续敲击时设备和小程序仍保持完整节奏，
So that高速诵读不会漏字、重字或明显延迟。

**覆盖需求：** FR-E-003、FR-E-008、FR-C-013、FR-C-014、NFR1、NFR2、NFR3

**Acceptance Criteria:**

**Given** 真机以 1 秒 20 次输入并保持稳定 4G
**When** 连续运行设备队列、音频、HTTPS、backend 和 frontend
**Then** 所有正式输入恰好计数一次，设备 7 字带和小程序 17 槽流不跳字/重字，音频合并为可辨识节奏
**And** 稳定网络下至少 95% 事件在发生后 1 秒内完成小程序展示

**Given** 网络弱化、动画积压或渲染失败
**When** 系统触发追平和降动效策略
**Then** 字符队列完整保留，动画可加速但不合并字符，失败不阻塞持久化和同步
**And** 测试输出事件序列、确认时间、渲染时间和漏/重/乱序结果

### Story 7.5: 验证完成、自动模式与跨轮一致性

As a 设备和小程序用户，
I want 自动模式和真实敲击在完成及新轮次时保持同一语义，
So that不同输入来源不会产生不同历史结果。

**覆盖需求：** FR-E-011、FR-C-004、FR-C-016、AD-3、AD-19、NFR8

**Acceptance Criteria:**

**Given** 真机自动模式、实体敲击和设备触摸分别推进到末字
**When** backend 确认完成并执行从头开始/退出
**Then** 三类输入得到相同完成锁定、统计、历史、cloud 状态和小程序弹窗结果
**And** 自动模式完成时停止，新的 `round_id` 只由显式动作创建，旧篇章仍可回看

**Given** 完成确认延迟、动作重复或设备重启发生在跨轮边界
**When** 同步合同处理 `pending_completion` 和 `action_id`
**Then** 不会把旧轮次差量写入新轮次，不创建重复轮次，也不静默回退高水位
**And** 保存设备、backend、frontend 三端的 round/游标/完成对账证据

### Story 7.6: 完成样机验收与风险回写

As a 项目维护者，
I want 用一套可复核证据验收核心体验，
So that后续实现不会把编译通过误当成产品完成。

**覆盖需求：** NFR1、NFR2、NFR3、NFR4、NFR5、NFR6、NFR7、NFR8、NFR9、AD-13、UX-DR24

**Acceptance Criteria:**

**Given** Epic 1～7 的实现和 mock/真机测试完成
**When** 执行输入可靠性、离线恢复、端到端实时、续航、视觉对拍和状态闭包验收
**Then** 每项结果记录输入条件、设备/服务版本、原始日志、截图/视频或测量数据及结论
**And** 只有真实验证通过的项标记为通过，设计输入、编译成功和文档一致性不替代样机证据
**And** 首轮样机结束后记录创作者是否愿意持续使用的回看结果，单独标注为 SM-6，不把工程期主观判断写成已验证事实

**Given** 20 次/秒、Air780 发射峰值、一天续航或 1 秒实时目标未通过
**When** 按 PRD §14 风险策略复盘
**Then** 只收敛节流、活动窗口、电源/电池、默认亮度或实时承诺，不静默扩大产品范围
**And** 风险、决定和证据回写 `docs/hardware/`、架构主干或对应规格文档
