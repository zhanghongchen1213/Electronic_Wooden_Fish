#!/usr/bin/env python3
"""跨层同步契约门禁测试。

一次运行两类检查：

1. 正向门禁：真实交付物上 `docs/contracts` 下的契约正文与机器可读注册表必须逐值一致，
   期望字段与事件来源集合从 `prd.md` §7 与 `ARCHITECTURE-SPINE.md` 的清单行**机械提取**，
   不从本文件手抄第三份；
2. 负例门禁：必须失败的场景在**临时目录副本**上执行，真实交付物不被污染。

纯 Python 3 标准库，使用 `assert` + 失败计数 + 非 0 退出，不依赖 pytest
（本机 `import pytest` 失败）。范式与 `docs/contracts/canonical/tests/run_canonical_tests.py`
一致。

### 静态校验边界（如实声明，不得让门禁的承诺强于实现）

门禁能机械判定的是「指针文档的表述与清单完整性」：交付文件的存在性与编码卫生；契约正文与
注册表的 `contract_version`；§2 字段表（类型、单位、所有者、权威、取值域、承载通道、出处）、
§3 帧自有字段表（类型、单位、角色、取值域、承载通道）与 §9.1 命令载荷字段表（类型、单位、
取值域、出处）逐列一致；§5 三个状态词表的（wire 名、冻结中文显示词、含义）三元组一致；
§6.1 上报必填清单与 §6.2 响应必填清单的反引号标识符与注册表字段/命令载荷一致；§10.1 帧类型
表的（帧名、方向、内容、出处）四元组一致；§10.3 参数表的（默认值、测试上界、单位）一致；
§11 文件表的（承载事实、`schema_version`、字段、迁移说明）与 §11.1 持久化作用域表的
（作用域、载体、承载字段）一致；§12 拒绝条件的（条件、HTTP 状态、业务码）三元组一致；
承载通道四个取值与本文清单**双向闭合**（§6.1 必填集合 == 声明 `HTTPS 上报` 的字段集合；
§6.2 响应必填的 §2 字段、§10.1 帧载荷字段、§11 文件表与 §11.1 作用域承载的 §2 字段都必须
各自声明对应通道）；`prd.md` §7 与 `ARCHITECTURE-SPINE.md` §Structural Seed 清单行反引号
标识符提取出的期望集合逐值比对；样例字段声明完整性（`required` ⊆ `payload` ⊆ 已声明字段名）、
帧样例的 `contract_version` 等于注册表版本、样例中枚举型字段的取值落在声明取值域内；参数
测试上界；「承载通道含持久化文件的字段必有持久化落点」、backend 作用域等于文件表字段并集、
单事务分组唯一性与组内字段落点；`error` 对象只由 `error` 帧必带；离线积压阈值、判定输入、
backend 业务码与**达上限后的排空路径**；注册表 `endpoint`（含 `scheme_map` 与
`token_transport`）、`close_codes`、`ws_heartbeat`、`reconnect`（含 `strategy` 与
`hole_rule`）、`offline_backlog` 的语义键与正文取值一致；禁用词、经文正文、槽位常量、经文
长度、生产域名与 AppID 的扫描；三份被点名的指针文档不再把契约写成将来时（含「尚未/待/后续/
将来 + 冻结/定稿/收敛」一类同义表述）；spine 的正式输入枚举、字段清单、AD-5 分层措辞、
`automatic_tap` 禁止句与三条 Deferred 收敛标注。

**不参与正文↔注册表逐值比对的面**（交付前须人工核对，不得据门禁 PASS 推断已一致）：
§1.2 版本化与演进、§4 事件来源与累计语义的散列行、§6 活动窗口中除 §6.1/§6.2 两份必填清单行
外的说明文字、§7 轮次裁决正文、§8 恢复路径表的「语义」列、§10.2 Endpoint 派生规则的说明
文字，以及注册表顶层 `authority` 与 `ws_heartbeat.note` 两个自由叙述键。

**无法机械判定**：仓库内其它文档（未点名的 `docs/embedded/`、`Embedded/AGENTS.md`、UX 规范）
是否复制了字段表、事件来源枚举或 WebSocket 参数（门禁只读被点名的指针文档与 spine，不能证明
全仓无第二份副本）；`automatic_tap` 不得新增独立 cloud 字段或接口这一禁止项只能靠「注册表字段
名模式」与「契约正文必须保留该禁止语句」间接约束，无法证明实现层不存在额外接口；将来时扫描
只能覆盖已登记的表述族，不能证明指针文档的每一句话都不是将来时。门禁是构建与评审前的
fail-closed 检查，不是防篡改签名。
"""

import argparse
import json
import re
import shutil
import sys
import tempfile
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TESTS_DIR.parents[2]

CONTRACT_REL = "docs/contracts/sync-contract.md"
SCHEMA_REL = "docs/contracts/sync-contract.schema.json"
CONTRACTS_README_REL = "docs/contracts/README.md"
DOCS_README_REL = "docs/README.md"
BACKEND_DOC_REL = "docs/backend/经验-后端REST信封与微信登录.md"
MANIFEST_REL = "docs/contracts/canonical/heart-sutra.manifest.json"
CANONICAL_SOURCE_REL = "docs/contracts/canonical/heart-sutra.txt"
PRD_REL = "_bmad-output/planning-artifacts/prds/prd-Electronic_Wooden_Fish-2026-09-07/prd.md"
SPINE_REL = (
    "_bmad-output/planning-artifacts/architecture/"
    "architecture-Electronic_Wooden_Fish-2026-09-08/ARCHITECTURE-SPINE.md"
)

CONTRACT_VERSION_RE = re.compile(r"SC-\d+\.\d+\.\d+")
BACKTICK_IDENT_RE = re.compile(r"`([A-Za-z_][A-Za-z0-9_]*)`")
WIRE_NAME_RE = re.compile(r"[a-z][a-z0-9_]*")
OFFLINE_BACKLOG_LIMIT_RE = re.compile(r"达到\s*上限\s*(\d+)")

REQUIRED_TOP_KEYS = (
    "contract_version",
    "fields",
    "event_sources",
    "sync_states",
    "command_states",
    "time_states",
    "ws_frames",
    "frame_fields",
    "command_payload_fields",
    "ws_heartbeat",
    "reconnect",
    "offline_backlog",
    "close_codes",
    "business_codes",
    "persistence_files",
    "persistence_scopes",
    "samples",
)

# AC 1 点名要求契约定义 `action_id` 与 `applied_revision`，而 PRD §7 的「统一字段」句只列
# 15 个。这两个名字是需求固定项，不是字段语义的第二份真源（类型/单位/取值域仍只在契约中定义）。
REQUIRED_EXTRA_FIELDS = ("action_id", "applied_revision")

# AD-3 / Task 1.5：同步响应必须回传的确认字段。
CONFIRMATION_FIELDS = ("acked_total", "round_state", "round_cursor", "command_revision")

# 契约正文必须保留的禁止语句（`automatic_tap` 不新增独立 cloud 累计字段或同步接口）。
NO_INDEPENDENT_AUTOMATIC_TAP_CLAUSE = "不新增独立 cloud 累计字段或同步接口"
RESUME_FROM_RULE = "snapshot_seq + 1"
DISCARD_RULE = "seq <= last_applied_seq"
# PRD §7 正文使用 `last_applied_seq` 但未定义其所有者与落点；契约必须把它定义成 §2 字段，
# 否则去重基准（是快照水位还是另一个量、归哪一端、是否落盘）无从判定。
DISCARD_BASELINE_FIELD = "last_applied_seq"

# 冻结参数测试上界（Dev Notes「心跳与重连参数」）：默认值必须落在上界之内。
HEARTBEAT_INTERVAL_MAX_MS = 45000
HEARTBEAT_TIMEOUT_MAX_MS = 15000
RECONNECT_BACKOFF_MAX_MS = 30000
PARAM_PATHS = (
    "ws_heartbeat.interval_ms",
    "ws_heartbeat.timeout_ms",
    "reconnect.initial_backoff_ms",
    "reconnect.max_backoff_ms",
    "reconnect.max_attempts",
)

# AD-16 / Task 1.10：零数据库。
DATABASE_KEYWORDS = ("sqlite", "postgres", "mysql", "mariadb", "jdbc", "hikari", "jpa")

# Dev Notes「错误语义与拒绝条件」点名的同步域拒绝条件。
REQUIRED_REJECTION_CONDITIONS = (
    "轮次无法归属",
    "缺少基准高水位",
    "设备重置冲突",
    "队列已满",
    "经文版本不一致",
    "命令旧修订",
    "令牌失效",
    "协议字段非法",
)

# 契约与注册表不得出现的上限常量：视口槽位与经文长度都不许写成字段上界（AC 4）。
FORBIDDEN_BOUND_NUMBERS = (7, 13, 17, 260, 303)
BOUND_PHRASE_PATTERNS = (
    re.compile(r"(上限|上界|长度上限|至多|最多)\D{0,6}(7|13|17|260|303)(?!\d)"),
    re.compile(r"(7|13|17|260|303)\D{0,6}(上限|上界|长度上限)(?!\d)"),
)

CHANNEL_TOKENS = {
    "HTTPS 上报": "https-report",
    "HTTPS 响应": "https-response",
    "WS 帧": "ws-frame",
    "持久化文件": "persistence-file",
}

# 契约正文承载通道「持久化文件」的注册表取值（§11.1 落点闭合用）。
PERSISTENCE_CHANNEL_TOKEN = "persistence-file"
# 帧载荷字段必须在 §2 声明的通道。
FRAME_CHANNEL_TOKEN = "ws-frame"
# §6.1 / §6.2 的两份必填清单：承载通道的「HTTPS 上报」「HTTPS 响应」两个方向由它们闭合。
REPORT_CHANNEL_TOKEN = "https-report"
RESPONSE_CHANNEL_TOKEN = "https-response"
REPORT_SECTION_HEADING = "### 6.1 上报请求字段"
RESPONSE_SECTION_HEADING = "### 6.2 响应字段与内容"
REPORT_REQUIRED_MARKER = "必填："
RESPONSE_REQUIRED_MARKER = "响应必填："

# §8/§12：离线积压达上限后的排空路径。上游真源（PRD §7 §FR-E-004、epics AD-3）只规定
# 「达到上限统一拒绝并提示」，未定义排空；若 20004 同时阻断确认，差值将只增不减形成设备
# 永久无法同步的吸收态。契约按 AD-3「拒绝新输入」的字面取最小闭合解：拒绝的是**输入**，
# 不是确认，故差值随确认回落。这三个 token 是该裁决的正文冻结措辞。
OFFLINE_BACKLOG_DRAIN_TOKENS = ("不阻断确认", "继续上报", "恢复接受输入")

# 注册表语义键取值 ↔ 契约正文必须出现的字面表达。两个方向都要 fail-closed：注册表取值
# 偏离登记值即失败；正文丢失对应表达也失败（否则改反注册表时正文无从校对）。
SEMANTIC_VALUE_BINDINGS = {
    "endpoint.token_transport": {"query": "只经 query 参数"},
    "reconnect.strategy": {"exponential_full_jitter": "指数退避加 full jitter"},
    "reconnect.hole_rule": {"序号出现空洞先查询补齐再续播": "先查询补齐"},
}

# §10.2 Endpoint 派生：origin 的协议映射必须与正文的「`<source>` 映射为 `<target>`」逐项一致。
ENDPOINT_SCHEME_MAP = {"https": "wss", "http": "ws"}

# 指针文档不得把已冻结契约写成将来时：字面短语与同义表述族都必须失败。
FUTURE_TENSE_PHRASES = ("尚未冻结", "后续同步契约阶段", "再定稿", "待冻结")
FUTURE_TENSE_PATTERNS = (
    re.compile(r"(尚未|还没|未)[^。；\n]{0,10}(冻结|定稿|收敛)"),
    re.compile(r"待[^。；\n]{0,12}(冻结|定稿|收敛)"),
    re.compile(r"(后续|将来|以后|日后|届时)[^。；\n]{0,12}(冻结|定稿|收敛)"),
    re.compile(r"(冻结|定稿|收敛)[^。；\n]{0,10}(时再|后再|之后再)(定|做|执行|补)"),
)

FIELD_COLUMNS = ("字段", "类型", "单位", "所有者", "权威", "单调性与取值域", "承载通道", "出处")
FRAME_FIELD_COLUMNS = ("字段", "类型", "单位", "角色", "取值域", "承载通道")
COMMAND_PAYLOAD_COLUMNS = ("字段", "类型", "单位", "取值域", "出处")
SCOPE_COLUMNS = ("作用域", "载体", "承载字段")
STATE_COLUMNS = ("wire 名", "冻结中文显示词", "含义")
FRAME_COLUMNS = ("帧", "方向", "内容", "出处")
PARAM_COLUMNS = ("参数", "默认值", "测试上界", "单位")
PERSISTENCE_COLUMNS = ("文件", "承载事实", "schema_version", "字段", "迁移说明")
BUSINESS_CODE_COLUMNS = ("拒绝条件", "HTTP 状态", "业务码")

# §10.1 帧方向列的正文写法 → 注册表取值。
DIRECTION_TOKENS = {
    "backend → frontend": "backend_to_frontend",
    "frontend → backend": "frontend_to_backend",
}

TABLE_HEADINGS = {
    "fields": "## 2. 统一字段表",
    "frame_fields": "## 3. WebSocket 帧自有字段",
    "sync_states": "### 5.1 累计同步状态",
    "command_states": "### 5.2 命令维度",
    "time_states": "### 5.3 可信时间门禁",
    "command_payload": "### 9.1 待应用命令载荷字段",
    "ws_frames": "### 10.1 帧类型",
    "params": "### 10.3 心跳与重连参数",
    "persistence": "## 11. JSON 持久化文件粒度",
    "persistence_scopes": "### 11.1 持久化作用域与字段落点",
    "business_codes": "## 12. 错误语义与拒绝条件",
}

# §10.3 各参数的测试上界在注册表中的落点（契约正文「测试上界」列 ↔ 注册表 `test_bounds`）。
BOUND_BINDINGS = {
    "ws_heartbeat.interval_ms": ("ws_heartbeat", "interval_ms_max"),
    "ws_heartbeat.timeout_ms": ("ws_heartbeat", "timeout_ms_max"),
    "reconnect.max_backoff_ms": ("reconnect", "max_backoff_ms_max"),
}

# 单事务组泄漏检查的例外：PRD §7 的「相关 `action_id`」是同一字段名的两个落点族——篇章动作族
# 随轮次/水位落单事务文件，设置命令族随命令修订落命令文件（契约 §9 / §11.1）。`action_id` 在
# 单事务文件中的存在性由「单事务文件必须补齐 PRD §7 末条全部字段」断言保证，故此处不重复禁止；
# 其余轮次与水位字段仍严格禁止跨文件重复承载。
SINGLE_TRANSACTION_LEAK_EXEMPT = ("action_id",)

# 承载 `command_revision` 的持久化文件必须同时承载设置命令族的去重键（契约 §9 要求可重放）。
COMMAND_DEDUP_FIELDS = ("action_id",)

