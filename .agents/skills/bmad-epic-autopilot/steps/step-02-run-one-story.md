---
current_story_key: ''
current_story_path: ''
phase: ''
repair_stage: 'normal'
repair_attempt: 0
baseline_snapshot: ''
diff_file: ''
dev_receipt: ''
review_receipt: ''
review_depth: ''
review_mode: 'full'
risk_reasons: []
quality_debt: []
quality_debt_file: ''
no_code_change: false
external_blocker: ''
---

# Step 2: Run One Story（自动恢复核心循环）

本步骤将一个 story 从当前合法状态推进到 `done`。质量证据不完整时继续自动恢复或保守降级；不等待人类。

## RULES

- 你是编排器：不亲自执行 create-story/dev-story/code-review，不亲自写业务代码。
- 每个阶段的 child 只执行一轮；child 不得递归派发、不自行进入 review→fix 循环。
- 每次恢复必须有新证据、新策略或安全降级；同一错误不得空转。
- 测试失败/缺失、reviewer 失败/超时、普通 unresolved HIGH/MEDIUM 都写入 quality debt，不作为直接阻断。
- 构建、实现、scope、receipt 和状态问题优先自动修复；只有外部能力确实不可用才进入 blocked。

## 1. 重新读取状态并推导路径

每轮必须重新读取 `{sprint_status}` 中目标 key，不使用旧队列状态。

| 当前 status | 起跑阶段 | 结果 |
|---|---|---|
| `backlog` | A create-story | `ready-for-dev` |
| `ready-for-dev` / `in-progress` | B dev-story | `review` |
| `review` | C code-review | `done` 或继续恢复 |
| `done` | 跳过 | 无需派发 |
| `blocked` / 未知 | 先自动修复状态；无法恢复才结束 | `blocked` |

固定推导：

```text
current_story_path = {implementation_artifacts}/{story_key}.md
autopilot_state_root = {implementation_artifacts}/.autopilot/{epic_num}/{story_key}
```

B/C 必须验证 story 文件存在、可读、story key 匹配。缺失时先尝试从文件名、frontmatter 和最近 receipt 恢复；恢复失败才进入外部阻断。

## 2. 阶段 A：create-story

按 `subagent-prompts/phase-a-create-story.md` 派发全新 child，契约中增加：

```text
unattended=true
completion_policy=continue_on_quality_failure
quality_policy=best_effort
repair_stage=normal
```

A receipt 必须证明 story 文件和 `ready-for-dev` 状态。普通失败自动按以下顺序恢复：

1. 重派正常 child；
2. 使用 compact prompt，只保留 story key、sprint-status、receipt 路径；
3. 根据规划产物和现有 story 模板自动补齐最小 story 文件。

只有当前项目没有可用规划事实源或 Agent 工具完全不可用时，才生成外部阻断。

## 3. 阶段 B：dev-story、构建恢复和 story-local diff

进入 B 前调用：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py snapshot \
  --root {project-root} \
  --out {autopilot_state_root}/attempt-{repair_attempt}/before-dev
```

快照是脚本生成的参照物，不是备用项目副本。派发 `phase-b-dev-story.md` 时传入：

```text
unattended=true
completion_policy=continue_on_quality_failure
quality_policy=best_effort
safety_policy=conservative_degrade
quality_debt_file={autopilot_state_root}/quality-debt.json
repair_stage={repair_stage}
repair_attempt={repair_attempt}
baseline_snapshot={baseline_snapshot}
```

child 返回后：

1. 校验 B receipt 的结构、File List、story key 和 `review` 状态；
2. 测试命令可以为空，退出码可以非零，但必须写入 `test_status`、`quality_state` 和 `quality_debt`；
3. ESP-IDF/其他构建先按 runbook 诊断和自动修复，`idf.py` 127 必须激活并重建；五次仍失败时优先关闭受影响的新能力并以 `build_recovery=degraded` 安全收口，只有无法隔离危险路径才阻断；
4. 其他构建失败进入 `repairing-build`，定位最早错误、修复、重新构建；
5. 实现任务未完成进入 `repairing-implementation`，派发 focused repair child；
6. 不一致的 File List 由 diff 脚本重新计算并自动修复；
7. receipt 损坏时先备份，再调用：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py repair-receipt \
  --file {dev_receipt} --phase B --story-key {current_story_key} \
  --output {repaired_receipt} --diff {diff_file}
```

