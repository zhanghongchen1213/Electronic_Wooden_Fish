# 代理工作说明（AGENTS.md）

## Windows PowerShell 编码规则

本仓库包含带中文文本的 UTF-8 文件。本地 Windows 环境可能沿用 Windows PowerShell 5.1 的默认行为，从而破坏或误读非 ASCII 文本：

- 不带 BOM 的 UTF-8 文件，默认 `Get-Content` 可能按 GBK/ANSI 解码。
- 默认 `Set-Content` 可能写出 GBK/ANSI。
- 默认 `Out-File`、`>` 和 `>>` 可能写出 UTF-16LE。
- `$OutputEncoding` 可能是 `us-ascii`，这会破坏传给外部命令的管道内容。
- `cmd.exe` 启动时可能使用代码页 936，而不是 UTF-8。

这些规则对所有在本仓库工作的代理强制生效。

## 每条 PowerShell 命令都先初始化 UTF-8

在读取文件、写入文件、管道传递非 ASCII 文本，或运行任何输出可能包含中文的命令之前，先初始化 PowerShell 会话编码：

```powershell
$Utf8NoBom = New-Object System.Text.UTF8Encoding -ArgumentList $false
[Console]::InputEncoding = $Utf8NoBom
[Console]::OutputEncoding = $Utf8NoBom
$OutputEncoding = $Utf8NoBom
```

不要依赖当前终端、系统区域设置或 Windows PowerShell 默认值。

## 读取文件

搜索和定位文本时优先使用 `rg` 与 `rg --files`；它们能正确处理本仓库的 UTF-8 Markdown 文件。

当必须用 PowerShell 读取文本文件时，始终显式指定 UTF-8，并使用 `-LiteralPath`，尤其是路径包含中文或其他非 ASCII 字符时：

```powershell
Get-Content -LiteralPath "path\to\file.md" -Encoding UTF8 -Raw
[System.IO.File]::ReadAllText((Resolve-Path -LiteralPath "path\to\file.md").Path, [System.Text.Encoding]::UTF8)
```

如果默认 `Get-Content` 输出与显式 UTF-8 输出不同，丢弃默认输出，只使用显式 UTF-8 的结果。

## 写入文件

仓库文件必须保持 UTF-8。

修改受版本控制的文件时，优先使用补丁或编辑工具。不要用 PowerShell 默认的 `Set-Content`、`Out-File`、`>` 或 `>>` 写入仓库文件。

如果无法避免用 PowerShell 写文件，必须显式按 UTF-8 写入：

```powershell
$Utf8NoBom = New-Object System.Text.UTF8Encoding -ArgumentList $false
[System.IO.File]::WriteAllText($path, $text, $Utf8NoBom)
[System.IO.File]::AppendAllText($path, $text, $Utf8NoBom)
```

在 Windows PowerShell 5.1 中，不要假定 `-Encoding UTF8` 表示无 BOM 的 UTF-8。需要精确 UTF-8 输出时，使用上面的 .NET API。

## 运行外部命令

向外部工具管道传递文本前，先设置 `$OutputEncoding`。这会影响 `rg`、`git`、构建工具、脚本，以及任何通过标准输入接收中文文本的命令。

当命令必须经过 `cmd.exe`，并且可能读写中文文本时，在该命令内部强制使用 UTF-8 代码页：

```powershell
cmd /d /c "chcp 65001>nul & your-command"
```

能够直接使用 PowerShell 原生命令时，应结合上面的 UTF-8 初始化直接运行。

## 乱码与污染防护

如果看到 `纭`、`锛`、`涓`、`寮€` 等乱码，或看起来像错误替换后的中文碎片，应立即停止使用该输出，并用显式 UTF-8 重新读取源文件。

不要把乱码的 PowerShell 输出复制到笔记、记忆日志、文档、提交信息、生成产物或代码注释中。如果输出看起来像乱码，必须用显式 UTF-8 重新运行命令，并只使用修正后的文本。

如果生成或编辑后的文件以 `FF FE` 开头，它很可能是 UTF-16LE；继续之前应将其改写为 UTF-8。如果字节看起来像中文文本的 GBK/ANSI 编码，应重新按 UTF-8 读取或改写。

## 仓库作用范围

本文件只定义仓库内的代理行为。除非用户明确要求更大范围的更改，否则不要修改用户的 PowerShell 配置文件、全局终端设置或 `.gitattributes`。

## LVGL UI 字体字形闭合强制规则