# 契约正文必须写明的 `error` 对象作用域（其余帧不得携带）。
ERROR_ONLY_FRAME_CLAUSE = "在 `error` 帧出现"

# 契约正文必须写明的离线积压判定输入（PRD §4 术语「离线积压」）。
OFFLINE_BACKLOG_FIELDS = frozenset(("local_total", "acked_total"))

# 注册表语义键 ↔ 契约正文取值 token 的一致性断言（每项为 注册表路径、正文 token、
# 是否要求注册表值以字面量出现在该 token 中）。`reconnect.strategy` 与 `reconnect.hole_rule`
# 的取值绑定另见 SEMANTIC_VALUE_BINDINGS（需双向失败，非仅查正文 token 存在）。
PARAMETER_BINDING_TOKENS = (
    ("endpoint.rest_base_env", "`VITE_API_BASE_URL`", True),
    ("endpoint.rest_base_contains", "`/api/v1`", True),
    ("endpoint.path_suffix", "`/ws`", True),
    ("endpoint.token_query_field", "字段名固定为 `token`", True),
    ("ws_heartbeat.initiator", "心跳由 backend 发起", True),
    ("ws_heartbeat.ack_frame", "`heartbeat_ack`", True),
    ("offline_backlog.device_action", "拒绝新输入并提示", True),
)

# spine 必须保留的语义修订（AC 6）：AD-5 分层措辞、AD-1 的 automatic_tap 禁止句、
# 三条 Deferred 行的收敛标注，以及 A-2 对「跨文件非原子写顺序」的未收敛声明。
SPINE_AD5_REQUIRED = ("已下发命令的最新修订", "applied_revision", "待设备应用")
SPINE_AD5_FORBIDDEN = "「设备已应用命令的单调高水位」"
SPINE_CONVERGENCE_MARKER = "（2026-09-22，`SC-1.0.0`）"
SPINE_A2_ORDER_NOTE = "非原子写顺序不在"

# 探针以片段拼接构造：门禁自身同样受「契约、注册表与门禁不得出现 AI 痕迹词」约束，
# 因此扫描目标不以字面形式写在门禁里。
AI_MARKER_TOKENS = ("TO" + "DO", "dra" + "ft", "place" + "holder", "data-" + "pencil-id")
NODE_ID_TOKENS = ("qlj" + "P7", "W" + "fAs7", "Z6" + "Qge", "e8" + "Sgp", "g" + "GgAm", "Nt" + "M6r")

# 生产域名与 AppID 片段（部署值，不得进入契约）。
DEPLOYMENT_MARKERS = ("zhcmqtt", "woodenfish-api", "replace-with-ewf")


# 负例组数下界：负例套件本身没有任何断言约束，删组/改名会让门禁以「N/N 通过」掩盖守卫缩减。
MIN_NEGATIVE_TESTS = 31


class ContractError(Exception):
    """契约文件缺失、格式不可解析时的可诊断错误。"""


# --------------------------------------------------------------------------- 解析工具


def norm(text):
    """比对前剥离反引号与首尾空白，使正文与注册表可用 Markdown 装饰而不产生假差异。"""
    return str(text).replace("`", "").strip()


def split_row(line):
    cells = re.split(r"(?<!\\)\|", line.strip())
    if cells and not cells[0].strip():
        cells = cells[1:]
    if cells and not cells[-1].strip():
        cells = cells[:-1]
    return [cell.strip().replace("\\|", "|") for cell in cells]


def table_rows(md, key):
    heading = TABLE_HEADINGS[key]
    lines = md.splitlines()
    start = None
    for index, line in enumerate(lines):
        if line.strip().startswith(heading):
            start = index
            break
    if start is None:
        raise ContractError(f"契约正文缺少小节：{heading}")
    index = start
    while index < len(lines) and not lines[index].strip().startswith("|"):
        index += 1
    if index >= len(lines):
        raise ContractError(f"契约正文 {heading} 小节缺少 Markdown 表格")
    header = split_row(lines[index])
    index += 1
    if index < len(lines) and set(lines[index].strip()) <= set("|-: "):
        index += 1
    rows = []
    while index < len(lines) and lines[index].strip().startswith("|"):
        rows.append(split_row(lines[index]))
        index += 1
    return header, rows


def expect_header(header, columns, key):
    actual = [norm(cell) for cell in header]
    if actual != list(columns):
        raise ContractError(
            f"契约正文 {TABLE_HEADINGS[key]} 表头必须为 {'|'.join(columns)}，实际 {'|'.join(actual)}"
        )


def channels_from_cell(cell):
    tokens = []
    for part in re.split(r"[、,]", norm(cell)):
        part = part.strip()
        if not part:
            continue
        if part not in CHANNEL_TOKENS:
            raise ContractError(f"契约正文出现未知承载通道：{part}")
        tokens.append(CHANNEL_TOKENS[part])
    return tokens


