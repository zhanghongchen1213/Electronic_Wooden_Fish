# cloud 代理工作说明

本文件是 `cloud/` 软件层的编码与验证入口。cloud 只作为一个产品层存在，`backend/` 与 `frontend/` 是实现目录，不得在需求、Epic、Story 或验收中重新拆成两个产品。

## 1. 权威来源

按以下顺序读取事实：

1. `docs/hardware/定稿/`、`docs/hardware/电子木鱼-硬件网络清单.json`：硬件和设备状态字段事实。
2. `cloud/miniapp-design/ewf-miniapp-ui.pen` 与 `ewf-miniapp-ui-export.html`：小程序视觉真源和对拍基线。
3. `_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md`：产品行为、FR-C 需求、跨层合同和非目标。
4. `_bmad-output/planning-artifacts/architecture/`、`_bmad-output/planning-artifacts/ux-designs/`：架构不变量、视觉令牌和状态闭包。
5. `docs/backend/`、`docs/frontend/`：迁移经验和排障资料，只能作为实现参考。

现有迁移代码不是产品事实源。发现冲突时先记录冲突，再按上述优先级收敛。

## 2. cloud 层边界

- `cloud/backend` 保存身份、`acked_total`、经文游标、轮次、完成状态、命令修订、统计和设备状态的权威数据。
- `cloud/frontend` 只呈现 cloud 已确认的数据，不产生正式敲击，不把本地待同步数据伪装成正式进度。
- Embedded 与 cloud 通过冻结的 HTTPS JSON 同步合同通信；WebSocket 只用于 cloud 到小程序的已确认状态推送。
- 不引入第二套后端、数据库、设备身份模型或平行同步协议。
- MVP 不增加 BLE、Wi‑Fi、GPS 业务链路、OTA、多设备、迁移、社交或付费能力。

## 3. 接口与持久化规则

- 对外接口统一使用 `/api/v1` 和 `{code,message,data}` 信封，成功 `code=0`。
- 业务错误使用 HTTP 200 加业务码；协议错误使用准确 HTTP 状态，不把所有错误包装成成功。
- 高水位同步必须幂等：相同或更低 `local_total` 不得重复推进；响应必须可用于 Embedded 与小程序对账。
- 命令使用单调递增 `command_revision`。设备未应用时显示“待设备应用”，不能提前显示“已生效”。
- backend 使用 JSON 原子文件持久化：临时文件、写入、`fsync`、`rename`；禁止引入 SQLite、PostgreSQL 或其他数据库服务。
- canonical《心经》只能有一个来源，三端的 `scripture_version` 必须一致；版本不一致时停止推进并报告配置错误。
- WebSocket 差量必须带单调序号；断线恢复以查询快照水位为准，不能以 frontend 自身展示计数推断权威状态。

## 4. 鉴权和敏感数据

- 微信登录先取得并解析 code2Session 响应，再建立单身份单设备会话；`session_key` 不落盘、不进日志，openId 必须脱敏。
- 401 刷新使用单飞队列：并发请求只刷新一次；刷新失败必须唤醒所有等待者；刷新后的请求最多重试一次，仍失败统一回登录页且不重复跳转。
- 设备 Token、JWT、微信会话材料和内部配置不得进入前端包、页面文案或普通日志。
- 任何外部 HTTPS、域名、AppID、密钥和端口都从配置读取，不硬编码到页面或测试快照。

## 5. 小程序视觉与文案

- 页面必须完全依据 `cloud/miniapp-design/ewf-miniapp-ui.pen` 和同名 HTML 复刻，不从历史候选稿或迁移项目复制布局。
- 对拍覆盖 LOGIN、READING、RECORDS、DEVICE、SETTINGS、OVERLAY 的全部状态；每行 17 槽、最新字焦点、唯一心经进度、四项底部导航和设置待应用语义必须保持一致。
- 小程序不出现可点击电子木鱼、不产生正式敲击、不预览未来经文。
- 页面可见文案只能是产品文案、状态文案和 canonical《心经》内容。禁止出现 AI 思维链、调试说明、Node ID、`data-pencil-id`、TODO、draft、placeholder 或作者提示。
- 颜色不能是唯一状态通道；同步、失败、待应用和权限错误同时使用文字或图标表达。

## 6. 测试与并行开发

- cloud Epic 4–6 必须可使用 mock device、mock API、mock WebSocket 或固定快照独立验证，不等待真实 Embedded 完成。
- 修改同步字段、命令修订、序号、错误码或状态枚举时，先更新契约样例和测试，再修改实现。
- backend 至少覆盖：高水位重复/回退、版本不一致、轮次完成、命令乱序、原子写恢复、401 单飞刷新和断线补齐。
- frontend 至少覆盖：首登空态、逐字追加、回放排队、完成弹窗、待设备应用、失败重试和权限错误。
- Epic 7 才执行真实 Air780EGP HTTPS、设备 ACK/应用修订、WebSocket 与小程序端到端联调；联调结果必须回写 PRD 所引用的合同或验证文档。

## 7. 文件与编码

- 文本文件使用 UTF-8 无 BOM；修改中文文件前确认没有 `FF FE`、乱码或错误替换字符。
- 搜索优先使用 `rg`/`rg --files`；不要用默认 PowerShell 重定向写仓库文件。
- 每次修改完成后检查 `git diff --check`，并确认没有新增无关文件、平行实现或重复契约。