本仓库曾因字体生成器与校验器只追踪 SquareLine 基准字体，遗漏状态投影和 C 代码运行时切换字体，导致设备把中文显示为方框。根因、修复结构和排查手册统一记录在 `docs/troubleshooting/LVGL运行时字体缺字问题根因与解决方案.md`；涉及 UI 文本或字体时必须先阅读该文档，不得另建相互冲突的处理口径。

以下规则对 `lvgl-design/`、`components/ui/` 以及向 UI 提供运行时文本的服务代码强制生效：

- 全量字形闭合必须同时覆盖 SquareLine 原始标签、固件实际导出 C 标签、状态投影标签、运行时动态标签和代码驱动的字体切换；只检查设计稿静态文字不算完成。
- 修改 `watch-lvgl.html`、`.spj`、`.ecomp`、`project_manifest.json`、`ui_manifest_projection.c`、`lv_label_set_text()`、运行时模型文案、格式化字符串、字号、字重或图标字体时，必须同步检查并按需更新 `lvgl-design/squareline_studio/font_glyph_contract.json`。
- 同一逻辑标签如果会被 C 代码切换字体，必须用合同字段 `font_codes` 显式列出全部可能字体。不得只登记 SquareLine 默认字体，也不得依赖标签文本与字体切换的执行顺序来规避缺字。
- 禁止手工编辑字体 `.c` 位图数组、复制其他字号字体或只修改固件侧字体。字体必须由 `.fcfg` 和生成脚本统一重建，并保证 SquareLine 资产与 `components/ui/generated/fonts` 字节一致。
- 出现 `□`、`�` 或乱码时，必须先确认原始 Unicode 码点和页面当前实际字体；禁止用删字、改成 ASCII、换近义词等方式掩盖字体资源缺陷。

完成任何 UI 文本或字体变更后，至少执行：

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  lvgl-design/squareline_studio/tools/test_font_runtime_projection_coverage.py

PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_font_coverage.py

PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_squareline_project.py \
  --html lvgl-design/watch-lvgl.html \
  --project-dir lvgl-design/squareline_studio

PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/validate_phase3_integration.py
```

如果字体覆盖尚未闭合，使用以下命令重新生成，不得跳过失败项：

```bash
PYTHONDONTWRITEBYTECODE=1 python3 \
  lvgl-design/squareline_studio/tools/generate_squareline_project.py \
  --html lvgl-design/watch-lvgl.html \
  --project-dir lvgl-design/squareline_studio \
  --fonts-only \
  --sync-generated-fonts components/ui/generated/fonts
```

最终验收必须满足 `missing_glyphs: 0`、`squareline_firmware_font_sources_identical: true`、全部项目校验通过，并使用仓库固定的 ESP-IDF v5.5.4 完整构建通过。任一条件不满足时不得发布或刷写固件。

## ESP-IDF v5.5.4 ESP32-S3 API 校验规则

这些规则对本仓库所有 ESP32-S3 固件开发、故事创建、故事实现、代码审查和 BMad epic 自动执行流程强制生效。

### 官方版本锚点

- ESP-IDF 版本固定为 `v5.5.4`，目标芯片固定为 `esp32s3`。
- 官方发布页：<https://github.com/espressif/esp-idf/releases/tag/v5.5.4>
- 官方源码 tag：<https://github.com/espressif/esp-idf/tree/v5.5.4>
- ESP32-S3 英文 API 总入口：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/index.html>
- ESP32-S3 中文 API 总入口：<https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/index.html>

### FreeRTOS 与系统 API 白名单

- FreeRTOS 总览：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/freertos.html>
- ESP-IDF FreeRTOS：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/freertos_idf.html>
- ESP-IDF FreeRTOS 增补功能：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/freertos_additions.html>
- FreeRTOS 源码入口：<https://github.com/espressif/esp-idf/tree/v5.5.4/components/freertos>
- 系统 API：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/index.html>
- Kconfig 配置参考：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/kconfig-reference.html>
- API 稳定性约定：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/api-conventions.html>

### 常用 API 分类入口

- 外设驱动：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/index.html>
- 网络通信：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/network/index.html>
- 应用协议：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/protocols/index.html>
- 存储与分区：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/storage/index.html>
- SoC 能力：<https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/soc_caps.html>
- ESP32-S3 SoC 源码：<https://github.com/espressif/esp-idf/tree/v5.5.4/components/soc/esp32s3>
- ESP32-S3 HAL 源码：<https://github.com/espressif/esp-idf/tree/v5.5.4/components/hal/esp32s3>
- ESP32-S3 ROM 绑定：<https://github.com/espressif/esp-idf/tree/v5.5.4/components/esp_rom/esp32s3>
- ESP32-S3 硬件支持：<https://github.com/espressif/esp-idf/tree/v5.5.4/components/esp_hw_support/port/esp32s3>

### 函数使用强制校验流程

- 凡新增或修改 ESP32-S3 内部函数调用、ESP-IDF API、FreeRTOS API、外设驱动、SoC/HAL/LL/ROM 调用、相关宏、类型或 Kconfig 配置项，只要 story Dev Notes 或当前审查记录中没有该具体符号的 v5.5.4 ESP32-S3 证据，必须先查官方链接再使用；不能仅凭记忆、常识或其它版本经验生成代码。
- 凡对函数签名、头文件、行为、线程/中断语义、Kconfig 条件或目标芯片适用性有任何不确定，必须立即按同一流程查证。
- 校验顺序固定为：先查 `https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/` 下的对应文档，再查 `https://github.com/espressif/esp-idf/blob/v5.5.4/...` 下同一 tag 的头文件声明和实现，最后确认 `CONFIG_IDF_TARGET_ESP32S3`、`SOC_*` 能力宏、Kconfig 条件和组件依赖。
- 禁止把 `freertos.org`、vanilla FreeRTOS 文档、ESP-IDF `latest`/`stable` 页面、其它芯片目标页面或其它 IDF 版本页面作为最终依据。
- ESP-IDF FreeRTOS 与 vanilla FreeRTOS 存在行为差异。任务栈大小、SMP 调度、临界区、tick/idle hook、heap 能力、`xTaskCreatePinnedToCore()`、`...WithCaps()` 等 IDF 扩展必须以 ESP-IDF v5.5.4 ESP32-S3 文档和源码为准。