def load_contract(root):
    md_path = root / CONTRACT_REL
    schema_path = root / SCHEMA_REL
    if not md_path.exists():
        raise ContractError(f"契约正文不存在：{CONTRACT_REL}")
    if not schema_path.exists():
        raise ContractError(f"契约注册表不存在：{SCHEMA_REL}")
    md = md_path.read_text(encoding="utf-8")
    try:
        data = json.loads(schema_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise ContractError(f"注册表不是合法 JSON：{SCHEMA_REL}：{error}")
    if not isinstance(data, dict):
        raise ContractError(f"注册表顶层必须是 JSON 对象：{SCHEMA_REL}")
    return md, data


def read_text(root, rel):
    path = root / rel
    if not path.exists():
        raise ContractError(f"缺少上游文件：{rel}")
    return path.read_text(encoding="utf-8")


def registry_names(data):
    names = [entry.get("name") for entry in data.get("fields", [])]
    return [name for name in names if isinstance(name, str)]


def frame_owned_names(data):
    names = [entry.get("name") for entry in data.get("frame_fields", [])]
    return [name for name in names if isinstance(name, str)]


def command_payload_names(data):
    names = [entry.get("name") for entry in data.get("command_payload_fields", [])]
    return [name for name in names if isinstance(name, str)]


def declared_names(data):
    return (
        set(registry_names(data))
        | set(frame_owned_names(data))
        | set(command_payload_names(data))
    )


def field_channels(data):
    """统一字段名 → 承载通道（注册表取值），供帧载荷与持久化落点闭合检查使用。"""
    channels = {}
    for entry in data.get("fields", []):
        name = entry.get("name")
        if isinstance(name, str):
            channels[name] = list(entry.get("channel") or [])
    return channels


def scripture_probe(root):
    """从 canonical 源文本取一段正文作为扫描探针，避免门禁自身复制经文。

    探针不可得（源缺失、不可读或过短）属上游缺失，必须报为失败：静默返回空值会让
    经文正文扫描悄悄跳过，门禁承诺的 fail-closed 就不再成立。
    """
    path = root / CANONICAL_SOURCE_REL
    if not path.exists():
        raise ContractError(
            f"缺少 canonical 源文本，经文正文扫描无法执行：{CANONICAL_SOURCE_REL}"
        )
    try:
        text = "".join(path.read_text(encoding="utf-8").split())
    except (OSError, UnicodeDecodeError) as error:
        raise ContractError(f"canonical 源文本不可读：{CANONICAL_SOURCE_REL}：{error}")
    if len(text) < 16:
        raise ContractError(
            f"canonical 源文本过短，无法构造经文探针：{CANONICAL_SOURCE_REL}"
        )
    return text[:16]


# --------------------------------------------------------------------------- 上游提取


def prd_section(root):
    text = read_text(root, PRD_REL)
    match = re.search(r"^## 7\. 跨层同步合同$(.*?)^## ", text, re.M | re.S)
    if not match:
        raise ContractError(f"未在 {PRD_REL} 找到 §7「跨层同步合同」章节")
    return match.group(1)


def prd_sentence(root, marker):
    section = prd_section(root)
    for line in section.splitlines():
        if marker in line:
            rest = line.split(marker, 1)[1]
            if "。" not in rest:
                raise ContractError(
                    f"{PRD_REL} §7 的「{marker}」句在标记后不含句号，期望清单可能已被折行"
                    "截断，覆盖断言无法执行"
                )
            return rest.split("。", 1)[0]
    raise ContractError(f"未在 {PRD_REL} §7 找到「{marker}」句")


def prd_expected_fields(root):
    names = BACKTICK_IDENT_RE.findall(prd_sentence(root, "统一字段："))
    if not names:
        raise ContractError(
            f"{PRD_REL} §7 的「统一字段：」句未提取到任何反引号标识符，PRD 期望字段集合为空，"
            "覆盖断言无法执行"
        )
    return names


def prd_expected_events(root):
    names = BACKTICK_IDENT_RE.findall(prd_sentence(root, "统一事件来源："))
    if not names:
        raise ContractError(
            f"{PRD_REL} §7 的「统一事件来源：」句未提取到任何反引号标识符，事件来源断言无法执行"
        )
    return names


def prd_single_transaction_fields(root):
    for line in prd_section(root).splitlines():
        if "同一持久化事务" in line:
            names = BACKTICK_IDENT_RE.findall(line)
            if not names:
                raise ContractError(
                    f"{PRD_REL} §7 的「同一持久化事务」条目未提取到任何反引号标识符，"
                    "单事务分组断言无法执行"
                )
            return names
    raise ContractError(f"未在 {PRD_REL} §7 找到「同一持久化事务」条目")


def spine_section(root):
    text = read_text(root, SPINE_REL)
    match = re.search(r"^### 同步字段清单(.*?)^### ", text, re.M | re.S)
    if not match:
        raise ContractError(f"未在 {SPINE_REL} 找到「同步字段清单」小节")
    return match.group(1)


def spine_field_line(root):
    for line in spine_section(root).splitlines():
        if line.strip().startswith("`"):
            return line.strip()
    raise ContractError(f"{SPINE_REL} 的「同步字段清单」未给出字段清单行")


def spine_expected_fields(root):
    line = spine_field_line(root)
    head = line.split("事件来源：", 1)[0]
    return BACKTICK_IDENT_RE.findall(head)


def spine_expected_events(root):
    line = spine_field_line(root)
    if "事件来源：" not in line:
        raise ContractError(f"{SPINE_REL} 的字段清单行缺少「事件来源：」")
    tail = line.split("事件来源：", 1)[1]
    return BACKTICK_IDENT_RE.findall(tail.split("。", 1)[0])


def spine_conventions_events(root):
    for line in read_text(root, SPINE_REL).splitlines():
        if line.startswith("| 命名") and "事件来源固定" in line:
            tail = line.split("事件来源固定", 1)[1]
            return BACKTICK_IDENT_RE.findall(tail.split("；", 1)[0])
    raise ContractError(f"{SPINE_REL} §Consistency Conventions 未声明事件来源枚举")


def spine_state_words(root):
    for line in read_text(root, SPINE_REL).splitlines():
        if line.startswith("| 状态与跨切") and "同步状态词表固定：" in line:
            tail = line.split("同步状态词表固定：", 1)[1]
            sync_part, rest = tail.split("（累计）", 1)
            command_part = rest.split("；", 1)[1].split("（命令", 1)[0]
            return (
                [word.strip() for word in sync_part.split("/") if word.strip()],
                [command_part.strip()],
            )
    raise ContractError(f"{SPINE_REL} §Consistency Conventions 未声明同步状态词表")


def spine_ad_section(root, ident):
    """取 spine 中某个不变量（如 AD-5）的整段文本，避免全文件 token 命中掩盖局部回退。"""
    text = read_text(root, SPINE_REL)
    match = re.search(
        rf"^#### {re.escape(ident)} —(.*?)^#### ", text, re.M | re.S
    )
    if not match:
        raise ContractError(f"未在 {SPINE_REL} 找到 {ident} 小节")
    return match.group(1)


def spine_deferred_row(root, marker):
    """取 Deferred 表中含指定标记的表格行（不变量正文里的同名标记不计）。"""
    rows = [
        line for line in read_text(root, SPINE_REL).splitlines()
        if line.startswith("|") and marker in line
    ]
    if len(rows) != 1:
        raise ContractError(
            f"{SPINE_REL} 的 Deferred 表必须恰有一行含「{marker}」，实际 {len(rows)} 行"
        )
    return rows[0]


# --------------------------------------------------------------------------- 正向规则


def rule_file_hygiene(root, md, data):
    failures = []
    for rel in (CONTRACT_REL, SCHEMA_REL):
        raw = (root / rel).read_bytes()
        if raw.startswith(b"\xef\xbb\xbf"):
            failures.append(f"{rel} 不得带 UTF-8 BOM")
        if raw.startswith((b"\xff\xfe", b"\xfe\xff")):
            failures.append(f"{rel} 不得为 UTF-16 编码")
        try:
            raw.decode("utf-8")
        except UnicodeDecodeError as error:
            failures.append(f"{rel} 不是合法 UTF-8：{error}")
            continue
        if b"\r\n" in raw:
            failures.append(f"{rel} 必须使用 LF 换行")
        if not raw.endswith(b"\n") or raw.endswith(b"\n\n"):
            failures.append(f"{rel} 必须以单个换行收尾")
    return failures


def rule_registry_shape(root, md, data):
    failures = []
    for key in REQUIRED_TOP_KEYS:
        if key not in data:
            failures.append(f"注册表 {SCHEMA_REL} 缺少顶层键：{key}")
    for key in ("fields", "event_sources", "sync_states", "command_states", "time_states",
                "ws_frames", "frame_fields", "command_payload_fields", "persistence_files",
                "persistence_scopes", "samples", "business_codes"):
        if key in data and not isinstance(data[key], list):
            failures.append(f"注册表顶层键 {key} 必须是数组")
    if "offline_backlog" in data and not isinstance(data["offline_backlog"], dict):
        failures.append("注册表顶层键 offline_backlog 必须是对象")
    return failures


def rule_contract_version(root, md, data):
    failures = []
    registry_version = data.get("contract_version")
    if not isinstance(registry_version, str) or not CONTRACT_VERSION_RE.fullmatch(registry_version):
        failures.append(
            f"注册表 contract_version 必须是 SC-<major>.<minor>.<patch> 形式，实际 {registry_version!r}"
        )
        registry_version = None
    md_versions = sorted(set(CONTRACT_VERSION_RE.findall(md)))
    if not md_versions:
        failures.append("契约正文未登记 contract_version（格式 SC-<major>.<minor>.<patch>）")
    elif len(md_versions) > 1:
        failures.append(f"契约正文出现多个 contract_version：{'、'.join(md_versions)}")
    elif registry_version is not None and md_versions[0] != registry_version:
        failures.append(
            f"contract_version 双真源：契约正文为 {md_versions[0]}，注册表为 {registry_version}"
        )
    return failures


def md_field_entries(md):
    header, rows = table_rows(md, "fields")
    expect_header(header, FIELD_COLUMNS, "fields")
    entries = {}
    for row in rows:
        if len(row) != len(FIELD_COLUMNS):
            raise ContractError(f"契约正文 §2 字段行列数不符：{'|'.join(row)}")
        name = norm(row[0])
        if not name:
            raise ContractError("契约正文 §2 字段表存在空字段名")
        if name in entries:
            raise ContractError(f"契约正文 §2 字段表重复定义字段：{name}")
        entries[name] = {
            "type": norm(row[1]),
            "unit": norm(row[2]),
            "owner": norm(row[3]),
            "authority": norm(row[4]),
            "domain": norm(row[5]),
            "channel": channels_from_cell(row[6]),
            "source": norm(row[7]),
        }
    return entries


def md_frame_field_entries(md):
    header, rows = table_rows(md, "frame_fields")
    expect_header(header, FRAME_FIELD_COLUMNS, "frame_fields")
    entries = {}
    for row in rows:
        if len(row) != len(FRAME_FIELD_COLUMNS):
            raise ContractError(f"契约正文 §3 帧自有字段行列数不符：{'|'.join(row)}")
        entries[norm(row[0])] = {
            "type": norm(row[1]),
            "unit": norm(row[2]),
            "role": norm(row[3]),
            "domain": norm(row[4]),
            "channel": channels_from_cell(row[5]),
        }
    return entries


def md_command_payload_entries(md):
    header, rows = table_rows(md, "command_payload")
    expect_header(header, COMMAND_PAYLOAD_COLUMNS, "command_payload")
    entries = {}
    for row in rows:
        if len(row) != len(COMMAND_PAYLOAD_COLUMNS):
            raise ContractError(
                f"契约正文 §9.1 命令载荷字段行列数不符：{'|'.join(row)}"
            )
        name = norm(row[0])
        if not name:
            raise ContractError("契约正文 §9.1 命令载荷字段表存在空字段名")
        if name in entries:
            raise ContractError(f"契约正文 §9.1 命令载荷字段表重复定义字段：{name}")
        entries[name] = {
            "type": norm(row[1]),
            "unit": norm(row[2]),
            "domain": norm(row[3]),
            "source": norm(row[4]),
        }
    return entries


def md_persistence_entries(md):
    header, rows = table_rows(md, "persistence")
    expect_header(header, PERSISTENCE_COLUMNS, "persistence")
    entries = {}
    for row in rows:
        if len(row) != len(PERSISTENCE_COLUMNS):
            raise ContractError(f"契约正文 §11 持久化行列数不符：{'|'.join(row)}")
        name = norm(row[0])
        fields_text = norm(row[3])
        entries[name] = {
            "facts": norm(row[1]),
            "schema_version": norm(row[2]),
            "fields": [] if fields_text in ("", "无") else sorted(BACKTICK_IDENT_RE.findall(row[3])),
            "migration": norm(row[4]),
        }
    return entries


def md_scope_entries(md):
    header, rows = table_rows(md, "persistence_scopes")
    expect_header(header, SCOPE_COLUMNS, "persistence_scopes")
    entries = {}
    for row in rows:
        if len(row) != len(SCOPE_COLUMNS):
            raise ContractError(f"契约正文 §11.1 作用域行列数不符：{'|'.join(row)}")
        scope = norm(row[0])
        entries[scope] = {
            "carrier": norm(row[1]),
            "fields": sorted(BACKTICK_IDENT_RE.findall(row[2])),
        }
    return entries


def md_frame_entries(md):
    header, rows = table_rows(md, "ws_frames")
    expect_header(header, FRAME_COLUMNS, "ws_frames")
    entries = {}
    for row in rows:
        if len(row) != len(FRAME_COLUMNS):
            raise ContractError(f"契约正文 §10.1 帧类型行列数不符：{'|'.join(row)}")
        name = norm(row[0])
        direction = norm(row[1])
        if direction not in DIRECTION_TOKENS:
            raise ContractError(f"契约正文 §10.1 出现未知帧方向：{direction}")
        entries[name] = {
            "direction": DIRECTION_TOKENS[direction],
            "content": norm(row[2]),
            "source": norm(row[3]),
        }
    return entries


def rule_field_table(root, md, data):
    failures = []
    md_fields = md_field_entries(md)
    registry_entries = data["fields"]
    registry_fields = {}
    for entry in registry_entries:
        name = entry.get("name")
        if not isinstance(name, str) or not name:
            failures.append("注册表 fields 存在缺少 name 的条目")
            continue
        if name in registry_fields:
            failures.append(f"注册表 fields 重复定义字段：{name}")
        registry_fields[name] = entry

    missing_in_md = sorted(set(registry_fields) - set(md_fields))
    if missing_in_md:
        failures.append(
            f"契约正文 §2 字段表缺少注册表已声明字段：{'、'.join(missing_in_md)}"
        )
    missing_in_registry = sorted(set(md_fields) - set(registry_fields))
    if missing_in_registry:
        failures.append(
            f"注册表 fields 缺少契约正文 §2 已声明字段：{'、'.join(missing_in_registry)}"
        )

    for name in sorted(set(md_fields) & set(registry_fields)):
        left = md_fields[name]
        right = registry_fields[name]
        for key, label in (
            ("type", "类型"),
            ("unit", "单位"),
            ("owner", "所有者"),
            ("authority", "权威"),
            ("domain", "单调性与取值域"),
            ("source", "出处"),
        ):
            left_value = left[key]
            right_value = norm(right.get(key, ""))
            if left_value != right_value:
                failures.append(
                    f"统一字段 {name} 的{label}双真源：契约正文为「{left_value}」，"
                    f"注册表为「{right_value}」"
                )
        right_channel = sorted(right.get("channel") or [])
        if sorted(left["channel"]) != right_channel:
            failures.append(
                f"统一字段 {name} 的承载通道双真源：契约正文为 {sorted(left['channel'])}，"
                f"注册表为 {right_channel}"
            )
        if not left["unit"]:
            failures.append(f"统一字段 {name} 必须显式给出单位（无单位时写「无」）")
        if not left["owner"] or not left["authority"]:
            failures.append(f"统一字段 {name} 必须给出所有者与权威")
        if not left["domain"]:
            failures.append(f"统一字段 {name} 必须给出取值域或单调性")
        if not left["channel"]:
            failures.append(f"统一字段 {name} 必须至少列出一种承载通道")
    return failures


def rule_field_coverage(root, md, data):
    failures = []
    registry_set = set(registry_names(data))
    expected = set(prd_expected_fields(root))
    absent = sorted(expected - registry_set)
    if absent:
        failures.append(
            f"注册表 fields 未覆盖 {PRD_REL} §7 的统一字段：{'、'.join(absent)}"
        )
    extra_absent = sorted(set(REQUIRED_EXTRA_FIELDS) - registry_set)
    if extra_absent:
        failures.append(f"注册表 fields 缺少 AC 1 点名字段：{'、'.join(extra_absent)}")

    spine_fields = set(spine_expected_fields(root))
    spine_absent = sorted(spine_fields - registry_set)
    if spine_absent:
        failures.append(
            f"注册表 fields 未覆盖 {SPINE_REL} 同步字段清单：{'、'.join(spine_absent)}"
        )
    return failures


def rule_event_sources(root, md, data):
    failures = []
    registry_events = []
    for entry in data["event_sources"]:
        if not isinstance(entry, dict) or not isinstance(entry.get("name"), str):
            failures.append("注册表 event_sources 存在缺少 name 的条目")
            continue
        registry_events.append(entry["name"])
        if norm(entry.get("cloud_specific_field", "")) != "无":
            failures.append(
                f"事件来源 {entry['name']} 不得声明独立 cloud 字段"
                f"（PRD §7：automatic_tap 不新增独立 cloud 计数或同步字段）"
            )

    prd_events = prd_expected_events(root)
    if sorted(registry_events) != sorted(prd_events):
        failures.append(
            f"event_sources 枚举未收敛：契约注册表为 {'、'.join(sorted(registry_events))}，"
            f"{PRD_REL} §7 为 {'、'.join(sorted(prd_events))}"
        )
    spine_events = spine_expected_events(root)
    if sorted(registry_events) != sorted(spine_events):
        failures.append(
            f"event_sources 与 {SPINE_REL} 同步字段清单枚举不一致："
            f"注册表 {'、'.join(sorted(registry_events))}，spine {'、'.join(sorted(spine_events))}"
        )
    conventions_events = spine_conventions_events(root)
    if sorted(registry_events) != sorted(conventions_events):
        failures.append(
            f"event_sources 与 {SPINE_REL} §Consistency Conventions 命名行不一致："
            f"注册表 {'、'.join(sorted(registry_events))}，spine {'、'.join(sorted(conventions_events))}"
        )

    md_events = set()
    for line in md.splitlines():
        if "事件来源枚举" in line:
            md_events.update(BACKTICK_IDENT_RE.findall(line))
    if md_events != set(registry_events):
        failures.append(
            f"event_sources 正文与注册表枚举不一致：正文 {'、'.join(sorted(md_events))}，"
            f"注册表 {'、'.join(sorted(registry_events))}"
        )
    return failures


def md_state_entries(md, key):
    header, rows = table_rows(md, key)
    expect_header(header, STATE_COLUMNS, key)
    entries = []
    for row in rows:
        if len(row) != len(STATE_COLUMNS):
            raise ContractError(f"契约正文 {TABLE_HEADINGS[key]} 行列数不符：{'|'.join(row)}")
        entries.append({"wire": norm(row[0]), "display": norm(row[1]), "meaning": norm(row[2])})
    return entries


def rule_states(root, md, data):
    failures = []
    sync_displays, command_displays = spine_state_words(root)

    md_sync = md_state_entries(md, "sync_states")
    md_command = md_state_entries(md, "command_states")
    md_time = md_state_entries(md, "time_states")

    registry_sync = data["sync_states"]
    registry_command = data["command_states"]
    registry_time = data["time_states"]

    if [entry.get("display") for entry in registry_sync] != sync_displays:
        failures.append(
            f"sync_states 与 {SPINE_REL} 冻结状态词表不一致：注册表为 "
            f"{'、'.join(str(entry.get('display')) for entry in registry_sync)}，"
            f"spine 为 {'、'.join(sync_displays)}"
        )
    if len(registry_sync) != 5:
        failures.append(
            f"sync_states 必须恰为 5 个累计同步状态，实际 {len(registry_sync)} 个"
        )
    if len(registry_command) != 2:
        failures.append(
            f"command_states 必须恰为 2 个命令维度状态，实际 {len(registry_command)} 个"
        )

    command_display_set = {str(entry.get("display")) for entry in registry_command}
    sync_display_set = {str(entry.get("display")) for entry in registry_sync}
    overlap = sorted(command_display_set & sync_display_set)
    if overlap:
        failures.append(
            f"命令维度必须与累计同步状态互相独立：{'、'.join(overlap)} 同时出现在 "
            "sync_states 与 command_states"
        )
    if command_displays and not set(command_displays) <= command_display_set:
        failures.append(
            f"command_states 缺少 {SPINE_REL} 冻结的命令显示词："
            f"{'、'.join(sorted(set(command_displays) - command_display_set))}"
        )

    if [entry.get("display") for entry in registry_time] != ["待校时"]:
        failures.append("time_states 必须恰为「待校时」一个可信时间门禁状态")
    if "待校时" not in md:
        failures.append("契约正文必须声明「待校时」状态（AD-6）")

    for label, md_entries, registry_entries in (
        ("sync_states", md_sync, registry_sync),
        ("command_states", md_command, registry_command),
        ("time_states", md_time, registry_time),
    ):
        if [(entry["wire"], entry["display"], entry["meaning"]) for entry in md_entries] != [
            (str(entry.get("wire")), str(entry.get("display")), norm(entry.get("meaning", "")))
            for entry in registry_entries
        ]:
            failures.append(
                f"{label} 的 wire 名、显示词与含义在契约正文与注册表之间不一致"
            )

    wires = [entry.get("wire") for entry in registry_sync + registry_command + registry_time]
    for wire in wires:
        if not isinstance(wire, str) or not WIRE_NAME_RE.fullmatch(wire):
            failures.append(f"状态 wire 名必须为全小写下划线形式：{wire!r}")
    if len(set(wires)) != len(wires):
        failures.append("状态 wire 名在三个枚举之间必须唯一")
    return failures


def rule_ws_frames(root, md, data):
    failures = []
    md_entries = md_frame_entries(md)

    registry_frames = data["ws_frames"]
    registry_types = [str(frame.get("type")) for frame in registry_frames]
    if sorted(md_entries) != sorted(registry_types):
        failures.append(
            f"WS 帧类型集合在契约正文与注册表之间不一致：正文 "
            f"{'、'.join(sorted(md_entries))}，注册表 {'、'.join(sorted(registry_types))}"
        )

    registry_by_type = {str(frame.get("type")): frame for frame in registry_frames}
    for frame_type in sorted(set(md_entries) & set(registry_by_type)):
        left = md_entries[frame_type]
        right = registry_by_type[frame_type]
        for key, label in (("direction", "方向"), ("content", "内容"), ("source", "出处")):
            if left[key] != norm(right.get(key, "")):
                failures.append(
                    f"WS 帧 {frame_type} 的{label}双真源：契约正文为「{left[key]}」，"
                    f"注册表为「{norm(right.get(key, ''))}」"
                )

    agreed = {}
    for key in ("contract_version_field", "discriminator_field", "seq_field", "error_object_field"):
        values = {str(frame.get(key)) for frame in registry_frames}
        if len(values) != 1 or "" in values or "None" in values:
            failures.append(
                f"每类 WS 帧必须声明同一个 {key}，实际取值集合为 {sorted(values)}"
            )
            continue
        agreed[key] = values.pop()
    if len(agreed) != 4:
        return failures

    # `error` 对象只在 `error` 帧必带（§3 与 §10.1 的唯一说法）。
    error_required = sorted(
        str(frame.get("type")) for frame in registry_frames
        if frame.get("error_object_required") is True
    )
    if error_required != ["error"]:
        failures.append(
            f"error 对象必须且只能由 error 帧必带，实际声明必带的帧为 {error_required}"
        )
    if ERROR_ONLY_FRAME_CLAUSE not in md:
        failures.append(
            f"契约正文必须写明 `error` 对象的作用域：{ERROR_ONLY_FRAME_CLAUSE}"
        )

    md_frame_fields = md_frame_field_entries(md)
    registry_frame_fields = {entry.get("name"): entry for entry in data.get("frame_fields", [])}
    if set(md_frame_fields) != set(registry_frame_fields):
        failures.append(
            "帧自有字段集合在契约正文 §3 与注册表 frame_fields 之间不一致："
            f"正文 {'、'.join(sorted(md_frame_fields))}，"
            f"注册表 {'、'.join(sorted(str(name) for name in registry_frame_fields))}"
        )
    agreed_names = set(agreed.values())
    if agreed_names != set(md_frame_fields):
        failures.append(
            f"WS 帧声明的版本化/判别/序号/错误字段必须正是契约正文 §3 声明的帧自有字段："
            f"帧声明 {'、'.join(sorted(agreed_names))}，正文 {'、'.join(sorted(md_frame_fields))}"
        )
    if not any(entry["role"] == "版本化字段" for entry in md_frame_fields.values()):
        failures.append("契约正文 §3 必须声明角色为「版本化字段」的帧自有字段")
    if not any(entry["role"] == "单调序号" for entry in md_frame_fields.values()):
        failures.append("契约正文 §3 必须声明角色为「单调序号」的帧自有字段")

    for name in sorted(set(md_frame_fields) & set(registry_frame_fields)):
        left = md_frame_fields[name]
        right = registry_frame_fields[name]
        for key, label in (("type", "类型"), ("unit", "单位"), ("role", "角色"), ("domain", "取值域")):
            if left[key] != norm(right.get(key, "")):
                failures.append(
                    f"帧自有字段 {name} 的{label}双真源：契约正文为「{left[key]}」，"
                    f"注册表为「{norm(right.get(key, ''))}」"
                )
        if left["channel"] != sorted(right.get("channel") or []):
            failures.append(f"帧自有字段 {name} 的承载通道双真源")

    channels = field_channels(data)
    payload_universe = set(registry_names(data)) | set(command_payload_names(data))
    for frame in registry_frames:
        payload_fields = list(frame.get("payload_fields") or [])
        unknown = sorted(set(payload_fields) - payload_universe)
        if unknown:
            failures.append(
                f"WS 帧 {frame.get('type')} 的 payload_fields 引用了未声明字段：{'、'.join(unknown)}"
            )
        for name in sorted(set(payload_fields) & set(channels)):
            if FRAME_CHANNEL_TOKEN not in channels[name]:
                failures.append(
                    f"WS 帧 {frame.get('type')} 的载荷字段 {name} 必须在契约 §2 声明「WS 帧」"
                    f"承载通道，实际通道为 {channels[name]}"
                )
        if frame.get("monotonic_seq") is not True:
            failures.append(f"WS 帧 {frame.get('type')} 必须声明单调序号语义")

    if RESUME_FROM_RULE not in md:
        failures.append(f"契约正文必须写明续订起点规则：{RESUME_FROM_RULE}")
    if DISCARD_RULE not in md:
        failures.append(f"契约正文必须写明序号丢弃规则：{DISCARD_RULE}")
    if norm(data["reconnect"].get("resume_from", "")) != RESUME_FROM_RULE:
        failures.append("注册表 reconnect.resume_from 必须为 snapshot_seq + 1")
    if DISCARD_RULE not in norm(data["reconnect"].get("discard_rule", "")):
        failures.append(f"注册表 reconnect.discard_rule 必须登记序号丢弃规则：{DISCARD_RULE}")
    return failures


def md_section_lines(md, heading):
    """取以 heading 开头的小节行（到下一个标题为止）。"""
    lines = md.splitlines()
    start = None
    for index, line in enumerate(lines):
        if line.strip().startswith(heading):
            start = index
            break
    if start is None:
        raise ContractError(f"契约正文缺少小节：{heading}")
    end = len(lines)
    for cursor in range(start + 1, len(lines)):
        if lines[cursor].lstrip().startswith("#"):
            end = cursor
            break
    return lines[start:end]


def md_clause_tokens(md, heading, marker):
    """取小节内含 marker 的段落（含其后缩进续行）的反引号标识符。

    §6.1 与 §6.2 的必填清单在正文中跨行书写，故不能只读 marker 所在的一行；同时限定在
    小节内搜索，避免 §6.2 的「响应必填：」被 §6.1 的「必填：」抢先匹配。
    """
    section = md_section_lines(md, heading)
    for index, line in enumerate(section):
        if marker not in line:
            continue
        chunk = [line]
        cursor = index + 1
        while cursor < len(section):
            candidate = section[cursor]
            if not candidate.strip() or candidate.lstrip().startswith(("|", "-", ">", "#")):
                break
            chunk.append(candidate)
            cursor += 1
        return BACKTICK_IDENT_RE.findall("\n".join(chunk))
    raise ContractError(f"契约正文 {heading} 缺少「{marker}」清单")


def rule_channel_directions(root, md, data):
    """承载通道四个取值必须与本文各清单双向闭合（§2 通道列与 §6.1/§6.2/§11/§11.1 收敛）。

    只断言「通道声明」或只断言「清单列出」都会放过单侧漂移：字段声明了某方向的通道而清单
    不承载它，或清单承载了字段而该字段没声明对应通道，都会被指出。WS 帧方向由
    `rule_ws_frames` 断言。
    """
    failures = []
    channels = field_channels(data)
    registry_field_names = set(registry_names(data))
    payload_names = set(command_payload_names(data))

    def carriers_of(token):
        return {
            name for name in registry_field_names
            if token in (channels.get(name) or [])
        }

    # 方向一：HTTPS 上报（§6.1 必填清单）。§2 声明该通道的字段集合必须与清单相等。
    report_required = set(md_clause_tokens(md, REPORT_SECTION_HEADING, REPORT_REQUIRED_MARKER))
    unknown_report = sorted(report_required - registry_field_names)
    if unknown_report:
        failures.append(f"§6.1 上报必填清单引用了 §2 未声明的字段：{'、'.join(unknown_report)}")
    report_carriers = carriers_of(REPORT_CHANNEL_TOKEN)
    if report_required != report_carriers:
        failures.append(
            "「HTTPS 上报」承载通道与 §6.1 上报必填清单不闭合：仅清单 "
            f"{sorted(report_required - report_carriers)}，仅通道 "
            f"{sorted(report_carriers - report_required)}"
        )

    # 方向二：HTTPS 响应（§6.2 响应必填清单）。
    response_required = set(
        md_clause_tokens(md, RESPONSE_SECTION_HEADING, RESPONSE_REQUIRED_MARKER)
    )
    unknown_response = sorted(response_required - registry_field_names - payload_names)
    if unknown_response:
        failures.append(f"§6.2 响应必填清单引用了未声明字段：{'、'.join(unknown_response)}")
    response_carriers = carriers_of(RESPONSE_CHANNEL_TOKEN)
    missing_response = sorted((response_required & registry_field_names) - response_carriers)
    if missing_response:
        failures.append(
            f"§6.2 响应必填字段未在 §2 声明「HTTPS 响应」承载通道：{'、'.join(missing_response)}"
        )
    missing_payload = sorted(payload_names - response_required)
    if missing_payload:
        failures.append(
            f"§6.2 响应必填清单必须包含全部命令载荷字段：缺少 {'、'.join(missing_payload)}"
        )

    # 方向三：持久化文件（§11 文件表与 §11.1 作用域承载的 §2 字段必须声明该通道）。
    carried = set()
    for entry in data["persistence_files"]:
        carried |= set(entry.get("fields") or [])
    for entry in data["persistence_scopes"]:
        carried |= set(entry.get("fields") or [])
    persistence_carriers = carriers_of(PERSISTENCE_CHANNEL_TOKEN)
    missing_persistence = sorted((carried & registry_field_names) - persistence_carriers)
    if missing_persistence:
        failures.append(
            "§11 文件表或 §11.1 作用域承载的字段未在 §2 声明「持久化文件」承载通道："
            f"{'、'.join(missing_persistence)}"
        )

    # 方向二的反向：§2 声明「HTTPS 响应」的字段必须被 §6.2 响应必填清单承载。只断言
    # 「清单 ⊆ 通道」会放过反向漂移：字段声明了响应通道而清单不承载它，下游据 §2 实现响应、
    # 与据 §6.2 实现响应的两端就会分叉。§2 自述「通道声明的方向必须有对应清单承载」。
    extra_response = sorted(response_carriers - response_required)
    if extra_response:
        failures.append(
            "§2 声明「HTTPS 响应」承载通道但 §6.2 响应必填清单不承载的字段："
            f"{'、'.join(extra_response)}"
        )

    # 方向四的反向：§2 声明「WS 帧」的字段必须被 §10.1 的某一帧载荷承载。
    frame_payloads = set()
    for frame in data["ws_frames"]:
        frame_payloads |= set(frame.get("payload_fields") or [])
    extra_frame = sorted(carriers_of(FRAME_CHANNEL_TOKEN) - frame_payloads)
    if extra_frame:
        failures.append(
            "§2 声明「WS 帧」承载通道但 §10.1 没有任何帧载荷承载的字段："
            f"{'、'.join(extra_frame)}"
        )
    return failures


def resolve_path(data, dotted):
    node = data
    for part in dotted.split("."):
        if not isinstance(node, dict) or part not in node:
            return None
        node = node[part]
    return node


def rule_params(root, md, data):
    failures = []
    header, rows = table_rows(md, "params")
    expect_header(header, PARAM_COLUMNS, "params")
    seen = []
    for row in rows:
        if len(row) != len(PARAM_COLUMNS):
            raise ContractError(f"契约正文 §10.3 参数行列数不符：{'|'.join(row)}")
        path = norm(row[0])
        seen.append(path)
        default_text = norm(row[1])
        bound_text = norm(row[2])
        unit = norm(row[3])
        value = resolve_path(data, path)
        if path.split(".")[-1].endswith("_ms"):
            if unit != "ms":
                failures.append(f"参数 {path} 的单位必须为 ms，实际「{unit}」")
        elif unit != "次":
            failures.append(f"参数 {path} 的单位必须为 次，实际「{unit}」")
        if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
            failures.append(f"参数 {path} 必须是正整数，实际 {value!r}")
            continue
        try:
            default = int(default_text)
        except ValueError:
            failures.append(f"参数 {path} 的默认值必须是整数，实际「{default_text}」")
            continue
        if default != value:
            failures.append(
                f"参数 {path} 双真源：契约正文默认值为 {default}，注册表为 {value}"
            )
        if bound_text != "无":
            try:
                bound = int(bound_text)
            except ValueError:
                failures.append(f"参数 {path} 的测试上界必须是整数或「无」，实际「{bound_text}」")
                continue
            if value > bound:
                failures.append(f"参数 {path} 的默认值 {value} 超出测试上界 {bound}")

        binding = BOUND_BINDINGS.get(path)
        registry_bound = None
        if binding is not None:
            registry_bound = resolve_path(data, f"{binding[0]}.test_bounds.{binding[1]}")
        if bound_text == "无":
            if registry_bound is not None:
                failures.append(
                    f"参数 {path} 的测试上界双真源：契约正文为「无」，"
                    f"注册表 test_bounds 为 {registry_bound}"
                )
        else:
            try:
                bound_value = int(bound_text)
            except ValueError:
                continue
            if registry_bound != bound_value:
                failures.append(
                    f"参数 {path} 的测试上界双真源：契约正文为 {bound_value}，"
                    f"注册表 test_bounds 为 {registry_bound}"
                )

    absent = sorted(set(PARAM_PATHS) - set(seen))
    if absent:
        failures.append(f"契约正文 §10.3 缺少参数：{'、'.join(absent)}")

    heartbeat = data["ws_heartbeat"]
    reconnect = data["reconnect"]
    for path, value, bound in (
        ("ws_heartbeat.interval_ms", heartbeat.get("interval_ms"), HEARTBEAT_INTERVAL_MAX_MS),
        ("ws_heartbeat.timeout_ms", heartbeat.get("timeout_ms"), HEARTBEAT_TIMEOUT_MAX_MS),
        ("reconnect.max_backoff_ms", reconnect.get("max_backoff_ms"), RECONNECT_BACKOFF_MAX_MS),
    ):
        if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
            failures.append(f"参数 {path} 必须是正整数，实际 {value!r}")
        elif value > bound:
            failures.append(f"参数 {path} 的默认值 {value} 超出冻结测试上界 {bound}")
    if not isinstance(reconnect.get("initial_backoff_ms"), int):
        failures.append("reconnect.initial_backoff_ms 必须是正整数")
    if not isinstance(reconnect.get("max_attempts"), int):
        failures.append("reconnect.max_attempts 必须是正整数")
    return failures


def rule_samples(root, md, data):
    failures = []
    universe = declared_names(data)
    frames = {str(frame.get("type")): frame for frame in data["ws_frames"]}
    enum_domains = {}
    for entry in list(data.get("fields", [])) + list(data.get("command_payload_fields", [])):
        name = entry.get("name")
        match = re.fullmatch(r"enum\(([^)]*)\)", norm(entry.get("type", "")))
        if isinstance(name, str) and match:
            enum_domains[name] = [part.strip() for part in match.group(1).split("|")]
    for sample in data["samples"]:
        sample_id = str(sample.get("id"))
        payload = sample.get("payload")
        required = sample.get("required")
        if not isinstance(payload, dict):
            failures.append(f"样例 {sample_id} 必须提供 payload 对象")
            continue
        if not isinstance(required, list):
            failures.append(f"样例 {sample_id} 必须提供 required 列表")
            continue
        undeclared = sorted(set(payload) - universe)
        if undeclared:
            failures.append(
                f"样例 {sample_id} 使用了未在 fields 或 frame_fields 中声明的字段："
                f"{'、'.join(undeclared)}"
            )
        undeclared_required = sorted(set(required) - universe)
        if undeclared_required:
            failures.append(
                f"样例 {sample_id} 的 required 引用了未声明字段：{'、'.join(undeclared_required)}"
            )
        missing = sorted(set(required) - set(payload))
        if missing:
            failures.append(f"样例 {sample_id} 缺少必填字段：{'、'.join(missing)}")

        if sample.get("kind") != "ws_frame":
            continue
        frame_type = str(sample.get("frame_type"))
        frame = frames.get(frame_type)
        if frame is None:
            failures.append(f"样例 {sample_id} 引用了注册表未声明的帧类型：{frame_type}")
            continue
        for key in ("contract_version_field", "seq_field", "discriminator_field"):
            name = str(frame.get(key))
            if name not in payload:
                failures.append(f"样例 {sample_id} 缺少帧 {frame_type} 的 {name} 字段")
        discriminator = str(frame.get("discriminator_field"))
        if discriminator in payload and payload[discriminator] != frame_type:
            failures.append(
                f"样例 {sample_id} 的 {discriminator} 必须为 {frame_type}，"
                f"实际为 {payload[discriminator]!r}"
            )

    # 样例取值也必须落在本文声明的取值域内：帧样例的 `contract_version` 与契约版本不一致时，
    # 该样例本身就是「契约自己声明应被拒绝」的帧，下游 mock 会照抄出违约载荷。
    version_field_names = {
        str(frame.get("contract_version_field")) for frame in data["ws_frames"]
    }
    contract_version = data.get("contract_version")
    for sample in data["samples"]:
        sample_id = str(sample.get("id"))
        payload = sample.get("payload")
        if not isinstance(payload, dict):
            continue
        for name, allowed in enum_domains.items():
            if name in payload and str(payload[name]) not in allowed:
                failures.append(
                    f"样例 {sample_id} 的 {name} 取值 {payload[name]!r} 不在本文声明的取值域 "
                    f"{allowed} 内"
                )
        if sample.get("kind") != "ws_frame":
            continue
        frame = frames.get(str(sample.get("frame_type")))
        if frame is None:
            continue
        version_field = str(frame.get("contract_version_field"))
        if version_field in version_field_names and payload.get(version_field) != contract_version:
            failures.append(
                f"样例 {sample_id} 的 {version_field} 必须等于注册表 contract_version "
                f"{contract_version!r}，实际 {payload.get(version_field)!r}"
            )

    response_ids = [str(sample.get("id")) for sample in data["samples"]
                    if sample.get("kind") == "https_response"]
    if not response_ids:
        failures.append("samples 必须包含至少一个 HTTPS 响应样例")
    for sample in data["samples"]:
        if sample.get("kind") != "https_response":
            continue
        absent = sorted(set(CONFIRMATION_FIELDS) - set(sample.get("payload") or {}))
        if absent:
            failures.append(
                f"同步响应样例 {sample.get('id')} 缺少 AD-3 要求的确认字段：{'、'.join(absent)}"
            )

    # 样例的 `required` 必须覆盖契约正文的必填清单：`required` 是样例自证的，只与自身 payload
    # 比对会放过「样例漏掉契约必填字段」——按 §6.1/§6.2 实现的一端与按样例建模的 mock 会分叉。
    required_specs = {
        "https_request": (REPORT_SECTION_HEADING, REPORT_REQUIRED_MARKER),
        "https_response": (RESPONSE_SECTION_HEADING, RESPONSE_REQUIRED_MARKER),
    }
    for sample in data["samples"]:
        spec = required_specs.get(str(sample.get("kind")))
        if spec is None:
            continue
        mandatory = set(md_clause_tokens(md, spec[0], spec[1])) & universe
        absent_required = sorted(mandatory - set(sample.get("required") or []))
        if absent_required:
            failures.append(
                f"样例 {sample.get('id')} 的 required 未覆盖契约正文必填清单："
                f"缺少 {'、'.join(absent_required)}"
            )
    return failures


def rule_persistence(root, md, data):
    failures = []
    md_entries = md_persistence_entries(md)
    registry_files = [str(entry.get("name")) for entry in data["persistence_files"]]
    if sorted(md_entries) != sorted(registry_files):
        failures.append(
            f"持久化文件清单在契约正文 §11 与注册表之间不一致：正文 "
            f"{'、'.join(sorted(md_entries))}，注册表 {'、'.join(sorted(registry_files))}"
        )

    for entry in data["persistence_files"]:
        name = str(entry.get("name"))
        left = md_entries.get(name)
        if left is None:
            continue
        for key, label in (
            ("facts", "承载事实"),
            ("schema_version", "schema_version"),
            ("fields", "字段"),
            ("migration", "迁移说明"),
        ):
            if key == "schema_version":
                right_value = str(entry.get("schema_version"))
                left_value = left[key]
            elif key == "fields":
                right_value = sorted(entry.get("fields") or [])
                left_value = left[key]
            else:
                right_value = norm(entry.get(key, ""))
                left_value = left[key]
            if left_value != right_value:
                failures.append(
                    f"持久化文件 {name} 的{label}双真源：契约正文为「{left_value}」，"
                    f"注册表为「{right_value}」"
                )

    single = [entry for entry in data["persistence_files"] if entry.get("single_transaction")]
    if len(single) != 1:
        failures.append(
            f"承载单事务字段组的持久化文件必须唯一，实际 {len(single)} 个"
        )
    if single:
        carrier = single[0]
        group = set(prd_single_transaction_fields(root))
        absent = sorted(group - set(carrier.get("fields") or []))
        if absent:
            failures.append(
                f"单事务文件 {carrier.get('name')} 缺少 PRD §7 末条要求的字段：{'、'.join(absent)}"
            )
        leak_group = group - set(SINGLE_TRANSACTION_LEAK_EXEMPT)
        for entry in data["persistence_files"]:
            if entry is carrier:
                continue
            leaked = sorted(leak_group & set(entry.get("fields") or []))
            if leaked:
                failures.append(
                    f"单事务字段 {'、'.join(leaked)} 不得同时落在 {entry.get('name')}"
                )

    # §9：设置命令族必须使用可重放的 `action_id`，其去重键与命令修订同落一个文件。
    command_carriers = [
        entry for entry in data["persistence_files"]
        if "command_revision" in (entry.get("fields") or [])
    ]
    if len(command_carriers) != 1:
        failures.append(
            f"承载 command_revision 的持久化文件必须唯一，实际 {len(command_carriers)} 个"
        )
    else:
        command_fields = set(command_carriers[0].get("fields") or [])
        missing_dedup = sorted(set(COMMAND_DEDUP_FIELDS) - command_fields)
        if missing_dedup:
            failures.append(
                f"承载 command_revision 的持久化文件 {command_carriers[0].get('name')} 必须同时"
                f"承载设置命令族的去重键：{'、'.join(missing_dedup)}"
            )

    for entry in data["persistence_files"]:
        version = entry.get("schema_version")
        if not isinstance(version, int) or isinstance(version, bool) or version < 1:
            failures.append(f"持久化文件 {entry.get('name')} 必须给出正整数 schema_version")
        if not norm(entry.get("migration", "")):
            failures.append(f"持久化文件 {entry.get('name')} 必须给出迁移说明")
        if norm(entry.get("root", "")) != "app.data-dir":
            failures.append(f"持久化文件 {entry.get('name')} 的根必须为 app.data-dir")

    blob = json.dumps(data, ensure_ascii=False).lower()
    for keyword in DATABASE_KEYWORDS:
        if keyword in blob:
            failures.append(f"注册表不得引入数据库相关字段（AD-16 零库）：命中 {keyword}")
    return failures


def rule_persistence_scope(root, md, data):
    """§11.1：承载通道含「持久化文件」的字段必须有落点，且作用域表两侧一致。"""
    failures = []
    md_scopes = md_scope_entries(md)
    registry_scopes = {str(entry.get("scope")): entry for entry in data["persistence_scopes"]}

    if set(md_scopes) != set(registry_scopes):
        failures.append(
            f"持久化作用域集合在契约正文 §11.1 与注册表之间不一致：正文 "
            f"{'、'.join(sorted(md_scopes))}，注册表 {'、'.join(sorted(registry_scopes))}"
        )

    for scope in sorted(set(md_scopes) & set(registry_scopes)):
        left = md_scopes[scope]
        right = registry_scopes[scope]
        right_carrier = norm(right.get("carrier", ""))
        if left["carrier"] != right_carrier:
            failures.append(
                f"持久化作用域 {scope} 的载体双真源：契约正文为「{left['carrier']}」，"
                f"注册表为「{right_carrier}」"
            )
        right_fields = sorted(right.get("fields") or [])
        if left["fields"] != right_fields:
            failures.append(
                f"持久化作用域 {scope} 的承载字段双真源：契约正文为 {left['fields']}，"
                f"注册表为 {right_fields}"
            )
        unknown = sorted(set(right_fields) - declared_names(data))
        if unknown:
            failures.append(f"持久化作用域 {scope} 引用了未声明字段：{'、'.join(unknown)}")

    channels = field_channels(data)
    persisted = sorted(
        name for name, tokens in channels.items()
        if PERSISTENCE_CHANNEL_TOKEN in tokens
    )
    homes = set()
    for entry in data["persistence_scopes"]:
        homes |= set(entry.get("fields") or [])
    orphans = sorted(set(persisted) - homes)
    if orphans:
        failures.append(
            f"承载通道含持久化文件的字段缺少持久化落点：{'、'.join(orphans)}"
        )

    backend = registry_scopes.get("backend")
    if backend is None:
        failures.append("持久化作用域必须包含 backend")
    else:
        union = set()
        for entry in data["persistence_files"]:
            union |= set(entry.get("fields") or [])
        backend_fields = set(backend.get("fields") or [])
        if backend_fields != union:
            failures.append(
                f"backend 作用域的承载字段必须等于 §11 文件表字段列的并集：作用域多出 "
                f"{sorted(backend_fields - union)}，文件表多出 {sorted(union - backend_fields)}"
            )
    return failures


def rule_command_payload(root, md, data):
    """§9.1：待应用命令的载荷字段只在契约中定义一次，且由命令帧与响应携带。"""
    failures = []
    md_fields = md_command_payload_entries(md)
    registry_fields = {}
    for entry in data.get("command_payload_fields", []):
        name = entry.get("name")
        if not isinstance(name, str) or not name:
            failures.append("注册表 command_payload_fields 存在缺少 name 的条目")
            continue
        if name in registry_fields:
            failures.append(f"注册表 command_payload_fields 重复定义字段：{name}")
        registry_fields[name] = entry

    missing_in_md = sorted(set(registry_fields) - set(md_fields))
    if missing_in_md:
        failures.append(
            f"契约正文 §9.1 缺少注册表已声明的命令载荷字段：{'、'.join(missing_in_md)}"
        )
    missing_in_registry = sorted(set(md_fields) - set(registry_fields))
    if missing_in_registry:
        failures.append(
            f"注册表 command_payload_fields 缺少契约正文 §9.1 已声明字段："
            f"{'、'.join(missing_in_registry)}"
        )

    for name in sorted(set(md_fields) & set(registry_fields)):
        left = md_fields[name]
        right = registry_fields[name]
        for key, label in (("type", "类型"), ("unit", "单位"), ("domain", "取值域"), ("source", "出处")):
            right_value = norm(right.get(key, ""))
            if left[key] != right_value:
                failures.append(
                    f"命令载荷字段 {name} 的{label}双真源：契约正文为「{left[key]}」，"
                    f"注册表为「{right_value}」"
                )
        if not left["unit"]:
            failures.append(f"命令载荷字段 {name} 必须显式给出单位（无单位时写「无」）")

    states = [frame for frame in data["ws_frames"] if str(frame.get("type")) == "command_state"]
    if not states:
        failures.append("注册表 ws_frames 必须声明 command_state 帧")
    for frame in states:
        absent = sorted(set(registry_fields) - set(frame.get("payload_fields") or []))
        if absent:
            failures.append(
                f"command_state 帧必须携带待应用命令载荷：缺少 {'、'.join(absent)}"
            )
    for sample in data["samples"]:
        if sample.get("kind") != "https_response":
            continue
        absent = sorted(set(registry_fields) - set(sample.get("payload") or {}))
        if absent:
            failures.append(
                f"同步响应样例 {sample.get('id')} 必须携带待应用命令载荷：缺少 {'、'.join(absent)}"
            )
    return failures


def rule_offline_backlog(root, md, data):
    """离线积压（队列已满）的阈值、判定输入与业务码必须唯一确定。"""
    failures = []
    backlog = data["offline_backlog"]
    limit = backlog.get("limit")
    if not isinstance(limit, int) or isinstance(limit, bool) or limit <= 0:
        failures.append(f"注册表 offline_backlog.limit 必须是正整数，实际 {limit!r}")
    if backlog.get("comparison") != ">=":
        failures.append(
            f"注册表 offline_backlog.comparison 必须为 \">=\"（差值达到上限即拒绝），"
            f"实际 {backlog.get('comparison')!r}"
        )
    definition = set(backlog.get("definition_fields") or [])
    if definition != set(OFFLINE_BACKLOG_FIELDS):
        failures.append(
            f"离线积压的判定输入必须恰为 {'、'.join(sorted(OFFLINE_BACKLOG_FIELDS))}"
            f"（PRD §4 术语「离线积压」），实际 {sorted(definition)}"
        )
    unknown = sorted(definition - declared_names(data))
    if unknown:
        failures.append(f"offline_backlog.definition_fields 引用了未声明字段：{'、'.join(unknown)}")

    codes = {str(entry.get("condition")): str(entry.get("code")) for entry in data["business_codes"]}
    if codes.get("队列已满") != str(backlog.get("backend_code")):
        failures.append(
            f"offline_backlog.backend_code 与 §12「队列已满」业务码不一致："
            f"{backlog.get('backend_code')} / {codes.get('队列已满')}"
        )

    match = OFFLINE_BACKLOG_LIMIT_RE.search(md)
    if not match:
        failures.append("契约正文必须写明离线积压达上限的阈值（形如「达到上限 <N>」）")
    elif isinstance(limit, int) and not isinstance(limit, bool) and int(match.group(1)) != limit:
        failures.append(
            f"离线积压阈值双真源：契约正文为 {match.group(1)}，注册表为 {limit}"
        )
    if "大于 1000" in md:
        failures.append("离线积压阈值语义必须是「达到上限即拒绝」，不得写成「大于 1000」")

    # 排空路径：若 20004 同时阻断确认，差值只增不减，设备将永久无法同步（吸收态）。
    # 上游真源只规定「达到上限拒绝新输入」，故契约必须显式写出「拒绝的是输入、不是确认」。
    if backlog.get("backend_confirms_at_limit") is not True:
        failures.append(
            "注册表 offline_backlog.backend_confirms_at_limit 必须为 true：达上限时 backend "
            "仍须幂等推进 acked_total，否则差值只增不减形成不可恢复的吸收态"
        )
    drain_rule = norm(backlog.get("drain_rule", ""))
    if not drain_rule:
        failures.append("注册表 offline_backlog 必须给出达上限后的排空路径 drain_rule")
    else:
        for token in OFFLINE_BACKLOG_DRAIN_TOKENS:
            if token not in drain_rule:
                failures.append(
                    f"注册表 offline_backlog.drain_rule 必须写明排空路径：缺少「{token}」"
                )
    flat_md = flat(md)
    for token in OFFLINE_BACKLOG_DRAIN_TOKENS:
        if token not in flat_md:
            failures.append(
                f"契约正文必须写明离线积压达上限后的排空路径：缺少「{token}」"
            )
    return failures


def rule_parameter_bindings(root, md, data):
    """注册表语义键与契约正文关键取值 token 的一致性（§10.2/§10.3）。"""
    failures = []
    for path, token, literal in PARAMETER_BINDING_TOKENS:
        value = resolve_path(data, path)
        if value is None:
            failures.append(f"注册表缺少语义键：{path}")
            continue
        if not isinstance(value, str) or not value:
            failures.append(f"注册表语义键 {path} 必须是非空字符串，实际 {value!r}")
            continue
        if token not in md:
            failures.append(
                f"注册表 {path} = {value} 未在契约正文中登记（缺少「{token}」）"
            )
        elif literal and value not in token:
            failures.append(
                f"注册表 {path} = {value} 与契约正文登记的取值不一致（正文为「{token}」）"
            )

    # 双向取值绑定：注册表取值必须落在登记集合内，且该取值对应的正文表达必须存在。
    flat_md = flat(md)
    for path, allowed in SEMANTIC_VALUE_BINDINGS.items():
        value = resolve_path(data, path)
        if value not in allowed:
            failures.append(
                f"注册表 {path} = {value!r} 不是契约正文登记的取值"
                f"（允许：{'、'.join(sorted(allowed))}）"
            )
            continue
        token = allowed[value]
        if token not in flat_md:
            failures.append(
                f"注册表 {path} = {value} 未在契约正文中登记（缺少「{token}」）"
            )
    scheme_map = (data.get("endpoint") or {}).get("scheme_map")
    if not isinstance(scheme_map, dict) or set(scheme_map) != set(ENDPOINT_SCHEME_MAP):
        failures.append(
            f"注册表 endpoint.scheme_map 的键必须恰为 "
            f"{'、'.join(sorted(ENDPOINT_SCHEME_MAP))}，实际 {scheme_map!r}"
        )
    else:
        for source, target in sorted(ENDPOINT_SCHEME_MAP.items()):
            if scheme_map[source] != target:
                failures.append(
                    f"注册表 endpoint.scheme_map[{source}] 必须为 {target}，"
                    f"实际 {scheme_map[source]!r}"
                )
            elif f"`{source}` 映射为 `{target}`" not in flat_md:
                failures.append(
                    f"契约正文 §10.2 必须写明端点派生映射「`{source}` 映射为 `{target}`」"
                )

    close_codes = data["close_codes"]
    for code in close_codes.get("standard") or []:
        if f"`{code}`" not in md:
            failures.append(f"关闭码 {code} 必须出现在契约正文 §10.3")
    low = close_codes.get("business_min")
    high = close_codes.get("business_max")
    if f"`{low}`" not in md or f"`{high}`" not in md:
        failures.append(f"业务关闭码段 {low}–{high} 必须出现在契约正文 §10.3")
    segment = re.search(r"关闭码取值域(.*?)私有段", md, re.S)
    if not segment:
        failures.append("契约正文 §10.3 必须给出关闭码取值域句")
    else:
        declared = {int(code) for code in re.findall(r"`(\d{4})`", segment.group(1))}
        declared -= {low, high}
        registry_standard = {int(code) for code in close_codes.get("standard") or []}
        if declared != registry_standard:
            failures.append(
                f"关闭码白名单双真源：契约正文为 {sorted(declared)}，"
                f"注册表 close_codes.standard 为 {sorted(registry_standard)}"
            )
    no_retry_line = next((line for line in md.splitlines() if "不再重连" in line), "")
    if not no_retry_line:
        failures.append("契约正文必须写明哪些关闭码不再重连")
    for code in close_codes.get("no_retry_on") or []:
        if f"`{code}`" not in no_retry_line:
            failures.append(
                f"关闭码 {code} 必须出现在正文「不再重连」句中（注册表 close_codes.no_retry_on）"
            )
    if data["reconnect"].get("single_active_socket") is not True:
        failures.append("注册表 reconnect.single_active_socket 必须为 true（单一活跃 socket）")
    elif "单一活跃 socket" not in md:
        failures.append("契约正文必须写明「单一活跃 socket」约束")
    return failures


def rule_business_codes(root, md, data):
    failures = []
    for entry in data["business_codes"]:
        status = entry.get("http_status")
        code = str(entry.get("code"))
        if not isinstance(status, int) or isinstance(status, bool):
            failures.append(f"业务码条目 {entry.get('condition')} 缺少 HTTP 状态")
            continue
        if not re.fullmatch(rf"{status}\d{{2}}", code):
            failures.append(
                f"业务码 {code} 不符合 {{HTTP 状态}}{{两位序号}} 形式"
                f"（条件：{entry.get('condition')}）"
            )
    conditions = {str(entry.get("condition")) for entry in data["business_codes"]}
    absent = sorted(set(REQUIRED_REJECTION_CONDITIONS) - conditions)
    if absent:
        failures.append(f"business_codes 未覆盖同步域拒绝条件：{'、'.join(absent)}")

    header, rows = table_rows(md, "business_codes")
    expect_header(header, BUSINESS_CODE_COLUMNS, "business_codes")
    md_codes = set()
    for row in rows:
        if len(row) != len(BUSINESS_CODE_COLUMNS):
            raise ContractError(f"契约正文 §12 拒绝条件行列数不符：{'|'.join(row)}")
        md_codes.add((norm(row[0]), norm(row[1]), norm(row[2])))
    registry_codes = {
        (str(entry.get("condition")), str(entry.get("http_status")), str(entry.get("code")))
        for entry in data["business_codes"]
    }
    if md_codes != registry_codes:
        only_md = sorted(md_codes - registry_codes)
        only_registry = sorted(registry_codes - md_codes)
        failures.append(
            f"同步域拒绝条件在契约正文 §12 与注册表之间不一致："
            f"仅正文 {only_md}，仅注册表 {only_registry}"
        )
    return failures


def rule_hygiene_scan(root, md, data):
    failures = []
    try:
        probe = scripture_probe(root)
    except ContractError as error:
        failures.append(str(error))
        probe = None
    if probe is not None and probe in "".join(md.split()):
        failures.append(f"契约正文不得包含经文正文（命中探针 {probe}）")
    if probe is not None and probe in "".join(json.dumps(data, ensure_ascii=False).split()):
        failures.append(f"注册表不得包含经文正文（命中探针 {probe}）")

    declarations = []
    for entry in md_field_entries(md).items():
        declarations.append(("统一字段", entry[0], entry[1]["type"], entry[1]["domain"]))
    for entry in md_frame_field_entries(md).items():
        declarations.append(("帧自有字段", entry[0], entry[1]["type"], entry[1]["domain"]))
    for entry in data["fields"]:
        declarations.append(
            ("统一字段", str(entry.get("name")), str(entry.get("type")), str(entry.get("domain")))
        )
    for entry in data.get("frame_fields", []):
        declarations.append(
            ("帧自有字段", str(entry.get("name")), str(entry.get("type")), str(entry.get("domain")))
        )
    number_pattern = re.compile(r"(?<!\d)(" + "|".join(str(v) for v in FORBIDDEN_BOUND_NUMBERS) + r")(?!\d)")
    for kind, name, type_text, domain_text in declarations:
        for label, text in (("类型", type_text), ("取值域", domain_text)):
            hit = number_pattern.search(text)
            if hit:
                failures.append(
                    f"{kind} {name} 的{label}不得把视口槽位或经文长度写成上界：命中 {hit.group(1)}"
                )
    combined = md + "\n" + json.dumps(data, ensure_ascii=False)
    for pattern in BOUND_PHRASE_PATTERNS:
        hit = pattern.search(combined)
        if hit:
            failures.append(f"契约不得把视口槽位或经文长度写成字段上界：命中「{hit.group(0)}」")

    if "HS-" in combined:
        failures.append(
            "契约不得自行写死 scripture_version 的取值：只允许引用 canonical manifest"
            "（命中 HS- 版本字面量）"
        )
    if MANIFEST_REL not in combined:
        failures.append(f"契约必须引用 canonical manifest：{MANIFEST_REL}")
    if number_pattern.search(json.dumps(data.get("samples", []), ensure_ascii=False)):
        failures.append("样例不得出现视口槽位或经文长度字面量")

    lowered = combined.lower()
    for marker in DEPLOYMENT_MARKERS:
        if marker in lowered:
            failures.append(f"契约不得写入生产域名或 AppID 部署值：命中 {marker}")
    if re.search(r"wx[0-9a-f]{16}", lowered):
        failures.append("契约不得写入微信 AppID 字面量")

    for token in AI_MARKER_TOKENS:
        if token.lower() in lowered:
            failures.append(f"契约或注册表不得出现 AI 痕迹词：命中 {token}")
    for token in NODE_ID_TOKENS:
        if token in combined:
            failures.append(f"契约或注册表不得出现设计工具节点标识：命中 {token}")

    for entry in data["fields"]:
        name = str(entry.get("name"))
        if name.startswith("automatic_tap"):
            failures.append(
                f"字段 {name} 违反 PRD §7：automatic_tap 不得新增独立 cloud 累计字段或同步接口"
            )
    for entry in data.get("frame_fields", []):
        name = str(entry.get("name"))
        if name.startswith("automatic_tap"):
            failures.append(f"帧自有字段 {name} 不得承载 automatic_tap 的独立 cloud 语义")
    if NO_INDEPENDENT_AUTOMATIC_TAP_CLAUSE not in md:
        failures.append(
            "契约正文必须保留「automatic_tap 不新增独立 cloud 累计字段或同步接口」的禁止语句"
        )
    return failures


def rule_discard_baseline(root, md, data):
    """§8 的序号丢弃基准必须是被定义过的字段且有持久化落点。

    `last_applied_seq` 只被引用而无所有者/落点时，「重启后是否重复消费 delta」无从判定，
    backend 与小程序可各自实现出不同的去重边界。本契约把它定义为 frontend 的持久化字段
    （初值取自快照水位，此后独立推进）；该定义必须真的落在 §2 与 §11.1 上。
    """
    failures = []
    if DISCARD_RULE not in md:
        failures.append(f"契约正文必须写明序号丢弃规则：{DISCARD_RULE}")
        return failures
    entries = {str(entry.get("name")): entry for entry in data.get("fields", [])}
    entry = entries.get(DISCARD_BASELINE_FIELD)
    if entry is None:
        failures.append(
            f"契约正文引用了序号丢弃基准 {DISCARD_BASELINE_FIELD}，但它未在 §2 统一字段表声明"
            "（所有者、语义与落点无从判定）"
        )
        return failures
    if PERSISTENCE_CHANNEL_TOKEN not in (entry.get("channel") or []):
        failures.append(
            f"{DISCARD_BASELINE_FIELD} 必须声明「持久化文件」承载通道："
            "未落盘时重启会重复消费已应用序号"
        )
    homes = set()
    for scope in data.get("persistence_scopes", []):
        homes |= set(scope.get("fields") or [])
    if DISCARD_BASELINE_FIELD not in homes:
        failures.append(
            f"{DISCARD_BASELINE_FIELD} 必须在 §11.1 持久化作用域表给出落点"
        )
    return failures


def flat(text):
    """把换行与连续空白压平成单空格，供跨行书写的正文表达做字面匹配。

    正文按 80 列折行，「`https` 映射为 `wss`」这类表达会被换行切断；只按原始字节匹配会
    把「同一句话」误判为缺失，或反过来放过改写。
    """
    return re.sub(r"\s+", " ", text)


def future_tense_hits(text):
    """返回文本中把契约写成将来时的命中（字面短语 + 同义表述族）。"""
    hits = []
    for phrase in FUTURE_TENSE_PHRASES:
        if phrase in text:
            hits.append(phrase)
    for pattern in FUTURE_TENSE_PATTERNS:
        for match in pattern.finditer(text):
            hits.append(match.group(0))
    return hits


def rule_index_docs(root, md, data):
    failures = []
    contracts_readme = read_text(root, CONTRACTS_README_REL)
    if "sync-contract.md" not in contracts_readme:
        failures.append(f"{CONTRACTS_README_REL} 必须指向契约正文 sync-contract.md")
    version = data.get("contract_version")
    if isinstance(version, str) and version and version not in contracts_readme:
        failures.append(f"{CONTRACTS_README_REL} 必须登记 contract_version = {version}")
    for phrase in ("尚未创建", "待冻结"):
        if phrase in contracts_readme:
            failures.append(f"{CONTRACTS_README_REL} 仍声称契约「{phrase}」")
    if "canonical/" not in contracts_readme:
        failures.append(f"{CONTRACTS_README_REL} 必须登记 canonical/ 子目录的分工")
    contracts_hits = future_tense_hits(contracts_readme)
    if contracts_hits:
        failures.append(
            f"{CONTRACTS_README_REL} 仍把已冻结契约写成将来时：命中 {'、'.join(contracts_hits)}"
        )

    docs_readme = read_text(root, DOCS_README_REL)
    line = None
    for candidate in docs_readme.splitlines():
        if candidate.strip().startswith("- `contracts/`"):
            line = candidate
            break
    if line is None:
        failures.append(f"{DOCS_README_REL} 目录导览缺少 `contracts/` 行")
    else:
        if "sync-contract.md" not in line:
            failures.append(f"{DOCS_README_REL} 的 `contracts/` 行必须指向 sync-contract.md")
        for phrase in ("待冻结", "占位"):
            if phrase in line:
                failures.append(f"{DOCS_README_REL} 的 `contracts/` 行仍含「{phrase}」")
        docs_line_hits = future_tense_hits(line)
        if docs_line_hits:
            failures.append(
                f"{DOCS_README_REL} 的 `contracts/` 行仍把已冻结契约写成将来时："
                f"命中 {'、'.join(docs_line_hits)}"
            )

    # AC 5：仓库内引用跨层契约的下游文档不得再把已冻结契约写成将来时。
    # 字面短语与同义表述族都要失败——只查字面短语会放过「待后续冻结后再定」这类改写。
    backend_doc = read_text(root, BACKEND_DOC_REL)
    future_hits = future_tense_hits(backend_doc)
    if future_hits:
        failures.append(
            f"{BACKEND_DOC_REL} 仍把已冻结契约写成将来时：命中 {'、'.join(future_hits)}"
        )
    if isinstance(version, str) and version and version not in backend_doc:
        failures.append(f"{BACKEND_DOC_REL} 必须登记契约已冻结的 contract_version = {version}")
    return failures


def rule_spine(root, md, data):
    failures = []
    spine_fields = set(spine_expected_fields(root))
    for name in ("pending_completion", "snapshot_seq", "replay_cursor", "action_id"):
        if name not in spine_fields:
            failures.append(f"{SPINE_REL} 同步字段清单缺少 {name}")
    spine_text = read_text(root, SPINE_REL)
    if "两类正式输入" in spine_text:
        failures.append(f"{SPINE_REL} AD-1 仍保留「仅有两类正式输入」的旧措辞")
    if "待校时" not in spine_text:
        failures.append(f"{SPINE_REL} 必须保留「待校时」的可信时间门禁状态")

    # AC 6：AD-5 的分层措辞必须留在 AD-5 段内，回退成单一高水位措辞即失败。
    ad5 = spine_ad_section(root, "AD-5")
    for token in SPINE_AD5_REQUIRED:
        if token not in ad5:
            failures.append(f"{SPINE_REL} AD-5 缺少分层措辞「{token}」")
    if SPINE_AD5_FORBIDDEN in ad5:
        failures.append(
            f"{SPINE_REL} AD-5 回退为单一高水位旧措辞：命中 {SPINE_AD5_FORBIDDEN}"
        )
    ad1 = spine_ad_section(root, "AD-1")
    if NO_INDEPENDENT_AUTOMATIC_TAP_CLAUSE not in ad1:
        failures.append(
            f"{SPINE_REL} AD-1 必须保留「{NO_INDEPENDENT_AUTOMATIC_TAP_CLAUSE}」的禁止语句"
        )
    markers = spine_text.count(SPINE_CONVERGENCE_MARKER)
    if markers < 3:
        failures.append(
            f"{SPINE_REL} 必须保留三条 Deferred 收敛标注 {SPINE_CONVERGENCE_MARKER}，"
            f"实际 {markers} 处"
        )
    if SPINE_A2_ORDER_NOTE not in spine_deferred_row(root, "ASSUMPTION A-2"):
        failures.append(
            f"{SPINE_REL} A-2 的收敛标注必须写明跨文件非原子写顺序仍未收敛"
            f"（缺少「{SPINE_A2_ORDER_NOTE}」）"
        )
    return failures


RULES = (
    rule_file_hygiene,
    rule_registry_shape,
    rule_contract_version,
    rule_field_table,
    rule_field_coverage,
    rule_event_sources,
    rule_states,
    rule_ws_frames,
    rule_channel_directions,
    rule_discard_baseline,
    rule_params,
    rule_samples,
    rule_persistence,
    rule_persistence_scope,
    rule_command_payload,
    rule_offline_backlog,
    rule_parameter_bindings,
    rule_business_codes,
    rule_hygiene_scan,
    rule_index_docs,
    rule_spine,
)


def collect_failures(root):
    """只读检查：返回中文诊断列表，不修改任何文件。"""
    try:
        md, data = load_contract(root)
    except ContractError as error:
        return [str(error)]
    failures = []
    for rule in RULES:
        try:
            failures.extend(rule(root, md, data))
        except ContractError as error:
            failures.append(str(error))
        except (KeyError, IndexError, TypeError, AttributeError, ValueError, StopIteration) as error:
            # 注册表顶层键缺失或类型不符、或上游清单行无法解析（如 `split` 解包失败）时，
            # 规则会直接抛错。此处把它转成中文诊断，否则异常会中断整轮检查：
            # rule_registry_shape 已收集的诊断与全部负例都会被丢掉。
            failures.append(
                f"{SCHEMA_REL} 结构不完整，{rule.__name__} 无法完成检查："
                f"{type(error).__name__}: {error}"
                f"（请先修正注册表顶层键与字段类型）"
            )
    seen = set()
    unique = []
    for item in failures:
        if item not in seen:
            seen.add(item)
            unique.append(item)
    return unique


# --------------------------------------------------------------------------- 负例工具


SANDBOX_RELS = (
    CONTRACT_REL,
    SCHEMA_REL,
    CONTRACTS_README_REL,
    DOCS_README_REL,
    BACKEND_DOC_REL,
    MANIFEST_REL,
    CANONICAL_SOURCE_REL,
    PRD_REL,
    SPINE_REL,
)


def make_sandbox(tmpdir):
    root = Path(tmpdir)
    for rel in SANDBOX_RELS:
        source = REPO_ROOT / rel
        if not source.exists():
            continue
        destination = root / rel
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    return root


def edit_text(path, transform):
    text = path.read_text(encoding="utf-8")
    updated = transform(text)
    assert updated != text, f"负例未能在 {path.name} 上构造改动"
    path.write_text(updated, encoding="utf-8", newline="\n")


def edit_json(path, transform):
    data = json.loads(path.read_text(encoding="utf-8"))
    transform(data)
    path.write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n"
    )


