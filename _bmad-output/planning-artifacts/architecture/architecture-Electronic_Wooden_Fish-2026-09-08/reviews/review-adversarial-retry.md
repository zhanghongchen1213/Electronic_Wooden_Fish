---
name: reviewer-gate-adversarial-retry
type: architecture-consistency-review
scope: AD-1..AD-20, ARCHITECTURE-SPINE.md, cloud/env-scripts/*
mode: read-only-re-review
date: 2026-09-22
---

# Reviewer Gate 简短复审

## Verdict

**CONDITIONAL PASS（AD-20 主要缺口已修正；正式部署仍有阻断项）**。

本次确认：本地启动/停止已固定 9218；构建生成 `saas.jar.sha256`；上传前校验 Jar 与 sidecar 且上传二者；`VITE_API_BASE_URL` 已明确为包含 `/api/v1` 的 REST base，WebSocket 由契约派生。此前 F1（端口优先级）、F2（完全无校验的同名 Jar）、F3（REST/WS base 语义）不再作为当前 Gate finding。

## Remaining findings

### F1（P1，正式部署阻断）明文 SSH 凭据与主机身份校验仍不安全

`upload-backend-jars.sh` 仍把 SSH 密码写入脚本，并在 `sshpass`/ASKPASS 路径中暴露；`StrictHostKeyChecking=no` 与 `/dev/null` known-hosts 禁用了主机身份校验。原型风险已在 Spine 接受，但正式部署前必须迁移到受控 secret、SSH key 与固定 known_hosts。

### F2（P1）SHA-256 sidecar 不是供应链/新鲜度证明

构建生成 `.sha256`、上传前验证并上传二者，已解决主要的误替换路径。但旧 Jar 与旧 sidecar 可整体复用，无法证明上传物来自本次提交或受信 CI。正式 CI/CD 应绑定 commit/build ID、保留 provenance，并使用受信制品仓库或签名校验。

### F3（P2）根目录解析与进程清理仍依赖环境假设

构建/上传脚本仍从多个环境变量、`$PWD` 和向上搜索选择仓库根；误设 `EWF_PROJECT_ROOT` 或另一 checkout 仍可能选择错误工程。`start`/`stop` 仍以 9218、`pkill` 命令行匹配和未验证 PID 文件清理进程，端口复用时可能误杀其他服务。建议验证唯一绝对根目录、构建 ID 和进程身份。

### F4（P2）frontend 发布 fail-closed 尚未落地

`.env` 的 API 域名与 `VITE_WX_APPID` 仍是 `replace-with-*` 占位值。当前只有文档要求 backend/frontend AppID 一致，没有构建时拒绝占位符、缺少 `/api/v1` 或 WS 派生失败的门禁。正式发布前应让构建直接失败。

## 对抗性复核结论

原 Scenario A（profile/CLI 端口分裂）已被固定 9218 消除；原 Scenario B（无关联的跨 checkout 同名 Jar）已被 Jar+sidecar 校验显著收窄，但仍可复用旧 Jar+旧 sidecar，转化为 F2 的 provenance/新鲜度问题。当前不建议标记 unconditional PASS，直到 F1/F2/F4 有可执行证据。