## PWR 实体键与板级电源所有权

- PWR 是主板实体电源键，运行态输入连接 ESP32-S3 `IO46`；BOOT0 连接 `IO0`。GPIO46 同时是 strapping pin，应用启动后只允许作为输入读取，不得输出驱动。
- 板级电源电路独立完成 PWR 长按关机与长按开机，不依赖应用固件。硬件关机后 USB 停止枚举；长按 PWR 重新开机后 USB 恢复枚举。
- 关机状态同时按下 PWR 与 BOOT0，由板级电源路径与 ESP32-S3 Boot ROM 进入下载模式；应用固件不实现、不模拟也不接管该路径。
- 应用固件只处理设备已运行时、释放后确认的 PWR 短按：熄屏时点亮屏幕，亮屏时依次循环切换四个常驻页面；屏幕熄灭完全由自动熄屏策略负责，不提供手动熄屏入口。持续按住只用于避免误判为短按，不得产生软件本机关机 terminal action。
- 禁止使用 PMIC 臆造调用、`esp_deep_sleep_start()`、`esp_restart()`、仅关屏或无限循环模拟本机关机。任何 PWR 输入都不得生成外骨骼 BLE 关机请求。
- PWR 有效电平和去抖参数必须以运行态目标板实测为依据；板级长按阈值只做物理验收，不得用 host fake clock 冒充硬件证据。

## ESP-IDF 固件 C/H 编码规则

这些规则对 `main/` 和 `components/` 下的所有项目固件源码强制生效，不包括生成输出、`build/` 与 `managed_components/`。

### ESP 日志语言与标签

- 通过 ESP 日志宏（`ESP_LOGE`、`ESP_LOGW`、`ESP_LOGI`、`ESP_LOGD`、`ESP_LOGV`）打印的所有字符串必须使用中文。
- 中文日志中的技术标识符如果是字面名称，可以保留英文，包括芯片名、协议名、寄存器名、函数名、宏名、ESP-IDF 错误名和稳定错误码。
- 每个使用 ESP 日志的 `.c` 文件都必须在日志 TAG 定义区定义一个本地标签：

```c
static const char *TAG = "MODULE_NAME";
```

- 不使用 ESP 日志的文件不要定义 `TAG`。
- `TAG` 取值必须是 ASCII 且保持稳定。使用目录或模块缩写，例如 `MAIN`、`BSP_BOARD`、`BSP_KEY`、`PLAT_AT_CORE`、`PLAT_I2C`、`SVC_UI` 和 `APP_STATE`。

### 文件头

每个项目 `.c` 和 `.h` 文件都必须以下面的 Doxygen 文件头开头。文件名、简述、详情、作者和日期必须准确：

```c
/**
 * @file     file_name.c
 * @brief    一句话说明文件职责
 * @details  补充说明该文件在模块边界、初始化、状态或资源管理中的作用。
 * @author   ZHC
 * @date     YYYY-MM-DD
 */
```

中文注释和日志文本必须使用无 BOM 的 UTF-8。

