# Electronic_Wooden_Fish — docs 索引

> 治理范式沿用以 `legbot_watch/docs/README.md` 为蓝本的「事实优先级」。本文档只做索引与分层说明；各分层文档自行持有其权威内容，不互相复制。

最终 UI 文案治理见根 `AGENTS.md` §3.7；`CLAUDE.md` 镜像同一硬禁令。设计/实现说明不得进入用户可见页面。

## 事实优先级（从高到低，冲突时高者覆盖低者）

| 层 | 文档位置 | 权威内容 |
| --- | --- | --- |
| 产品简报 | `_bmad-output/planning-artifacts/briefs/brief-Electronic_Wooden_Fish-2026-09-08/brief.md` | 产品目标、用户问题、范围和成功标准 |
| 产品行为 | `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md` | FR（FR-E/B/F）、UJ、SM、跨层同步字段、MVP 边界 |
| 架构不变量 | `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` | AD-1~19、跨层边界、结构 seed、技术栈、同步契约前置项 |
| 史诗/故事 | 审查通过后由 `bmad-create-epics-and-stories` 重新生成 | 当前刻意未创建，避免在基线审查前固化过期拆分 |
| UX/视觉 | `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/` | DESIGN.md / EXPERIENCE.md / UI_CONTRACT-{device,miniapp}.md（final；两份 Pen 为冻结真源） |
| 设备实现/排障 | `docs/embedded/` | 固件实现规格与从 legbot 迁移的踩坑知识（见其 README） |
| 硬件原理图基线 | `docs/hardware/` | GPIO/I²C/电源/FPC/网络清单与嘉立创 EDA 分层落图记录；`[必须样机验证]` 不得写成样机已通过 |
| 交接 | `docs/handoffs/` | 面向后续 agent 的当前状态与任务（见 2026-09-08…handoff.md） |

## 目录导览

- `handoffs/` — BMAD 阶段与任务交接文档（当前：UI 设计移交后续视觉 agent）。
- `embedded/` — 设备侧（Embedded/固件）实现与排障文档；多篇从 `legbot_watch/docs` 迁移/适配（详见其 README 的来源与排除表）。
- `contracts/` — 跨层同步契约 `sync-contract.md`（**待后续 spec/S0 阶段冻结**；本目录当前仅占位说明）。
- `backend/`、`frontend/` — 后端/小程序**开发经验文档**（自 miaowu 迁移，见各 README）；模块实现规格 `requirements.md` 仍待对应 spec 阶段创建。
- `hardware/` — 硬件原理图设计基线、机器网络清单和原厂/厂商外围电路资料索引；两份用户提供的 PDF 仅作只读参考，不覆盖原文件。

## 事实流规约

- 分层文档不得复制完整上游文档，也不得自行定义新事件来源、经文游标或同步状态（新语义一律先进 sync-contract / spine）。
- 板级 GPIO、电源、FPC、器件连接、I²C 地址和硬件验证边界只在 `docs/hardware/` 维护；架构主干只引用硬件事实，不重复板级合同。
- legbot_watch 是**只读蓝本**（本仓 `docs/embedded/` 与根 `AGENTS.md` 的设备规则据此迁移），不在本仓改 legbot。
