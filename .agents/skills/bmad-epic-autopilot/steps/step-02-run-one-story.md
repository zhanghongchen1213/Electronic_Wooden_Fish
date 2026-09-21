---
current_story_key: '' # set at runtime: the story being driven this iteration
current_story_path: '' # set at runtime: absolute path to the story .md
phase: '' # set at runtime: A | B | C
attempt: 0 # set at runtime: 1 or 2 (retry-once policy)
baseline_snapshot: '' # set at runtime before B
diff_file: '' # set at runtime after B, story-local unified diff
dev_receipt: '' # set at runtime after B
review_receipt: '' # set at runtime after C
review_depth: '' # set at runtime: lite | standard | deep
risk_reasons: [] # set at runtime by phase C classifier
risk_file: '' # set at runtime by deterministic classifier
blocked_reason: '' # set at runtime for fail-closed exits
---

# Step 2: Run One Story（核心循环步）

> 主 Agent 每处理一个 story 都重读本步骤文件一次。本步骤把一个 story
> 从其当前合法状态推进到 `done`，或者在第二次失败后 fail-closed 终止整个 epic。

## RULES

- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`
- 你是编排器：**绝不**亲自执行 create-story/dev-story/code-review，**绝不**亲自写业务代码。
- 每个阶段派发**一个全新 general-purpose 子 Agent**；child 只执行一次阶段，不得递归派发或自发 review→fix 循环。
- **零变更性 git 操作**（你与子 Agent 都不得 commit/branch/stash/reset/checkout）。快照和 diff helper 只能写入 `{autopilot_state_root}` 或临时目录。
- **不等待人类**。所有 checkpoint、决策和异常按本文件的自动化契约处理；只有重试一次后仍无法推进才终止 epic。
- **状态 + receipt 双门禁**：sprint-status 行、story 文件、阶段 receipt、测试退出码和 review quorum 必须共同满足，不能只 grep 状态。

## INSTRUCTIONS

### 1. 重新读取当前 story 并解析起跑阶段

在每轮进入本步骤时重新读取 `{sprint_status}` 中目标 key 的实际 status，不使用旧队列快照。按下表选择阶段：

| 当前 status | 起跑阶段 | 允许的阶段结果 |
|---|---|---|
| `backlog` | A create-story | `ready-for-dev` |
| `ready-for-dev` | B dev-story | `review` |
| `in-progress` | B dev-story | `review` |
| `review` | C code-review | `done` 或 `review` |
| `done` | 跳过 | 无需派发 |
| `blocked` 或未知值 | 终止 | `blocked` |

设置 `{current_story_path}` = `{implementation_artifacts}/{current_story_key}.md`。
无论从哪个状态开始都必须先推导该路径；B/C 起跑时必须验证文件存在、可读且 story key 匹配，失败立即进入 step-03 blocked 分支。
backlog 的 A 阶段允许 story 文件尚不存在，因为 create-story 的职责就是创建它；A 仍必须在 receipt 中证明创建后的路径和 story key。
若起跑阶段为 C，必须从 `{autopilot_state_root}/{current_story_key}/` 下读取最近一次完整的
`story-local.diff`、B receipt 和 test_command；任一缺失或 diff 为空都进入 blocked，不能重新用累计工作树 diff 猜测。

### 2. 阶段 A — create-story

按模板 `subagent-prompts/phase-a-create-story.md` 原样派发全新 child，替换 `{story_key}`、`{sprint_status}`、`{receipt_file}`；本阶段 receipt 路径固定为 `{autopilot_state_root}/{current_story_key}/attempt-{attempt}/create-story.json`。

期望结果：

- `uv run --no-cache {skill-root}/scripts/autopilot_state.py validate-receipt --file {autopilot_state_root}/{current_story_key}/attempt-{attempt}/create-story.json --phase A --story-key {current_story_key} --expected-status ready-for-dev` 成功；
- story 文件已创建且路径正确；
- sprint status 从 `backlog` 变为 `ready-for-dev`；
- A receipt 存在且内容与 story key 一致。

未达标：只允许使用同模板、同变量派发一个全新的 `attempt=2` child；第二次仍未达标立即终止 epic。

### 3. 阶段 B — dev-story 与 story-local 快照

进入 B 前调用：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py snapshot \\
  --root {project-root} \\
  --out {autopilot_state_root}/{current_story_key}/attempt-{attempt}/before-dev
```

