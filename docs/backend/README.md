# docs/backend — 软件层·后端（EWF backend）

> 范围：EWF `cloud/backend`（Spring Boot，JSON 零库，AD-16/17/18）。经验源= `miaowu`（HEAD b1e0a660，2026-09-08 迁移）——沿用其 REST `/api/v1` + `{code,message,data}` 信封、错误码、全局异常、微信登录、JWT、env-scripts 骨架。后端**模块实现规格** `requirements.md` 仍待 backend spec 阶段（PRD addendum §4），本目录当前只承载迁移的开发经验。

## 文档索引
| 文件 | 内容 |
| --- | --- |
| `经验-后端REST信封与微信登录.md` | miaowu 后端经验：信封/错误码/全局异常/JWT/微信登录坑/并发建号(JSON 零库版)/时区/env-scripts；每节「miaowu 做法 → EWF 怎么用」 |

## EWF 边界
- **单实例单进程、JSON 原子文件持久化（零数据库，存 `cloud/backend/data/`，AD-16）**；只用 miaowu 的 controller/service/DTO/信封/JWT 结构，**不迁 repository/SQL/Flyway**。
- 单身份单设备：微信登录→固定 `device_id`（FR-B-001）；无角色/租户/starId/订阅治理。
- 日界/可信时间按 backend 配置时区（AD-6）。

## 排除表（miaowu 未迁）
| 主题 | 理由 |
| --- | --- |
| PostgreSQL + Flyway + RowMapper/触发器/GIS | EWF JSON 零库 |
| 角色自适应/租户 membership/`@RequireRole` AOP/starId/订阅冻结 | EWF 单身份单设备 |
| Jeepay 支付/华为 OBS/图片审核 | EWF 无支付/无对象存储 |
| frontend-web 管理后台/多域部署/cloudflared 支付回调隧道 | EWF 本地联调可简化为本地起停+真机预览 |
