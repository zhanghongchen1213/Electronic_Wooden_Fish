#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Deterministic state and recovery helpers for the Epic Autopilot.

The workflow is model-driven, but these operations are intentionally boring and
machine-readable: capture a worktree snapshot, derive a story-local diff,
repair mechanically recoverable evidence, and validate the sprint-status
document before an agent is dispatched.  Writes are limited to explicitly
supplied output files and recovery backups.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path


ALLOWED_STATUSES = {
    "backlog",
    "ready-for-dev",
    "in-progress",
    "review",
    "done",
    "blocked",
}

QUALITY_STATES = {"clean", "degraded", "unverified"}
TEST_STATES = {"passed", "failed", "missing", "not-run"}
BUILD_STATES = {"passed", "failed", "not-applicable", "degraded"}
REVIEW_OUTCOMES = {"clean", "autofixed", "degraded", "unverified", "review", "blocked"}
REPAIR_STAGES = {
    "normal",
    "repairing-implementation",
    "repairing-build",
    "repairing-evidence",
    "repairing-review",
    "degraded-complete",
}

HIGH_RISK_PATTERNS = (
    (re.compile(r"\b(auth|authentication|authorization|permission|token|secret|password|security)\b|认证|权限|密钥|令牌|安全", re.I), "认证/权限/安全"),
    (re.compile(r"\b(api|schema|protocol|serialization|serialize|deserializ|network|http|websocket|grpc)\b|接口|协议|序列化|网络", re.I), "API/协议/网络契约"),
    (re.compile(r"\b(persist|persistence|migration|storage|database|transaction|atomic)\b|持久化|迁移|存储|事务", re.I), "持久化/迁移/数据兼容"),
    (re.compile(r"\b(concurr|async|await|thread|race|retry|state.?machine|timer|timeout)\w*\b|并发|异步|重试|状态机|定时器", re.I), "并发/异步/重试/状态机"),
    (re.compile(r"\b(hardware|gpio|power|ota|firmware|driver|embedded|esp32|lvgl)\b|硬件|GPIO|电源|固件|驱动|OTA", re.I), "硬件/电源/OTA/固件"),
    (re.compile(r"\b(build|dependency|dependencies|package|lockfile|ci|generated|codegen)\b|构建|依赖|生成代码|CI", re.I), "构建/依赖/生成代码"),
)
DOC_SUFFIXES = {".md", ".mdx", ".txt", ".rst", ".adoc"}
CONFIG_SUFFIXES = {".toml", ".yaml", ".yml", ".ini", ".cfg", ".conf", ".json"}
TEST_MARKERS = ("test", "spec", "fixture", "mock", "sample", "testdata", "测试", "样例")


def emit(payload: dict, code: int = 0) -> int:
    print(json.dumps(payload, ensure_ascii=False, sort_keys=True))
    return code


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValueError("receipt root must be a mapping")
    return value


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def derive_file_list(diff_path: Path) -> list[str]:
    content = diff_path.read_text(encoding="utf-8")
    paths, _, _, _ = diff_paths_and_stats(content)
    return sorted(paths)


def repair_receipt(path: Path, phase: str, story_key: str, output: Path, diff_path: Path | None) -> dict:
    """Repair only mechanically recoverable receipt fields.

    The repaired receipt is deliberately marked ``ok=false`` when evidence is
    incomplete.  The orchestrator must rerun the affected phase before it can
    accept the receipt; this command can never manufacture a successful phase.
    """
    original = None
    if path.is_file():
        original = path.with_name(path.name + ".repair-backup")
        if not original.exists():
            shutil.copy2(path, original)
    try:
        value = load_json(path) if path.is_file() else {}
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError):
        value = {}
    value["story_key"] = story_key
    value["phase"] = phase
    if diff_path and diff_path.is_file():
        try:
            derived = derive_file_list(diff_path)
        except (OSError, UnicodeError):
            derived = []
        if derived:
            value["file_list"] = derived
            value["diff_file"] = str(diff_path.resolve())
    if phase == "B":
        value.setdefault("story_path", "")
        value.setdefault("test_commands", [])
        value.setdefault("test_exit_codes", [])
        value.setdefault("build_attempts", [])
        value.setdefault("build_exit_codes", [])
        value.setdefault("build_log_paths", [])
        value.setdefault("final_build_exit_code", None)
        value.setdefault("build_recovery", "not-applicable")
        value.setdefault("change_kind", "runtime")
        value.setdefault("runtime_behavior_changed", True)
        value.setdefault("status", "review")
        value.setdefault("implementation_complete", False)
    elif phase == "C":
        value.setdefault("spec_file", "")
        value.setdefault("review_mode", "full")
        value.setdefault("review_depth", "deep")
        value.setdefault("risk_reasons", [])
        value.setdefault("active_layers", [])
        value.setdefault("mandatory_layers", [])
        value.setdefault("completed_layers", [])
        value.setdefault("failed_layers", [])
        for field in ("findings", "patches", "deferred", "rejected", "unresolved_high_medium", "unresolved_catastrophic"):
            value.setdefault(field, 0)
        value.setdefault("test_command", "")
        value.setdefault("test_exit_code", None)
        value.setdefault("outcome", "unverified")
        value.setdefault("failure_reason", "receipt-repaired-rerun-required")
        value.setdefault("status", "review")
    value.setdefault("quality_state", "unverified")
    value.setdefault("quality_debt", [{"category": "receipt-repaired", "blocking": False}])
    value["repair_stage"] = "repairing-evidence"
    value["ok"] = False
    write_json(output, value)
    return {
        "ok": True,
        "repaired": True,
        "receipt_file": str(output.resolve()),
        "backup_file": str(original.resolve()) if original else None,
        "rerun_required": True,
    }


