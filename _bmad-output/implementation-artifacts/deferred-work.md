# Deferred Work

## Deferred from: code review of 4-1-建立-canonical-心经-与版本校验.md (2026-09-22)

- `cloud/env-scripts/start-local-test-backend.sh:149` 打包并启动同一个后端 jar（内嵌 `canonical/heart-sutra.json`），却未接入 `build-prod-backend.sh` 已落地的 canonical fail-closed 门禁，本地联调可运行在版本不一致的经文资源上。spec Task 4.1 只要求 `build-prod-backend.sh` 接入；是否让本地便捷入口也 fail-closed 属产品决策，且该脚本与 `build-prod-backend.sh` 一样在 `cloud/backend/pom.xml`（Story 4.3）就位前跑不通。
- 标点口径的冻结 UX 证据只覆盖锚点区：`docs/contracts/canonical/heart-sutra.txt` 含 43 个 `，`，而 `Embedded/lvgl-design/ewf-device-ui-export.html` 全文只有 7 个 `，`（全部落在前两行 13 槽锚点内）。源文本 260 个汉字与 manifest `provenance` 所引转录本逐字一致，但相对该转录本另有 `；`/`！`/`：`→`，` 等映射未给出可复核规则。标点占槽、直接影响第 3 行起的 13 槽换行内容，建议在 README/provenance 显式声明「标点位置为依 UX 风格口径的编辑决定」并给出映射规则。
- 三端产物同时暴露 `totalChars: 303` 与 `consumableHan: 260`，而运行期进度分母 `scripture_chars_total` 应为 260（可消费字数）。产物未写 `scripture_chars_total`（未触红线），但 Story 4.3/6.x 若直接取 `totalChars` 作分母会得到 303。属下游接口纪律，宜在 4.3 落地时以测试固定。
- 新增契约未在上位索引登记：`docs/contracts/README.md` 仍写「本目录当前仅占位说明」，`docs/README.md` 目录导览同样，`cloud/AGENTS.md` §1 权威来源清单也没有 canonical 一项。下游 4.2/4.3/6.1 按事实优先级读索引时可能另立一份经文资源定义（与 FR-C-002 唯一来源、`cloud/AGENTS.md` §3 相悖）。修复涉及 agent 上下文/规则文件，按审查规则不由 code review 自动修改。
- 打包期 canonical 门禁没有自动验证，且在当前仓库状态下不可达：`cloud/env-scripts/build-prod-backend.sh:104-117` 新增的 `check` 块既不被任何测试调用（仓内无 CI），又因脚本在 `resolve_project_root` 处要求 `cloud/backend/pom.xml` 而在本次交付时提前退出。删掉或改弱该门禁块不会让任何断言失败。把打包脚本纳入自动验证宜与 Story 4.3（补 pom）一并落地。
- 三端产物新增检测规则缺少可重复执行的负例，门禁套件也没有自动入口：`build-prod-backend.sh` 只调 `check` 不调 `run_canonical_tests.py`，套件仅由人工命令运行。本轮已补「头部偏移载荷缺失」「后端字段缺失」两条负例（负例 9/10），其余新增规则（`EWF_SCRIPTURE_STEP_COUNT`、三端 `source_sha256`、`title`、`firstConsumable`、`nonConsumableChars`）的反向验证仍为一次性人工实测。将套件接入构建/CI 属 Story 4.3 范围。
- `docs/contracts/canonical/__pycache__/scripture_tool.cpython-312.pyc` 未被忽略、会随新目录入库：该字节码缓存在本次新建的交付目录内，根 `.gitignore` 无 `__pycache__/` 规则（仅 `Embedded/.gitignore` 有），`git status -uall` 已将其列为待提交。它不在故事 File List 与 story-local diff 中，属编译缓存残留；修复应为仓库级忽略规则（`.gitignore` 属工作区基建，不在本 Story diff 范围内）。

