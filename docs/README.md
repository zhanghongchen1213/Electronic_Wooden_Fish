# Electronic_Wooden_Fish — docs 索引

> 治理范式沿用以 `legbot_watch/docs/README.md` 为蓝本的「事实优先级」。本文档只做索引与分层说明；各分层文档自行持有其权威内容，不互相复制。

## 事实优先级（从高到低，冲突时高者覆盖低者）

| 层 | 文档位置 | 权威内容 |
| --- | --- | --- |
| 产品行为 | `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/{prd.md,addendum.md}` | FR（FR-E/B/F）、UJ、SM、跨层同步字段、MVP 边界 |
| 架构不变量 | `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` | AD-1~19、板级 GPIO 合同、结构 seed、技术栈、同步契约前置项 |
| 史诗/故事 | `_bmad-output/planning-artifacts/epics.md` | 53 个 story（E1–E4 / S0–S4），实施与验收依据 |
| UX/视觉 | `_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/` | DESIGN.md / EXPERIENCE.md / UI_CONTRACT-{device,miniapp}.md（当前 draft） |
| 设备实现/排障 | `docs/embedded/` | 固件实现规格与从 legbot 迁移的踩坑知识（见其 README） |
| 交接 | `docs/handoffs/` | 面向后续 agent 的当前状态与任务（见 2026-09-08…handoff.md） |

## 目录导览

- `handoffs/` — BMAD 阶段与任务交接文档（当前：UI 设计移交后续视觉 agent）。
- `embedded/` — 设备侧（Embedded/固件）实现与排障文档；多篇从 `legbot_watch/docs` 迁移/适配（详见其 README 的来源与排除表）。
- `contracts/` — 跨层同步契约 `sync-contract.md`（**待 S0 冻结**，见 `epics.md` Story S0.1；本目录当前仅占位说明）。
- `backend/`、`frontend/` — 后端/小程序**开发经验文档**（自 miaowu 迁移，见各 README）；PRD addendum §4 期望的模块实现规格 `requirements.md` 仍待对应 spec 阶段创建。

## 事实流规约

- 分层文档不得复制完整上游文档，也不得自行定义新事件来源、经文游标或同步状态（新语义一律先进 sync-contract / spine）。
- legbot_watch 是**只读蓝本**（本仓 `docs/embedded/` 与根 `AGENTS.md` 的设备规则据此迁移），不在本仓改 legbot。