def expect_failure(root, label, tokens):
    failures = collect_failures(root)
    assert failures, f"{label}：门禁不应通过（未报告任何失败）"
    text = "\n".join(failures)
    for token in tokens:
        assert token in text, f"{label}：失败信息未指出 {token}，实际诊断：{text}"


def drop_contract_line_ident(marker, name):
    """在契约正文含 marker 的段落内删掉一个反引号标识符（连同其分隔的顿号）。"""
    def transform(text):
        lines = text.splitlines(keepends=True)
        for index, line in enumerate(lines):
            if marker not in line:
                continue
            cursor = index
            while cursor < len(lines):
                candidate = lines[cursor]
                if candidate.strip() and not candidate.lstrip().startswith(("|", "-", ">", "#")):
                    for fragment in (f"`{name}`、", f"`{name}`。"):
                        if fragment in candidate:
                            lines[cursor] = candidate.replace(fragment, "", 1)
                            return "".join(lines)
                    cursor += 1
                    continue
                break
            raise AssertionError(f"{marker} 段落内未找到 `{name}`")
        raise AssertionError(f"契约正文缺少「{marker}」")
    return transform


def drop_field_row(contract, name):
    edit_text(
        contract,
        lambda text: "".join(
            line for line in text.splitlines(keepends=True) if f"| `{name}` |" not in line
        ),
    )