- 打包期 canonical 门禁没有自动验证，且在当前仓库状态下不可达：`cloud/env-scripts/build-prod-backend.sh:104-117` 的 fail-closed `check` 块不被任何测试引用（`run_canonical_tests.py` 只以 `subprocess` 驱动 `scripture_tool.py`，全仓检索 `build-prod-backend` 只命中文档与 `upload-backend-jars.sh` 的提示文案，仓内无 CI 目录），删掉或改弱该块不会让任何断言失败；`bash -n` 只是语法解析。且该块当前不可达——脚本在 `resolve_project_root`/pom 检查处（第 62/80 行）因缺 `cloud/backend/pom.xml` 提前退出（pom 由 Story 4.3 提供）。把它纳入自动验证需为其搭 java/mvn stub 沙箱，属新增范围，宜与 4.3 一并落地。
- 同样打包并启动 `cloud/backend` 的本地入口未接入同一条 fail-closed 门禁：`cloud/env-scripts/start-local-test-backend.sh:149` 以 `mvn -DskipTests clean package` 产出同一个 `saas.jar`（内嵌 `canonical/heart-sutra.json`）并在 `:166` 以 `java -jar` 启动，与已被门禁覆盖的生产脚本共享同一后端目录、同一 JDK/Maven 闸门，但不执行 `check`；任何测试都未引用该脚本，故其不采用不会被断言标记。spec Task 4.1 只登记 `build-prod-backend.sh`，本地便捷入口是否也要 fail-closed 属产品决策，且该脚本同样在 pom 就位前跑不通。
- 生成头 header-only 的 `static const` 形态使每个 include 的编译单元各持一份载荷副本：`docs/contracts/canonical/generated/ewf_scripture_canonical.h` 的 `EWF_SCRIPTURE_DISPLAY_UTF8[909]` 与 `EWF_SCRIPTURE_STEP_OFFSETS[260]` 为 `static`（内部链接），实测两份 TU 取地址不同、单 TU 目标文件含 921 字节 rodata，合计约 2.2 KB/TU 重复量；spec Dev Notes 的 C 头草图用 `extern` 声明 + 唯一实现，`docs/embedded/style/C编码规范-Agent版.md` 也要求头文件不定义对象。设备侧接入点是 Story 3.1（Epic 边界禁止本 Story 往 `Embedded/` 放承载 `.c`），故「维持 header-only」与「另生成 `.c` 承载定义」的取舍留给 3.1 裁定；`README.md:14,90` 已显式记录该决策。
- 收敛登记（2026-09-22，Story 4.2 「冻结跨层同步契约」）：上面「新增契约未在上位索引登记」一条中属**文档索引**的部分已闭合——`docs/contracts/README.md` 不再声明契约「尚未创建」，并已登记 `sync-contract.md`（`contract_version` = `SC-1.0.0`）与 `canonical/` 的分工；`docs/README.md` 的 `contracts/` 导览行已由「待后续 spec/S0 阶段冻结；本目录当前仅占位说明」改为指向已冻结契约。该条里的 `cloud/AGENTS.md` §1 权威来源清单 `canonical/` 条目**不在 Story 4.2 范围内**（属 agent 规则文件，由后续需要时处置），故本条不构成该项全部闭合。

- 收敛登记（2026-09-22，Story 4.3「建立 backend 启动、接口信封与环境基线」）：上面两条「打包期 canonical 门禁没有自动验证，且在当前仓库状态下不可达」中的**可达性与可失败性**已闭合——`cloud/backend/pom.xml` 就位后 `cloud/env-scripts/build-prod-backend.sh` 的 `check` 块真的执行（实测日志可见「canonical 经文版本校验（FR-C-002 / AD-4，版本不一致即停）」），且在临时副本上把 `cloud/backend/src/main/resources/canonical/heart-sutra.json` 的 `scripture_version` 改成 `HS-9.9.9` 后，脚本以退出码 1 打印「构建失败：canonical 经文版本校验不通过，停止打包」。**仍开放**：仓内无 CI，也没有任何测试或脚本调用 `build-prod-backend.sh`，删掉或改弱该块仍不会让任何断言失败——故此处只写「可达性与可失败性已实测」，不写「已自动验证」。
- 收敛登记（2026-09-22，Story 4.3）：上面「`start-local-test-backend.sh` 未接入同一条 fail-closed 门禁」**仍开放**。Story 4.3 只补 backend 工程与骨架，未改 `cloud/env-scripts/**`；本地便捷入口仍以 `mvn -DskipTests clean package` 产出并启动同一个 `saas.jar` 而不执行 `check`。是否让本地入口也 fail-closed 属产品决策，本 Story 明确不改。
- 收敛登记（2026-09-22，Story 4.3）：上面「`totalChars` 303 与 `consumableHan` 260 混用风险」已按「以测试固定分母唯一来源」完成本 Story 可做的部分——本 Story 未引入任何进度分母，新增的 `cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ScriptureResourceBaselineTest.java` 以资源内部关系断言 `counts.consumableHan` 等于交付的可消费字序列长度与逐步消费步数、不等于 `counts.totalChars`，且 `counts` 中只有 `consumableHan` 等于可消费字数（实测把 `consumableHan` 改成 `303` 即红）。真正的分母使用点在 4.6 / Epic 5 / Epic 6，届时须直接引用该字段。
- 收敛登记（2026-09-22，Story 4.3）：上面「根 `.gitignore` 无 `__pycache__/` 规则」**仍开放**，属工作区基建；Story 4.1/4.2 已确立「`.gitignore` 不在 Story diff 范围内」，本 Story 同样未改 `.gitignore`。
- 新增登记（2026-09-22，Story 4.3，不属 4-1 review 的条目）：本 Story 实测发现两项运行期卫生缺口，均**未被仓库级 `.gitignore` 覆盖**（已用 `git check-ignore -v` 逐条实测）：`cloud/logs/**`（`stop.sh` 与 `start-local-test-backend.sh` 的 `LOG_DIR` 默认 `$PROJECT_ROOT/logs`，即 `cloud/logs/`，承载脚本日志与 PID 文件）与 `cloud/backend/data/**`（4.5 的 JSON 原子落点）。按 4.1 既有口径「`.gitignore` 属工作区基建，不在 Story diff 范围内」，本 Story **不修改 `.gitignore`**，仅登记；为避免把运行期产物留进工作区，本 Story 实测起停时以 `APP_LOG_ROOT` 指向仓库外目录运行。同批登记一处文档事实落差：`AGENTS.md` §1 仍把 `cloud/backend/` 标注为「**空占位**」，而本 Story 已把该目录变为可构建、可启动的 Spring Boot 工程；该文件属 agent 规则文件，按 4.2 已确认口径不由 Story 修改，故只登记不就地改写。

