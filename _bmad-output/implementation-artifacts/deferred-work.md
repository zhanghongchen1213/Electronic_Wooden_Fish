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

## Deferred from: code review of 2-1-统一有效敲击队列与输入闸门.md (2026-09-23)

- 主机回归绕过真实 `state_service_run()` owner/队列循环，直接写入 owner 原子变量并调用静态 apply；需增加可控停止的真实任务循环验证，确认 gate 更新能在固件运行路径中应用。
- `state_service_publish_tap_gate()` 的有界队列满/超时行为没有测试证据；需在不消费 state queue 的条件下验证稳定失败原因与旧快照保持。
- 统一 tap service 的 1–20 次突发且无消费者的运行时背压没有覆盖；现有用例每次提交后立即消费，需补充第 17–20 次输入的稳定 `queue_full` 与顺序证据。
- `tap_input_service_submit()` 未初始化分支未填充稳定 typed 原因；该问题不在本次 story-local diff 改动面，后续与服务生命周期契约一并收敛。
- 首次 TAP_GATE 更新前的启动窗口语义未冻结；需裁定是否要求首个 owner 快照确认后才允许正式输入。

## Deferred from: code review of 4-4-实现微信登录与单设备身份.md (attempt-4, 2026-09-23)

本轮 story-local diff 与 attempt-3 相同（repair 未产生代码改动），历次累积的验证缺口由 autofix 实际补齐（17 项 patch），以下为仍开放项。

- 跨进程并发下 `Files.move(..., ATOMIC_MOVE)` 会静默替换既有 `identity.json`，`FileAlreadyExistsException` 分支在单实例内不可达；需在确定部署形态后改用 `FileChannel.tryLock` 或 move 后回读校验。
- 数据目录（父目录）未 fsync，掉电后已应答给客户端的 `deviceId` 可能丢失；AC2 只要求文件级 force + 原子 rename。
- 过滤器以「`/api/` 前缀之外一律放行」而非「默认拒绝」；改为显式白名单需先重设计 4.3 的非 `/api` 探针测试面。
- 过滤器用未解码的 `request.getRequestURI()` 判断前缀，容器按解码后路径映射，百分号编码路径可能绕过；当前生产无受保护端点，需与路径解析策略一并裁定。
- HEAD/OPTIONS 未进入公开白名单；本 Story 未建立 CORS/预检策略，需由 frontend/API 部署契约裁定。
- 401 响应缺 RFC 9110 `WWW-Authenticate` 质询头；AC4 只定义 HTTP 状态与业务码，新增响应头属契约扩展。
- `jwt.secret` 仍为占位值时构造期无日志线索，表现为 fail closed 的 40102/50200；可观测性改进需与运维日志基线一起定。
- refresh token 无 `jti`/单次使用标记，泄露后 30 天内可无限换发；Story 明确不实现 tokenVersion，需先扩展同步契约与身份 schema。
- AC6 要求的四条 RED→GREEN 证据仍未记录命令、首个失败断言与恢复结论；「取消同 openid 互斥」这条负例因同 openid 派生同一 device_id 而恒真不可失败，需改用不同写入者可见的观测量。
- 敏感日志窄扫描范围仍小于 Task 5.4 枚举（Authorization/JWT 明文/AppSecret/响应原文未扫）；本轮已修复恒真断言并补脱敏断言。
- 受保护探针抛异常后的 ThreadLocal 清理无覆盖；本轮已覆盖认证失败与成功路径，controller 异常路径需新增抛异常探针。
- 「重启后 identity 映射保持」无 API 级覆盖；现有断言以重建 `IdentityStore` 实例模拟重启。
- `POST /api/v1/health` 由 405/40500 变为 401/40103，与 Dev Notes「不改变 health 的 HTTP 语义」有张力；这是 AC3 方法级白名单的必然结果，语义归属需在契约层裁决。
- `JwtTokenProvider.parse` 每次重建 parser，未按 Dev Notes 预构建线程安全实例；属性能优化。
- Completion Notes 未说明同 openid 互斥的实现选型（实现为方法级 `synchronized` 全局写锁）。

## Deferred from: code review of 4-5-建立-json-原子持久化基线.md (2026-09-23)

