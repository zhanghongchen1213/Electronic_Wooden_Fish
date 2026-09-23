# 编排级 Definition of Done（每个 story 收尾自检）

主 Agent 在宣布某个 story `done`、循环取下一个之前，逐项核对。任一项不满足 → 当作失败，转 `steps/step-03-finalize.md` 失败分支。

## 阶段完整性
- [ ] 该 story 实际经过了所需的全部阶段（按起跑 status：backlog→A/B/C；ready-for-dev→B/C；review→C），无跳阶段。
- [ ] 阶段 A、B、C **各自由一个全新 general-purpose 子 Agent** 执行，无复用上一个子 Agent 的上下文。
- [ ] 每个阶段的子 Agent 提示词都是从 `subagent-prompts/` 对应模板**原样加载**、仅替换变量，未即兴改写。
- [ ] child 没有递归派发、没有内部 review→fix 循环；普通业务/证据失败同一阶段最多由编排器重试一次；明确的模型容量启动失败且零状态/零 receipt 不计入次数，持续用全新 child 重试。

## 状态推进验证（以 sprint-status.yaml 为唯一裁判）
- [ ] 阶段 A 后，grep 验证该 story 由 `backlog` → `ready-for-dev`（若起跑即跳过 A 则免）。
- [ ] 阶段 B 后，grep 验证该 story → `review`。
- [ ] 阶段 C 后，grep 验证该 story → `done`。
- [ ] 每次验证都是**重读 sprint-status.yaml 实际行**得出，未轻信子 Agent 自述。
- [ ] 涉及 ESP-IDF 的 B receipt 记录每次构建日志、退出码、最早错误/环境诊断、修复和最终退出码；`127` 已先重新激活并重建成功，未把环境错误直接当作终止。
- [ ] 每次阶段都有机器 receipt；C 的 receipt 明确记录 `review_depth`、mandatory layers、failed layers、findings、测试退出码和 outcome。
- [ ] C 之前已持久化风险分类 JSON；receipt 经过 helper 结构校验，深度只能保持或向 `deep` 升级。
- [ ] `done` 只在 mandatory reviewer 全部成功、无 unresolved HIGH/MEDIUM、patch 后测试通过且 receipt 完整时写入。

## 失败与重试纪律
- [ ] 任一阶段未达期望状态时，**仅重试 1 次**（全新子 Agent），未做第 2 次以上重试。
- [ ] 重试后仍未达标的，已**中止整个 epic**（未跳过该 story 继续下一个）。
- [ ] `review` 未闭合时保持 `review`，没有错误回退到 dev-story；未知状态、坏 YAML、缺文件、空 diff 均 fail-closed。

## 边界约束
- [ ] 全程**零变更性 git 操作**（无 commit/branch/stash/reset/checkout/merge），改动留当前分支工作区。
- [ ] 主 Agent 全程未亲自执行 create-story/dev-story/code-review，未亲自写业务代码，未读 epics/architecture/UX/实现代码。
- [ ] 主 Agent 上下文中该 story 仅以 1 行状态留存，无长过程回灌。
- [ ] C 使用 story-local diff，不使用累计 `git diff HEAD`；实际 changed paths 与 B receipt 的 File List 完全一致。
- [ ] 所有技能查找仅限当前项目 `.agents/.claude` 或注册表，没有跨项目 fallback。