## Deferred from: code review of 1-2-闭合-pvdf-唤醒与-adc-二次确认.md (2026-09-22)

- 候选事件发布到尚无消费者的队列：`EVENT_BUS_QUEUE_DEPTH = 16`（`Embedded/components/platform/event_bus/event_bus.c:12`），`event_bus_queue()` / `event_bus_group()` 在全仓无调用者，本 Story 是唯一发布者，故一次上电周期内第 17 条及以后的候选事件 `event_bus_publish()` 必然超时并打 `ESP_LOGW`，而快照计数继续增长、两条证据链分叉。根因是消费者属 Story 2.1 的有效敲击队列；本 Story 已按 spec 发布且保留有界告警（符合 AD-7「不得静默丢弃」），单方面加大队列深度只是推迟同一问题。
- ISR 投递失败后比较器中断永久关闭且无重武装路径：`Embedded/components/BSP/PVDF/pvdf_bsp.c` 的 ISR 在边沿先 `gpio_intr_disable`，重武装只在任务侧 `handle_wake_message()` 出口发生；`xQueueSendFromISR` 返回值被忽略，队列满时该脚中断永不再开且无中文日志。与 Story 1.1 复审已 deferred 的第 1 项形态完全相同（`key.c` 的 `key_pwr_isr`），spec 明确该形态为 `low` 且要求本 Story 不要顺手修，实现按 Task 3 要求镜像 `key.c` 语义。
- 确认窗口内中断全程关闭，AD-7「合并为同一在飞确认」在固件中不可达：`drain_merged_wake_messages()` 与策略的合并分支只能被主机用例触达，窗口内到达的敲击实际是被整击丢失而非合并。因窗口 20 ms 显著小于 20 次/秒的 50 ms 间隔，行为结果等价于「丢最新」，对已冻结样本表无害；覆盖该场景需样机敲击波形数据。
- 自检观察预算等于本项 timeout：`Embedded/components/services/selftest_service/selftest_service.c` 的 `handle_pvdf_input()` 用 `observe_budget_ms = context->timeout_ms`（2000U），用满时引擎会按 `elapsed_ms > timeout_ms` 覆盖为 `FAIL`/`SELFTEST_ITEM_TIMEOUT`，丢失 `PVDF_ISR_NOT_ARMED` 与 `HW_OI_008_PENDING` 标记。与 Story 1.1 复审 deferred 第 3 项同型；本项 `isr_registered` 通常在首轮轮询即就绪，仅在链路上电失败时触发。
- 4 条 `detail_code` 出口未携带 `HW_OI_008_PENDING` 标记（`PVDF_CANCELLED` / `PVDF_ISR_NOT_ARMED` / `PVDF_SNAPSHOT_FAILED` / `PVDF_SELFCHECK_READ_FAILED`）：AC 2.3 的证据口径主要约束该项观察结论（成功/降级/不匹配三条已携带），取消与内部读取失败路径是否同样携带属自检口径决策。
- AC 2.2 要求的各类样本计数只落在中文日志、未落入 `detail_code`：`selftest_item_result_t` 唯一文本通道是 48 字节 `detail_code`，日志不进 typed 结果，进入 state/UI 的只剩裸 PASS/FAIL 码。spec 自身存在张力——AC 2.2 要求计数落 `detail_code`，Task 5 要求该字段写稳定码，二者无法同时满足，需先统一口径。
- 降级路径写死 3300 mV 满量程（`EWF_PVDF_BSP_FALLBACK_FULL_SCALE_MV`）：spec 点名的反模式（「不得凭记忆写死电压数字」），但仅在无校准句柄的降级路径参与上限判定且已带未冻结注释；移除需给出运行期可得的替代量程来源。
- BSP 的越界判据与错误码映射只被源码文本扫描覆盖，无宿主可执行测试：`pvdf_bsp.c` 与 `pvdf_input_service.c` 不被任何测试目标编译，把 `ESP_ERR_TIMEOUT`/`ESP_ERR_INVALID_RESPONSE` 映射为 `EWF_PVDF_SAMPLE_OUT_OF_RANGE` 的那段分支可被静默改坏而门禁全绿。spec Task 8 只要求源码扫描，抽纯函数会新增公共面，宜与未决项一并裁定。
- `EWF_PVDF_ADC_ATTEN_LEVEL`（`pvdf_bsp.h:38`）是无人引用的公开宏，与真正生效的 `EWF_PVDF_BSP_ADC_ATTEN` 构成双份真相：把 `12U` 改成其它值不改变行为也不使任何门禁失败。
- `arm_runtime` 的 `@brief`（「结束上电盲窗」）与实现语义矛盾，且盲窗锚点落在服务 `run()` 而非 `pvdf_bsp_init()`：`@details` 已正确描述「本调用之后仍有盲窗」，属头文件措辞不一致；锚点位置使该拒绝路径在启动流程中不被触达（仅自检重放覆盖），改动涉及 AC 1.4 的锚点定义。
- 样本表注释声称盲窗类别「附盲窗结束后的对照敲击」，表中无该样本：对照敲击仅存在于主机用例，未进入「固件与主机共用」的同一张表；增删样本需同步更新冻结的期望契约。
- `pvdf_bsp_init()` 部分失败时 `s_wake_enabled` 已置真但 `s_initialized` 仍假，`pvdf_bsp_wake_disable()` 因 `!s_initialized` 返回 `ESP_ERR_INVALID_STATE` 而无法回收：该路径会使板级初始化整体失败并终止启动，实际不产生可观测残留。
- 多条早退路径不经过 `set_error()`（`pvdf_bsp_wake_enable`/`wake_disable`/`comparator_level`/`isr_register`/`isr_unregister` 及各函数参数校验），`s_last_error` 会保留陈旧值，而服务与自检日志把它当作「本次失败原因」输出，可能打印与当次失败无关的稳定码。
- `publish_snapshot()` 使用非阻塞取锁（`xSemaphoreTake(..., 0)`），争用时静默丢弃快照：既有服务样板（`power_service`）的同一做法；快照为唯一读源，最坏情形是自检观察到滞后的 `blind_window_open`。
- 产品可见日志含设计/流程解释性文字（红线 14 措辞，如「本服务只产出候选事件，不推进正式累计」「本项只证明判定逻辑确定性」）：Story 1.1 既有日志为同一风格，属跨 Story 口径不一致，统一需与日志规范一并处理。