- 契约 §11 的加法迁移规则与严格字段集合相等冲突：`sync-contract.md` §11 / 注册表 `persistence_files[0].migration` 允许「新增字段省略读取并取默认值」，而 `VersionedJsonFile` 执行精确集合相等（AC2 亦如此要求）。需契约层裁定优先规则后再改实现。
- `AtomicJsonFile` 把 `FileAlreadyExistsException` 发布为可用并发哨兵，但 `ATOMIC_MOVE` 走 rename 语义、目标存在时静默替换，该哨兵在单实例内不可达（4.4 attempt-4 已登记同一事实）。该不可达保证现已从 `IdentityStore` 扩散为基元的对外契约；修法属跨进程语义，AD-16 不做。
- 四个 Store 各复制一份信封写路径（字段校验 → 补 `schema_version` → 序列化 + 尾换行 → 原子写 → 两个 catch 映射 50000），无断言钉住序列化形态；任一份副本漂移不会被门禁发现。收敛需在基元中定义信封写入入口。
- Spring 解析出的 `app.data-dir` 是否到达 `PersistenceRecovery` 与四个新 Store 无任何断言（测试均自行传路径，`dataDirectory()` 无调用方）；实测改坏恢复流程的属性键后 66 条门禁仍全绿，自定义数据目录部署下启动恢复会审计 `./data`。修法：在已有 `@DynamicPropertySource` 的全上下文测试中 `@Autowired PersistenceRecovery` 并断言 `dataDirectory()`。
- 严格读取面只校验字段名集合、不校验值类型：`{"schema_version":1,"device_id":42}` 被判「正常」而实际读取一律 50000；`acked_total` 类型错误时 `asInt()` 静默给出 0（AC3 禁止的默认高水位）。修法需冻结逐字段值类型，属契约层议题。
- `IdentityStore` 的 `50000` 文案变更与 AC1「对外行为保持不变」有张力（文案经 `GlobalExceptionHandler` 透出）。保留旧文案需给基元加消息覆写参数（新增公共面），无测试或契约依赖该文案。
- 负例 #4 实际变异落在 `ProgressStore.read()` 而非规格要求的恢复侧通配扫描：因恢复报告不暴露读到的数值，即便恢复真按通配读取孤儿 `.tmp` 也不会被现有断言证伪，AC3 在恢复侧缺少可失败的失败面。
- AC5 证据记录与实测不符：`/tmp/ewf45-mutation/` 缺 `green-5.log`（Completion Notes 称 `green-{1..5}.log`）；5 条负例未记录命令；日志仅在 `/tmp` 不可长期复现；同节日志描述自相矛盾；「对外可观测行为变化（唯一一处）」计数与字节上限说明不一致。
- `AtomicJsonFile` 失败路径 `finally` 中的 `Files.deleteIfExists(temporary)` 若抛 `IOException` 会掩盖正在传播的原始异常（含 `FileAlreadyExistsException` 哨兵）。该写法逐字继承自 4.4 内联实现；触发需「刚创建的临时文件删除失败」，日常不可达。

## Deferred from: create-story of 4-6-提供状态快照与恢复基准 (2026-09-23)

本 Story（4.6）的 Dev Notes「契约缺口登记」四条，以及两条实现时新增的登记项。本 Story **不修改契约**（`docs/contracts/**` 一个字节都不改），以下均需契约层先裁定。

- **缺口 1：REST 查询路径未被契约冻结（裁决 B）。** §12 只冻结 `/api/v1` 前缀与 `{code,message,data}` 信封；§10.2 与注册表 `endpoint`（`rest_base_env`/`rest_base_contains`/`scheme_map`/`path_suffix`/`token_transport`/`token_query_field`）只描述 WebSocket。本 Story 取 `GET /api/v1/sync/snapshot`，并把请求面限制为契约已冻结的基线操作数 `acked_total`（§6.2 差量操作数）与 `snapshot_seq`（§8 续订基准）。→ 需 Epic 5 对齐：§6.2 的上报响应与 Epic 5 的查询/补齐路径必须复用同一个响应 DTO 与同一路径族，否则会出现两套查询形状。
- **缺口 2：`snapshot_seq` 的单调性措辞与 `acked_total` 不一致（裁决 A）。** §2 对 `snapshot_seq` 写「单调递增」，对 `acked_total` 写「单调不减」。§11.1 把 `snapshot_seq` 的持久化作用域冻结为 `frontend`，backend 无法落盘它（第六个状态文件被 `PersistenceFileContractTest` 拒绝，`progress.json` 字段列被契约冻结），因此 backend 只能从 `acked_total` 导出，**只能保证不后退**。→ 需契约层裁定：把 `snapshot_seq` 改为「单调不减」，或为「严格递增」引入可持久化的序号来源（会同时触及 §11.1 与 §11 文件表）。
- **缺口 3：WebSocket 帧 `seq` 空间与 `snapshot_seq` 的对齐未冻结（裁决 A 交接）。** §2 只说「只消费 `seq` 大于快照水位的帧」，未冻结 `seq` 的取值来源，也未说明它与 `snapshot_seq` 是否同一计数器。本 Story 把导出值定在「已确认敲击」序号空间，并据此要求 Story 5.5 的帧 `seq` 与之对齐。→ 属 Epic 5 的契约议题。
- **缺口 4：§12 未为「客户端声明高于已确认水位」单列拒绝条件（裁决 D）。** 本 Story 把它归入 `20002 BASELINE_HIGH_WATER_MISSING`——该声明从未被确认，故不构成有效基准（与「缺少基准高水位」同码）。→ 需契约层确认是否单独设码，或明确写成本 Story 的归并口径。
- **缺口 5（实现新增）：§12 未为「请求无法归入当前已确认轮次」（`20001`）冻结判定输入。** 本 Story 取唯一判据：权威 `progress.json` 的**轮次相关字段**落在契约取值域外（`round_id` 非 `integer ≥ 1`、`round_cursor` 非 `integer ≥ 0`、或 `round_state` 不在 `{in_progress, completed}`）。次要点：本 Story 的请求面不含 `round_id`（裁决 B 不允许新增契约未授权字段），因此无法按请求携带的轮次直接判定归属。→ 需契约层裁定判定输入（例如是否要求写路径的请求必须携带 `round_id`，以及查询路径是否也应携带）。
- **缺口 6（实现新增）：契约未为设备状态镜像的「未上报」表达保留取值。** `network_mode` 的取值域只有 `connected|no_signal|disabled`，没有 `unknown`。本 Story 在 `device_state.json` 缺失时取 `no_signal`（明确不取 `connected`，避免伪造在线），`battery_percent` 取 0、`audio_config_version` 取 0、`firmware_version` 取空串。→ 需契约层裁定未上报时的表达（增设 `unknown` 需递增 `contract_version`）。

## Deferred from: code review of 4-6-提供状态快照与恢复基准.md (2026-09-23)