def _story_status_from_file(path: Path) -> str | None:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        return None
    match = re.search(r"(?im)^status:\s*['\"]?([a-z-]+)", text)
    status = match.group(1) if match else None
    return status if status in ALLOWED_STATUSES else None


def _done_receipt_exists(artifacts: Path, key: str) -> bool:
    """Only trust a story's ``done`` claim when a valid C receipt supports it."""
    root = artifacts / ".autopilot"
    candidates = sorted(
        set(root.glob(f"*/{key}/attempt-*/code-review.json"))
        | set(root.glob(f"*/{key}/attempt-*/review.json"))
        | set(root.glob(f"*/{key}/attempt-*/*review*.json"))
    )
    for receipt in reversed(candidates):
        try:
            value = load_json(receipt)
        except Exception:
            continue
        if (
            value.get("story_key") == key
            and value.get("phase") == "C"
            and value.get("status") == "done"
            and value.get("ok") is True
            and value.get("outcome") in {"clean", "autofixed", "degraded", "unverified"}
            and value.get("unresolved_catastrophic", 0) == 0
        ):
            return True
    return False


def repair_status(path: Path, artifacts: Path, epic: str | None = None) -> dict:
    """Recover a malformed sprint-status from story files and receipts.

    Existing valid statuses win.  Recovered values are conservative and never
    promote a story to ``done`` without a valid C receipt.
    """
    backup = None
    if path.is_file():
        backup = path.with_name(path.name + ".repair-backup.yaml")
        if not backup.exists():
            shutil.copy2(path, backup)
    existing: dict = {}
    try:
        from ruamel.yaml import YAML

        yaml = YAML(typ="safe")
        if path.is_file():
            with path.open("r", encoding="utf-8") as handle:
                loaded = yaml.load(handle)
            if isinstance(loaded, dict):
                existing = loaded
    except Exception:
        existing = {}

    development = existing.get("development_status")
    if not isinstance(development, dict):
        development = {}
    recovered = 0
    stories = sorted(artifacts.glob("*.md"))
    for story in stories:
        key = story.stem
        if not re.match(r"^\d+-\d+-", key):
            continue
        if epic and not key.startswith(f"{epic}-"):
            continue
        current = development.get(key)
        if current in ALLOWED_STATUSES:
            continue
        status = _story_status_from_file(story)
        if status == "done" and not _done_receipt_exists(artifacts, key):
            status = "review"
        if status is None:
            attempts = sorted((artifacts / ".autopilot").glob(f"*/{key}/attempt-*/*-story.json"))
            attempts += sorted((artifacts / ".autopilot").glob(f"*/{key}/attempt-*/code-review.json"))
            for receipt in reversed(attempts):
                try:
                    candidate = load_json(receipt).get("status")
                except Exception:
                    candidate = None
                if candidate in ALLOWED_STATUSES:
                    status = candidate
                    break
        development[key] = status or "backlog"
        recovered += 1

    # Also repair malformed story entries that have no corresponding markdown
    # file in the current scan.  Keep epic/retrospective metadata untouched.
    for key, current in list(development.items()):
        if not isinstance(key, str) or not re.match(r"^\d+-\d+-", key):
            continue
        if current in ALLOWED_STATUSES:
            continue
        story = artifacts / f"{key}.md"
        status = _story_status_from_file(story) if story.is_file() else None
        if status == "done" and not _done_receipt_exists(artifacts, key):
            status = "review"
        if status is None:
            status = "backlog"
        development[key] = status
        recovered += 1

    for key, current in list(development.items()):
        if current == "contexted":
            development[key] = "in-progress"
            recovered += 1
            continue
        if not isinstance(key, str) or not re.match(r"^epic-\d+$", key):
            continue
        if current in ALLOWED_STATUSES:
            continue
        prefix = key.removeprefix("epic-") + "-"
        story_statuses = [
            value for story_key, value in development.items()
            if isinstance(story_key, str) and story_key.startswith(prefix)
            and re.match(r"^\d+-\d+-", story_key)
            and value in ALLOWED_STATUSES
        ]
        development[key] = "done" if story_statuses and all(value == "done" for value in story_statuses) else (
            "in-progress" if story_statuses else "backlog"
        )
        recovered += 1

    existing["development_status"] = development
    from ruamel.yaml import YAML

    yaml = YAML()
    yaml.default_flow_style = False
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        yaml.dump(existing, handle)
    return {
        "ok": True,
        "recovered_stories": recovered,
        "status_file": str(path.resolve()),
        "backup_file": str(backup.resolve()) if backup else None,
    }