## Deferred from: code review of 1-3-启用-usb-serial-jtag-诊断路径.md (2026-09-22)

- 交付物 `story-local.diff` 不是可应用的补丁：生成器丢失了 `\ No newline at end of file` 标记行，`.gitignore` 的删除行内容被拼成 `docs/embedded/build_records+docs/embedded/build_records`，下一文件的 `--- a/Embedded/tests/run_host_tests.py` 头被粘进 `.gitignore` 新增行，该 hunk 单独取出后 `git apply --check` 报 corrupt patch（exit 128）。不影响本 Story 的代码——工作区的 `.gitignore` 第 49/50 行与 `run_host_tests.py` 已独立核对为正确，File List 与该产物解析出的 5 个文件一致；修复归属 autopilot 的 diff 生成器。
- 故事 Debug Log 自述实现期真的启动过一次 `idf.py openocd`（无设备主机，OpenOCD 报 `Error: esp_usb_jtag: could not find or open device!`），并删除了该次中间回执与 `Embedded/build/openocd_out.txt`：不构成红线 1 违规（红线 1 枚举 `flash`/`app-flash`/`erase-flash`/`monitor`），最终代码也已修正为先过端口门禁；但它违反本 Story 自己冻结的「同一 fail-closed 条件集覆盖三路」，且删除产物使该次设备侧动作失去可追溯回执。属记录与流程治理，归 epic retrospective。

## Deferred from: code review of 4-2-冻结跨层同步契约.md (2026-09-22)

