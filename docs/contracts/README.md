# docs/contracts — 跨层契约

本目录承载两类**只定义一次**的跨层真源，分工互不重叠：

| 子目录 / 文件 | 权威内容 |
| --- | --- |
| `sync-contract.md` | 跨层同步协议：统一字段、事件来源枚举、状态词表、轮次与序号边界、恢复与冲突路径、命令修订分层（`command_revision` 为已下发最新修订、`applied_revision` 为设备已应用高水位）与待应用命令载荷字段、JSON 持久化文件粒度与持久化作用域（含单事务分组）、WebSocket 帧/心跳/重连参数与同步域错误码 |
| `sync-contract.schema.json` | 上述契约的机器可读注册表（派生自正文，供 Epic 4–6 的 mock 与用例直接消费） |
| `tests/run_sync_contract_tests.py` | 契约门禁：正向逐值比对与必须失败的负例（负例在临时副本上执行） |
| `canonical/` | canonical《心经》**经文资源**：唯一正文落盘来源、`scripture_version` 与三端经文产物，详见 `canonical/README.md` |

**契约已冻结**：`contract_version` = `SC-1.0.0`，正文与注册表同时生效。字段表、事件来源枚举与
WebSocket 参数只在该契约中定义一次；PRD、`ARCHITECTURE-SPINE.md`、`docs/embedded/`、
`docs/backend/`、`docs/frontend/` 与 UX 规范一律只引用、不复制。`contract_version` 变更时，
本文的登记值与契约正文、注册表同步更新。

- 架构 Deferred 的 `[ASSUMPTION A-2]`（JSON schema 与文件粒度）与 WebSocket 帧/心跳/重连
  参数已由该契约收敛；生产反向代理空闲超时仍属部署阶段 Deferred。
- 任何 backend / Embedded / frontend 模块的实现 spec 拆分都以该契约为先行输入。
- 相关权威引用：`ARCHITECTURE-SPINE.md`（同步字段清单 §Structural Seed）与 PRD §7。
