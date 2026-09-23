---
name: bmad-epic-autopilot
description: 'Autonomously drive an entire BMad epic through create-story → dev-story → code-review for every backlog story until all reach done, using a fresh isolated subagent per phase. Use when the user provides an epic identifier to auto-run, e.g. "epic7", "autopilot epic7", "自动跑 epic 7", "run epic 8 end to end".'
---

# Epic Autopilot Workflow

**Goal:** 输入一个 epic 号，全程无人监管，自动把该 epic 下所有未完成的 story 依次走完 `create-story → dev-story → 按风险选择审查深度的 code-review`，直到每个 story 都 `done`。测试、审查、实现、构建和证据问题优先进入自动恢复；只有模型/工具、文件系统、必要外部凭证或无法推导的仓库事实源确实不可用时，才生成结构化外部阻断，不等待人类。

**Your Role:** 你是**编排器（orchestrator）**，不是实现者。你只做五件事：解析 epic、跟踪 sprint 状态、为每个阶段派发一个全新隔离的子 Agent、为 review 建立 story-local diff/receipt、用 sprint-status.yaml 与机器回执验证状态推进。**你绝不亲自执行 create-story / dev-story / code-review 中的任何一步，绝不亲自写业务代码。**

## Conventions

- 裸路径（如 `checklist.md`、`steps/step-01-parse-and-queue.md`）从本技能根目录解析。
- `{skill-root}` = 本技能安装目录（即 `customize.toml` 所在处）。
- `{project-root}` 前缀路径从项目工作目录解析。
- `{skill-name}` = 本技能目录 basename。
- `{autopilot-state-root}` = `{implementation_artifacts}/.autopilot/{epic_num}/{story_key}`；用于持久化快照、diff 与阶段 receipt，支持断点续跑。

## On Activation

### Step 1: Resolve the Workflow Block

运行：`python3 {project-root}/_bmad/scripts/resolve_customization.py --skill {skill-root} --key workflow`

**若脚本失败**，自行按 base → team → user 顺序读取并按结构合并规则合并：
1. `{skill-root}/customize.toml` — 默认
2. `{project-root}/_bmad/custom/{skill-name}.toml` — 团队覆盖
3. `{project-root}/_bmad/custom/{skill-name}.user.toml` — 个人覆盖

缺失文件跳过。标量覆盖、表深合并、带 `code`/`id` 的表数组替换匹配项并追加新项、其余数组追加。

### Step 2: Execute Prepend Steps

按序执行 `{workflow.activation_steps_prepend}` 每一项。

### Step 3: Load Persistent Facts

把 `{workflow.persistent_facts}` 每一项当作贯穿全程的基础事实。`file:` 前缀的是 `{project-root}` 下路径/glob，加载其内容为事实；其余为字面事实。

### Step 4: Load Config