- `docs/README.md:9-18` 的「事实优先级（冲突时高者覆盖低者）」表没有 `docs/contracts/` 层：本次把契约冻结为 `SC-1.0.0` 后，契约与 PRD 已存在实际分歧（如契约 §9 `applied_revision ≥ command_revision` 对 PRD `:324` 的 `==`；契约 §8「大于 1000」对 PRD `:138`「上限为 1000 次」），但该表未给契约任何层级，下游 agent 无优先级可依。该表在本 Story 之前即存在，本次只改了目录导览行；把冻结契约放进哪一层属口径裁定。
- 契约与上游的 `current_revision` 名称落差未登记：PRD `:324` 写 `applied_revision == current_revision`、`epics.md:734` 写 `applied_revision >= current_revision`，契约 §9 统一为 `applied_revision >= command_revision` 且全文检索无 `current_revision`；Story 裁决第 3 条只记录 `==` 与 `≥` 的取舍，未记录字段改名，Epic 5 依 PRD/epics 检索该操作数会落空。规范修复落在 PRD/epics，不属 Story 4.2 的改动范围。
- `automatic_tap` 的 3 秒周期锚点被泛化：Dev Notes 与 PRD §3.6/§4/§6.1 明确 3 秒，契约 `sync-contract.md:84` 只写「固定周期锚点」（AC 1 未要求该值，登记备用）；同文件 §12 表把 `40101`/`40001`（协议级 HTTP 状态）列在「业务码」列，同节正文又将其称为协议错误，措辞口径不统一。两项均为一致性登记，非当前缺陷。
- `docs/backend/经验-后端REST信封与微信登录.md:129`（「JSON schema/文件粒度尚未冻结（ASSUMPTION A-2）…到 `sync-contract.md` 收敛时再定稿」）与 `:5`（「应在…后续同步契约阶段冻结」）、`Embedded/AGENTS.md:251`（「和后续 `docs/contracts/sync-contract.md` 为准」）仍把已冻结契约写成将来时，而 spine `:292-294` 与 `docs/contracts/README.md:17-18` 已标注 A-2/WS 参数「已收敛」。AC 5 与 `rule_index_docs` 只覆盖两份被点名的 README，故这些站点不在任何断言内；下游按事实优先级读 `docs/backend/` 者可能自定一份 schema。`Embedded/AGENTS.md` 属 agent 规则文件，按项目规则不由 code review 直接修改。
- 门禁负例套件对 15 条规则中的 6 条零覆盖（`rule_file_hygiene`、`rule_registry_shape`、`rule_field_coverage`、`rule_persistence`、`rule_business_codes`、`rule_index_docs` 置为恒真后负例仍 13/13），`BOUND_PHRASE_PATTERNS` 的失败路径也无断言（`test_negative_8` 第二组同时触发 md↔注册表双真源，去掉该规则后仍通过）。本轮已实测这 6 条规则的失败路径都能正确触发，故当前门禁仍是 fail-closed；缺口在「规则被改弱时无人察觉」。按 triage 规则（修法属新增用例的复杂度扩张）本轮不修，登记为后续可排期项。
- 契约正文 §2「承载通道」列、§11 持久化表与注册表之间的其余 Decision 项（§2 通道对帧载荷的互相否定、`scripture_version`/`applied_revision`/`replay_cursor` 无 §11 承载文件、`error` 是否每帧必带、§11 四列与注册表 `endpoint`/`close_codes`/`reconnect` 未参与逐值比对、`action_id` 单事务组对 `commands.json` 的过度约束）已逐条写入 story 文件的 `### Review Findings`，需人工裁定后由下一轮 dev-story 处置，不在 deferred 内重复。

### Deferred from: code review of 4-2-冻结跨层同步契约.md（attempt-2 独立复审，2026-09-22）

