# docs/contracts — 跨层契约

**唯一权威**：`sync-contract.md`（跨层同步字段、状态词表、`round_id`、`command_revision`=已应用高水位、快照水位 S/序号、JSON schema 与文件粒度、WebSocket 帧/心跳等）。

- 当前**尚未创建**：按 `epics.md` Story **S0.1（跨层同步数据契约冻结）** 前置冻结，位置即本目录 `sync-contract.md`。
- 在 S0 落地前，任何 backend / Embedded / frontend 模块的实现 spec 拆分都应以此为先行输入（架构 Deferred：A-2/A-1 闭合条件）。
- 相关权威引用：`ARCHITECTURE-SPINE.md`（同步字段清单 §Structural Seed）与 PRD §7。