### `.c` 文件布局

`.c` 文件按以下顺序组织：

1. 文件头
2. `#include` 引用区
3. 日志 TAG 定义区
4. 宏、常量、类型、变量定义区
5. `static` 内部函数声明区
6. 外部函数实现
7. `static` 内部函数实现

所有 `static` 内部函数都必须在 `.c` 文件顶部附近的内部函数声明区声明。它们的实现可以出现在外部函数实现区之后的任意位置，默认优先放在公开或外部函数之后。

### `.h` 文件布局

`.h` 文件按以下顺序组织：

1. 文件头
2. 头文件保护宏包裹层
3. `#include` 引用区
4. 业务 `#define` 定义区
5. 结构体、枚举、类型定义区
6. 外部函数声明区

头文件保护宏和可选的 `extern "C"` 包裹层属于结构性包裹层，不属于业务 `#define` 区，也不需要中文注释。

### 中文注释

- 头文件中声明的每个公开函数都必须有中文 Doxygen 注释，并按需包含 `@brief`、`@param` 和 `@return`。
- 每个业务 `#define`、枚举值、结构体字段、常量变量，以及新引入的模块级变量，都必须有中文解释性注释。
- 函数体内只在关键位置添加简洁中文注释：资源分配与释放、并发边界、错误处理、状态转换、硬件所有权，以及不明显的约束。
- 避免逐行添加噪声注释，不要用注释简单复述赋值或返回语句。

### 兼容性

- 应用这些格式与文档规则时，不要改变函数签名、公开数据结构、硬件资源取值或运行时行为。
- 保留 BSP 所有权规则：开发板 GPIO、总线实例和禁用 SDMMC 的策略由 `components/BSP` 定义，其他模块通过项目 API 使用。

## 云端线上联调单一事实来源

- `cloud/` 是本项目唯一的线上联调系统。不要新建平行云实现。
- `docs/contracts/cloud_api_contract.md` 是手环与后端通信合同；`cloud/specs/online-test-system_design.md` 是架构与验收标准；`cloud/README.md` 是启动和部署手册。
- 修改接口字段、架构边界或部署方式时，必须同步更新上述对应职责文档，禁止在其他文件维护第二套口径。
- 生产后端地址固定为 `https://watch-api.xianlitech.com`，生产前端地址固定为 `https://watch.xianlitech.com`。
- 本地后端默认端口为 `9219`，本地前端默认端口为 `5173`。
- 后端固定使用 Java 17 + Spring Boot 3.5.16；前端固定使用 Vue 3 + TypeScript + Vite + Element Plus。
- 当前云端只支持单后端实例，以 JSON 原子落盘保存设备、支付状态和最新 telemetry；不保存完整 telemetry 历史，但每台设备额外保存最近 100 个合法 `fixed` GPS 轨迹点。
- 管理设备列表只返回 `trackPointCount`，完整轨迹通过 `GET /api/v1/admin/devices/{mac}/track` 按需获取；地图配置通过 `GET /api/v1/admin/map/config` 获取。
- 高德地图 Key 与安全密钥通过后端 YAML 字段配置，并允许环境变量 `AMAP_JS_API_KEY`、`AMAP_SECURITY_JS_CODE` 覆盖；安全密钥不得进入前端包或管理响应。公开代理 `GET /api/v1/map/_AMapService/**` 只转发到 `https://restapi.amap.com/**`，必须移除客户端 `jscode` 后注入后端配置值。
- 任一高德配置为空时，后端必须正常启动且地图配置返回 `configured=false`；设备列表、支付管理和无地图功能不得受影响。
- 前端设备时间使用每秒更新的相对时间并以 tooltip 提供绝对时间；轨迹通过按需弹窗加载，必须覆盖未配置、无点、单点、多点、失败重试、关闭销毁和鉴权失效状态。
- 固件云地址必须是 HTTPS。默认 ML307R 使用 `auth=0`，固件不内置 PEM，默认 NVS 不保存 CA；日志必须明确提示服务端身份未校验。
- 可选 CA 字段只保留兼容读取能力，当前默认配置不得填充，也不得让 CA 缺失阻塞云请求。
- 固件只初始化完全空白的云配置，不按 URL 内容识别、迁移或改写已保存配置；已有、部分或损坏配置保持不变。
- 设备共享 Token 不得进入前端包、UI 或普通日志。管理端使用无状态 HTTP Basic，不引入登录 Session 或 Cookie。
- `auth=0` 仅适用于线上测试和客户联调。进入真实收费运营前，必须另行设计服务端身份认证、设备独立密钥和 OTA 更新机制。