修复 receipt 只会生成 `ok=false` 的证据骨架，随后必须重跑 B，不得伪造成功。

生成 story-local diff：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py diff \
  --before {baseline_snapshot} --root {project-root} \
  --out {autopilot_state_root}/attempt-{repair_attempt}/story-local.diff \
  --exclude {current_story_path} --exclude {sprint_status} \
  --exclude {autopilot_state_root}
```

空 diff、路径不一致或 helper 失败时，先验证快照和排除项，再重建；确认快照有效且 story 的实现任务已经明确无需代码变更时，记录 `file_list=[]`、`change_kind=none`、`implementation_complete=true`、`no_code_change=true` 和质量债务，然后以 `unverified` 进入 C；只有无法读写工作树才阻断。

## 4. 阶段 C：风险路由与一次性 code-review

调用 `classify-risk` 并保存结果。若 B 已确认 `no_code_change=true`，跳过空 diff 分类器，固定 `review_depth=standard`、`risk_reasons=["确认无代码变更"]`，并保留 `quality_debt_reasons=["空 diff 未执行审查"]`。普通 diff 的分类器输出两组信息：

```text
review_depth
risk_reasons
quality_debt_reasons
```

测试缺失/失败只能进入 `quality_debt_reasons`，不单独升级小 story 的深度。

向 C child 传入完整契约：

```text
review_depth: {review_depth}
risk_reasons: {risk_reasons}
review_mode: {review_mode}
action_policy: autofix
unattended: true
completion_policy: continue_on_quality_failure
quality_policy: best_effort
safety_policy: conservative_degrade
quality_debt_file: {autopilot_state_root}/quality-debt.json
no_code_change: {no_code_change}
story_key: {current_story_key}
spec_file: {current_story_path}
diff_file: {diff_file}
receipt_file: {review_receipt}
test_command: {test_command}
sprint_status: {sprint_status}
repair_stage: repairing-review
repair_attempt: {repair_attempt}
```

C child 只做一轮 gather → review → triage → action。普通 findings 自动 patch/defer；reviewer 失败记为 `unverified`；真实危险问题必须自动修复或进入保守降级。

C receipt 允许：

```text
outcome: clean | autofixed | degraded | unverified | review | blocked
status: done | review | blocked
```

允许 `degraded`/`unverified` 在没有未隔离危险路径时写入 `done`。测试失败、mandatory reviewer 失败和普通 unresolved HIGH/MEDIUM 不再触发重试或阻断。

## 5. 自动恢复层级

每个阶段按以下顺序选择恢复策略，attempt 只在策略变化时递增：

```text
normal
repairing-implementation
repairing-build
repairing-evidence
repairing-review
degraded-complete
```

每次恢复必须记录：错误指纹、输入、修复动作、验证结果、新 receipt。连续三次错误指纹不变时切换下一层，不重复相同 prompt。

模型容量错误的 child 启动顺序：normal → compact → ultra-compact。三种方式均未启动且没有任何状态/代码/receipt 变化，才输出 `capacity-unavailable`。

## 6. 状态修复和收尾

若 sprint-status YAML 损坏，调用：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py repair-status \
  --file {sprint_status} \
  --implementation-artifacts {implementation_artifacts} \
  --epic {epic_num}
```

原文件先备份为 `.repair-backup.yaml`，恢复值优先来自合法 receipt，其次来自 story frontmatter，最后才是 `backlog`。

当前 story 只要实现/安全降级、构建、scope、receipt 和 status 完成，就记录 `degraded-complete` 并进入 `done`。完成后重新读取下一个 story 的实际状态，不依赖旧队列快照。

只有以下条件全部无法恢复时才进入 `step-03-finalize.md` 的外部阻断分支：模型/工具不可用、文件系统不可读写、必要外部凭证/真实设备无替代方案、事实源无法推导，或危险路径无法隔离。

## NEXT

- 还有非 done story：重新读取并遵循本步骤。
- 队列耗尽：读取并遵循 `./step-03-finalize.md` 成功分支。
- 外部能力不可用：读取并遵循 `./step-03-finalize.md` 外部阻断分支。