def drop_registry_field(schema, name):
    edit_json(schema, lambda data: data.__setitem__(
        "fields", [entry for entry in data["fields"] if entry.get("name") != name]
    ))


def drop_spine_identifier(spine, name):
    edit_text(spine, lambda text: text.replace(f"`{name}` · ", "").replace(f" · `{name}`", ""))


def rewrite_field_row(name, old_fragment, new_fragment):
    """在契约正文 §2 字段表里定位某个字段行并替换其中一个片段。"""
    def transform(text):
        lines = text.splitlines(keepends=True)
        for index, line in enumerate(lines):
            if line.startswith(f"| `{name}` |") and old_fragment in line:
                lines[index] = line.replace(old_fragment, new_fragment)
                return "".join(lines)
        raise AssertionError(f"契约正文 §2 未找到可改写的字段行：{name}")
    return transform


def drop_contract_line(fragment):
    """删除契约正文中含指定片段的整行。"""
    def transform(text):
        return "".join(
            line for line in text.splitlines(keepends=True) if fragment not in line
        )
    return transform


def set_field(schema, name, key, value):
    def transform(data):
        next(entry for entry in data["fields"] if entry["name"] == name)[key] = value
    edit_json(schema, transform)


# --------------------------------------------------------------------------- 负例门禁


