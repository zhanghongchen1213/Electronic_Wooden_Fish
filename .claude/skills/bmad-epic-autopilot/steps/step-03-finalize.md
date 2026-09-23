# Step 3: Finalize & Report

## RULES

- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`
- 本步骤只汇报，不派发子 Agent、不做 git 操作。

## INSTRUCTIONS

按进入本步骤的分支，输出对应报告，然后**结束工作流**。

### 分支 1：成功（队列全部 done）

```
✅ Epic-{epic_num} 自动编排完成
本次推进到 done 的 story：<逐行列出本次跑过的 key>
队列总计：<N> 个 story，全部 done。
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
⛔ Epic-{epic_num} 自动编排已中止
卡点 story：{current_story_key}
失败阶段：<A create-story | B dev-story | C code-review>
当前实际 status：<从 sprint-status.yaml 读到的值>
子 Agent 报告的阻塞：<精确原因>
已完成的 story：<本次中止前已 done 的 key 列表>
处置建议：人工介入该 story 后，可重新触发 `epic-{epic_num}` 续跑（工作流会从第一个非 done story、按其当前 status 自动选起跑阶段）。
```

### 分支 4：fail-closed（输入/证据/审查门禁失败）

```
⛔ Epic-{epic_num} 自动编排已安全终止
卡点 story：{current_story_key}
阶段：<A create-story | B dev-story | C code-review>
阻塞类型：<sprint-status-invalid | missing-story | missing-receipt | diff-scope-mismatch | mandatory-review-failed | verification-failed | unresolved-high-medium | unknown-status>
当前实际 status：<从 sprint-status.yaml 读到的值或 unavailable>
证据：<receipt/diff/test/reviewer 失败的精确路径和摘要>
已完成的 story：<本次终止前已 done 的 key 列表>
处置：未继续派发后续 story，未等待人类，未将未闭合审查标为 done。
```

`review` 未闭合属于 C 阶段失败；普通失败最多重试一次，第二次仍未满足 mandatory
review、测试和 receipt 门禁时进入本分支，不回退到 dev-story。若子 Agent 在真正启动前明确因模型容量错误退出，且没有任何状态、story、代码或 receipt 变化，则该次不计入尝试次数，应继续派发全新 child；一旦出现状态或文件变化，则按普通失败处理。

## END

工作流结束。不自动运行 retrospective（它是 optional，超出本工作流范围）。
