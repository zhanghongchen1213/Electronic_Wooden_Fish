# 子 Agent 提示词模板 · 阶段 B（dev-story）

> 主 Agent 用法：用 Agent 工具起一个**全新、无历史上下文的 general-purpose** 子 Agent，把下面 `=== PROMPT START ===` 到 `=== PROMPT END ===` 之间的全文**原样**作为 prompt，仅替换 `{story_key}`、`{story_path}`、`{sprint_status}`、`{receipt_file}`、`{baseline_snapshot}`、`{quality_debt_file}`、`{repair_stage}`、`{repair_attempt}`。不要增删改写其余内容。

=== PROMPT START ===

你的唯一任务：对 story `{story_key}`（文件 `{story_path}`）执行 BMad 的 **dev-story** 流程，按其 Tasks/Subtasks 实现代码与测试，并把它在 `{sprint_status}` 中推进到 `review`。本次由 Epic Autopilot 以无人监管 best-effort 模式调用：实现必须闭合；构建先自动修复，仍无法完成时关闭受影响的新能力并安全降级；测试可以缺失或失败，但必须如实记录并继续推进，不得等待人类。

开发前基线快照位于 `{baseline_snapshot}`。不要修改快照；它只用于外层编排器在开发完成后生成 story-local diff。

## macOS ESP-IDF 构建恢复门禁

当 story 的 Tasks、Dev Notes 或实际修改涉及 `Embedded/`、ESP-IDF、CMake、固件、硬件、驱动或构建系统时，必须先完整读取当前项目的 `docs/embedded/macos_esp_idf_hardware_test_runbook.md`，并严格使用其中的环境激活、构建日志和产物核验步骤。不得读取其他项目的 runbook，也不得切换到其他 IDF 版本。

构建失败不是本阶段的即时终止条件。尤其是 `idf.py: command not found` 或 shell 退出码 `127`，先按环境问题诊断，不得把它当成代码编译失败直接回报：

1. 保存本次完整构建日志和退出码；从当前项目 `Embedded/` 目录重新执行 runbook 要求的 `source` 激活脚本。
2. 在同一激活 shell 中验证 `command -v idf.py` 与 `idf.py --version`，确认使用 runbook 指定的 ESP-IDF 版本；不要只重试未激活的 `idf.py` 命令。
3. 对其他非零退出码，定位日志中最早的真实 `error:`、`fatal error:`、`undefined reference` 或第一个失败命令，先修复该根因，再重跑完整构建；不能只摘录最后的 `ninja failed`。
4. 每次修复后使用从 1 开始递增的构建尝试编号和新日志路径重新构建，并再次核验生成产物。不得执行 `flash`、`app-flash` 或 `monitor` 代替构建验证。

构建诊断最多允许 5 次尝试；每次必须有新的证据或实际修复，同一错误没有新证据时不得空转。5 次后仍无法成功时，优先关闭受影响的新能力并记录 `safety_degraded=true`、`disabled_capabilities`、`degradation_reason` 和验证证据；只有无法安全隔离时才报告外部阻断。构建成功或已安全降级后才可把 sprint status 推进到 `review`。测试缺失或失败不阻断状态推进，必须写入 `test_status`、`quality_state` 和 `quality_debt`；不得声称测试通过。

执行方式（不得有歧义）：
- **必须用 Skill 工具调用技能 `bmad-dev-story`**，args 传入 `{story_path}`（story 文件绝对路径），然后**逐字遵循该技能 SKILL.md 的工作流**（RED-GREEN-REFACTOR、逐任务验证、Definition of Done），不得自行臆造或简化流程。
- 若当前项目的 `.agents/skills`、`.claude/skills` 或已注册 skill 中找不到 `bmad-dev-story`，使用本 prompt 的最小 dev-story 应急执行器完成 story；只有当前项目事实源和应急执行器都不可用时才报告外部阻断，禁止读取其他项目路径的同名 skill。

