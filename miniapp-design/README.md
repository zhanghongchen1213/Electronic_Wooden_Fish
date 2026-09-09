# miniapp-design —— 微信小程序 UI 设计工作区

承载电子木鱼**微信小程序端 UI** 的「pen → HTML → uni-app 复刻」流水线。

```text
① UX 契约   _bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-*/DESIGN.md + EXPERIENCE.md
② 轨道契约   同目录 UI_CONTRACT-miniapp.md（页面/状态/组件闭包、scss 令牌映射）
③ 设计稿     ewf-miniapp-direction.pen/.html（Stage 3：10 种候选）→ ewf-miniapp-ui.pen/.html（选中后 Stage 4 全帧闭包）
④ 复刻       cloud/frontend/src/pages/{reading,records,device,settings}.vue（scss 令牌取自 DESIGN；Pinia + mock）
```

约束：小程序**无点击木鱼、不产生敲击**（FR-F-002），只呈现 backend 已确认进度（AD-2）；令牌只准取 DESIGN.md palette。

## EWF Stage 3 风格候选

- Pencil 源：`ewf-miniapp-direction.pen`（组件 master + `[UI][STYLES][MINIAPP]` + 10 个 sibling frame）
- 离线 HTML：`ewf-miniapp-direction.html`
- 生成命令：`python3 lvgl-design/export_ewf_style_board_html.py --input miniapp-design/ewf-miniapp-style-board-source.html --output miniapp-design/ewf-miniapp-direction.html --surface miniapp --manifest miniapp-design/miniapp-style-options.json`
- 候选校验：`python3 lvgl-design/validate_ewf_ui_closure.py --surface miniapp --stage candidates --html miniapp-design/ewf-miniapp-direction.html --manifest miniapp-design/miniapp-style-options.json`
- 文案卫生：`python3 lvgl-design/ui_text_hygiene.py --surface miniapp --html miniapp-design/ewf-miniapp-direction.html --mode candidates`

每个候选固定 390×844，使用连续 append-only 正文和唯一 `scripture-progress`；`ewf-miniapp-ui.*` 必须等作者选定 style 后再创建。HTML 不依赖网络资源，且禁止出现 `woodfish`、`device_touch`、Node ID 或作者说明。

候选中的经文片段与 42/260 进度仅为视觉样例；运行时只消费 backend 已确认事件，分母由 S0.3 `scripture_chars_total` 提供，不写入 canonical 经文源。

候选校验会忽略颜色后比较布局/排版/组件结构指纹，用于发现近重复候选；它只是自动预警，最终风格签收仍以逐屏渲染审阅为准。

可复现测试：`PYTHONPATH=lvgl-design python3 -m unittest discover -s lvgl-design/tests -p 'test_*.py'`
