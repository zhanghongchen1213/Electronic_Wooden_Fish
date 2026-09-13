---
title: '重构电子木鱼硬件二次开发文档'
type: 'refactor'
created: '2026-09-12'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '54b014e09732c142a123a3193fa4507011005a5e'
context:
  - '{project-root}/AGENTS.md'
  - '{project-root}/docs/README.md'
  - '{project-root}/docs/hardware/电子木鱼-硬件网络清单.json'
  - '{project-root}/_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** 现有硬件基线已包含大量网络和器件事实，但缺少面向二次开发者的完整阅读主线，无法直观判断 ESP32-S3 GPIO 合法性、总线冲突、电源路径和 PCB 走线风险。

**Approach:** 参考 legbot_watch 硬件二次开发文档的组织方式，重构 EWF 硬件主文档，并同步 JSON、README 和必要的派生硬件文档，形成“官方资料约束 + 项目冻结合同 + 待验证项”的统一静态审查资料包。

## Boundaries & Constraints

**Always:** 以 Espressif 官方 ESP32-S3-WROOM-1-N16R8 资料核对 GPIO、strapping、Flash/PSRAM 保留脚和 USB；以 spine、网络清单、PDF 和现有 EWF 文档作为项目事实；保留“已确认 / 推荐初值 / 必须样机验证”状态；文档必须明确 PCB/样机尚未验收。

**Never:** 不修改两个 PDF；不引入 legbot 的旧 GPIO、GPS、BLE、Wi‑Fi、CH340X、AMS1117、TPS631000 或双 Type-C；不把静态审查写成 ERC、DRC 或样机通过；不静默覆盖 spine 与项目合同中的冲突。

## I/O & Edge-Case Matrix

| 场景 | 输入 / 状态 | 预期行为 | 错误处理 |
|---|---|---|---|
| GPIO 审查 | 逐个项目 GPIO 与官方限制对照 | 输出允许性、启动风险、保留脚和项目结论 | 冲突标记为设计问题并进入开放项 |
| 总线审查 | GPIO、I²C 地址、电源轨、连接器清单 | 无重复、无地址冲突、关键资源完整 | 缺项列入风险矩阵，不伪造结论 |
| 文档交叉核对 | 主文档、JSON、spine、BSP | 关键 GPIO/电源边界一致 | 不一致项停止签收并修正文档 |
</frozen-after-approval>

## Code Map

- `docs/hardware/电子木鱼-硬件原理图设计基线.md` -- 主文档，重构为资料来源、硬件总览、GPIO/总线审查、电源、外设、布线、开发路线和风险矩阵。
- `docs/hardware/电子木鱼-硬件网络清单.json` -- 机器可校验的 GPIO、电源、I²C、连接器、测试点和开放项事实。
- `_bmad-output/planning-artifacts/architecture/architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md` -- 板级 GPIO 合同及电源/USB 不变量。
- `docs/embedded/bsp/外设硬件软件二开.md` -- 派生 GPIO/总线口径，需在必要时保持一致。
- `docs/README.md` -- 硬件文档索引和事实优先级说明。
- `/Users/hongchenke/Documents/Github/legbot_watch/docs/hardware/ESPWatch-S3-4G_硬件软件二次开发文档.md` -- 结构参考，只读，不修改。

## Tasks & Acceptance

**Execution:**
- [x] 重构主硬件文档，加入官方依据、GPIO 合法性结论、总线/电源审查、PCB 走线清单、开发路线和开放项矩阵。
- [x] 核对 JSON、spine、BSP、README 的关键 GPIO、I²C 地址和电源边界一致性。
- [x] 运行 JSON、GPIO、地址、文案卫生和 diff 检查。

**Acceptance Criteria:**
- Given 主文档，when 阅读 GPIO 和总线章节，then 每个已用 GPIO 都有方向、约束、合法性结论和验证状态。
- Given 主文档与 JSON/spine/BSP，when 交叉检索 IO8、IO9、IO11、IO16、IO19/20、IO43/44、IO45/46，then 口径一致且明确待验证项。
- Given 静态检查命令，when 执行，then JSON 可解析、GPIO 无重复、I²C 地址无冲突、文档无禁用内部标识。

## Implementation Notes

实施时保持现有未提交改动不动；只修改本任务涉及的硬件文档和必要的实现规格记录。官方资料如本地不存在，使用 Espressif 官方来源并在文档中保留可追溯链接或资料名称。

## Review Triage Log

- false — GPIO 分类表经过补充后覆盖全部 32 个已用普通 GPIO；IO8、IO38、IO39、IO41 已明确列出。
- false — 启动模式已区分正常 SPI 启动（IO0=1）与联合下载（IO0=0、IO46=0），不会把下载电平写成正常运行要求。
- false — GPIO3 的 JTAG strapping/eFuse 条件已补充，RGB_DATA 仍保留该启动约束。
- false — ERC、PCB DRC 与样机功能验收已拆为独立阶段并要求分别归档证据。
- false — 验证缺口审查未发现遗漏；JSON、GPIO、I²C、diff 检查均已执行。

## Verification

**Commands:**
- `python3 -m json.tool docs/hardware/电子木鱼-硬件网络清单.json` -- JSON 解析成功。
- `python3 - <<'PY' ...` -- 检查 GPIO 重复、I²C 地址冲突和必需字段。
- 完成 Pencil 文案扫描，并由作者对导出的同名 HTML 执行文案卫生检查；硬件文档不承载 UI 过程文案。
- `git diff --check` -- 无空白错误。
