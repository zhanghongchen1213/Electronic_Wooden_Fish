# Electronic_Wooden_Fish — Claude 工作规则

本文件与根目录 `AGENTS.md` 同步，`AGENTS.md` 是仓库唯一强制规则层。

## 最终页面文案硬禁令

- 最终设备 UI、Pen 画面、静态 HTML、uni-app 页面、前端可见文案和交付截图中，严禁出现 AI 思维链、内部推理、设计过程、调试说明、Node ID、`data-pencil-id`、`TODO`、`draft`、`placeholder` 或作者提示。
- `qljP7`、`WfAs7`、`Z6Qge`、`e8Sgp`、`gGgAm`、`NtM6r` 等内部节点标识不得进入用户可见文本或生产 HTML 属性；交付 HTML 必须清除 `data-pencil-id`。
- 页面只允许产品文案、状态文案和 canonical《心经》内容。设计意图、实现解释和校验备注只存在于文档或工作日志。
- UX 交付前完成 Pencil 文案节点扫描和逐屏视觉检查；作者导出同名 HTML 后再运行 HTML 卫生检查，失败时不得进入 SquareLine 或 uni-app 复刻。

## UX 真源

- PRD、产品简报、架构主干决定行为与数据语义。
- `DESIGN.md` 决定视觉身份，`EXPERIENCE.md` 决定行为体验。
- `UI_CONTRACT-device.md` 与 `UI_CONTRACT-miniapp.md` 决定两份冻结 Pen 的页面与状态闭包。
- `.pen`、作者导出的同名 HTML 和前端实现不得自行发明产品状态或文案。