def error_fingerprint(*values: str) -> str:
    normalized = "\n".join(value.strip() for value in values if value and value.strip())
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()[:16]


def diff_paths_and_stats(content: str) -> tuple[set[str], set[str], int, int]:
    paths: set[str] = set()
    deleted: set[str] = set()
    additions = removals = 0
    pending_old: str | None = None
    for line in content.splitlines():
        if line.startswith("diff --git a/"):
            match = re.match(r"diff --git a/(.+) b/(.+)$", line)
            if match:
                paths.add(match.group(1))
            continue
        if line.startswith("--- ") and (line[4:].startswith("a/") or line[4:].startswith("/dev/null")):
            value = line[4:].split("\t", 1)[0]
            pending_old = value.removeprefix("a/")
            continue
        if line.startswith("+++ ") and (line[4:].startswith("b/") or line[4:].startswith("/dev/null")):
            value = line[4:].split("\t", 1)[0]
            new_path = value.removeprefix("b/")
            if pending_old and pending_old != "/dev/null":
                paths.add(pending_old)
            if new_path != "/dev/null":
                paths.add(new_path)
            elif pending_old and pending_old != "/dev/null":
                deleted.add(pending_old)
            pending_old = None
            continue
        if line.startswith("+") and not line.startswith("+++"):
            additions += 1
        elif line.startswith("-") and not line.startswith("---"):
            removals += 1
    return paths, deleted, additions, removals


def receipt_test_state(receipt: dict) -> tuple[bool, list[str]]:
    reasons: list[str] = []
    commands = receipt.get("test_commands")
    if not commands:
        command = receipt.get("test_command")
        commands = [command] if command else []
    if not commands or any(not isinstance(item, str) or not item.strip() for item in commands):
        reasons.append("测试命令缺失")

    codes = receipt.get("test_exit_codes")
    if codes is None and "test_exit_code" in receipt:
        codes = [receipt["test_exit_code"]]
    if isinstance(codes, dict):
        codes = list(codes.values())
    elif not isinstance(codes, list):
        codes = [] if codes is None else [codes]
    if not codes or any(item != 0 for item in codes):
        reasons.append("测试未能以退出码 0 完成")
    return not reasons, reasons


def quality_state(receipt: dict) -> tuple[str, list[str]]:
    """Return a truthful, non-blocking quality state for a phase receipt."""
    explicit = receipt.get("quality_state")
    if isinstance(explicit, str) and explicit in QUALITY_STATES:
        return explicit, []
    _, reasons = receipt_test_state(receipt)
    if reasons:
        missing = any("缺失" in reason for reason in reasons)
        return ("unverified" if missing else "degraded"), reasons
    return "clean", []


