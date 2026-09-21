# miniapp-design —— 微信小程序 UX 真源

本目录只承载冻结的 Pencil 设计源：

- ewf-miniapp-ui.pen：MINI-06 小程序 UX 唯一视觉真源。
- assets/paper-fiber-128.png：Pen 使用的本地纸张纹理。

页面闭包固定为 LOGIN、READING、RECORDS、DEVICE、SETTINGS、OVERLAY，共 14 个状态语义。画布基线为 390×844，顶部安全区 62px，内容左右边距 20px，底部导航高 56px。

## 当前交互口径

- 小程序只呈现 backend 已确认进度，不提供电子木鱼，也不产生敲击。
- 阅读流每行保持 17 个字符槽，标点计槽；新字追加到已有前缀后的下一个槽位。
- 正文槽使用 Noto Serif SC 18px，最新字使用 28px 焦点色并持续带 2px 下划线。
- 进度只保留一组，显示已确认字数、总字数和百分比。
- 设置页提供音量 0–100 滑杆、亮度低/中/高分段、熄屏 5/15/30 秒分段；状态区分待设备应用和已生效。
- 完成弹窗保留全文，并提供从头开始、退出两个完整按钮。

## 导出约定

同名 HTML 由作者从 ewf-miniapp-ui.pen 导出，落在本目录时使用 ewf-miniapp-ui.html。仓库不再保存方向稿、候选板、导出副本、组装脚本或旧校验工具。

## 下游复刻

uni-app 页面按冻结 Pen 与作者导出的同名 HTML 对拍；数据接 Pinia、mock 和同步契约，不从历史候选稿取值。

