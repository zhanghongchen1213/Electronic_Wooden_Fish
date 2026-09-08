# docs/frontend — 软件层·小程序（EWF frontend）

> 范围：EWF `cloud/frontend`（uni-app Vue3+TS+Vite+Pinia，仅微信小程序，AD-18）。经验源= `miaowu`（HEAD b1e0a660，2026-09-08 迁移）——沿用其 `VITE_API_BASE_URL` + 统一 `api/request`（信封+鉴权头）、401 单飞刷新、登录态、Pinia、env-scripts 模式。前端**模块实现规格** `requirements.md` 仍待 frontend spec 阶段（PRD addendum §4），本目录当前只承载迁移的开发经验。

## 文档索引
| 文件 | 内容 |
| --- | --- |
| `经验-小程序请求链路与登录态.md` | miaowu 前端经验：request 封装/401 单飞刷新/令牌提前过期/登录态失效 UX/合法域名/分包 2MB/真源护栏；每节「miaowu 做法 → EWF 怎么用」 |

## EWF 边界
- **仅微信小程序**（无 h5/app/多平台分支）；不显示可点击木鱼、只呈现 backend 已确认进度（FR-F-002/AD-2）。
- 单身份单设备：登录直达唯一设备；Pinia/本地存储只放非权威展示状态。
- 无上传需求可省 OBS/uploadFile 链路。

## 排除表（miaowu 未迁）
| 主题 | 理由 |
| --- | --- |
| H5/App/alipay/baidu/toutiao 多端、frontend-web 管理后台 | EWF 仅微信小程序 |
| OBS 直传/图片审核/uploadFile(policy 式) | EWF 无对象存储/上传（按需保留简版 upload 与否） |
| 公众号/小红书运营链路、登录强绑手机号/头像库 | EWF 单作者、无运营 |