def classify_risk(diff_path: Path, receipt_path: Path) -> dict:
    if not diff_path.is_file() or not receipt_path.is_file():
        return {
            "ok": False,
            "review_depth": "deep",
            "risk_reasons": ["diff 或 dev receipt 缺失"],
        }
    try:
        content = diff_path.read_text(encoding="utf-8")
        receipt = load_json(receipt_path)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as exc:
        return {"ok": False, "review_depth": "deep", "risk_reasons": [f"风险输入不可读: {exc}"]}
    if not content.strip():
        return {"ok": False, "review_depth": "deep", "risk_reasons": ["story-local diff 为空"]}

    diff_paths, deleted, additions, removals = diff_paths_and_stats(content)
    listed = receipt.get("file_list")
    reasons: list[str] = []
    if not isinstance(listed, list) or not listed or any(not isinstance(item, str) for item in listed):
        return {"ok": False, "review_depth": "deep", "risk_reasons": ["dev receipt 缺少有效 File List"]}
    else:
        listed_paths = {item.removeprefix("./") for item in listed}
        if not diff_paths:
            return {"ok": False, "review_depth": "deep", "risk_reasons": ["无法从 diff 解析 changed paths"]}
        if diff_paths != listed_paths:
            return {"ok": False, "review_depth": "deep", "risk_reasons": ["diff 与 File List 不一致"]}

    _, test_reasons = receipt_test_state(receipt)
    quality_reasons = list(test_reasons)
    text_for_signals = f"{content}\n" + "\n".join(listed_paths)
    for pattern, reason in HIGH_RISK_PATTERNS:
        if pattern.search(text_for_signals):
            reasons.append(reason)
    if re.search(r"\bjson\b", text_for_signals, re.I) and re.search(
        r"\b(state|record|data|storage|persist|database|transaction|migration)\b|状态|记录|数据|存储|持久化|迁移",
        text_for_signals,
        re.I,
    ):
        reasons.append("JSON 持久化/数据兼容")

    changed_lines = additions + removals
    production_paths = {
        path for path in listed_paths
        if Path(path).suffix.lower() not in DOC_SUFFIXES
        and not any(marker in path.lower() for marker in TEST_MARKERS)
        and Path(path).suffix.lower() not in CONFIG_SUFFIXES
    }
    if deleted:
        reasons.append("删除已有文件/行为")
    if len(production_paths) > 5:
        reasons.append("生产文件超过 5 个")
    if changed_lines > 300:
        reasons.append("变更超过约 300 行")

    runtime_changed = receipt.get("runtime_behavior_changed")
    change_kind = receipt.get("change_kind")
    allowed_lite_kinds = {"docs", "comments", "format", "test-data", "simple-config", "constant"}
    if runtime_changed not in (True, False):
        reasons.append("runtime_behavior_changed 缺失或无效")
    if change_kind is not None and change_kind not in allowed_lite_kinds | {"runtime", "ui", "service", "test"}:
        reasons.append("change_kind 无法识别")
    lite_paths = listed_paths and all(
        Path(path).suffix.lower() in DOC_SUFFIXES
        or Path(path).suffix.lower() in CONFIG_SUFFIXES
        for path in listed_paths
    )
    lite_eligible = (
        not reasons
        and runtime_changed is False
        and (lite_paths or change_kind in allowed_lite_kinds)
        and len(listed_paths) <= 2
        and changed_lines <= 80
    )
    if lite_eligible:
        return {
            "ok": True,
            "review_depth": "lite",
            "risk_reasons": ["文档/测试数据/简单配置小变更，运行时行为未变"],
            "quality_debt_reasons": sorted(set(quality_reasons)),
        }
    if reasons:
        return {
            "ok": True,
            "review_depth": "deep",
            "risk_reasons": sorted(set(reasons)),
            "quality_debt_reasons": sorted(set(quality_reasons)),
        }
    if runtime_changed is False and not lite_paths and change_kind not in allowed_lite_kinds | {"test"}:
        reasons.append("无法证明变更仅限文档/测试数据/简单配置")
    return {
        "ok": True,
        "review_depth": "standard" if not reasons else "deep",
        "risk_reasons": sorted(set(reasons)) or ["普通业务逻辑变更"],
        "quality_debt_reasons": sorted(set(quality_reasons)),
    }


RECEIPT_FIELDS = {
    "A": {"story_key", "story_path", "status", "phase", "ok"},
    "B": {
        "story_key", "story_path", "phase", "file_list", "test_commands",
        "test_exit_codes", "change_kind", "runtime_behavior_changed", "build_attempts",
        "build_exit_codes", "build_log_paths", "final_build_exit_code", "build_recovery",
        "status", "ok",
    },
    "C": {
        "story_key", "spec_file", "diff_file", "review_mode", "phase", "review_depth", "risk_reasons", "active_layers",
        "mandatory_layers", "completed_layers", "failed_layers", "findings", "patches",
        "deferred", "rejected", "unresolved_high_medium", "test_command",
        "test_exit_code", "outcome", "failure_reason", "status", "ok",
    },
}


def requires_embedded_build(file_list: list[object]) -> bool:
    """Return whether the changed production paths require the ESP-IDF build gate."""
    for item in file_list:
        if not isinstance(item, str):
            continue
        path = item.replace("\\", "/").removeprefix("./").lower()
        if path.startswith("embedded/"):
            return True
    return False


