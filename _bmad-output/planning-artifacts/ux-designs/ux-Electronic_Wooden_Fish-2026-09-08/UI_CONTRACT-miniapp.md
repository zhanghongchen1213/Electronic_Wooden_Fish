---
name: Electronic_Wooden_Fish
description: "微信小程序阅读、记录、设备与设置页面闭包。"
type: ui-contract
surface: miniapp
status: final
created: 2026-09-08
updated: 2026-09-13
selected_style: MINI-06
source_master: A358t
frozen_sources:
  device_pen: lvgl-design/ewf-device-ui.pen
  miniapp_pen: miniapp-design/ewf-miniapp-ui.pen
  html_export: author-managed
  export_scripts: removed
sources:
  - "{planning_artifacts}/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/DESIGN.md"
  - "{planning_artifacts}/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/EXPERIENCE.md"
  - "{planning_artifacts}/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
  - "{planning_artifacts}/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
style_candidates: 1
authority: "UX spines (DESIGN.md / EXPERIENCE.md) > 本契约 > pen/HTML > uni-app pages"
---

# UI_CONTRACT · 微信小程序轨（uni-app / 阅读纸面）

> 本契约把 UX 契约落到可复刻闭包。小程序不提供电子木鱼，也不是输入源；只呈现 backend 已确认进度。MINI-06 的 Pen 为冻结视觉真源，同名 HTML 由作者导出。

> **无盒化原则**：只保留语义必要的填充面；其余页面使用 1px 刻线与留白分层。印谱签名层以小型居中印章与低对比阅读基线表达。

## 1. 画布与令牌映射（→ uni-app scss）

Pen 与作者导出 HTML 的对拍基线：手机画布 390×844；顶部安全区 62px；页面左右边距 20px；底部导航 56px，并保留底部安全间距。实际微信设备只做等比例缩放，不改变单栏阅读语义。

| 令牌 (DESIGN) | scss 变量（建议） | 用法 |
| --- | --- | --- |
| `{colors.mini.surface.0}` | `$bg` | 页面纸面底 |
| `{colors.mini.surface.1}` | `$card` | 卡片/弹窗 |
| `{colors.mini.surface.2}` | `$fill-muted` | 禁用/分隔底 |
| `{colors.mini.text.primary}` | `$ink` | 正文 |
| `{colors.mini.text.secondary}` | `$ink-2` | 说明 |
| `{colors.mini.text.muted}` | `$ink-3` | 空态/弱化 |
| `{colors.brand.amber.400}` / `{colors.brand.amber.300}` | `$accent` / `$accent-bright` | 当前字/进度/完成/重点 |
| `{colors.mini.divider}` | `$divider` | 分隔线 |
| `{colors.mini.focus}` | 见 `$accent` | 阅读行当前字 |
| miniapp reading / reading-focus / body / caption | font-reading / font-reading-focus / … | 18/28/16/12，正文行高 1.7 |

## 2. 命名规则

`[UI][PAGE:<PageId>][ST:<StateId>][CMP:<CompId>][VAR:<Name>]`
PageId：`LOGIN|READING|RECORDS|DEVICE|SETTINGS|OVERLAY`；CMP 只能使用 §5 的 canonical 名称。
- canonical frame 固定 390×844；页面使用 MINI-06 纸面、居中印章、低对比阅读基线和单栏信息流。full frame 不添加外层展示板或装饰背景。

## 3. 页面闭包表（pen/HTML 帧 → uni-app 页，绑定 FR-F）

| PageId | uni-app 页面 | 内容 | 绑定 |
| --- | --- | --- | --- |
| READING | pages/reading | 持续累积心经正文（在线/回放/断线）+ 总进度百分比 + 完成弹窗 + 空态 | FR-F-002/003/004/005；S3.2~S3.5 |
| RECORDS | pages/records | 今日/近7/近30/累计/连续 | FR-F-006；S4.1 |
| DEVICE | pages/device | 电量/4G/最后同步/待同步/待设备应用/失败+刷新/立即同步/重试 | FR-F-007；S4.2 |
| SETTINGS | pages/settings | 音量/亮度/熄屏 镜像下发（离线=待设备应用） | FR-F-008；S4.3 |
| LOGIN | 启动/直达 | 登录直达唯一设备（引导/权限错误） | FR-F-001/010；S3.1 |
| OVERLAY | 全局弹窗 | 完成礼花 + 从头开始/退出 | FR-F-005；S3.5 |

## 4. 状态闭包（帧清单）