def test_negative_1_missing_unified_field():
    """负例 1：从契约正文与注册表同时删除 PRD §7 的统一字段。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        drop_field_row(root / CONTRACT_REL, "battery_percent")
        drop_registry_field(root / SCHEMA_REL, "battery_percent")
        expect_failure(root, "负例 1", ("battery_percent",))


def test_negative_2_event_sources_collapsed_to_two():
    """负例 2：事件来源写成两类（删 automatic_tap）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        contract = root / CONTRACT_REL
        edit_text(
            contract,
            lambda text: text.replace("`physical_pvdf`、`device_touch`、`automatic_tap`",
                                      "`physical_pvdf`、`device_touch`"),
        )
        edit_json(
            root / SCHEMA_REL,
            lambda data: data.__setitem__(
                "event_sources",
                [entry for entry in data["event_sources"] if entry.get("name") != "automatic_tap"],
            ),
        )
        edit_text(
            root / SPINE_REL,
            lambda text: text.replace(
                "`physical_pvdf`、`device_touch`、`automatic_tap`", "`physical_pvdf`、`device_touch`"
            ),
        )
        expect_failure(root, "负例 2", ("automatic_tap",))


def test_negative_3_contract_version_diverges():
    """负例 3：contract_version 在 md 与注册表不一致。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        contract = root / CONTRACT_REL
        edit_text(contract, lambda text: text.replace("`SC-1.0.0`", "`SC-1.0.1`", 1))
        expect_failure(root, "负例 3", ("contract_version",))


def test_negative_4_type_or_unit_diverges():
    """负例 4：同一字段的类型或单位在 md 与注册表不一致。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["fields"] if entry["name"] == "local_total"
            ).__setitem__("unit", "次"),
        )
        expect_failure(root, "负例 4（单位）", ("local_total",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / CONTRACT_REL,
            lambda text: text.replace("| `battery_percent` | integer 0–100 |",
                                      "| `battery_percent` | integer |", 1),
        )
        expect_failure(root, "负例 4（类型）", ("battery_percent",))


def test_negative_5_parameter_out_of_bounds():
    """负例 5：心跳与退避参数越界或缺单位。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["ws_heartbeat"].__setitem__("interval_ms", 50000),
        )
        expect_failure(root, "负例 5（心跳间隔越界）", ("ws_heartbeat.interval_ms",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["ws_heartbeat"].__setitem__("timeout_ms", 20000),
        )
        expect_failure(root, "负例 5（心跳超时越界）", ("ws_heartbeat.timeout_ms",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["reconnect"].__setitem__("max_backoff_ms", 60000),
        )
        expect_failure(root, "负例 5（退避上限越界）", ("reconnect.max_backoff_ms",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)

        def strip_unit(data):
            data["ws_heartbeat"]["interval"] = data["ws_heartbeat"].pop("interval_ms")

        edit_json(root / SCHEMA_REL, strip_unit)
        expect_failure(root, "负例 5（心跳间隔缺 _ms 单位）", ("interval_ms",))


def test_negative_6_command_state_merged_into_sync_states():
    """负例 6：把「待设备应用」并入累计同步状态枚举。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["sync_states"].append(
                {"wire": "pending_apply", "display": "待设备应用", "meaning": "命令维度混入"}
            ),
        )
        expect_failure(root, "负例 6", ("sync_states", "待设备应用"))


