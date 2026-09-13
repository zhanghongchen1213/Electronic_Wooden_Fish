# Electronic_Wooden_Fish — AGENTS 说明

> 本文件为仓库**唯一强制规则层**（面向一切在此工作的 agent），设备侧规则多由 `legbot_watch`（只读蓝本）迁移固化。中文沟通；先读 `docs/README.md`（事实优先级）与本文件，再读 `docs/handoffs/2026-09-08-bmad-status-ui-design-handoff.md` 了解当前阶段与任务。

## 1. 仓库地图

| 路径 | 内容 |
| --- | --- |
| `Embedded/` | ESP-IDF 固件（**尚未创建工程**，占位待 E1 起建；结构照搬 legbot `components/{BSP,ui,services}`+`main`） |
| `cloud/backend/`、`cloud/frontend/` | Spring Boot(JSON 零库) 与 uni-app 小程序（**空占位**） |
| `docs/` | 设备/后端/契约/交接文档（见 `docs/README.md`） |
| `docs/embedded/` | 设备侧实现与排障（自 legbot 迁移，见其 README 来源/排除表） |
| `docs/handoffs/` | 交接文档（当前：UI 设计移交） |
| `lvgl-design/` | 设备 OLED(LVGL) UX 真源（冻结 Pen；同名 HTML 由作者导出） |
| `miniapp-design/` | 小程序 UX 真源（冻结 Pen；同名 HTML 由作者导出） |
| `_bmad-output/planning-artifacts/` | BMAD 规划产物（brief/prd/architecture/epics/ux-designs） |

## 2. BMAD 状态指针

- 阶段：planning 完成（PRD/架构/epics final）＋ UX 规范 final；设备 `DEVICE-01`（母版 `hbTEa`）与小程序 `MINI-06`（母版 `A358t`）均已锁定为 UX 视觉基线，**尚未进入 sprint/build**。源码目录为空，绿地。
- 两份 Pen 是当前视觉真源；同名 HTML 由作者导出。方向稿、候选板、导出脚本、SquareLine 规划文件和旧校验工具已清理。
- 权威与下一步的**唯一入口**：`docs/handoffs/2026-09-08-bmad-status-ui-design-handoff.md`。
- 菜单码：`SP` sprint → `BD` build；`CU` bmad-ux（UX 已在跑，后续 Update）。改动产出前先 `git status` 了解未提交集合。

## 3. 设备侧强制规则（防再踩；来源 legbot，EWF 化）

### 3.1 LVGL 字体字形闭合（EWF UI 缺字免疫）
- 改设备 Pen 的中文文案、字号字重或图标字体，**必须同步 UX 规范中的字体口径**；SquareLine 工程和字体合同在固件实施阶段重新建立。
- 字形闭合须覆盖五路：SquareLine 原始标签、导出 C 标签、状态投影标签、运行时动态标签、代码字体切换。同一逻辑标签多字体必须用合同 `font_codes` 显式列出全部字体。
- **禁止**手工编辑字体 `.c` 位图、禁止复制其他字号字体、禁止只改固件侧；字体须由 `.fcfg`+生成脚本重建，保证 SquareLine 资产与 `Embedded/components/ui/generated/fonts` **字节一致**。
- 出现 `□`/乱码：**先查码点 + 当前实际字体**，禁止删字 / 改 ASCII / 换近义词掩盖。
- 终验门槛：实现阶段重新建立字体合同后，完成缺字检查、字体一致性检查和 ESP-IDF 全量构建。
- 详见 `docs/embedded/troubleshooting/LVGL-运行时字体缺字根因与解决.md`（EWF 化排障文档；不得另建冲突口径）。

### 3.2 ui_task 独占 + generated/bindings 两层分离（legbot 分散规则，EWF 集中固化）
- **LVGL 只由 `ui_task` 独占调用**（display/input/SquareLine 对象树）。其他任务不得直改 UI。
- `Embedded/components/ui/generated/`（SquareLine 导出）**只放布局/字体/事件空桩**；**不得**读取 NVS / 其它 service 私有状态、写业务路由。
- 业务渲染/路由/typed intent 全在 `bindings/`（`ui_manifest_projection.c`、`ui_runtime_binding.c`、`ui_runtime_events.c` 等）。`generated/` 与 `bindings/` 是两层，互不越界。
- SquareLine 1.6.1 只原生导出 LVGL 8.3.11；固件锁 **LVGL 8.4.0**（`Embedded/main/idf_component.yml`），每次导出后在 8.4.0 重编译，禁止降级。