def validate_receipt(path: Path, phase: str, story_key: str, expected_status: str | None) -> dict:
    if phase not in RECEIPT_FIELDS:
        return {"ok": False, "reason": "unknown-receipt-phase", "phase": phase}
    if not path.is_file():
        return {"ok": False, "reason": "receipt-missing", "path": str(path)}
    try:
        receipt = load_json(path)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as exc:
        return {"ok": False, "reason": "receipt-invalid-json", "error": str(exc)}
    missing = sorted(RECEIPT_FIELDS[phase] - set(receipt))
    if missing:
        return {"ok": False, "reason": "receipt-fields-missing", "fields": missing}
    if receipt.get("story_key") != story_key:
        return {"ok": False, "reason": "receipt-story-mismatch"}
    if receipt.get("phase") != phase:
        return {"ok": False, "reason": "receipt-phase-mismatch"}
    status = receipt.get("status")
    if status not in ALLOWED_STATUSES:
        return {"ok": False, "reason": "receipt-status-invalid", "status": status}
    if expected_status is not None and status != expected_status:
        return {"ok": False, "reason": "receipt-status-unexpected", "status": status}
    if receipt.get("ok") is not True:
        return {"ok": False, "reason": "receipt-not-ok"}
    if phase == "A" and status != "ready-for-dev":
        return {"ok": False, "reason": "create-story-status-invalid", "status": status}
    if phase == "B":
        if not isinstance(receipt["file_list"], list):
            return {"ok": False, "reason": "file-list-invalid"}
        if not receipt["file_list"]:
            if receipt.get("change_kind") != "none" or receipt.get("implementation_complete") is not True:
                return {"ok": False, "reason": "file-list-invalid"}
        if not isinstance(receipt["test_commands"], list):
            return {"ok": False, "reason": "test-commands-invalid"}
        if not isinstance(receipt["test_exit_codes"], list):
            return {"ok": False, "reason": "test-exit-codes-invalid"}
        if any(not isinstance(code, int) or isinstance(code, bool) for code in receipt["test_exit_codes"]):
            return {"ok": False, "reason": "test-exit-codes-invalid"}
        if not isinstance(receipt["runtime_behavior_changed"], bool):
            return {"ok": False, "reason": "runtime-behavior-flag-invalid"}
        build_attempts = receipt["build_attempts"]
        build_exit_codes = receipt["build_exit_codes"]
        build_log_paths = receipt["build_log_paths"]
        if not isinstance(build_attempts, list) or not isinstance(build_exit_codes, list) or not isinstance(build_log_paths, list):
            return {"ok": False, "reason": "build-receipt-arrays-invalid"}
        if len(build_attempts) > 5:
            return {"ok": False, "reason": "build-attempt-limit-exceeded"}
        if len(build_attempts) != len(build_exit_codes) or len(build_attempts) != len(build_log_paths):
            return {"ok": False, "reason": "build-receipt-length-mismatch"}
        if any(not isinstance(code, int) or isinstance(code, bool) for code in build_exit_codes):
            return {"ok": False, "reason": "build-exit-codes-invalid"}
        if any(not isinstance(path, str) or not path.strip() or not Path(path).is_absolute() for path in build_log_paths):
            return {"ok": False, "reason": "build-log-paths-invalid"}
        record_codes: list[int] = []
        record_paths: list[str] = []
        record_numbers: list[int] = []
        for record in build_attempts:
            if not isinstance(record, dict):
                return {"ok": False, "reason": "build-attempt-invalid"}
            attempt = record.get("attempt")
            code = record.get("exit_code")
            log_path = record.get("log_path")
            if not isinstance(attempt, int) or isinstance(attempt, bool) or attempt < 1:
                return {"ok": False, "reason": "build-attempt-number-invalid"}
            if not isinstance(code, int) or isinstance(code, bool):
                return {"ok": False, "reason": "build-attempt-exit-code-invalid"}
            if not isinstance(log_path, str) or not log_path.strip() or not Path(log_path).is_absolute():
                return {"ok": False, "reason": "build-attempt-log-path-invalid"}
            if not isinstance(record.get("diagnosis"), str) or not record["diagnosis"].strip():
                return {"ok": False, "reason": "build-attempt-diagnosis-invalid"}
            if not isinstance(record.get("fix"), str) or not record["fix"].strip():
                return {"ok": False, "reason": "build-attempt-fix-invalid"}
            record_numbers.append(attempt)
            record_codes.append(code)
            record_paths.append(log_path)
        if record_numbers != list(range(1, len(build_attempts) + 1)):
            return {"ok": False, "reason": "build-attempt-number-sequence-invalid"}
        if record_codes != build_exit_codes or record_paths != build_log_paths:
            return {"ok": False, "reason": "build-attempt-summary-mismatch"}
        final_build_exit_code = receipt["final_build_exit_code"]
        if final_build_exit_code is not None and (
            not isinstance(final_build_exit_code, int) or isinstance(final_build_exit_code, bool)
        ):
            return {"ok": False, "reason": "final-build-exit-code-invalid"}
        if receipt["build_recovery"] not in {"resolved", "not-applicable", "degraded", "blocked"}:
            return {"ok": False, "reason": "build-recovery-invalid"}
        if receipt["build_recovery"] == "not-applicable":
            if build_attempts or build_exit_codes or build_log_paths or final_build_exit_code is not None:
                return {"ok": False, "reason": "non-applicable-build-receipt-not-empty"}
        elif receipt["build_recovery"] == "resolved":
            if not build_attempts or final_build_exit_code != 0 or build_exit_codes[-1] != 0:
                return {"ok": False, "reason": "build-not-resolved"}
        elif receipt["build_recovery"] == "degraded":
            if (
                receipt.get("build_status") != "degraded"
                or receipt.get("safety_degraded") is not True
                or not isinstance(receipt.get("disabled_capabilities"), list)
                or not receipt.get("disabled_capabilities")
            ):
                return {"ok": False, "reason": "build-degradation-unsupported"}
        else:
            return {"ok": False, "reason": "build-recovery-blocked"}
        if requires_embedded_build(receipt["file_list"]) and receipt["build_recovery"] == "not-applicable":
            return {"ok": False, "reason": "embedded-build-required"}
        if "implementation_complete" in receipt and receipt["implementation_complete"] is not True:
            return {"ok": False, "reason": "implementation-incomplete"}
    if phase == "C":
        if not isinstance(receipt["spec_file"], str):
            return {"ok": False, "reason": "spec-file-invalid"}
        if not isinstance(receipt["diff_file"], str) or not receipt["diff_file"].strip():
            return {"ok": False, "reason": "diff-file-invalid"}
        if receipt["review_mode"] not in {"full", "no-spec"}:
            return {"ok": False, "reason": "review-mode-invalid"}
        if receipt["review_mode"] == "full" and not receipt["spec_file"].strip():
            return {"ok": False, "reason": "spec-file-invalid"}
        if receipt["review_mode"] == "no-spec" and receipt["spec_file"].strip():
            return {"ok": False, "reason": "review-mode-spec-mismatch"}
        if receipt["review_depth"] not in {"lite", "standard", "deep"}:
            return {"ok": False, "reason": "review-depth-invalid"}
        for field in ("active_layers", "mandatory_layers", "completed_layers", "failed_layers", "risk_reasons"):
            if not isinstance(receipt[field], list):
                return {"ok": False, "reason": f"{field}-invalid"}
        for field in ("findings", "patches", "deferred", "rejected", "unresolved_high_medium"):
            if not isinstance(receipt[field], int) or isinstance(receipt[field], bool) or receipt[field] < 0:
                return {"ok": False, "reason": f"{field}-invalid"}
        if receipt["outcome"] not in REVIEW_OUTCOMES:
            return {"ok": False, "reason": "review-outcome-invalid"}
        if not isinstance(receipt["test_command"], str):
            return {"ok": False, "reason": "test-command-invalid"}
        if receipt["test_exit_code"] is not None and (
            not isinstance(receipt["test_exit_code"], int)
            or isinstance(receipt["test_exit_code"], bool)
        ):
            return {"ok": False, "reason": "test-exit-code-invalid"}
    optional_quality = {
        "quality_state": QUALITY_STATES,
        "test_status": TEST_STATES,
        "build_status": BUILD_STATES,
        "repair_stage": REPAIR_STAGES,
    }
    for field, allowed in optional_quality.items():
        if field in receipt and receipt[field] not in allowed:
            return {"ok": False, "reason": f"{field}-invalid"}
    if "quality_debt" in receipt and not isinstance(receipt["quality_debt"], list):
        return {"ok": False, "reason": "quality-debt-invalid"}
    if "quality_debt_file" in receipt and (
        not isinstance(receipt["quality_debt_file"], str)
        or not receipt["quality_debt_file"].strip()
        or not Path(receipt["quality_debt_file"]).is_absolute()
    ):
        return {"ok": False, "reason": "quality-debt-file-invalid"}
    if "disabled_capabilities" in receipt and not isinstance(receipt["disabled_capabilities"], list):
        return {"ok": False, "reason": "disabled-capabilities-invalid"}
    if "safety_degraded" in receipt and not isinstance(receipt["safety_degraded"], bool):
        return {"ok": False, "reason": "safety-degraded-invalid"}
    if "unresolved_catastrophic" in receipt and (
        not isinstance(receipt["unresolved_catastrophic"], int)
        or isinstance(receipt["unresolved_catastrophic"], bool)
        or receipt["unresolved_catastrophic"] < 0
    ):
        return {"ok": False, "reason": "unresolved-catastrophic-invalid"}
    if "implementation_complete" in receipt and not isinstance(receipt["implementation_complete"], bool):
        return {"ok": False, "reason": "implementation-complete-invalid"}
    if "no_code_change" in receipt and not isinstance(receipt["no_code_change"], bool):
        return {"ok": False, "reason": "no-code-change-invalid"}
    return {"ok": True, "phase": phase, "status": status, "receipt_file": str(path.resolve())}