将 snapshot 路径作为本阶段事实传给 child。按模板 `subagent-prompts/phase-b-dev-story.md` 原样派发全新 child，替换 `{story_key}`、`{story_path}`、`{sprint_status}`、`{receipt_file}`、`{baseline_snapshot}`；本阶段 receipt 路径固定为 `{autopilot_state_root}/{current_story_key}/attempt-{attempt}/dev-story.json`。

child 返回后必须验证：

- `uv run --no-cache {skill-root}/scripts/autopilot_state.py validate-receipt --file {autopilot_state_root}/{current_story_key}/attempt-{attempt}/dev-story.json --phase B --story-key {current_story_key} --expected-status review` 成功；
- sprint status 到达 `review`；
- B receipt 存在，包含 File List、实际测试命令和退出码；
- 测试命令退出码为 0；
- File List 不为空且每个路径可追溯。

然后调用：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py diff \\
  --before {baseline_snapshot} \\
  --root {project-root} \\
  --out {autopilot_state_root}/{current_story_key}/attempt-{attempt}/story-local.diff \\
  --exclude {current_story_path} \\
  --exclude {sprint_status} \\
  --exclude {autopilot_state_root}
```

验证实际 changed paths 与 B receipt 的 File List 完全一致；漏列、超列、空 diff 或 helper 失败均为本阶段失败。只有第一次失败才允许用全新 child 重试 B；第二次失败终止 epic。

### 4. 阶段 C — 风险路由与一次性 code-review

在派发 C child 前调用确定性分类器，并把结果持久化：

```text
uv run --no-cache {skill-root}/scripts/autopilot_state.py classify-risk \\
  --diff {diff_file} \\
  --dev-receipt {dev_receipt} \\
  > {autopilot_state_root}/{current_story_key}/attempt-{attempt}/risk-classification.json
```

读取该 JSON 的 `review_depth` 与 `risk_reasons`；`ok=false`、字段缺失或深度不在
`lite|standard|deep` 时立即 blocked。分类器按风险优先：无法证明 lite 条件时至少为
`standard`，命中高风险或无法判断关键输入时为 `deep`，不得把未知降为 lite。
将 `review_mode=full`、`{review_depth}`、`{risk_reasons}` 原样传给 C child；child 可以复核但不得降级。

向 `bmad-code-review` 传入完整契约；本阶段 receipt 路径固定为 `{autopilot_state_root}/{current_story_key}/attempt-{attempt}/code-review.json`：

```text
review_depth: {review_depth}
risk_reasons: {risk_reasons}
review_mode: full
action_policy: autofix
unattended: true
story_key: {current_story_key}
spec_file: {current_story_path}
diff_file: {diff_file}
receipt_file: {review_receipt}
test_command: {test_command}
sprint_status: {sprint_status}
```

C child 只允许执行一次 gather → review → triage → action；不得自己再次 review、重新选择 diff、等待用户、或把 `review` 递归推进成 `done`。

C child 返回后先执行 `validate-receipt --file {review_receipt} --phase C --story-key {current_story_key}`；结构校验失败、receipt 的 `ok` 非 true 或字段缺失都视为 C 失败。receipt 的 `review_depth` 必须等于传入深度或仅向 `deep` 升级，绝不能降级；升级时以 receipt 中的 active/mandatory layers 重新核对门禁。

成功门禁：

- review receipt 存在；
- 选中的 mandatory layers 全部完成；
- 没有 unresolved HIGH/MEDIUM；
- patch 后测试退出码为 0；
- outcome 为 `clean` 或 `autofixed`；
- sprint status 为 `done`。

如果 outcome 为 `review` 或仍为 `review`，只允许再派发一次全新 C child；不回到 B。第二次仍未 `done` 或 receipt 不完整，终止整个 epic。

如果 C 返回 `in-progress`、`blocked`、未知状态或写入状态与 receipt 冲突，视为失败；不得自动改跑 B。

### 5. 阶段通用 retry 与收尾

每个阶段严格执行：

1. `attempt=1` 派发全新 child；
2. 读取 sprint-status 和机器 receipt；
3. 未达标时 `attempt=2`，只重派同一阶段的全新 child；
4. 第二次仍未达标，记录 key、阶段、实际 status、receipt 错误和 child 阻塞，进入 step-03 blocked 分支。

当前 story 达到 `done` 后，对照 `{skill-root}/checklist.md` 自检，再重新读取下一个 story 的 status。不要依赖旧队列状态。

## NEXT

- 还有非 done story → 重读并遵循本文件。
- 队列耗尽 → Read fully and follow `./step-03-finalize.md` 成功分支。
- 任一阶段第二次失败、输入损坏、mandatory review 失败或 unresolved HIGH/MEDIUM → Read fully and follow `./step-03-finalize.md` blocked 分支。