- 负例套件对 4 条规则零覆盖（`rule_file_hygiene`、`rule_registry_shape`、`rule_field_coverage`、`rule_business_codes` 置为恒真后负例仍 25/25），且 `test_negative_1` 的实际命中来自 `rule_samples`/`rule_ws_frames`/`rule_persistence_scope` 而非「未覆盖 PRD §7 字段」；story 记录「每条规则的可失败性另由 13 组负例在临时副本上逐条证明」与事实不符。本轮复测这 4 条规则的失败路径都能正确触发（BOM+CRLF、删顶层键、PRD 字段双侧删除、码形非法双侧一致），故门禁今日仍 fail-closed；补齐负例属用例扩张，登记为后续可排期项。
- 集合/计数类断言弱于其声明：注册表可加入 PRD §7 之外的越权字段（正文与注册表双侧同步改即全绿）、spine 同步字段清单可静默删除未被钉住的标识符、三条 Deferred 收敛标注用全文计数（`markers < 3`）判定（删一条并在别处复制标记即可绕过）。三者均需新增断言，非直接纠正。
- 扫描断言会产生误报：样例任一取值等于 7/13/17/260/303（如 `seq: 13`）即触发「样例不得出现视口槽位或经文长度字面量」；§12 表达正确语义的句子（「阈值语义不是「大于 1000」」）一旦重排到同一行即被判「不得写成「大于 1000」」（当前交付文本因「大于」与「1000」之间换行而未被误判）。修法为语境感知匹配。
- `rule_params` 对无绑定项的参数在正文写出具体测试上界时会报「测试上界双真源：契约正文为 N，注册表为 None」，该失败无法通过修改注册表消除（注册表没有该键的表达位）。当前 §10.3 两条上界为「无」故未触发。
- `brightness` 的 wire 取值 `mid` 与既冻结 UX 传说 `low/medium/high`（`UI_CONTRACT-device.md:149`）拼写不一致，且 §9.1 未登记 wire 名↔中文显示词（低/中/高）的映射。契约是 wire 名冻结真源，UX 侧传说属设计说明；口径统一归属 Epic 6/UX 侧裁定。
- §11.1 的 device 作用域自称载体为 FR-E-004，却漏掉该 FR 要求设备持久化的「待同步事件段」；§8「离线差量按 `round_id` 分段提交」也没有对应的持久化事实与落点。设备本地文件形状不在本 Story 冻结范围（属 Story 2-2），补登记属新增条目。
- 每帧必带的 `seq` 对 `frontend → backend` 帧（`heartbeat_ack`）没有可判定的语义空间（§3 的取值域按接收方消费书写），`samples` 也未提供 `heartbeat`/`heartbeat_ack`/`completion` 样例；前端自增与回显两种实现都能自证，需 Epic 5 先确定握手语义。
- §9.1 的 `timeout` 用 `enum(5|15|30)` 表达数值域，JSON 线型（数字或字符串枚举）未定义，而样例写数字 `15`；门禁只逐值比对 `type` 字符串、不校验样例取值与 `type` 是否相符。按注册表建模的 mock 可能产出 `"15"` 而与按样例实现的一侧在 `40001` 上互相判错。
- story 的 Debug Log 仍记「负例 13/13」与 13 行诊断表，而本轮交付的门禁为 19 条规则 + 25 组负例（新增组无诊断记录）。证据账目与交付物不同步；不重写 dev 的调试记录，由下一轮 dev-story 或 4-2 收尾时同步。
- `Embedded/AGENTS.md:251` 仍写「…和**后续** `docs/contracts/sync-contract.md` 为准」，是 AC 5 那一类「把已冻结契约写成将来时」在仓库内的最后一处站点；该文件属本 Story 红线禁改的 `Embedded/**` 且为 agent 规则文件，正确落点是 agent 规则文件的口径裁定（同上一节同名条目）。

## Deferred from: code review of 4-2-冻结跨层同步契约.md (2026-09-22)

- 关闭码 `1006` 在 §10.3 同段内互斥（「只记录不处理」与「按上表退避重连」）：注册表 `close_codes.no_retry_on = [1000,1008]` 把 `1006` 归入重连侧，但正文留有两种读法。需与「`1006` 由本地合成、不可作为 wire 码处理」的 RFC 语义一并裁定后收敛措辞。
- §10.3 声称关闭码取值域「依 RFC 6455」却只枚举 4 个标准码，`1002/1003/1007/1009/1010/1011/1012/1013` 无重连与否的规则；补齐属新增条文。
- 以 `python3 -O` 运行时 `assert` 被剥离，负例套件 35/35 全绿却零校验（已实测复现）。登记命令为无 `-O` 的 `python3 …`；修法为在 `main` 入口对 `__debug__` 判否即非 0 退出。
- 负例沙箱健壮性：`make_sandbox` 对缺失源文件静默 `continue`，`run_negatives` 的异常元组不含 `OSError`，上游取样文件缺失时整轮 traceback 中断、其余负例无结论。缺输入应 fail-closed 而非可跳过。
- 经文正文探针只取 canonical 源前 16 个非空白字符，把后段经文写入契约或注册表时门禁不报；需多窗口探针或全长分片断言。
- `ARCHITECTURE-SPINE.md:160` 的 `[ASSUMPTION A-2]` 仍是收敛前措辞（「须经 sync-contract 收敛」），与同文件 `:293` Deferred 行的「文件粒度已收敛（2026-09-22，`SC-1.0.0`）」不一致；刷新该交叉引用需裁定 spine 叙述面的改动幅度。
- A-2 的 Deferred 行改写后未再显式保留「日统计桶 schema 仍未冻结」，而 `daily_stats.json` 的字段列只写「无」；桶形状的裁定归属记录页（Epic 6）与实现 Story。
- 负例 31 对 `rule_discard_baseline` 非 load-bearing（摘掉该规则后负例 31 仍通过，改动实际由 `rule_persistence_scope` 抓住）；该规则的两条独有断言无任何负例覆盖，且与 `rule_channel_directions`／`rule_persistence_scope` 重叠，是否保留为纵深防御需裁定。
- `docs/embedded/guides/低功耗策略与实测验收.md:26` 仍写「正式输入只有两类」、`:97` 仍写「积压 >1000 拒绝新输入」，与已冻结契约的三类枚举及「差值达到 1000 即拒绝」相反；该文件不在 4-2 的 story-local diff 内（补改会破坏编排器的 diff↔File List 一致性）。
- `_bmad-output/planning-artifacts/epics.md:93` 与 `:564` 仍写「`docs/contracts/sync-contract.md` 当前只有 README 占位；…冻结字段、帧格式、重连…」。本轮实测推翻 `deferred-work.md` 4-1 小节里「`Embedded/AGENTS.md:251` 是 AC 5 那一类在仓库内的最后一处站点」的结论，后续收敛需把 `epics.md` 一并纳入。
- `error` 帧的 `error.code` 取值域未定义，唯一的 `ws_error` 样例把它写成 `/api/v1` 信封码 `40001`；门禁不对 object 型样例做取值校验。样例与 §10.1/§12 当前一致，规范化的取值域句属新增条文。