def repo_files(root: Path) -> list[Path]:
    """Return tracked plus untracked (but not ignored) files in stable order."""

    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-co", "--exclude-standard", "-z"],
        check=True,
        capture_output=True,
    )
    paths = [item for item in result.stdout.decode("utf-8").split("\0") if item]
    return [root / item for item in sorted(paths)]


def copy_worktree(root: Path, target: Path) -> int:
    target.mkdir(parents=True, exist_ok=False)
    count = 0
    for source in repo_files(root):
        if not source.is_file():
            continue
        relative = source.relative_to(root)
        destination = target / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        count += 1
    return count


def file_bytes(path: Path | None) -> bytes | None:
    if path is None:
        return None
    try:
        return path.read_bytes() if path.is_file() else None
    except OSError:
        return None


def unified_diff(before: bytes | None, after: bytes | None, relative: str) -> str:
    if before == after:
        return ""
    missing_before = before is None
    missing_after = after is None
    if before is None:
        before = b""
    if after is None:
        after = b""
    if b"\0" in before or b"\0" in after:
        return (
            f"diff --git a/{relative} b/{relative}\n"
            f"Binary files a/{relative} and b/{relative} differ\n"
        )

    import difflib

    old = before.decode("utf-8", errors="replace").splitlines(keepends=True)
    new = after.decode("utf-8", errors="replace").splitlines(keepends=True)
    fromfile = "/dev/null" if missing_before else f"a/{relative}"
    tofile = "/dev/null" if missing_after else f"b/{relative}"
    parts: list[str] = []
    for line in difflib.unified_diff(
        old,
        new,
        fromfile=fromfile,
        tofile=tofile,
        lineterm="\n",
    ):
        if line.endswith("\n"):
            parts.append(line)
        else:
            # difflib 不标注缺失的行尾换行。不补这个标记，该行就会与随后的 hunk 头或
            # 下一个文件头粘连，使生成的 diff 不是可应用补丁（git apply 报 corrupt patch）。
            parts.append(f"{line}\n\\ No newline at end of file\n")
    result = "".join(parts)
    if not result and (missing_before or missing_after):
        return f"--- {fromfile}\n+++ {tofile}\n"
    return result


