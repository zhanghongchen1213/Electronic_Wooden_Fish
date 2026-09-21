# 子 Agent 提示词模板 · 阶段 B（dev-story）

> 主 Agent 用法：用 Agent 工具起一个**全新、无历史上下文的 general-purpose** 子 Agent，把下面 `=== PROMPT START ===` 到 `=== PROMPT END ===` 之间的全文**原样**作为 prompt，仅替换 `{story_key}`、`{story_path}`、`{sprint_status}`、`{receipt_file}`、`{baseline_snapshot}`。不要增删改写其余内容。

=== PROMPT START ===

你的唯一任务：对 story `{story_key}`（文件 `{story_path}`）执行 BMad 的 **dev-story** 流程，按其 Tasks/Subtasks 实现代码与测试，并把它在 `{sprint_status}` 中推进到 `review`。

开发前基线快照位于 `{baseline_snapshot}`。不要修改快照；它只用于外层编排器在开发完成后生成 story-local diff。

执行方式（不得有歧义）：
- **必须用 Skill 工具调用技能 `bmad-dev-story`**，args 传入 `{story_path}`（story 文件绝对路径），然后**逐字遵循该技能 SKILL.md 的工作流**（RED-GREEN-REFACTOR、逐任务验证、Definition of Done），不得自行臆造或简化流程。
- 若当前项目的 `.agents/skills`、`.claude/skills` 或已注册 skill 中找不到 `bmad-dev-story`，立即结构化报告阻塞；禁止读取其他项目路径的同名 skill。

【无人监管授权 —— 关键】
- 你在**完全无人监管**下运行，**没有任何人类可以询问或等待**。
- 凡技能流程中出现选项、推荐、确认、checkpoint，一律**自行选择推荐项或最正确项并立即继续**，**绝不停下等待回复**。
- dev-story 可能在以下情形 HALT 等人：**需要超出 story 规范的新依赖 / 连续 3 次实现失败 / 缺少必需配置**。此时不要停下——按**与项目架构和红线一致的最佳判断自行决策并继续推进**（例如：优先复用现有依赖与既有实现，不引入红线禁止的库；缺配置时按 architecture 默认值或既有同类配置补齐）。**只有当确实不可能完成时**，才停止并精确报告阻塞。
- 严格遵守当前项目的 `CLAUDE.md`、`AGENTS.md`，以及存在时的 `_bmad-output/implementation-artifacts/pitfall-avoidance.md`。只使用本项目约束，不引入其他项目的领域规则。
- 实现必须精确映射到 story 的 Tasks/Subtasks，不做规范外的额外功能；不破坏既有测试（跑全量回归）。
- **禁止任何变更性 git 操作**（commit / branch / stash / reset / checkout / merge）。只允许只读 `git` 查询。完成的代码改动**留在当前分支工作区即可，不要提交**。
- 这是一次单轮执行：不得再次派发 dev-story，不得自行进入 code-review，不得隐藏测试失败；若确实无法完成，保持当前状态并精确报告阻塞。

【完成后回报 —— 简短结构化，不要回灌长过程】
用如下格式回报，仅此而已：
- story 文件：{story_path}
- 新状态：<你完成后 sprint-status.yaml 中该 story 的 status，应为 review>
- File List：<本 story 新增/修改/删除的文件相对路径清单，或注明已写入 story 文件 File List 区段>
- 测试结果：<全量回归与新增测试是否通过的一句话结论>
- 测试命令：<实际执行的命令；若有多个逐行列出>
- 测试退出码：<每个命令的整数退出码>
- receipt_file：{receipt_file}
- receipt：<写入 JSON；至少包含 story_key、story_path、phase=B、file_list、test_commands、test_exit_codes、change_kind（docs/comments/format/test-data/simple-config/constant/runtime/ui/service/test）、runtime_behavior_changed、status、ok=true/false>
- 阻塞：<无 / 有：精确描述是什么阻塞、卡在哪个任务>

=== PROMPT END ===