本轮 `bmad-code-review`（review_depth=deep、四层全开：blind-hunter / edge-case-hunter / verification-gap / acceptance-auditor）的 defer 条目。前六条是**契约层裁定**项（本 Story 与本次 review 均不改契约）；后三条是低风险工程项。

- **落后基准被判 `20003`，与 §12/§6.3 明文相反，且 §8 的判定输入不是本端点的参数。** `StateSnapshotService.decisionCode` 把「客户端声明低于已确认水位」映射为 `DEVICE_RESET_CONFLICT`，而契约 §12「同值或更低高水位是幂等 no-op，不是错误」、§6.3 同款措辞；§8 的「设备重置（`local_total` 低于 cloud 基准）」判定输入是 `local_total`，本端点请求面（裁决 B）只有 `acked_total`/`snapshot_seq`。可达后果：权威水位在两次查询之间前进一次后，客户端带上一次响应里的 `acked_total` 再查就会被判「设备重置冲突、请按云端基准重建」，而契约把同一提交定义为 no-op。spec 的裁决 D 已定此口径，故本轮不自动改写；需契约层确认「越前/落后/未携带」三者的码值与判定输入。
- **查询端点把 `snapshot_seq` 与 `acked_total` 跨计数器比较。** 注册表自带样例 `ws_snapshot` 是 `seq: 512` 与 `snapshot_seq: 512` 而同帧 `acked_total: 128`，`ws_delta` 是 `seq: 513`/`acked_total: 129`——帧 `seq` 与 `acked_total` 之间存在常量偏移，与裁决 A 的「同处一个已确认敲击序号空间」论证相反。按 §8「从 `snapshot_seq + 1` 续订」携带 WS 帧水位的合法客户端会被判 `20002`，永久无法续订或重建基准（现无 WS 实现，故今日不可达，属 Epic 5 的对齐面）。spec 的缺口 3 只登记了「`seq` 取值来源未冻结」，未登记该查询侧后果；需契约层先冻结 `snapshot_seq` 的空间归属。
- **响应回传 7 个未在 §2 声明 `https-response` 通道的字段。** `device_id`、`local_total`、`applied_revision`、`battery_percent`、`network_mode`、`audio_config_version`、`firmware_version` 的 §2 承载通道集合均为「HTTPS 上报、WS 帧、持久化文件」，不含「HTTPS 响应」；§6.2 还明确写「已生效判定所需的设备已应用高水位由请求携带，不在响应必填内」。它们进入响应的唯一理由是 AC1 要求把 `ws_frames[snapshot].payload_fields` 并进 17 字段闭包。§13 的门禁只检查「§6.2 响应必填须声明对应通道」这一方向，故该冲突恒绿且此前未登记。修法只有「为 REST 查询新增一个通道并递增 `contract_version`」或「收敛响应面」两条，均在现有 6 条缺口之外。
- **`NO_CONFIRMED_ROUND_ID = 1` 哨兵与真实第 1 轮在 wire 上同形。** 未初始化（`progress.json` 缺失）时下发 `round_id=1`、`round_cursor=0`、`round_state=in_progress`、`pending_completion=false`，与一个真实存在的第 1 轮在首字被确认前上报的四元组完全一致；`StateSnapshotResponse` javadoc 的「不可能被误读为一个真实轮次」不成立，客户端按 §8/AC5 的 `round_id` 路由时无法区分「尚无已确认轮次」与「轮次 1 进行中」。契约把 `round_id` 冻结为 `integer ≥ 1`，域内无可用哨兵，修法（增设 `unknown` 需递增 `contract_version`）属契约层。
- **`20001` 分支顶替轮次标识却仍原样下发累计水位。** `AuthoritativeState.toResponse` 在不可归属时把 `round_id`/`round_state`/`round_cursor` 换成常量（**即使权威 `round_id` 与 `round_cursor` 本身在域内**，例如仅 `round_state` 越域），却不改写 `acked_total` 与 `local_total`，客户端可能把 `local_total − acked_total` 的差量归入一个不存在的第 1 轮。同一份不合法文件存在双口径：类型越域走 fail closed `50000`，取值越域却降级为 `20001` + 顶替值。AC5「轮次相关字段只取自 `progress.json`」与 裁决 E「不给可归属的模糊水位」在此分支冲突，需规格层/契约层裁定。
- **读取面不校验取值域（含负水位导致的永久拒绝）。** `intField`/`textField`/`booleanField` 只校验字段类型；`progress.json` 的 `acked_total`/`local_total` 为负时被放行，`snapshot_seq` 以负数上线，而请求面由 `@Min(0)` 保证声明非负，于是 `declared > confirmed` 恒成立、该端点对一切客户端**永久回 20002**；`commands.json` 的 `brightness`/`timeout`/`volume` 与 `device_state.json` 的 `network_mode`/`battery_percent` 越域值亦原样下发。契约未冻结逐字段值域校验（4.5 已登记同一缺口），需契约层先裁定。
- 越域基准参数的实际 `400` 文案会把内部 Java 参数路径透出到客户端：实测 body 为 `{"code":40000,"message":"snapshot.ackedTotal: acked_total 不可为负"}`（`GlobalExceptionHandler.handleConstraintViolation` 直接拼接 `violation.getMessage()`）。该分支此前在生产路径不可达，是本端点让它第一次可达。修法需去掉类级 `@Validated`（本次受审 spec 明确要求 `@Validated`）或改共享异常处理器的文案拼装（影响其他端点），属决策项。
- `StateSnapshotService.commandApplied` 与 `assemble` 在生产路径无调用方（仅测试引用；控制器只用 `resolve`），故 §9 的已生效判定没有任何已交付出口，客户端只能自行派生；`StateSnapshotContractTest`/`StateSnapshotServiceTest` 之外无第二处引用。删除会与本次受审 spec 的 Task 2.1/2.4 冲突，故记债务而不自动改。
- `deferred-work.md` 的 4-3 条目「错误信封无法携带 `data`……归 Epic 5」已被本 Story 的 `ApiResponse.error(int, String, T)` 部分收敛，但本 Story 的登记节没有按该文件既有惯例补「收敛登记」行（残留：异常层路径仍只产出两键信封，本 Story 刻意未改）。Epic 5 若照旧条目理解会重复改造信封 API 或另造携带形状。