def normalize_excludes(root: Path, excludes: list[str]) -> set[str]:
    normalized = set()
    for value in excludes:
        candidate = Path(value)
        if candidate.is_absolute():
            try:
                candidate = candidate.resolve().relative_to(root.resolve())
            except ValueError:
                continue
        normalized.add(candidate.as_posix().strip("/"))
    return {item for item in normalized if item}


def is_excluded(relative: str, excludes: set[str]) -> bool:
    return any(relative == item or relative.startswith(f"{item}/") for item in excludes)


def build_diff(before_dir: Path, root: Path, output: Path, excludes: list[str]) -> dict:
    before_files = {
        path.relative_to(before_dir).as_posix(): path
        for path in before_dir.rglob("*")
        if path.is_file()
    }
    after_files = {
        path.relative_to(root).as_posix(): path
        for path in repo_files(root)
        if path.is_file()
    }
    ignored = normalize_excludes(root, excludes)
    paths = sorted(
        path for path in (set(before_files) | set(after_files)) if not is_excluded(path, ignored)
    )
    chunks: list[str] = []
    changed: list[str] = []
    added = removed = 0
    for relative in paths:
        before = file_bytes(before_files.get(relative))
        after = file_bytes(after_files.get(relative))
        chunk = unified_diff(before, after, relative)
        if not chunk:
            continue
        chunks.append(chunk)
        changed.append(relative)
        if before is None:
            added += 1
        elif after is None:
            removed += 1

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("".join(chunks), encoding="utf-8")
    content = output.read_bytes()
    return {
        "ok": True,
        "diff_file": str(output.resolve()),
        "changed_paths": changed,
        "added_files": added,
        "removed_files": removed,
        "diff_bytes": len(content),
        "diff_sha256": hashlib.sha256(content).hexdigest(),
        "changed_file_count": len(changed),
    }