### 3.3 编码 / 日志 / API
- C/H 规范与中文文件头/注释模板：`docs/embedded/style/C编码规范-Agent版.md`（EWF 设备侧 C 编码强制基线）。
- 所有 ESP 日志正文中文，模块 TAG 用 ASCII 稳定串；每个用日志的 `.c` 定义 `static const char *TAG = "…"`。
- ESP-IDF v5.5.x：**新增函数调用先查官方文档再使用**，禁止凭记忆或其它版本经验；禁用 freertos.org / `latest` / `stable` 作为最终依据。
- 读/写仓库文本文件默认 UTF-8 无 BOM；见到乱码（`纭/锛/涓`、`FF FE` 开头=UTF-16LE）即停并改写，禁止把乱码写入文档/commit。

### 3.4 电源与按键
- PWR=IO46（strapping，**只读输入**）、BOOT0=IO0、EN=复位。**固件不接管硬件关机**：禁止用 `esp_deep_sleep_start`/`esp_restart`/仅关屏/空循环模拟关机；PWR 只做熄屏点亮 / 亮屏切页。
- 4G 发射/低电：先保证累计落盘再执行其它（AD-13）；充电期间暂停输入与音频（FR-E-001）。
- sdkconfig 锚点（EWF，随 E1 建立）：`CONFIG_LV_COLOR_DEPTH=16`+`CONFIG_LV_COLOR_16_SWAP=y`、`CONFIG_USJ_ENABLE_USB_SERIAL_JTAG=y`、`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y`、`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`、`CONFIG_LV_MEM_CUSTOM=y`、`CONFIG_LV_DISP_DEF_REFR_PERIOD=8` 等（详见 legbot `sdkconfig.defaults` 蓝本；USB 连接时禁用自动 light sleep 防串口失联）。

### 3.5 EWF 差异红线（与 legbot 不同，勿照搬错误方向）
- **无** BLE/Wi-Fi/GPS/外骨骼/云支付 业务链路；`lvgl-design` 与 docs 迁移内容中这些字句仅为排除说明。
- 设备顶部可呈现 4G/Wi-Fi/蓝牙/GPS 信号组件及 connected/no-signal/disabled 状态；这些图标只表达本地能力状态，不改变 MVP 无 BLE/Wi-Fi/GPS 业务链路的边界。
- 4G = **Air780EGP**（IO43/44 UART + DTR IO10/RST IO15/NET_STATUS 兼容位 IO16）；`IO8` 已按硬件原理图基线释放给 **`BQ_OTG_EN`**，M100 `GNSS_VCC` 留 NC/测试点，不连接 ESP32。AT/联网经验已迁移至 **`docs/embedded/4g/`**（源 `main_control`，HEAD `fb458b9`）；勿从 legbot 引（其用 ML307R 不适用）。完整电源/FPC/载板网络见 `docs/hardware/电子木鱼-硬件网络清单.json`。
- 后端持久化 = **JSON 原子文件、零数据库**（AD-16，覆盖早期 SQLite）。
- 键盘输入含 PVDF（比较器 IO11 唤醒 + ADC IO9 确认，ADC1_CH8），legbot 无此，按固件 story E2.5 实现。
- 引脚/板级唯一权威：`ARCHITECTURE-SPINE.md` §板级合同；冲突先改 spine 再调代码。