## Deferred from: code review of 4-3-建立-backend-启动-接口信封与环境基线.md (2026-09-22)

- 错误信封无法携带 `data`，与契约 §12 对 `20003`/`20004` 的要求相反：`cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/response/ApiResponse.java:46-48` 的 `error(int,String)` 让 `data` 恒为 `null`，并被类级 `@JsonInclude(NON_NULL)` 从 JSON 中删除；而 `docs/contracts/sync-contract.md` §12 明确要求「业务拒绝（HTTP 200 + `2xxxx` 业务码）仍是一次响应，§6.2 的响应必填字段照常回传……不得只回 `{code,message}`」（`20003` 的 cloud 基准、`20004` 的已确认高水位都由该次响应携带，§8「禁止无限重试」依赖它）。门禁中唯一命名业务错误的断言（`cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/EnvelopeContractTest.java:82`）把「2xxxx 错误不带 data」写成期望形状。补上它要改错误信封 API 与 Epic 5 的同步语义，超出 Story 4.3 的骨架范围，归 Epic 5。
- 新增门禁不在任何自动化路径上运行：本 Story 交付的 16/17 项测试只由人工 `cd cloud/backend && mvn -q test` 执行；仓库仅有的两个后端入口 `cloud/env-scripts/build-prod-backend.sh:120`（`mvn -B -U clean package -DskipTests`）与 `cloud/env-scripts/start-local-test-backend.sh:149`（`mvn -DskipTests clean package`）都跳过测试，仓内亦无任何 CI 定义。实测复现本 Story 的 RED 实验 B（删掉 `NoResourceFoundException` 分支）后，`build-prod-backend.sh` 仍退出码 0 并产出可上传的 `saas.jar` 与 `.sha256`，而未匹配路径此时已退回 500。Story 4.3 明令不得改 `cloud/env-scripts/**`（Task 5.4），接线属 CI/部署切片。
- `RuntimeBaselineTest` 把 AppID 占位值字面量化进门禁：`cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/RuntimeBaselineTest.java:40-41` 硬编码 `replace-with-ewf-wechat-app-id` / `replace-with-ewf-local-wechat-app-id`，并在 `:85`、`:117-120` 用它做期望值。Task 4.6 只要求「`.env` 的 `VITE_WX_APPID` 等于 `application.yml` 的 `wechat.mini.app-id`（`.env.development` 同理）」这一跨端一致性，实现却额外钉住「占位值尚未替换」，等于在 Story 内提前落地了红线 6 明确划归部署阶段的替换检查。后果：部署阶段按计划替换 AppID 时该门禁必然变红，且失败文案会写成「应与 `.env` 的 AppID 一致」（实际两边一致），误导排障。

### Deferred from: code review of 4-3-建立-backend-启动-接口信封与环境基线.md（2026-09-22，attempt-2 复核）

