# EWF SquareLine Studio 规划

本目录的 `ewf_project_manifest.json` 是基于只读蓝本 `legbot_watch/lvgl-design/squareline_studio` 的 EWF 设备侧设计规划，不是假装已经生成的 `.spj` 工程。

## 已核对的蓝本约束

- `watch-lvgl.sll` 固定 SquareLine 1.6.1、导出 LVGL 8.3.11、410×502、RGB565、No layout；EWF 固件目标仍为 LVGL 8.4.0，导出后必须重新编译。
- `project_manifest.json` 使用“常驻主壳 + 按需 Screen”：主壳承载横向 pager，设置页使用 378×344 纵向 viewport 和两个 page group；真实设置屏一屏最多显示 4 行。
- 蓝本个人授权观测上限为 10 Screen / 150 widgets / 1 component；其实际工程 537 widgets / 4 components，GUI 保存被授权限制阻断。EWF 的 142 个已解析 Screen 子树节点只是当前估算，含组件 master 的序列化开销仍需 Stage 5 实测。
- 蓝本把状态补丁顺序固定为 baseline reset → canonical state patch → full-page variant patch → model projection。EWF 改为 baseline reset → component variant patch → screen variant patch → model projection，优先局部组件。
- 当前复制进来的 `generate_squareline_project.py` 仍含 legbot 的 `PAGE_STATES`（例如 `mH17G`/`tNUHq` 等旧节点映射）；在完成 EWF logical name/GUID 映射前禁止直接生成 `.spj`，否则会把蓝本页面误认成 EWF 页面。

## EWF 映射

- 常驻 `ui_scr_main_shell`：`hbTEa` 木鱼、`o1Uwn5` 经文、`RsuSH` 统计，横向位置 0/410/820。
- 按需 `ui_scr_settings`：`W3247`；亮度/熄屏沿用 `mH17G`、`achTu`、`Pz5g1`，音量与立即同步为 EWF 自有组件；`木鱼版本`、`木鱼ID` 放在第二滚动页。
- `RsuSH` 只保留 PRD 的今日敲击与累计敲击；近 7 日、近 30 日和连续天数不放在设备端。
- `mDOlU` 是可复用 7 字带母版，`ZauNI` 承载 empty/base/mid/full 状态；每个状态只留一个可见最新字槽（empty 无最新字，base=槽3，mid=槽7，full=槽4）。
- `JmQTi` 是可复用心经进度母版，充电/完成整屏只覆写其状态与文案，不复制进度树。
- `PawlG`、`lo9C1`、`fevaG` 共享 10 字行、标点计槽和同一滚动几何；`lo9C1` 是 tail 母版、`fevaG` 是 review 变体，`PawlG` 是 tail 页面实例，tail 才提升最新字。
- `VphYz` 的四个信号位置使用单节点运行时颜色/图标补丁，不复制隐藏状态树；`tap-rings` 固定三个椭圆节点，flash 160ms。

| 类型 | EWF 组件 | 变体样式 | 载体 |
| --- | --- | --- | --- |
| 共享 master | `statusbar` | 信号 connected/no-signal/disabled；同步 ok/pending/busy/fail；充电/低电 | `VphYz`，单槽运行时补丁 |
| 交互组合 | `woodfish` | idle / pressed；唯一 `device_touch` 触区 | `hbTEa` 页面组合 |
| 共享 master | `woodfish-anatomy` | idle / pressed；非交互器物识别 | `n2gMHJ` |
| 变体组件 | `tap-rings` | 三椭圆 idle / 160ms flash | `n2eoh` |
| 变体组件 | `charcell` | empty / base / mid / full；唯一 latest glyph | `ZauNI` 状态板 |
| 共享 master | `scripture-progress` | 0 / 12 / 84 / 196 / 260；充电/完成复用实例 | `JmQTi`、`mZiJw`、`n8HOn` |
| 变体组件 | `scripture-history` | tail / review；两者同为 13 槽行 | `PawlG`、`lo9C1`、`fevaG` |
| 共享 master | `stat-card` | today / total；今日另有 trusted / untrusted | `ZtT4f`、`Hlo77` |
| 变体组件 | `brightness-selector` | low / medium / high，medium 为默认选中 | `YxGuI` |
| 变体组件 | `timeout-selector` | 5s / 15s / 30s，15s 为默认选中 | `oLRuW` |
| 变体组件 | `sync-btn` | BASE / BUSY / OK / PENDING / FAIL | `cQ5wd`、`fWCbZ` |
| 共享 master | `device-identity` | woodfish-version / woodfish-id | `j6zSeO` |

整屏变体仅保留 `ZTLH6`（充电输入/音频暂停）与 `VkR3m`（完成锁定/遮罩）；两者的 statusbar、字带、进度、木鱼与三环均引用共享母版，设置同步、未校时和字带进度不另建 Screen。

命名空间约定：`device/stat-card` 与 `mini/statcard` 是两个不同表面组件；不要在 SquareLine 生成阶段把它们合并为同一 `.ecomp`。

Stage 5 才按此规划把 HTML 真源重定向到 SquareLine 生成器，登记 logical name、组件 descendant 和字体合同；本阶段不手工生成字体位图或 C 文件。当前设备 Pen 的最新节点已在 Pencil 会话中完成、导出并保存；字体合同目前属于设计阶段登记，不能替代固件终验。