### 本轮 review 的自动修复（已应用并实测通过）

- `StateSnapshotContractTest.非法基准参数回参数错误`（新增）：断言 `acked_total=-1` 与非数值 `snapshot_seq` 回 `400` + `40000`。此前该 400 的唯一护栏是一对 `@Min(0)`，去掉后 `-1` 会静默流进选码分支变成 `200` + `20003`，而 85 条门禁无一失败。
- `StateSnapshotServiceTest` 的 `字段类型不符不静默回退`（扩写）：在原有 `intField`（`acked_total`）用例之外补 `round_state` 写数字与 `pending_completion` 写字符串两例。此前 `textField`/`booleanField` 的 fail-closed 护栏无任何测试，退化后会把「文件损坏」变成貌似合理的 `20001` 业务拒绝。
- `StateSnapshotService.exportSnapshotSeq` 的 javadoc 更正：原文称「缺失（未初始化）或该字段不是整数时取 `NO_COUNT`」，与实现（文件存在而字段非法即 fail closed 抛 `50000`）相反，会误导读者以为存在 AC3 明令禁止的静默回退。

## Deferred from: code review of 2-2-实现本地高水位事务持久化与离线积压 (2026-09-23)

- `progress_store_nvs.c` 加载/保存语义零测试覆盖：NVS 主机不可测，ESP-IDF 编译通过且 dev receipt 已声明 hardware_pending；样机到位后按真机闭环 runbook 验证掉电一致性与恢复链路（并入该 story 的 QD-2.2-1）。
- `legbot_services.c` 启动/停止序列变更无主机测试：`contract_checks` 静态断言只覆盖服务数量，启动图顺序（progress 先于输入边界停止）无回归护栏；启动图主机测试基建留待后续 story 一并建立。
- 主机测试未记录积压拒绝计数：AC4 主文"快照或日志"已由每次拒绝的中文日志（`SVC_PROGRESS` 拒绝敲击日志）满足；替身计数需在 progress_service 新增公共计数器，作为增强项留待后续，不为本项扩展公共面。
- `progress_service_advance_acked_total` 取锁 `portMAX_DELAY` 无界（`timeout_ticks` 只约束发布等待）：与同文件既有服务取锁模式一致，owner 持锁段为有界落盘操作，当前无阻塞证据；接口语义调整留待 Story 2.5 接入时统一评估。


## Deferred from: create-story of 5-1-实现高水位幂等同步 (2026-09-23)

1. **`POST /api/v1/sync/report` 路径未被契约冻结**（与 4.6 缺口 1 同族）：实现按裁决 D 落地该路径；需契约层补 REST 路径表或标明「实现路径 ⊆ 未冻结但可用」。
2. **`https_sync_response` 样例 9 字段 vs 本 Story/4.6 的 17 字段响应 DTO**：实现复用 17 字段 `StateSnapshotResponse`；需契约层裁定样例是否扩展或标明「最小必填 ⊆ 实现闭包」。
3. **写路径 `20001` 的判定输入**：本 Story 采用「请求轮次相对权威轮次无法归属」的具体分支（请求 `round_id` 小于权威，或权威 `pending_completion==true` 时提交更大新 `round_id`；取值域外亦 `20001`）；需契约层补 `condition` 判定输入表。
4. **`action_id` 在纯高水位同步（非篇章动作）下的去重粒度**：本 Story 取「同 `action_id` → 返回首次确认后的权威快照」；若上游意图是「仅篇章动作去重」，需契约澄清。

## Deferred from: code review of 5-1-实现高水位幂等同步.md (2026-09-23)

- 同 `action_id` 但改写请求积压差（`local_total−acked_total`）时，短路径仍按**当前请求**重算 `20004`：同体网络重试结果一致；异体同键是否应冻结首次业务码需契约澄清去重粒度（与 create-story 缺口 4 同族）。
- 轮次/状态非法取值经 Bean Validation 回 HTTP 400/`40000`，与裁决 B「域外 → 20001」存在张力；code-review 保守保留 Task 3.2 协议校验路径，契约层若要求业务码统一再改。
- `round_state=completed` 且 `pending_completion=true` 的矛盾组合未在写路径拒绝：完成态状态机属 Story 5.2。
- `ProgressSyncService.deviceLocks` 按 `deviceId` 无限增长：MVP 单身份单设备，低风险；多设备前再做弱引用/驱逐。
- 拒绝响应（如 `20003`）未与成功路径做相同的 17 字段注册表闭包断言：现有用例已覆盖信封三键与关键基准字段，完整闭包可后续补强。

## Deferred from: create-story of 5-2-实现轮次-完成与跨轮动作 (2026-09-23)