- `/error` 是可达的非信封出口，回 500 且不受 `GlobalExceptionHandler` 约束：Spring Boot 由 `spring-boot-starter-web` 自动装配 `ErrorMvcAutoConfiguration`/`BasicErrorController`，实测 `GET /error`、`POST /error`、`GET /error?x=1` 一律 `HTTP/1.1 500` + `{"timestamp":"…","status":999,"error":"None"}`，`Accept: text/html` 回 Whitelabel HTML 页。它与 `cloud/backend/src/main/java/top/zhcmqtt/ewf/backend/common/response/ApiResponse.java:12`「这是 REST 层唯一的响应形状，不得再发明第二种裸响应」的承诺相反，而 `EnvelopeContractTest` 的边界声明原先只承认容器级错误——本轮已按 Task 4.8 把该出口如实补进边界声明（见 story 的本轮 patch）。**未闭合部分**：让 `/error` 也回信封需自定义 `ErrorController`（触碰红线 2「生产代码面只有 Task 1–3 列出的文件」）或改 `application.yml` 的 `server.error.*`（AD-20 裁决不改），属后续故事的产品决策。
- 契约门禁无法发现同一 HTTP 状态类内的码值改义交换：`cloud/backend/src/test/java/top/zhcmqtt/ewf/backend/ErrorCodeContractTest.java:51-64` 经 `registeredCodeOf` 按**码值**反查后断言 `http_status == code/100`，注册表的 `condition` 从未与 Java 常量名绑定。实测把 `ErrorCode.DEVICE_RESET_CONFLICT`(20003) 与 `ErrorCode.QUEUE_FULL`(20004) 的常量值互换后 `mvn test` 仍 18/18 绿、退出码 0——交换后任何按名取 `QUEUE_FULL` 的分支都会回 20003（被客户端读作「设备重置冲突」）。机械绑定常量名与 `condition` 正是 Task 4.2 明令不得引入的「第三真源」，故不就地收敛；码值真正被端点引用时（Epic 5 的同步分支）语义会自然钉住。
- 新增协议级码 `40500`/`41500` 未同步 `docs/backend/经验-后端REST信封与微信登录.md`：该文档 `code 区间表`（第 41 行起）只列 `400xx`/`401xx`/`403xx`/`404xx`/`409xx`/`410xx`/`500xx`，无 `405xx`/`415xx`；`GlobalExceptionHandler 的映射`（第 49 行起）无 405/415/406/缺参任何一行，且把 `40002 PARAM_MISSING`、`40003 PARAM_TYPE_MISMATCH` 留给缺参与类型不匹配，而本实现把这两类与校验失败一并回 `40000`。该文档是 story Dev Notes 列出的「迁移经验（实现参考，非产品事实）」来源，4.4/Epic 5 若照其映射表实现协议级错误会得出与本实现不一致的码。修法是改 `docs/**`，而 spec 的「明确不触碰」列表含 `docs/**`。
- `Accept` 头不含 JSON 时错误分支的信封不可投递：真机（`saas.jar` + `local` profile）实测 `Accept: application/xml` 下 `GET /api/v1/not-a-real-endpoint` → `404` + `Content-Length: 0`、`POST /api/v1/health` → `405` + `Allow: GET` + 空 body，状态码正确但信封丢失，日志出现 `Failure in @ExceptionHandler …handleNotFound`；隔离副本中业务错误分支（`handleBusinessException`）在同样 `Accept` 下外抛为 `jakarta.servlet.ServletException`（真机即 500）。该缺口与 406 的裁定同源——客户端已声明不接受 JSON，此时任何 JSON 信封在协议上都不可投递——故与 406 一并保持现状；强制投递需改内容协商策略（固定错误响应的 `Content-Type`），属设计决定。
## Deferred from: code review of 2-1-统一有效敲击队列与输入闸门 (2026-09-22)

- 三类来源没有实际生产者接入统一投递 API：`submit_physical_pvdf`、`submit_device_touch`、`submit_automatic_tap` 仅有声明/实现，没有 PVDF、触摸或自动敲击调用点；统一队列运行时不会收到业务事件。该项为当前审查的 unresolved HIGH/MEDIUM，需补齐接入并重新审查。

## Deferred from: code review of 4-4-实现微信登录与单设备身份.md (2026-09-22)

- 跨 JVM 并发下 `IdentityStore` 的 `ATOMIC_MOVE` 与目标文件已存在时的 `CREATE_NEW` 语义未锁定；架构当前限定单实例单进程，本 Story 不扩展跨进程锁协议。
- `JwtTokenProvider` 的极端 `expirationMillis` long 溢出边界未防护；当前部署配置为固定正值，亚秒与 long 溢出不属于本 Story 的日常可达配置。
- `expiresIn` 对小于 1 秒生命周期的舍入未定义；产品 JWT 生命周期契约以秒级配置为主，亚秒配置需另行裁定。
- `EnvelopeContractTest` 为适配受保护路径优先级删除了原 405/Allow 与 ERROR 日志断言；恢复需单独设计带有效 access token 的 405 测试，不在本轮扩大测试面。

## Deferred from: code review of 4-4-实现微信登录与单设备身份.md (attempt-2, 2026-09-22)

- `EnvelopeContractTest` 为适配受保护路径优先级删除了原 405/Allow 与 ERROR 日志断言；恢复需单独设计带有效 access token 的 405 场景，不在本轮扩大测试面。

## Deferred from: code review of 4-4-实现微信登录与单设备身份.md (attempt-3, 2026-09-22)

- 微信上游响应体大小未设上限；当前契约未定义统一 payload 限额，需先裁定阈值后再增加保护。
- Authorization access token 长度未设上限；当前仅 refresh 请求有 4096 字符上限，access header 限额需与网关/部署约束统一。
- `identity.json` 文件大小未设上限；需结合 JSON 持久化基线定义文件大小策略。
- OPTIONS 预检请求未进入公开白名单；本 Story 未建立 CORS/浏览器预检策略，需由 frontend/API 部署契约裁定。
- `EnvelopeContractTest` 删除 405/Allow 与 ERROR 日志断言；恢复需设计携带有效 access token 的 405 场景。