def test_negative_7_scripture_version_hardcoded():
    """负例 7：契约自行写死 scripture_version 的取值。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["fields"] if entry["name"] == "scripture_version"
            ).__setitem__("domain", "取值固定为 HS-9.9.9"),
        )
        expect_failure(root, "负例 7（注册表）", ("HS-",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / CONTRACT_REL,
            lambda text: text.replace("不定义 `scripture_version` 的取值",
                                      "固定 `scripture_version` 为 HS-9.9.9", 1),
        )
        expect_failure(root, "负例 7（契约正文）", ("HS-",))


def test_negative_8_viewport_slot_or_length_as_bound():
    """负例 8：把经文长度或槽位常量写成字段上界。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["fields"] if entry["name"] == "round_cursor"
            ).__setitem__("domain", "0 到 260"),
        )
        expect_failure(root, "负例 8（经文长度上界）", ("round_cursor", "260"))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / CONTRACT_REL,
            lambda text: text.replace("| `firmware_version` | string | 无 |",
                                      "| `firmware_version` | string 长度上限 17 | 无 |", 1),
        )
        expect_failure(root, "负例 8（槽位常量上界）", ("firmware_version", "17"))


def test_negative_9_automatic_tap_independent_cloud_field():
    """负例 9：把 automatic_tap 定义为独立 cloud 累计字段或去掉禁止语句。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["fields"].append(
                {
                    "name": "automatic_tap_total",
                    "type": "integer ≥ 0",
                    "unit": "无",
                    "owner": "设备",
                    "authority": "设备上报",
                    "domain": "自动敲击独立累计",
                    "channel": ["https-report"],
                    "source": "PRD §7",
                }
            ),
        )
        expect_failure(root, "负例 9（独立字段）", ("automatic_tap",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / CONTRACT_REL,
            lambda text: text.replace("不新增独立 cloud 累计字段或同步接口", "并入同一有效敲击队列", 1),
        )
        expect_failure(root, "负例 9（去掉禁止语句）", ("automatic_tap",))


def test_negative_10_frame_missing_contract_version_field():
    """负例 10：WS 帧缺少契约版本字段。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "snapshot"
            ).pop("contract_version_field"),
        )
        expect_failure(root, "负例 10（注册表）", ("contract_version_field",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / CONTRACT_REL,
            lambda text: "".join(
                line for line in text.splitlines(keepends=True)
                if "| `contract_version` | string | 无 | 版本化字段 |" not in line
            ),
        )
        expect_failure(root, "负例 10（契约正文）", ("contract_version",))


def test_negative_11_sample_field_mismatch():
    """负例 11：样例加入未声明字段，或抽掉必填字段。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                sample for sample in data["samples"] if sample["id"] == "ws_delta"
            )["payload"].__setitem__("undeclared_field", 1),
        )
        expect_failure(root, "负例 11（未声明字段）", ("undeclared_field",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                sample for sample in data["samples"] if sample["id"] == "https_sync_response"
            )["payload"].pop("command_revision"),
        )
        expect_failure(root, "负例 11（抽掉必填字段）", ("command_revision",))


def test_negative_12_spine_enum_and_field_list_regressed():
    """负例 12：spine 事件来源改回两类，或删掉同步字段清单中的新字段。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / SPINE_REL,
            lambda text: text.replace(
                "事件来源：`physical_pvdf`、`device_touch`、`automatic_tap`。",
                "事件来源：`physical_pvdf`、`device_touch`。",
            ),
        )
        expect_failure(root, "负例 12（事件来源两类）", ("automatic_tap",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        drop_spine_identifier(root / SPINE_REL, "pending_completion")
        expect_failure(root, "负例 12（字段清单缺项）", ("pending_completion",))


def test_negative_13_forbidden_marker_in_contract():
    """负例 13：契约或注册表出现设计过程痕迹词与工具节点标识。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        marker = AI_MARKER_TOKENS[0]
        edit_text(root / CONTRACT_REL, lambda text: text + f"\n{marker}\n")
        expect_failure(root, "负例 13（痕迹词）", (marker,))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        node = NODE_ID_TOKENS[0]
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["fields"] if entry["name"] == "device_id"
            ).__setitem__("domain", f"不透明标识 {node}"),
        )
        expect_failure(root, "负例 13（节点标识）", (node,))


def test_negative_14_spine_ad5_wording_regressed():
    """负例 14：把 spine AD-5 的 Rule 回退为「命令修订 = 设备已应用高水位」旧措辞。

    这正是 AC 6 要求消除、且此前只被人工发现的自相矛盾：回退后 spine 与契约 §9 冲突。
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / SPINE_REL,
            lambda text: text.replace(
                "其语义固定为 **backend 已下发命令的最新修订**（FR-B-007）；"
                "设备已应用命令的单调高水位是独立字段 `applied_revision`（单调不减）",
                "其语义固定为**「设备已应用命令的单调高水位」**（FR-B-007）",
            ),
        )
        expect_failure(root, "负例 14", ("AD-5",))


def test_negative_15_frame_payload_without_frame_channel():
    """负例 15：帧载荷字段未在 §2 声明「WS 帧」承载通道（通道表与载荷互相否定）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "snapshot"
            )["payload_fields"].append("scripture_version"),
        )
        expect_failure(root, "负例 15（载荷字段缺 WS 帧通道）", ("scripture_version",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["fields"] if entry["name"] == "local_total"
            )["channel"].remove("ws-frame"),
        )
        expect_failure(
            root, "负例 15（字段通道缺 WS 帧）", ("local_total", "承载通道")
        )


def test_negative_16_action_id_persistence_home():
    """负例 16：`action_id` 的两族落点被破坏；轮次字段仍不得跨文件承载。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["persistence_files"]
                if entry["name"] == "commands.json"
            )["fields"].remove("action_id"),
        )
        expect_failure(root, "负例 16（设置命令族缺去重键）", ("action_id",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["persistence_files"]
                if entry["name"] == "progress.json"
            )["fields"].remove("action_id"),
        )
        expect_failure(root, "负例 16（单事务文件缺 action_id）", ("action_id",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["persistence_files"]
                if entry["name"] == "commands.json"
            )["fields"].append("round_state"),
        )
        expect_failure(root, "负例 16（轮次字段跨文件承载）", ("round_state",))


