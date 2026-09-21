---
epic_num: '' # set at runtime: the target epic number, e.g. "7"
story_queue: [] # set at runtime: ordered list of {key, status} for this epic's stories, in file order
autopilot_state_root: '' # set at runtime: implementation artifacts/.autopilot/<epic>
blocked_reason: '' # set at runtime for fail-closed exits
---

# Step 1: Parse Epic & Build Story Queue

## RULES

- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`
- 触发本工作流的那句话 **就是意图**，不是提示。
- 本步骤**只读**：不得修改任何文件、不得派发任何子 Agent、不得做任何 git 操作。
- 只对 sprint-status.yaml 做**小切片**读取（grep 目标行段），不要整文件灌进上下文。

## INSTRUCTIONS

1. **解析 epic 号 `{epic_num}`。** 从触发语中提取数字 N，容错以下写法：`epic7` / `epic 7` / `Epic-7` / `自动跑 epic 7` / 裸 `7`。
   - 若无法从触发语解析出明确的单个 epic 号 → 报告"未能识别 epic 号，请用如 `epic7` 的形式重新触发"，**结束工作流**（不进入 step-02）。

2. **先验证 sprint-status 输入。** `{sprint_status}` 必须存在、可读、UTF-8 可解析，且 `development_status` 必须是 mapping。优先调用 `uv run --no-cache {skill-root}/scripts/autopilot_state.py validate-status --file {sprint_status} --epic {epic_num}`；失败时进入 step-03 的 blocked 分支，不得伪装成“epic 不存在/无可执行”。

3. **定位并读取该 epic 的状态区段。** 在 `{sprint_status}`（即 `{implementation_artifacts}/sprint-status.yaml`）中用小切片读取从 `epic-{epic_num}:` 行起、到下一个 `epic-{下一个数字}:` 或 `epic-{epic_num}-retrospective:` 行止之间的内容。
   - 推荐命令（只读）：`grep -nE "^  ?(epic-{epic_num}|{epic_num}-|epic-{epic_num}-retrospective)" {sprint_status}` 配合按行号定位区段；必要时用 Read 带 offset/limit 只取该行段。

4. **判定 epic 是否存在。** 若文件中不存在 `epic-{epic_num}:` 行 → 报告"sprint-status.yaml 中不存在 epic-{epic_num}"，**走 step-03 的"无可执行"分支结束**。这只适用于已验证存在且可解析的 sprint-status 文件。

5. **构建有序队列 `{story_queue}`。** 按**文件出现顺序**收集该区段内所有形如 `{epic_num}-<m>-<slug>: <status>` 的 story 行（排除 `epic-{epic_num}:` 自身和 `epic-{epic_num}-retrospective:`）。每项记 `{key, status}`。状态必须属于 `backlog`、`ready-for-dev`、`in-progress`、`review`、`done`、`blocked`；其他值进入 blocked 分支。
   - `key` 必须是 yaml 里**逐字**的完整 story key（含中文 slug），后续派发子 Agent 与 grep 验证都用它。

6. **初始化状态目录。** 设置 `{autopilot_state_root}` = `{implementation_artifacts}/.autopilot/{epic_num}`。每个 story 的快照、story-local diff、阶段 receipt 放在其下的 slug 目录中；不得写入技能目录。

7. **空跑 / 已完成判定。** 若 `{story_queue}` 为空，或其中每一项 status 都是 `done` → 报告"epic-{epic_num} 已全部完成（或无 story），无可执行项"，**走 step-03 的"已完成"分支结束**（**绝不派发任何子 Agent**）。这就是 epic6 的预期路径。

8. **回报队列概览**（1 行/story，仅 key + status），然后进入下一步处理第一个非 done story。

## NEXT

Read fully and follow `./step-02-run-one-story.md`