Pen 中每个 PageId 只保留 canonical screen；局部状态进入组件状态板。状态语义保持 14 帧；同名 HTML 由作者从冻结 Pen 导出。OVERLAY.DONE 保留为独立遮罩 screen。

```
[LOGIN][BASE] 启动直达引导
[LOGIN][PERM_ERROR] 权限错误（一句话+重开授权）
[READING][LIVE] 在线追加：已有正文保留、最新字放大高亮、下划线贴在最新字下方、总进度百分比
[READING][REPLAY] 离线回放中（顶部提示「回放中 · 新事件排队」）
[READING][OFFLINE] 断线（查询/回放模式 banner）
[READING][EMPTY] 无历史/首登空态
[READING][DONE] 完成 100%：全文保留，等待完成弹窗操作
[OVERLAY][DONE] 独立完成弹窗：礼花 + 从头开始/退出（覆盖阅读背景）
[RECORDS][BASE] 五统计
[RECORDS][EMPTY] 无历史空态
[DEVICE][BASE] 电量/4G/最后同步/待同步/待设备应用/失败
[DEVICE][FAIL_RETRY] 失败 + 重试入口
[SETTINGS][BASE] 三设置默认
[SETTINGS][PENDING] 下发后待设备应用（非已生效）
```

## 5. 组件清单

| CompId | 组件 | 规格 |
| --- | --- | --- |
| readingline | 累积阅读流 | 每行固定 17 个字符槽，标点计槽；backend 已确认字按序 append，已有前缀保留、自然换行、可回看；只保留一个最新焦点 |
| char-focus | 当前字聚焦 | 正文 18px，最新字 28px，使用焦点色；最新字保持高亮，直到下一次确认；2px 下划线紧跟最新字下方 |
| reading-archive | 已完成篇章 | **行为规格**：由 `READING.DONE` 承载，非独立视觉帧；完成后保留全文，“从头开始”在下方开启新 `round_id` 区块，旧篇可折叠 |
| scripture-progress | 心经总进度 | 仅一组组件；显示 `confirmed_chars / scripture_chars_total · percent%`，轨道与文本同时表达，percent 限制 0–100。填充色 `{colors.mini.focus}`；含等高等亮的刻度尺（纯质感，不编码数据） |
| statcard | 统计卡 | `components.mini.statcard`；大数+label，单位明确为敲击/天 |
| devstatus-row | 设备状态行 | 值=`{colors.mini.text.secondary}` |
| state-banner | 全局状态条 | 四语义 tone：ok/pending/warn/danger（配 icon+字） |
| sync-action | 立即同步/重试 | 触发态 busy/pending/fail |
| setting-mirror | 设置镜像行 | 值显「待设备应用」或已生效 |
| empty | 空态 | icon + `{colors.mini.text.muted}` + 动作 |
| settings-controls | 设置控件 | 音量 0–100 滑杆；亮度低/中/高分段；熄屏 5/15/30 秒分段；选中态与当前值同时可见 |
| modal-done | 完成弹窗 | 礼花 `{colors.brand.amber.300}`；动作：从头开始/退出 |
| confetti | 礼花（完成确认后） | 仅在 backend 确认后展示 |
| bottom-nav | 一级导航 | `阅读/记录/设备/设置` 四项；当前项强调，其余项弱化；每项触区 ≥44×44 |
| page-header | 页标题带 | 标题左对齐 x20、状态右对齐至 370；y 62 起（safe_top） |
| round-index | 篇章落款 | 「第 N 次诵读」，`{typography.miniapp.caption}`，置于篇末而非页首 |
| login-title | 登录品牌语 | `{typography.miniapp.lede}` 42/600，左对齐，全 app 最大字 |
| login-copy | 登录说明 | `{typography.miniapp.body}` 16/400 |
| login-action | 登录按钮 | 350×56，`rounded.miniapp.control`，填充 `{colors.mini.text.primary}` |
| style-signature | 印谱签名层 | 六页共用、位置按页重组：小型居中印章 / 版记 folio / 1px 刻线；仅阅读页保留低对比 17 槽基线 |

## 6. 复刻与对拍
- miniapp-design/ewf-miniapp-ui.pen 是冻结视觉真源；作者导出同名 HTML 后，前端按该 HTML 与本契约逐页对拍。
- uni-app 复刻使用现有 frontend 页面、Pinia store、mock 和同步契约。
- 对拍检查覆盖布局、令牌、状态文案和 14 个状态，不从候选稿或历史导出物取值。


## 7. FR / story 校验
- 覆盖 FR-F-001~010 全部页面与空/失败态；对照 epics S3.1~S3.6、S4.1~S4.5。