1. **`POST /api/v1/sync/round-action` 与 body 字段 `action`（`restart|exit`）未被契约冻结**（与 4.6/5.1 REST 路径缺口同族）。实现按 Story 裁决 D；不得改契约字节。
2. **「退出」是否同属 §9 篇章动作族**：契约正文点名「从头开始」「立即同步」；exit 语义在 §7/FR-C-004。本 Story 按篇章动作族 `action_id` 落 `progress.json`；需契约补枚举。
3. **完成确认触发的精确判定输入表**（末字 + `pending_completion`/`completed` 意图）需契约 `condition` 级补强。
4. **完成锁定期间是否仍确认更高 `local_total`**：本 Story 取「累计可推进、游标冻结」；若产品要求锁定期间连累计也拒，需契约/PRD 澄清（FR-C-004「输入不计数」主要由设备 gate 保证）。

## Deferred from: code review of 5-2-实现轮次-完成与跨轮动作.md (2026-09-23)

- 裁决 A「未达末字 `pending_completion=true` → 20001」与裁决 B「pending 期间允许同轮把游标推到末字」文案互相矛盾；实现保守保留 A，新写入下 pending 无法在未完成确认态稳定驻留。需后续 story/契约统一口径后再改行为或改 javadoc。
- `roundAction` 同键短路径（含进程内已消费集合）不区分 `restart|exit`：同键跨动作碰撞返回当前权威快照。客户端须为每次动作使用新 `action_id`；若需按动作类型区分，要扩展 `progress.json` 字段闭包（契约/Store 决策）。
- 篇章动作幂等集合 `consumedRoundActionIds` 仅进程内有效：进程重启后回退为 `progress.action_id` 单槽语义；多进程文件锁与持久化幂等集属本 Story 明确未证明面。

## Deferred from: create-story of 5-3-实现可信时间和历史统计 (2026-09-23)

1. **`GET /api/v1/sync/stats` 与 `HistoryStatsResponse` 字段未被契约冻结**（与 4.6/5.1/5.2 REST 缺口同族）。实现按 Story 裁决 C/D；不得改契约字节。
2. **`daily_stats.json` 桶键/桶值形状仍不在契约 §11 字段列**——本 Story 实现冻结为 `yyyy-MM-dd` → `{confirmed_taps}`；是否升格进注册表属契约修订，不在本 Story 改字节。`DailyStatsStore.allowedFields()` 继续空集。
3. **连续天数在「今日为 0」时是否中断**：本 Story 取裁决 F（昨日起点）；若产品要「严格含今日否则清零」，需 PRD/UX 澄清。
4. **查询发现 `Σ ≠ acked_total` 时是否允许只读自愈写盘**：本 Story 禁止查询写盘；若运维需要，属后续 story。

## Deferred from: code review of 5-3-实现可信时间和历史统计.md (2026-09-23)

- GET `/api/v1/sync/stats`（`HistoryStatsService.query`）未与 `ProgressSyncService` 的 `deviceLocks` 共享：并发 report 归档与 stats 查询可能读到撕裂的 progress/buckets 组合。查询串行化会引入跨服务锁面；MVP 单设备低频，后续若出现并发撕裂再收敛。

## Deferred from: create-story of 5-4-实现命令修订和待设备应用收敛 (2026-09-23)

1. **`POST /api/v1/sync/command` 与 `SettingsCommandRequest`（含可选 `base_revision`）未被契约冻结**（与 4.6/5.1/5.2/5.3 REST 缺口同族）。实现按 Story 裁决 A/B；不得改契约字节。
2. **epics/PRD「current_revision」与契约 `command_revision` 名称落差**——实现与测试只使用 `command_revision`；规范修补属文档修订，不改契约字节。
3. **「立即同步」是否占用设置族或篇章族 `action_id`、是否递增 `command_revision`**——契约 §9 称立即同步无值载荷；本 Story 不做，需后续 story 冻结。
4. **WebSocket `command_state` 帧与设置下发的推送时机**——属 Story 5.5；本 Story 仅 REST 快照可读。

## Deferred from: code review of 5-4-实现命令修订和待设备应用收敛.md (2026-09-23)

- `applied_revision` 已超前于当前 `command_revision` 时，新设置下发按 Task 2.3 保留 applied，可能使 `commandApplied` 立刻为真；下调 applied 会破坏 5.1 单调不减。MVP 依赖设备报告 applied≤command；若产品要求「新载荷必待应用」需后续单独裁决。
- 跨 `deviceId` 并发写共享 `CommandsStore`：产品为单身份单设备；`deviceLocks` 按 deviceId 符合边界，多设备共享文件属既有模型限制。
- AC #3「设置下发只写 commands」与 AC #2 `applied > command` 收敛分支缺少正面测试断言；实现路径已只写 commands，属验证缺口。

## Deferred from: create-story of 5-5-实现-websocket-差量推送 (2026-09-23)

本 Story（5.5）**不修改契约**（`docs/contracts/sync-contract.md` 与 `sync-contract.schema.json` 只读引用）。以下缺口需契约层或后续 Epic 裁定：

1. **注册表样例 `ws_snapshot` 的 `seq/snapshot_seq` 与 `acked_total` 偏移**（样例 `seq=512` vs `acked_total=128`）vs 4.6/5.5 导出口径（`snapshot_seq := acked_total`）。本 Story 实现对齐 4.6 导出，不把样例偏移当义务。
2. **`snapshot_seq`「单调递增」vs `acked_total`「单调不减」措辞**（4.6 已登记）——仍开放；backend 只能保证不后退。
3. **非进度帧（`command_state`/`completion`/`heartbeat`/`error`）的 `seq` 是否允许偏离敲击空间、重连是否需重放**——本 Story 取「会话单调 + 重连靠 REST」；若产品要强重放需先改契约。
4. **WS 关闭码业务段取值表未逐事件冻结**——实现选用标准 `1000` 替换旧连接、协议错误用 `4001`；正式码表可后续补进契约。
5. **生产 WSS 反代 idle 超时**——architecture Deferred，非本 Story；真机 1s 端到端与小程序回放 UI 属 Epic 6/7。
6. **收敛登记（相对 5.4 缺口 4）：** WebSocket `command_state` 帧与设置下发推送时机已由本 Story 交付；契约样例偏移与措辞张力仍见上列 1–3。