【无人监管授权 —— 关键】
- 你在**完全无人监管**下运行，**没有任何人类可以询问或等待**。
- 凡技能流程中出现选项、推荐、确认、checkpoint，一律**自行选择推荐项或最正确项并立即继续**，**绝不停下等待回复**。
- dev-story 可能在以下情形 HALT 等人：**需要超出 story 规范的新依赖 / 连续 3 次实现失败 / 缺少必需配置**。此时不要停下——按**与项目架构和红线一致的最佳判断自行决策并继续推进**（例如：优先复用现有依赖与既有实现，不引入红线禁止的库；缺配置时按 architecture 默认值或既有同类配置补齐）。对 ESP-IDF 构建而言，未激活导致的 `idf.py` 缺失或退出码 `127` 不是“确实不可能完成”，必须先执行上面的构建诊断恢复循环。**只有构建恢复上限耗尽或确实不可能完成时**，才停止并精确报告阻塞。
- 严格遵守当前项目的 `CLAUDE.md`、`AGENTS.md`，以及存在时的 `_bmad-output/implementation-artifacts/pitfall-avoidance.md`。只使用本项目约束，不引入其他项目的领域规则。
- 实现必须精确映射到 story 的 Tasks/Subtasks，不做规范外的额外功能；不破坏既有测试（跑全量回归）。
- **禁止任何变更性 git 操作**（commit / branch / stash / reset / checkout / merge）。只允许只读 `git` 查询。完成的代码改动**留在当前分支工作区即可，不要提交**。
- 这是一次单轮执行：不得再次派发 dev-story，不得自行进入 code-review，不得隐藏测试失败；若本轮实现仍不完整，记录 `implementation_complete=false`、错误指纹和下一恢复阶段，让外层派发 focused repair，不得等待人类。
- 自动恢复上下文：当实现任务未完成、receipt 不完整或构建失败时，记录 `repair_stage` 和错误指纹，按外层传入的恢复阶段继续修复。不得因为测试失败、没有测试、lint 失败或验证缺口而 HALT；这些都进入质量债务。

【file_list 口径 —— 硬性】
`file_list` 与回报中的 File List 必须与本 story 的 story-local diff 的 changed paths **完全同一集合**，因此只列本 story 改动的、会被 git 看见的实现文件。以下两类**不得列入**：

- story 文件本身（`{story_path}`）与 `{sprint_status}` —— 外层 diff 会显式 `--exclude` 它们；
- 任何被 `.gitignore` 忽略的路径（用 `git check-ignore -v <path>` 判定），例如 `docs/embedded/build_records/` 下的构建记录 —— 外层 diff 基于 `git ls-files -co --exclude-standard` 生成，看不见被忽略的文件；构建证据请在 receipt 的 `build_log_paths` 字段中引用，不要放进 `file_list`。

两个集合不等时，外层先用 story-local diff 机械重建 File List，并把修复动作写入 evidence receipt；只有 diff helper、工作树或文件系统本身不可用时才阻断 C。

【完成后回报 —— 简短结构化，不要回灌长过程】
用如下格式回报，仅此而已：
- story 文件：{story_path}
- 新状态：<你完成后 sprint-status.yaml 中该 story 的 status，应为 review>
- File List：<本 story 新增/修改/删除的文件相对路径清单，或注明已写入 story 文件 File List 区段>
- 测试结果：<全量回归与新增测试是否通过的一句话结论>
- 测试命令：<实际执行的命令；若有多个逐行列出>
- 测试退出码：<每个命令的整数退出码>
- 构建尝试：<每次 attempt 的编号、日志绝对路径、退出码、最早错误/环境诊断与修复；非 ESP-IDF story 填 []>
- 构建最终退出码：<整数或 null；非 ESP-IDF story 填 null>
- 构建恢复：<resolved | not-applicable | degraded | blocked；degraded 必须同时写入 safety_degraded、disabled_capabilities、degradation_reason 和 verification_evidence>
- receipt_file：{receipt_file}
- receipt：<写入 JSON；至少包含 story_key、story_path、phase=B、file_list、test_commands、test_exit_codes、test_status（passed|failed|missing|not-run）、quality_state（clean|degraded|unverified）、quality_debt、quality_debt_file={quality_debt_file}、implementation_complete、repair_stage、change_kind（none/docs/comments/format/test-data/simple-config/constant/runtime/ui/service/test）、runtime_behavior_changed、build_attempts、build_exit_codes、build_log_paths、final_build_exit_code、build_recovery、build_status、safety_degraded、disabled_capabilities、degradation_reason、verification_evidence、status、ok=true/false>。若确认没有代码变更，使用 `file_list=[]`、`change_kind=none`、`implementation_complete=true`，并明确记录 `no_code_change=true`。
- 阻塞：<无 / 有：精确描述是什么阻塞、卡在哪个任务>

=== PROMPT END ===
