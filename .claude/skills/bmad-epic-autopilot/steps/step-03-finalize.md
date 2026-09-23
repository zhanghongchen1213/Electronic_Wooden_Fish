# Step 3: Finalize & Report

## RULES

- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`
- 本步骤只汇报，不派发子 Agent、不做 git 操作。质量债务、测试失败和 reviewer 未完成不进入本步骤的阻断分支；它们在成功报告中汇总。

## INSTRUCTIONS

按进入本步骤的分支，输出对应报告，然后**结束工作流**。

### 分支 1：成功（队列全部 done）

```
✅ Epic-{epic_num} 自动编排完成
本次推进到 done 的 story：<逐行列出本次跑过的 key>
队列总计：<N> 个 story，全部 done。
质量状态：<clean/degraded/unverified 汇总>
质量债务：<数量及账本路径>
```

### 分支 2：无可执行 / 已完成（来自 step-01）

```
ℹ️ Epic-{epic_num} 无可执行 story。
原因：<epic 不存在 / 队列为空 / 所有 story 已是 done>。
未派发任何子 Agent，未做任何改动。
```
（这是 epic6 这类"全 done"epic 的预期优雅结束路径。）

### 分支 3：中止（来自 step-02 失败）

```
⛔ Epic-{epic_num} 自动编排因外部能力不可用而终止
卡点 story：{current_story_key}
失败阶段：<A create-story | B dev-story | C code-review>
当前实际 status：<从 sprint-status.yaml 读到的值>
子 Agent 报告的阻塞：<精确原因>
已完成的 story：<本次中止前已 done 的 key 列表>
外部阻断：<model/tool-unavailable | filesystem-unavailable | required-external-input | irrecoverable-fact-source | unsafe-path-unisolated>
已执行的自动恢复：<normal/compact/ultra-compact/repair/degrade 摘要>
后续：不等待人工；外部能力恢复后重新触发 `epic-{epic_num}`，工作流将从第一个非 done story 续跑。
```

### 分支 4：fail-closed（输入/证据/审查门禁失败）

```
⛔ Epic-{epic_num} 自动编排已安全终止
卡点 story：{current_story_key}
阶段：<A create-story | B dev-story | C code-review>
阻塞类型：<external-capability-unavailable | unsafe-path-unisolated | irrecoverable-fact-source | filesystem-unavailable>
当前实际 status：<从 sprint-status.yaml 读到的值或 unavailable>
证据：<receipt/diff/test/reviewer 失败的精确路径和摘要>
已完成的 story：<本次终止前已 done 的 key 列表>
处置：已完成所有可用自动恢复和保守降级；未等待人类，未把失败测试伪装成通过。
```

`review` 未闭合先进入 repairing-review、evidence repair 或 conservative degrade。
只有模型/工具、文件系统、必需外部输入确实不可用，或危险路径无法修复/隔离时才进入本分支。

## END

工作流结束。不自动运行 retrospective（它是 optional，超出本工作流范围）。