## Deferred from: code review of 5-5-实现-websocket-差量推送.md (2026-09-23)

- 心跳 interval/timeout 路径无自动化覆盖：实现默认 30s/10s 与契约一致，但 `WebSocketPushContractTest` 未驱动真实超时关闭；可注入时钟或缩短测试配置属后续增强，真机/反代 idle 属 Epic 6/7。
- `report` 使 `applied_revision` 追上导致「已生效」翻转时的 `command_state` 推送缺少专门 WS 断言；设置下发与拒绝路径已覆盖，实现钩子存在。

## Deferred from: create-story of 5-6-实现错误语义-重试与安全边界 (2026-09-23)

本 Story（5.6）**不修改契约**（`docs/contracts/sync-contract.md` 与 schema 只读）。以下缺口登记：

1. **`20002` 缺少基准高水位**仍可能无写路径触发——是否需要专用 report 分支由后续产品裁定；本 Story 不发明触发条件（查询 snapshot 无基准已可验收）。
2. **401 响应缺 `WWW-Authenticate`**（4.4 deferred）——仍开放；非本 Story。
3. **Epic 6 登录页与 request 常量路径联调**——本 Story 固定 `/pages/login/index`；页面文件由 6.1/6.2 创建。
4. **生产合法域名与 HTTPS**——AD-20/运维，非本 Story。
5. **Problem Details 与信封双真源风险**——本 Story 已在 `application.yml` / `application-local.yml` 显式 `spring.mvc.problemdetails.enabled=false` 并用测试钉死；若未来有人重新启用，门禁需继续拒绝。

**交接更正（相对 4.4 / 5.5）：** 401 单飞刷新队列与统一 `api/request` 已由本 Story（5.6）交付；Epic 6 只接壳层、登录页与页面消费，不得再发明第二套 request。

## Deferred from: code review of 5-6-实现错误语义-重试与安全边界.md (2026-09-23)

- 单飞 `isRefreshing` 在唤醒订阅者前清零存在短暂双刷窗口；与「先 notify 再清零」的挂起风险权衡后本轮不引入世代锁；跨进程 refresh 竞态仍属未证明面。
- `ErrorSemanticsContractTest` 未独立签发真过期 access 断言精确 `40101`；现覆盖 refresh-当-access 的 40102/40101 并集。
- 同步拒绝矩阵 `assertAction` 仅钉 `acked_total`/`snapshot_seq`；完整 17 字段闭包依赖既有 ProgressSync/StateSnapshot 契约测试。

## Deferred from: create-story of 5-7-验证业务状态重启与原子恢复 (2026-09-23)

本 Story（5.7）**不修改契约**（`docs/contracts/sync-contract.md` 与 schema 只读）。以下缺口登记：

1. **篇章动作进程内幂等集 `consumedRoundActionIds` 跨重启**——仍开放（承接 5.2 code-review deferred）；本 Story 验收剧本规避「单槽已被 report 覆盖的旧键」，不把该集合持久化进 `progress.json`。
2. **独立 OS 进程 fork + kill 级重启门禁**——非本 Story 强制范围；单测以「同 data-dir 重建服务/Store + PersistenceRecovery」为合法近似；真进程级属运维/Epic 7 附录。
3. **跨文件崩溃窗口**（权威 `progress.json` 已写、派生尚未写）——既有「派生失败不回滚 + 幂等重放收敛」；本 Story 用重启后重放证明收敛，不引入两阶段提交或 WAL。
4. **GET `/stats` 与 `deviceLocks` 并发撕裂**——5.3 deferred；本 Story 单线程业务剧本不覆盖该竞态。
5. **掉电 / 多进程文件锁 / 生产磁盘 fsync 与挂载选项**——4.5/AtomicJsonFile 已知边界；本 Story Completion Notes 必须继续声明为未证明面。

## Deferred from: code review of 5-7-验证业务状态重启与原子恢复.md (2026-09-23)

- **跨文件崩溃窗口未注入：** create-story 登记「用重启后重放证明收敛」，但主剧本仅在派生全部写成功后同键重放。`action_id` 短路径下同键 report 可能不补写缺失的 `daily_stats`/`commands`/`device_state`，因此当前套件不能证明「权威已写、派生未写」窗口的收敛。需单独设计注入（或产品侧非短路径 catch-up）后再验收；不阻断 5.7 done。

## Deferred from: create-story of 6-1-建立-mini-06-小程序壳层-令牌和导航

1. 原生 tabBar vs 自定义 bottom-nav 的最终视觉像素差——以 MINI-06 HTML 对拍为准，6.9 终验前允许壳层近似（本 Story 采用 pages.json 原生 tabBar + selectedColor `#a66b3a`，自定义 BottomNav 组件保留备选）。
2. 字体 Noto Serif/Sans SC 小程序加载策略（包体 vs 系统回退）——壳层使用系统字体保底，正式字形闭合随 6.9。
3. 微信隐私协议/用户协议勾选文案——属 6.2 登录页产品文案，不在 6.1 发明。
4. 生产 `VITE_WX_APPID` 仍为 replace 占位——部署前替换；本 Story 只保证从 env 读取。
5. 交接：6.2 负责登录直达与权限态；6.3+ 填业务页；6.9 做 14 状态对拍终验。

