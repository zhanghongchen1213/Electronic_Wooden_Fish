# miniapp-design —— 微信小程序 UI 设计工作区

承载电子木鱼**微信小程序端 UI** 的「pen → HTML → uni-app 复刻」流水线。

```text
① UX 契约   _bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-*/DESIGN.md + EXPERIENCE.md
② 轨道契约   同目录 UI_CONTRACT-miniapp.md（页面/状态/组件闭包、scss 令牌映射）
③ 设计稿     ewf-miniapp-direction.pen/.html（Stage 3 方向稿）→ ewf-miniapp-ui.pen/.html（Stage 4 全帧闭包）
④ 复刻       cloud/frontend/src/pages/{reading,records,device,settings}.vue（scss 令牌取自 DESIGN；Pinia + mock）
```

约束：小程序**无点击木鱼、不产生敲击**（FR-F-002），只呈现 backend 已确认进度（AD-2）；令牌只准取 DESIGN.md palette。

## EWF Stage 3 方向稿

- Pencil 源：`ewf-miniapp-direction.pen`
- 离线 HTML：`ewf-miniapp-direction.html`
- 手机基线：390×844、顶部安全区 62px、页面边距 20px、底部导航 56px
- HTML 后处理：`python3 lvgl-design/postprocess_ewf_direction_html.py --input miniapp-design/ewf-miniapp-direction.html --output miniapp-design/ewf-miniapp-direction.html --surface miniapp --frame-name '[UI][PAGE:READING][ST:LIVE]'`
- 方向闭包检查：`python3 lvgl-design/validate_ewf_ui_closure.py --surface miniapp --stage direction`

方向稿只包含 `[READING][LIVE]` 一帧；独立 `[OVERLAY][DONE]` 与其余 13 帧在作者签收后补齐到 `ewf-miniapp-ui.*`。HTML 不依赖网络资源，且禁止出现 `woodfish`、`device_touch` 或可计数点击入口。
