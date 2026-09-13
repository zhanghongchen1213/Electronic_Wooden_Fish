# 电子木鱼交接：UX 真源已冻结

- **更新时间**：2026-09-13
- **当前状态**：planning 已完成；设备侧 DEVICE-01 与小程序侧 MINI-06 的 UX 框架均已锁定。
- **实现状态**：Embedded、backend、frontend 仍未进入实现；Stage 5 尚未开始。
- **HTML 约定**：同名 HTML 由作者从 Pen 导出，本仓库不保存导出脚本或自动组装器。

## 1. 唯一视觉真源

| 表面 | 冻结 Pen | 基线 |
| --- | --- | --- |
| 设备 OLED | lvgl-design/ewf-device-ui.pen | DEVICE-01，410×502，三主页 resident shell |
| 微信小程序 | miniapp-design/ewf-miniapp-ui.pen | MINI-06，390×844，单栏阅读流 |

UX 规范位于 _bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/：

- DESIGN.md：视觉令牌、字体、布局和组件外观。
- EXPERIENCE.md：信息架构、行为、状态和交互。
- UI_CONTRACT-device.md：设备页面与状态闭包。
- UI_CONTRACT-miniapp.md：小程序页面与状态闭包。

四份规范当前均为 final，状态语义仍按设备 15 帧、小程序 14 帧维护。

## 2. 设备侧冻结口径

- 设备 Pen 由 DEVICE-01 母版 hbTEa 承载。
- 保留木鱼页、经文页、统计页、下滑设置页和常驻状态栏。
- 经文页每行 13 个字符槽，最新字位于流尾并高亮。
- 木鱼页三环反馈为 160ms 运行时描边切换，母版只保留 idle 描边。
- 统计页区分今日敲击与累计敲击；设置默认音量 50、中亮度、15 秒熄屏。
- SquareLine、LVGL C、字体合同和 bindings 在实现阶段重新建立。

## 3. 小程序侧冻结口径

- 小程序 Pen 由 MINI-06 母版 A358t 承载。
- 页面为 LOGIN、READING、RECORDS、DEVICE、SETTINGS、OVERLAY，共 14 个状态语义。
- 阅读流只呈现 backend 已确认内容；每行 17 个字符槽，标点计槽。
- 正文槽统一 18px；最新字 28px，落在已有前缀后的下一个槽位，带持续 2px 下划线。
- 阅读页只保留一组心经进度。
- 设置页使用音量 0–100 滑杆、亮度低/中/高分段、熄屏 5/15/30 秒分段。
- 完成弹窗提供从头开始与退出两个完整按钮。
- 小程序不提供电子木鱼，也不产生敲击。

## 4. 已清理的中间产物

方向稿、候选风格板、导出 HTML 副本、组装脚本、旧校验工具、SquareLine 规划工程和临时素材均已从工作区删除。历史决策只保留在 PRD、UX 规范和审计记录中，不再作为现役输入。

## 5. 下一步

1. 作者从两份冻结 Pen 分别导出同名 HTML。
2. Embedded 依据设备 Pen 和 UI_CONTRACT-device 建立 SquareLine/LVGL 实现链。
3. frontend 依据小程序 Pen、作者导出的 HTML 和 UI_CONTRACT-miniapp 逐页复刻。
4. 任何视觉或状态变更先更新对应 UX 规范，再修改实现。

## 6. 当前证据

- 两份 Pen 已由 Pencil 保存并可在桌面端打开。
- 小程序 Pen 的页面边界扫描无问题，过程性可见文字扫描为空。
- 小程序阅读行结构为 17 槽，最新字位于下一个已用槽位。
- 本轮未修改任何导出脚本、HTML 或前端源码。

