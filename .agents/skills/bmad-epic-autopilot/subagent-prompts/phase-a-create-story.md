# 子 Agent 提示词模板 · 阶段 A（create-story）

> 主 Agent 用法：用 Agent 工具起一个**全新、无历史上下文的 general-purpose** 子 Agent，把下面 `=== PROMPT START ===` 到 `=== PROMPT END ===` 之间的全文**原样**作为 prompt，仅替换 `{story_key}`、`{sprint_status}`、`{receipt_file}`。不要增删改写其余内容。

=== PROMPT START ===

你的唯一任务：为 story `{story_key}` 执行 BMad 的 **create-story** 流程，生成该 story 的上下文文件并把它在 `{sprint_status}` 中推进到 `ready-for-dev`。

执行方式（不得有歧义）：
- **必须用 Skill 工具调用技能 `bmad-create-story`**，args 传入 `{story_key}`，然后**逐字遵循该技能 SKILL.md 的工作流**，不得自行臆造或简化流程。
- 若当前项目的 `.agents/skills`、`.claude/skills` 或已注册 skill 中找不到 `bmad-create-story`，立即结构化报告阻塞；禁止读取其他项目路径的同名 skill。

【无人监管授权 —— 关键】
- 你在**完全无人监管**下运行，**没有任何人类可以询问或等待**。
- 凡技能流程中出现选项、推荐、确认、checkpoint，一律**自行选择推荐项或最正确项并立即继续**，**绝不停下等待回复**。
- 严格遵守当前项目的 `CLAUDE.md`、`AGENTS.md`，以及存在时的 `_bmad-output/implementation-artifacts/pitfall-avoidance.md`。任何产出不得违反这些红线。
- 真源优先：story 需求以 `epics.md` 为准，技术约束以 architecture 为准，路由/状态以 `*-EXPERIENCE.md` 为准，不臆造功能、入口、金额、状态、角色。
- **禁止任何变更性 git 操作**（commit / branch / stash / reset / checkout / merge）。只允许只读 `git` 查询（如 `git log`/`git diff`）。
- 这是一次单轮执行：不得再次派发 create-story，不得等待用户，不得把失败隐藏成 ready-for-dev。

【完成后回报 —— 简短结构化，不要回灌长过程】
用如下格式回报，仅此而已：
- story_key：{story_key}
- 新状态：<你完成后 sprint-status.yaml 中该 story 的 status，应为 ready-for-dev>
- story 文件路径：<生成的 .md 绝对路径>
- receipt_file：{receipt_file}
- receipt：<写入 JSON；至少包含 story_key、story_path、status、phase=A、ok=true/false>
- 阻塞：<无 / 有：精确描述是什么阻塞、卡在哪一步>

=== PROMPT END ===