## Deferred from: create-story of 6-2-实现登录直达与权限状态

1. 《用户协议》《隐私政策》正式法务正文——本 Story 交付产品短文本地页，上架前由作者替换（页面可见文案不含 TODO/draft 等禁用词）。
2. LOGIN 帧印章/纸纹等视觉装饰像素差——逻辑两态（BASE / PERM_ERROR）本 Story 已交付，像素终验归 6.9。
3. 生产 `VITE_WX_APPID` / 公众平台 request 合法域名 / 真机微信授权弹窗——Epic 7 / 部署假设；本 Story 仅 Node mock 护栏。
4. deviceId 落盘策略——本 Story 按裁决 E 选择仅 Pinia 内存（不写 `ewf_device_id`），避免突破 5.6 storage 三键白名单；若后续展示页需要持久缓存再单独立项。
5. 交接：6.3+ 填阅读业务与快照/WebSocket；6.9 做 LOGIN 帧像素终验；Epic 7 真机合法域名与 AppID 三处对齐。

## Deferred from: code review of 6-2-实现登录直达与权限状态.md (2026-09-24)

- **页面级 uni.login→store→switchTab 串联未做组件挂载测试：** 护栏拆测 `loginWithWxCode` 与 `navigateToReadingAfterLogin`；未挂载 `pages/login/index.vue` 验证手势接线。真机授权与隐私弹窗归 Epic 7；不阻断 6.2 done。
- **App.onLaunch 门闸接线无专门断言：** `ensureAuthenticated` 行为已测；`App.vue` 调用点未做挂载 harness。价值低于再引 App 测试基建；不阻断 6.2 done。

## Deferred from: create-story of 6-3-实现在线心经持续阅读流

1. 完整 REPLAY/OFFLINE UI、「回放中·新事件排队」与 `replay_cursor` 持久化权威恢复——Story 6.4。
2. `completion` 礼花弹窗与篇章折叠归档——Story 6.5。
3. READING 像素终验与印谱/17 槽基线装饰差——Story 6.9。
4. 真机 WSS 合法域名与 SM-3 1s 端到端实测——Epic 7。
5. 动画基线/下限毫秒若 UX 日后冻结精确值，以 UX 更新为准并改常量（本 Story create-story 按裁决 B 先锁可测默认 320ms/48–80ms）。

## Deferred from: code review of 6-3-实现在线心经持续阅读流.md (2026-09-24)

- **阅读页 error 相位无产品态文案：** sync/WS 失败当前静默重查或停在 error 相位；AC 允许静默恢复，完整失败/REPLAY banner 归 Story 6.4。
- **快照未校验 scripture_version：** 本 Story `StateSnapshot` 对齐契约 17 字段，无该 wire；AD-4 版本不一致停推待契约扩展后接入。
- **onShow 时 liveActive 已 true 跳过重入水合：** 正常路径依赖 onHide→stopLiveSession；若宿主漏发 onHide，需另加固幂等重入。

## Deferred from: create-story of 6-4-实现断线查询与离线回放

1. READING REPLAY/OFFLINE 像素级对拍与印谱装饰——Story 6.9。
2. `completion` 礼花弹窗与篇章折叠归档——Story 6.5。
3. 真机断线恢复与 WSS 合法域名 / SM-3 1s——Epic 7。
4. 快照 wire 若未来增加 `scripture_version`，再接 AD-4 停推校验（承接 6.3 defer）。
5. 交接（dev-story 6.4）：6.5 接 `completion`+礼花（仅 live/replay 完成后，勿在 OFFLINE 误弹）；6.9 做 READING REPLAY/OFFLINE 像素终验；Epic 7 真机断线恢复与 SM-3。

## Deferred from: code review of 6-4-实现断线查询与离线回放.md (2026-09-24)

- **重连次数耗尽 → error 无护栏：** `scheduleReconnect`/`resyncFromSnapshot` 耗尽分支仅有 `maxAttempts===10` 常量断言；连续失败至 error 未钉住。主路径 1006→恢复已覆盖；可留 Epic 7 真机断线或后续加固。
- **同轮快照重入未清空 pendingDeltas：** `applySnapshot` 仅在 round 切换时清空队列；同轮 mustReplay/resync 保留旧 pending，多数靠 stale 丢弃，交错语义需产品确认后再改。

## Deferred from: create-story of 6-5-实现完成态-礼花和篇章归档

1. READING.DONE / OVERLAY.DONE 像素级对拍与印谱装饰——Story 6.9。
2. 记录页今日/周期/连续统计呈现——Story 6.6。
3. 真机完成遮罩 ↔ 小程序弹窗跨端一致性与 SM-3——Epic 7。
4. 篇章归档是否跨冷启动持久化及条数上限——产品未钉死；本 Story 实现为内存展示列表（登出/reset 清空），上限与淘汰策略待产品钉死后补。
5. 承接 5.2：`consumedRoundActionIds` 跨进程重启仅单槽——前端同键重试依赖 backend 单槽；勿假设无限进程内集合。
6. 真机「减少动效」系统开关与 `uni.getSystemInfo` 字段可得性——本 Story 以 `prefers-reduced-motion` / `reduceMotion` 尽力探测，真机终验留 6.9/Epic 7。
7. 交接：6.6 接记录页统计；6.9 做 READING.DONE / OVERLAY.DONE 像素终验；Epic 7 跨端完成验收。

## Deferred from: code review of 6-5-实现完成态-礼花和篇章归档.md (2026-09-24)