def load_yaml(path: Path) -> dict:
    try:
        from ruamel.yaml import YAML
    except ImportError as exc:  # pragma: no cover - environment diagnostic
        raise RuntimeError(f"ruamel.yaml unavailable: {exc}") from exc
    yaml = YAML(typ="safe")
    with path.open("r", encoding="utf-8") as handle:
        data = yaml.load(handle)
    if not isinstance(data, dict):
        raise ValueError("sprint-status root must be a mapping")
    development = data.get("development_status")
    if not isinstance(development, dict):
        raise ValueError("development_status must be a mapping")
    return development


def validate_status(path: Path, epic: str | None = None) -> dict:
    if not path.is_file():
        return {"ok": False, "reason": "sprint-status-missing", "path": str(path)}
    try:
        development = load_yaml(path)
    except Exception as exc:
        return {"ok": False, "reason": "sprint-status-invalid", "error": str(exc)}

    rows = []
    for key, status in development.items():
        if not isinstance(key, str) or not isinstance(status, str):
            return {"ok": False, "reason": "story-status-invalid", "key": str(key)}
        if status not in ALLOWED_STATUSES and not key.endswith("-retrospective"):
            return {"ok": False, "reason": "unknown-status", "key": key, "status": status}
        if epic is None or key.startswith(f"{epic}-") or key == f"epic-{epic}":
            rows.append({"key": key, "status": status})
    return {"ok": True, "stories": rows}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    snapshot = subparsers.add_parser("snapshot")
    snapshot.add_argument("--root", required=True, type=Path)
    snapshot.add_argument("--out", required=True, type=Path)

    diff = subparsers.add_parser("diff")
    diff.add_argument("--before", required=True, type=Path)
    diff.add_argument("--root", required=True, type=Path)
    diff.add_argument("--out", required=True, type=Path)
    diff.add_argument("--exclude", action="append", default=[])

    classify = subparsers.add_parser("classify-risk")
    classify.add_argument("--diff", required=True, type=Path)
    classify.add_argument("--dev-receipt", required=True, type=Path)

    repair_receipt_parser = subparsers.add_parser("repair-receipt")
    repair_receipt_parser.add_argument("--file", required=True, type=Path)
    repair_receipt_parser.add_argument("--phase", required=True, choices=sorted(RECEIPT_FIELDS))
    repair_receipt_parser.add_argument("--story-key", required=True)
    repair_receipt_parser.add_argument("--output", required=True, type=Path)
    repair_receipt_parser.add_argument("--diff", type=Path)

    repair_status_parser = subparsers.add_parser("repair-status")
    repair_status_parser.add_argument("--file", required=True, type=Path)
    repair_status_parser.add_argument("--implementation-artifacts", required=True, type=Path)
    repair_status_parser.add_argument("--epic")

    fingerprint = subparsers.add_parser("fingerprint")
    fingerprint.add_argument("values", nargs="*")

    receipt = subparsers.add_parser("validate-receipt")
    receipt.add_argument("--file", required=True, type=Path)
    receipt.add_argument("--phase", required=True, choices=sorted(RECEIPT_FIELDS))
    receipt.add_argument("--story-key", required=True)
    receipt.add_argument("--expected-status")

    status = subparsers.add_parser("validate-status")
    status.add_argument("--file", required=True, type=Path)
    status.add_argument("--epic")

    args = parser.parse_args(argv)
    try:
        if args.command == "snapshot":
            count = copy_worktree(args.root.resolve(), args.out.resolve())
            return emit({"ok": True, "snapshot_dir": str(args.out.resolve()), "file_count": count})
        if args.command == "diff":
            return emit(
                build_diff(
                    args.before.resolve(),
                    args.root.resolve(),
                    args.out.resolve(),
                    args.exclude,
                )
            )
        if args.command == "classify-risk":
            result = classify_risk(args.diff.resolve(), args.dev_receipt.resolve())
            return emit(result, 0 if result["ok"] else 1)
        if args.command == "repair-receipt":
            result = repair_receipt(
                args.file.resolve(), args.phase, args.story_key, args.output.resolve(),
                args.diff.resolve() if args.diff else None,
            )
            return emit(result)
        if args.command == "repair-status":
            return emit(
                repair_status(
                    args.file.resolve(), args.implementation_artifacts.resolve(), args.epic
                )
            )
        if args.command == "fingerprint":
            return emit({"ok": True, "fingerprint": error_fingerprint(*args.values)})
        if args.command == "validate-receipt":
            result = validate_receipt(
                args.file.resolve(), args.phase, args.story_key, args.expected_status
            )
            return emit(result, 0 if result["ok"] else 1)
        result = validate_status(args.file.resolve(), args.epic)
        return emit(result, 0 if result["ok"] else 1)
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        return emit({"ok": False, "reason": "helper-failed", "error": str(exc)}, 1)


if __name__ == "__main__":
    raise SystemExit(main())
