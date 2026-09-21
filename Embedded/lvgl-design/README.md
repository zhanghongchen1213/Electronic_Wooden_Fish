# lvgl-design —— 设备侧 UX 真源

本目录只承载冻结的 Pencil 设计源：

- ewf-device-ui.pen：DEVICE-01 设备侧 UX 唯一视觉真源。

设备画布固定为 410×502，圆弧屏安全区、三主页 resident shell、下滑设置页、状态栏、7 字带、经文页和完成遮罩均以该 Pen 为准。

## 当前交互口径

- 设备保留木鱼页、经文页、统计页和按需设置页。
- 木鱼页只允许有效设备输入进入统一敲击队列；经文页与统计页不产生屏幕敲击。
- 经文页使用连续 append-only 历史流，每行 13 个字符槽；最新字在流尾高亮。
- 三环金光是 160ms 运行时反馈，母版只保留 idle 描边。
- 统计页区分今日敲击与累计敲击；未校时显示待校时。
- 设置页默认音量 50、中亮度、15 秒熄屏；同步状态与待设备应用语义按 UX 契约执行。

## 导出约定

同名 HTML 由作者从 ewf-device-ui.pen 导出，落在本目录时使用 ewf-device-ui.html。仓库不再保存方向稿、候选板、导出副本、SquareLine 生成工程或旧校验脚本。

## 下游实现

后续 SquareLine、LVGL C 和 bindings 工程在实现阶段重新建立；实现必须先读取冻结 Pen，并按 UI_CONTRACT-device.md、DESIGN.md、EXPERIENCE.md 对拍。