### 3.6 软件层规则（backend / frontend，经验源 miaowu，HEAD b1e0a660）
- **backend（`cloud/backend`，Spring Boot）**：接口统一 `/api/v1` + `{code,message,data}` 信封（code=0 成功）；错误码 = `{HTTP 状态}{两位序号}`；业务错误回 **HTTP 200 + 业务码**（GlobalExceptionHandler），不破坏 HTTP 语义；微信登录走 `WechatMiniClient` 模式（code2Session 先取 String body 再解析、AppID 三处对齐、session_key 清空与 openId 脱敏、access_token 提前 300s 缓存）；JWT 过滤器分级 + `ThreadLocal finally clear()`；持久化 **JSON 原子文件零库**（AD-16，**禁止引入 SQL/DB**）；单身份单设备（无角色/租户）。
- **frontend（`cloud/frontend`，uni-app 仅微信小程序）**：单一 `VITE_API_BASE_URL` + 统一 `api/request`（信封+鉴权头，不散落请求）；**401 单飞刷新队列**（并发 401 只刷一次，失败唤醒所有等待防挂起，刷新后重试一次，仍失败 `uni.reLaunch` 登录页且不重复跳）；令牌本地提前过期判断；本地状态只放 Pinia/本地存储，不作权威数据（AD-2）；开发 `urlCheck:false`、生产在微信公众平台配 request 合法域名；分包控 2MB。
- 经验与排除详见 `docs/backend/`、`docs/frontend/` 及其 README。

### 3.7 最终页面文案与设计产物硬禁令
- 最终设备 UI、Pen 画面、静态 HTML、uni-app 页面、前端可见文案和交付截图中，**严禁**出现 AI 思维链、内部推理、设计过程、调试说明、Node ID、`data-pencil-id`、`TODO`、`draft`、`placeholder` 或作者提示。
- `qljP7`、`WfAs7`、`Z6Qge`、`e8Sgp`、`gGgAm`、`NtM6r` 等内部节点标识不得进入用户可见文本或生产 HTML 属性；交付 HTML 必须清除 `data-pencil-id`。
- UI 只允许产品文案、状态文案和 canonical《心经》内容；解释设计意图的文字只能留在文档或工作日志。
- 所有 UX 设计收口前必须通过 Pencil 文案节点扫描和逐屏视觉检查；发现禁用词、重复进度、孤立标点、开发者说明或节点标识时视为失败。HTML 导出后由作者自行运行对应卫生检查。

## 4. 问题排查索引（症状 → 文档）

| 症状 | 去读 |
| --- | --- |
| 中文 `□` 缺字 / 乱码 / 字体不符 | `docs/embedded/troubleshooting/LVGL-运行时字体缺字根因与解决.md` |
| 滚动/三页切换低帧率、横线 | `docs/embedded/troubleshooting/LVGL-滚动低帧率与快照直传.md` |
| 自动 light sleep 随机重启 / USB 日志失联 | `docs/embedded/troubleshooting/ESP32-S3-自动轻睡眠随机重启与USB日志失联.md` |
| 点击/敲击音无声或首音被吞 | `docs/embedded/troubleshooting/NS4150B-点击音启动时序.md` |
| 屏幕熄/亮闪屏、DISPON 时序 | `docs/embedded/troubleshooting/CO5300-熄亮屏闪屏与整帧时序.md` |
| 功耗优化与验收（禁把编译当达标） | `docs/embedded/guides/低功耗策略与实测验收.md` |
| 真机编译/烧录/监控/取证流程 | `docs/embedded/guides/真机闭环runbook-macos.md` |
| 外设初始化/引脚坑（CO5300/CST9217/CW2015/ES8311/NS4150B/PVDF/按键/USJ） | `docs/embedded/bsp/外设硬件软件二开.md` |
| C 代码写法/规范 | `docs/embedded/style/C编码规范-Agent版.md` |
| 4G 连不上 / AT 零响应 / 退避恢复 | `docs/embedded/4g/troubleshooting/Air780EGP-AT零响应与恢复.md` |
| Air780EGP 联网/HTTPS/波特率/PDP/DTR 经验 | `docs/embedded/4g/Air780EGP-AT联网与HTTPS经验.md` |

## 5. 规范来源（provenance）

- 本文件 §3 与 `docs/embedded/**` 均迁移/适配自 `legbot_watch`（HEAD `41a5ab8b9`，2026-09-08 迁移）；各文档头部含 provenance 块。
- `legbot_watch` 是**只读蓝本**：本仓不得反向修改 legbot；若某规则与 EWF 产品冲突，以 EWF PRD/ARCHITECTURE-SPINE/UX 契约为准并在迁移文档注明。