- `resyncFromSnapshot` 在 `displayFrozen` 且 `mustReplay` 时先写回 `displayBase` 再 `applySnapshot`：若持久化 replay 游标落后于已冻结全文，理论上可把完成弹窗下的正文回退；本轮未构造稳定复现路径，留待 6.9/联调场景钉死。

## Deferred from: create-story of 6-6-实现记录页统计

1. RECORDS.BASE/EMPTY 像素级对拍与印谱装饰——Story 6.9。
2. `GET /sync/stats` 仍未入契约注册表——承接 5.3 deferred；本 Story 前端按 DTO 消费，不改契约。
3. `pending_sync` 页眉短句是否产品钉死——推荐弱提示「含待同步」；若审查要改文案，走 6.9/文案修订而非发明营销句。
4. 设备页状态与立即同步——Story 6.7。
5. 设置镜像与待应用——Story 6.8。
6. 真机日界/连续天数跨端观感——Epic 7。
7. 交接：6.7 设备状态；6.8 设置；6.9 RECORDS 像素/印谱终验；Epic 7 跨端统计一致性。

## Deferred from: code review of 6-6-实现记录页统计.md (2026-09-24)

- `readingStream.replay.guards.test.ts` 在本 Story 为稳定偶发失败而固定 `Math.random→0` 并缩短 timer/flush microtask：属测试侧范围漂移，不改阅读流生产逻辑；是否回滚或抽到独立 flaky 修复留给后续回归专项。

## Deferred from: create-story of 6-7-实现设备状态页与同步动作

1. DEVICE.BASE/FAIL_RETRY 像素级对拍与印谱装饰——Story 6.9。
2. HTML 样例「76% · 未充电」与产品禁充电冲突——本 Story 已裁掉充电后缀；若设计要改 HTML 样例，走 UX 修订而非实现照抄。
3. backend「立即同步」`sync_now` 篇章动作（契约 §9 点名、RoundAction 未收）——后续 backend story / Epic 7；本 Story 前端以刷新相位诚实表达。
4. 快照缺少 `last_sync_at`——客户端相对时间；若产品要服务端权威时间，需契约变更（禁止本 Story 私加）。
5. 设置页镜像与命令下发——Story 6.8。
6. WS 驱动设备页实时刷新——可选增强，非本 Story 门禁。
7. 交接：6.8 设置镜像；6.9 DEVICE 像素/印谱/FAIL_RETRY 终验；backend `sync_now` / Epic 7 真机立即同步。

## Deferred from: code review of 6-7-实现设备状态页与同步动作.md (2026-09-24)

- 设备 `devstatus-row` 缺 HTML 侧 battery/network/last-sync/pending/command 图标通道：本 Story 以 label+value 文字双通道交付；图标资产与 DEVICE 像素终验移交 Story 6.9。

## Deferred from: create-story of 6-8-实现设置镜像与待应用状态

1. SETTINGS.BASE/PENDING 像素级对拍、印谱、图标双编码——Story 6.9。
2. HTML 导出无独立 `ST:PENDING` 帧名——语义由状态条文案切换；若 UX 日后补独立帧，走设计修订。
3. WS `command_state` 前端实时收敛——可选增强或 Epic 7 联调加强（本 Story 门禁以 REST onShow + 提交响应为准）。
4. 真机应用修订回传——Story 7.2。
5. `base_revision` 未进契约正文——5.4 已 deferred；本 Story 沿用字段，禁止改契约。
6. 设备页与设置页命令态双处展示——保持同谓词即可；跨页 store 同步非本 Story 门禁（各自 REST 水合）。
7. 交接：6.9 SETTINGS 像素/印谱/PENDING 终验；Epic 7.2 真机应用修订；可选 WS `command_state` 实时翻转增强。

## Deferred from: code review of 6-8-实现设置镜像与待应用状态.md (2026-09-24)

- `deriveApplyStatus` 对运行时 null/NaN 修订号未做 `Number.isFinite` 门闸：契约/类型恒为 number，未构造脏 wire 路径；若后端脏载荷出现再加守卫。
- 音量滑杆触区 ≥44×44 未做像素终验：行高已用 `$touch-min`；SETTINGS 触控/像素终验移交 Story 6.9。

## Deferred from: create-story of 6-9-完成空态-失败态-权限态和视觉卫生门禁

1. 待校时显式横幅——HistoryStats/Snapshot 无 trust 字段；无信号前不伪造；backend 补 `time_trust` 后再接 STATE_COPY 预留词表。
2. 作者 HTML 未导出全部 14 个独立 `ST:*` 节点——语义对拍以 UI_CONTRACT + 运行态为准；若 UX 补帧走设计修订。
3. 真机字体 Noto 包体 vs 系统回退、原生 tabBar 与自定义 bottom-nav 像素差——Epic 7 / 设计修订。
4. 截图级自动视觉回归 CI——超出本 Story；dev-story 以人工逐屏 + 自动化令牌/文案护栏为准。
5. `sync_now` 写路径、真机命令 ACK、WS 实时设备页——既有 defer，非本 Story。
6. 交接：Epic 7 真机端到端与样机证据（SM）；本 Story 闭合 Epic 6 小程序门禁（14 态护栏 + 卫生扫描已落地）。

## Deferred from: code review of 6-9-完成空态-失败态-权限态和视觉卫生门禁.md (2026-09-24)

- 设备页 `queueFull`/`lowBattery` 告警条可见性仅有源码 `toContain` + store getter，无 Vue 挂载运行时断言；要闭合需挂载试验架，超出当前门禁风格。
- 14 态护栏以文案金句/相位表驱动为主（裁决 B）；多数态未断言组件分支可达；像素/挂载验证记 Epic 7 / 人工逐屏。
