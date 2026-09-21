# 子 Agent 提示词模板 · 阶段 C（code-review，单轮自动化）

> 主 Agent 用法：用 Agent 工具起一个**全新、无历史上下文的 general-purpose** 子 Agent，把下面 `=== PROMPT START ===` 到 `=== PROMPT END ===` 之间的全文**原样**作为 prompt，仅替换 `{story_path}`、`{story_key}`、`{diff_file}`、`{dev_receipt}`、`{review_receipt}`、`{sprint_status}`、`{test_command}`、`{review_mode}`、`{review_depth}`、`{risk_reasons}`。不要增删改写其余内容。

=== PROMPT START ===

你的唯一任务：对 story `{story_key}`（spec `{story_path}`）执行**一次** BMad `bmad-code-review`，读取 story-local diff `{diff_file}` 和 dev receipt `{dev_receipt}`，按风险选择审查深度，自动处理确定性问题，并写入 review receipt `{review_receipt}`。

## 自动化契约

调用 `bmad-code-review` 时必须显式提供：

```text
review_depth: {review_depth}
risk_reasons: {risk_reasons}
review_mode: {review_mode}
action_policy: autofix
unattended: true
story_key: {story_key}
spec_file: {story_path}
diff_file: {diff_file}
receipt_file: {review_receipt}
test_command: {test_command}
sprint_status: {sprint_status}
```

不得让 code-review 自己重新选择目标、重新构造 `git diff HEAD`、询问 spec、等待 checkpoint 或跨项目加载 fallback skill。

## 风险分类

外层编排器已经基于 `{diff_file}` 和 `{dev_receipt}` 完成风险分类；你必须使用传入的 `{review_depth}` 和 `{risk_reasons}`，可以复核但不得降级。若输入与文件证据冲突，只能升级为 `deep` 并在 receipt 记录冲突。

风险规则（供复核）：

- `lite`：仅文档/注释/格式/测试数据/简单配置/孤立常量；不改变运行时行为；不超过 2 个文件和约 80 行；测试已通过；没有高风险信号。
- `standard`：普通业务逻辑、一般 UI、常规服务和测试变更。
- `deep`：认证、权限、安全、token、密钥、API、schema、协议、序列化、网络、JSON 持久化、迁移、并发、异步、重试、状态机、定时器、硬件、GPIO、电源、OTA、固件、驱动、构建、依赖、CI、生成代码、删除、跨组件、公共接口、测试失败、超过 5 个生产文件、约 300 行，或无法确定风险。

风险优先于体量；无法判断时必须选 `deep`，不得降级为 `lite`。

## 审查层路由

- `lite`：仅 `edge-case-hunter`，零 findings 合法。
- `standard`：`edge-case-hunter` + `verification-gap`；有 spec 时增加 `acceptance-auditor`。
- `deep`：全部审查层；有 spec 时包含 `acceptance-auditor`。
- 只有 active/mandatory layer 全部成功，才能把零 findings 当作 clean。
- Blind Hunter 不得制造最低数量 findings；没有证据就返回零发现。

## 自动修复策略

- 只自动应用 triage 明确标记为无歧义、兼容现有公共面的 `patch`。
- `false` 记录驳回证据；低风险 pre-existing 和 `maybe-false` 自动 defer。
- `decision-needed` 只有在项目规则给出唯一保守选择时才自动采用；否则记录 `unresolved`。
- unresolved HIGH/MEDIUM、mandatory reviewer 失败、测试失败、receipt 缺失或 File List 与实际 diff 不一致时，不能推进 `done`。
- 审查未闭合时保持 `{sprint_status}` 中的 `review`，绝不回退到 dev-story。

## 单轮与禁止事项

- 本 child 只执行一次 gather → review → triage → action。
- 不得自己再次 review、不得递归派生 reviewer、不得内部 review→fix 循环。
- 不得等待人类或发送 numbered choice。
- 不得修改 skill 定义、项目规则、无关文件或执行 commit/branch/stash/reset/checkout/merge。
- patch 后必须再次执行 `{test_command}`；命令为空或退出码缺失/非 0 都是 verification-failed，记录退出码。

## 完成回报

只输出以下结构化字段，并确保 `{review_receipt}` 已写入 JSON：

- story_key：{story_key}
- spec_file：{story_path}
- diff_file：{diff_file}
- review_mode：{review_mode}
- review_depth：<lite | standard | deep>
- risk_reasons：<逐项列出>
- active_layers：<逐项列出>
- mandatory_layers：<逐项列出>
- completed_layers：<逐项列出>
- failed_layers：<逐项列出；无则为 []>
- findings：<总数>
- patches：<应用数>
- deferred：<数量>
- rejected：<数量>
- unresolved_high_medium：<数量>
- test_command：{test_command}
- test_exit_code：<整数或 null>
- outcome：<clean | autofixed | review | blocked>
- failure_reason：<null 或 mandatory-review-failed / verification-failed / unresolved-high-medium / receipt-incomplete>
- phase：C
- ok：<true 表示 receipt 完整；即使 outcome=blocked 也必须为 true；false 仅表示无法写完整 receipt>
- 新状态：<done | review | blocked>
- 阻塞：<无 / 精确描述>

=== PROMPT END ===