def test_negative_17_persistence_scope_orphan():
    """负例 17：承载通道声明「持久化文件」的字段缺少 §11.1 落点。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data.__setitem__(
                "persistence_scopes",
                [s for s in data["persistence_scopes"] if s["scope"] != "frontend"],
            ),
        )
        expect_failure(root, "负例 17（frontend 作用域缺失）", ("replay_cursor",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                scope for scope in data["persistence_scopes"] if scope["scope"] == "backend"
            )["fields"].append("snapshot_seq"),
        )
        expect_failure(root, "负例 17（backend 作用域超出文件表并集）", ("并集",))


def test_negative_18_offline_backlog_undefined():
    """负例 18：离线积压的阈值、判定输入或比较语义不确定。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["offline_backlog"].__setitem__("limit", 1001),
        )
        expect_failure(root, "负例 18（阈值双真源）", ("阈值双真源",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["offline_backlog"].__setitem__("comparison", ">"),
        )
        expect_failure(root, "负例 18（比较语义不确定）", ("comparison",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["offline_backlog"].__setitem__(
                "definition_fields", ["local_total"]
            ),
        )
        expect_failure(root, "负例 18（判定输入不确定）", ("判定输入",))


def test_negative_19_value_table_diverges():
    """负例 19：既有正文表格各列与注册表出现分歧（承诺过的逐值比对必须真的会失败）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["sync_states"][0].__setitem__("meaning", "含义被改写"),
        )
        expect_failure(root, "负例 19（状态含义）", ("含义",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "snapshot"
            ).__setitem__("content", "内容被改写"),
        )
        expect_failure(root, "负例 19（帧内容）", ("内容",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "delta"
            ).__setitem__("direction", "frontend_to_backend"),
        )
        expect_failure(root, "负例 19（帧方向）", ("方向",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "command_state"
            ).__setitem__("source", "FR-C-099"),
        )
        expect_failure(root, "负例 19（帧出处）", ("出处",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                entry for entry in data["persistence_files"]
                if entry["name"] == "progress.json"
            ).__setitem__("schema_version", 2),
        )
        expect_failure(root, "负例 19（文件 schema_version）", ("schema_version",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["ws_heartbeat"]["test_bounds"].__setitem__(
                "interval_ms_max", 50000
            ),
        )
        expect_failure(root, "负例 19（测试上界）", ("测试上界",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["endpoint"].__setitem__("path_suffix", "/socket"),
        )
        expect_failure(root, "负例 19（endpoint 派生规则）", ("path_suffix",))


def test_negative_20_error_object_declared_on_every_frame():
    """负例 20：把 `error` 对象声明为每帧必带，或删掉正文的 `error` 作用域规定。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "delta"
            ).__setitem__("error_object_required", True),
        )
        expect_failure(root, "负例 20（多帧必带 error）", ("error 对象",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, lambda text: text.replace(ERROR_ONLY_FRAME_CLAUSE, "在任意帧出现"))
        expect_failure(root, "负例 20（正文缺 error 作用域）", ("作用域",))


def test_negative_21_command_payload_unfrozen():
    """负例 21：待应用命令的载荷字段未被冻结或未被承载。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data.__setitem__(
                "command_payload_fields",
                [e for e in data["command_payload_fields"] if e["name"] != "brightness"],
            ),
        )
        expect_failure(root, "负例 21（载荷字段未冻结）", ("brightness",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                frame for frame in data["ws_frames"] if frame["type"] == "command_state"
            )["payload_fields"].remove("volume"),
        )
        expect_failure(root, "负例 21（命令帧未承载载荷）", ("volume",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: next(
                sample for sample in data["samples"]
                if sample["id"] == "https_sync_response"
            )["payload"].pop("timeout"),
        )
        expect_failure(root, "负例 21（响应样例未承载载荷）", ("timeout",))


def test_negative_22_pointer_doc_future_tense():
    """负例 22：下游指针文档把已冻结契约写成将来时。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / BACKEND_DOC_REL,
            lambda text: text + "\n最终表在后续同步契约阶段冻结。\n",
        )
        expect_failure(root, "负例 22", ("将来时",))


def test_negative_23_spine_a2_convergence_overclaim():
    """负例 23：spine 的 A-2 收敛标注把未冻结的跨文件写顺序一并声称为已收敛。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / SPINE_REL,
            lambda text: text.replace(
                "非原子写顺序不在该契约冻结范围", "非原子写顺序已一并收敛"
            ),
        )
        expect_failure(root, "负例 23", ("A-2",))


def test_negative_24_ws_parameter_bindings():
    """负例 24：WS 语义键（关闭码、心跳发起方、单一活跃 socket）与正文取值不一致。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["close_codes"].__setitem__("standard", [1000, 1001, 1008]),
        )
        expect_failure(root, "负例 24（关闭码白名单退化）", ("关闭码白名单双真源",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["close_codes"]["no_retry_on"].append(1001),
        )
        expect_failure(root, "负例 24（不再重连码未登记）", ("不再重连",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["reconnect"].__setitem__("single_active_socket", False),
        )
        expect_failure(root, "负例 24（单一活跃 socket 未约束）", ("单一活跃 socket",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / SCHEMA_REL,
            lambda data: data["ws_heartbeat"].__setitem__("initiator", "frontend"),
        )
        expect_failure(root, "负例 24（心跳发起方不一致）", ("initiator",))


def test_negative_25_scripture_with_whitespace():
    """负例 25：经文正文以换行折断写入时仍必须被检出（扫描两侧同去空白）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        probe = scripture_probe(root)
        broken = probe[:8] + "\n" + probe[8:]
        edit_text(root / CONTRACT_REL, lambda text: text + f"\n{broken}\n")
        expect_failure(root, "负例 25", ("经文正文",))


def test_negative_26_channel_direction_unclosed():
    """负例 26：承载通道与 §6.1/§11/§11.1 的单侧漂移必须失败。

    §2 的通道列与三处承载清单是同一件事的四个写法；只改一侧时旧门禁只对 WS 帧方向有断言，
    HTTPS 上报与持久化两个方向静默。
    """
    # ① 两侧同时收回 acked_total 的「HTTPS 上报」通道（§6.1 仍把它列为上报必填）。
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, rewrite_field_row(
            "acked_total",
            "| HTTPS 上报、HTTPS 响应、WS 帧、持久化文件 |",
            "| HTTPS 响应、WS 帧、持久化文件 |",
        ))
        set_field(root / SCHEMA_REL, "acked_total", "channel",
                  ["https-response", "ws-frame", "persistence-file"])
        expect_failure(root, "负例 26（上报方向不闭合）", ("HTTPS 上报",))

    # ② §6.1 必填清单删掉一个字段（其通道仍声明上报）——通道侧与清单侧必须互相覆盖。
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL,
                  lambda text: text.replace("`battery_percent`、", "", 1))
        expect_failure(root, "负例 26（上报必填清单缺字段）", ("HTTPS 上报",))

    # ③ 两侧同时收回 battery_percent 的「持久化文件」通道（§11/§11.1 仍承载它）。
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, rewrite_field_row(
            "battery_percent", "、持久化文件 |", " |"))
        set_field(root / SCHEMA_REL, "battery_percent", "channel",
                  ["https-report", "https-response", "ws-frame"])
        expect_failure(root, "负例 26（持久化方向不闭合）", ("持久化文件",))

    # ④ §6.2 响应必填清单缺一个命令载荷字段。
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, lambda text: text.replace(
            "`command_revision`、`snapshot_seq`；待应用命令载荷字段 `volume`、`brightness`、"
            "`timeout`",
            "`command_revision`、`snapshot_seq`；待应用命令载荷字段 `volume`、`brightness`",
            1,
        ))
        expect_failure(root, "负例 26（响应必填缺命令载荷）", ("命令载荷",))


def test_negative_27_ws_semantic_value_bindings():
    """负例 27：注册表语义键取值与正文相反时必须失败（不是只查正文 token 存在）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: data["endpoint"].__setitem__(
            "scheme_map", {"https": "ws", "http": "wss"}))
        expect_failure(root, "负例 27（scheme_map 反向）", ("scheme_map",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, lambda text: text.replace(
            "`https` 映射为", "`https` 恒不用"))
        expect_failure(root, "负例 27（正文缺协议映射）", ("映射",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: data["endpoint"].__setitem__(
            "token_transport", "header"))
        expect_failure(root, "负例 27（令牌改走 header）", ("token_transport",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: data["reconnect"].__setitem__(
            "strategy", "fixed_delay"))
        expect_failure(root, "负例 27（重连策略无抖动）", ("reconnect.strategy",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: data["reconnect"].__setitem__(
            "hole_rule", "直接续播不补齐"))
        expect_failure(root, "负例 27（空洞不补齐）", ("reconnect.hole_rule",))


def test_negative_28_sample_values():
    """负例 28：样例取值必须与契约一致——版本递增后帧样例不得变成应被拒绝的帧。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: next(
            sample for sample in data["samples"] if sample["id"] == "ws_delta"
        )["payload"].__setitem__("contract_version", "SC-1.0.1"))
        expect_failure(root, "负例 28（帧样例版本不一致）", ("contract_version",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: next(
            sample for sample in data["samples"] if sample["id"] == "ws_snapshot"
        )["payload"].__setitem__("round_state", "done"))
        expect_failure(root, "负例 28（样例枚举越界）", ("取值域",))


def test_negative_29_pointer_doc_future_tense_synonym():
    """负例 29：指针文档用同义表述把已冻结契约写成将来时也必须失败。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / BACKEND_DOC_REL, lambda text: text.replace(
            "JSON schema 与文件粒度已由 `docs/contracts/sync-contract.md` §11 冻结",
            "JSON schema 与文件粒度待后续冻结后再定",
            1,
        ))
        expect_failure(root, "负例 29", ("将来时",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / BACKEND_DOC_REL, lambda text: text.replace(
            "JSON schema 与文件粒度已由 `docs/contracts/sync-contract.md` §11 冻结",
            "JSON schema 与文件粒度尚未定稿",
            1,
        ))
        expect_failure(root, "负例 29（尚未定稿）", ("将来时",))


def test_negative_30_offline_backlog_drain_missing():
    """负例 30：离线积压达上限后的排空路径缺失时必须失败（否则是吸收态）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: data["offline_backlog"].__setitem__(
            "backend_confirms_at_limit", False))
        expect_failure(root, "负例 30（达上限不确认）", ("backend_confirms_at_limit",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, drop_contract_line("| 达到上限后的排空 |"))
        expect_failure(root, "负例 30（正文缺排空路径）", ("排空",))


def test_negative_31_discard_baseline_undefined():
    """负例 31：丢弃基准 `last_applied_seq` 只被引用而未在 §2 定义时必须失败。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        drop_field_row(root / CONTRACT_REL, DISCARD_BASELINE_FIELD)
        drop_registry_field(root / SCHEMA_REL, DISCARD_BASELINE_FIELD)
        expect_failure(root, "负例 31", (DISCARD_BASELINE_FIELD,))


def test_negative_32_channel_declaration_without_carrier():
    """负例 32：§2 声明了某通道但对应清单不承载该字段（反向漂移）必须失败。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / CONTRACT_REL, drop_contract_line_ident(
            "响应必填：", "round_state"))
        expect_failure(root, "负例 32（响应通道无承载）", ("HTTPS 响应", "round_state"))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: next(
            frame for frame in data["ws_frames"] if frame["type"] == "snapshot"
        ).__setitem__(
            "payload_fields",
            [name for name in next(
                frame for frame in data["ws_frames"] if frame["type"] == "snapshot"
            )["payload_fields"] if name != "snapshot_seq"],
        ))
        expect_failure(root, "负例 32（帧通道无承载）", ("WS 帧", "snapshot_seq"))


def test_negative_33_sample_required_omits_contract_mandatory():
    """负例 33：响应样例的 required 抽掉契约正文必填字段时必须失败（自证式 required 不算）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(root / SCHEMA_REL, lambda data: next(
            sample for sample in data["samples"] if sample["id"] == "https_sync_response"
        ).__setitem__(
            "required",
            [name for name in next(
                sample for sample in data["samples"] if sample["id"] == "https_sync_response"
            )["required"] if name != "snapshot_seq"],
        ))
        expect_failure(root, "负例 33（样例 required 漏必填）", ("snapshot_seq", "required"))


def test_negative_34_prd_expected_set_extraction_failure():
    """负例 34：PRD §7 期望集合被折行截断到标记行时提取必须 fail-closed，不得静默变空。"""
    def fold(text):
        lines = text.splitlines(keepends=True)
        for index, line in enumerate(lines):
            if "统一字段：" in line:
                cut = line.index("统一字段：") + len("统一字段：") + 24
                lines[index] = line[:cut] + "\n" + line[cut:]
                return "".join(lines)
        raise AssertionError("PRD §7 未找到「统一字段：」句")
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / PRD_REL, fold)
        expect_failure(root, "负例 34（PRD 期望集合折行截断）", ("折行", "覆盖断言无法执行"))


def test_negative_35_pointer_readme_future_tense_synonym():
    """负例 35：被点名的两份 README 用同义表述退回将来时也必须失败。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(
            root / CONTRACTS_README_REL,
            lambda text: text + "\n- `sync-contract.md` 的字段表待后续冻结后再定。\n",
        )
        expect_failure(root, "负例 35（契约目录 README）", ("将来时",))

    def rewrite_docs_line(text):
        lines = text.splitlines(keepends=True)
        for index, line in enumerate(lines):
            if line.strip().startswith("- `contracts/`"):
                lines[index] = line.rstrip("\n") + "（字段表待后续冻结后再定）\n"
                return "".join(lines)
        raise AssertionError("docs/README.md 缺少 `contracts/` 行")
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_text(root / DOCS_README_REL, rewrite_docs_line)
        expect_failure(root, "负例 35（docs/README 导览行）", ("将来时",))


def run_positive(root):
    failures = collect_failures(root)
    if failures:
        for item in failures:
            print(f"FAIL 正向门禁：{item}")
        return False
    print("PASS 正向门禁：契约正文、注册表、指针文档与 spine 一致")
    return True


def run_negatives():
    tests = [value for name, value in sorted(globals().items()) if name.startswith("test_negative_")]
    if len(tests) < MIN_NEGATIVE_TESTS:
        print(
            f"FAIL 负例套件只收集到 {len(tests)} 组，少于登记下限 {MIN_NEGATIVE_TESTS}："
            "守卫可能被删减或改名，负例组数是本门禁的交付判据之一"
        )
        return False, len(tests)
    passed = 0
    for test in tests:
        try:
            test()
        except AssertionError as error:
            print(f"FAIL {test.__name__}: {error}")
            continue
        except (KeyError, IndexError, TypeError, AttributeError, ValueError, StopIteration) as error:
            # 负例自身的构造改动失败时（例如目标字段/样例已改名），不得让异常中断整轮：
            # 那会丢掉剩余负例的结论，只留下 traceback。
            print(
                f"FAIL {test.__name__}: 负例无法构造（{type(error).__name__}: {error}），"
                f"该组未产生结论"
            )
            continue
        passed += 1
        print(f"PASS {test.__name__}")
    print(f"负例门禁：{passed}/{len(tests)} 通过")
    return passed == len(tests), len(tests)


def main():
    parser = argparse.ArgumentParser(description="跨层同步契约门禁测试")
    parser.add_argument(
        "--root", default=str(REPO_ROOT), help="被检查的仓库根（只读；负例仍从真实交付物取样）"
    )
    args = parser.parse_args()

    positive_ok = run_positive(Path(args.root).resolve())
    negatives_ok, total = run_negatives()
    if positive_ok and negatives_ok:
        print("sync-contract: 全部门禁通过")
        return 0
    print("sync-contract: 门禁失败")
    if not negatives_ok:
        print(f"负例门禁未全部按预期失败（共 {total} 组）")
    return 1


if __name__ == "__main__":
    sys.exit(main())