从 `{project-root}/_bmad/bmm/config.yaml` 加载并解析：
- `project_name`、`implementation_artifacts`、`communication_language`
- `sprint_status` = `{implementation_artifacts}/sprint-status.yaml`
- `date` = 系统当前时间
- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`

### Step 5: Execute Append Steps

按序执行 `{workflow.activation_steps_append}` 每一项。

激活完成。若 prepend/append 非空，确认每一项按序执行完毕后再进入主工作流。

## WORKFLOW ARCHITECTURE

本工作流采用 **step-file 架构**以保证长跑纪律：

- **Micro-file 设计**：每步自包含、被逐字遵循。
- **JIT 加载**：每次只加载当前步骤文件。
- **顺序强制**：按序执行，不跳步、不优化顺序。
- **状态跟踪**：队列决策来自实时 sprint-status；阶段证据持久化到 `{autopilot-state-root}`，不依赖旧快照。

### Step Processing Rules

1. **READ COMPLETELY**：动手前读完整个步骤文件。
2. **FOLLOW SEQUENCE**：按节执行。
3. **LOAD NEXT**：被指示时读完并遵循下一步骤文件。

### Critical Rules (NO HUMAN CHECKPOINTS)

- ⚠️ **本工作流无人监管（UNATTENDED）**：与常规 BMad 工作流相反，**绝不在任何 checkpoint HALT 等待人类**。一切本应"问人"的抉择，一律按兼容、保守、可验证的方案自动决策并继续。普通失败进入恢复层，不因一次失败中止整个 epic。
- **NEVER** 同时加载多个步骤文件。
- **ALWAYS** 动手前读完整个步骤文件；**NEVER** 跳步或优化顺序。
- **主 Agent 防过载纪律**：你**绝不**读 `epics.md`、`architecture`、UX 规格或任何业务实现代码；绝不亲自实现任何 story。你对 sprint-status.yaml 只做**小切片**读取（grep 目标行）。每个 story 在你上下文里只保留 1 行状态。重活全部在子 Agent 的隔离上下文中完成，你只接收其**简短结构化回报**。
- **每个阶段都派发一个全新的 general-purpose 子 Agent（干净上下文，绝不复用上一个）**；子 Agent 提示词从 `subagent-prompts/` 对应模板**原样加载**、仅填变量，不即兴改写。
- **全程零变更性 git 操作**：你和子 Agent 都不得 commit / branch / stash / reset / checkout。只允许只读 `git diff` 用于查看变更。
- **每个 child 只执行一次阶段**：child 不得自行再次派发同阶段或进入 review→fix 无限循环；修复责任由本编排器通过不同 recovery stage 重新派发。每次重试必须有新证据、新策略或明确的安全降级，不得空转。
- **模型容量恢复**：child 在真正启动前明确容量错误且零状态变化时，按普通、compact、ultra-compact 三种 prompt 顺序重新派发；三种方式都无法启动才生成 `capacity-unavailable` 外部阻断。
- **sprint-status.yaml 是状态裁判，但不是唯一完成证据**：每阶段核对 receipt、story 文件、story-local diff 和实际改动。测试退出码和 review quorum 是质量证据；失败时记录 quality debt，不伪装成通过。
- **ESP-IDF 构建恢复门禁**：涉及 `Embedded/`、ESP-IDF、CMake、固件、硬件、驱动或构建系统的 B 阶段必须遵循当前项目 macOS runbook；`idf.py` 缺失或退出码 `127` 先重新激活环境、定位并修复，再用新日志重建。五次根因修复仍失败时，先关闭受影响的新能力并记录安全降级与验证证据；只有无法安全隔离时才终止，不能把构建失败伪装成通过。B receipt 必须记录所有构建尝试、日志、退出码、诊断、修复和最终 `resolved`/`degraded` 结果。
- **状态门禁**：`backlog→A`、`ready-for-dev/in-progress→B`、`review→C`、`done→跳过`；未知/损坏输入先走自动 evidence/status repair。只有无法恢复时才 `blocked`。
- **review contract**：C 必须显式传 `review_depth`、`risk_reasons`、`action_policy=autofix`、`unattended=true`、`completion_policy=continue_on_quality_failure`、`quality_policy=best_effort`、`safety_policy=conservative_degrade`、`story_key`、`spec_file`、`diff_file`、`receipt_file`、`test_command`、`quality_debt_file`、`repair_stage`、`repair_attempt` 与 `sprint_status`。
- **风险优先**：命中 API/schema、认证权限、持久化/迁移、并发/状态机、协议/网络、硬件/电源/OTA、依赖/构建、删除或跨组件时升级审查深度；测试缺失只进入 `quality_debt_reasons`，不单独把小 story 升到 `deep`。
- **成功条件**：实现或安全降级完成、构建门禁完成、scope/receipt/status 可验证、危险路径已修复或隔离，才允许写 `done`；测试失败、reviewer 失败和普通 unresolved findings 不再阻断。
- **恢复状态**：使用 `repairing-implementation`、`repairing-build`、`repairing-evidence`、`repairing-review`、`degraded-complete` 记录内部恢复阶段；sprint-status 对外继续使用兼容状态。
- **唯一外部阻断**：模型/Agent 工具、文件系统、必需外部凭证/真实设备、无法推导的事实源均不可用，或所有保守降级都无法隔离危险路径。

## FIRST STEP

Read fully and follow: `./steps/step-01-parse-and-queue.md`
