#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Deterministic, read-only state helpers for the Epic Autopilot.

The workflow is model-driven, but these operations are intentionally boring and
machine-readable: capture a worktree snapshot, derive a story-local diff, and
validate the sprint-status document before an agent is dispatched.  The helper
never writes inside the repository except to an explicitly supplied output
directory or output file.
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

    tests_ok, test_reasons = receipt_test_state(receipt)
    reasons.extend(test_reasons)
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
        or any(marker in path.lower() for marker in TEST_MARKERS)
        for path in listed_paths
    )
    lite_eligible = (
        not reasons
        and tests_ok
        and runtime_changed is False
        and (lite_paths or change_kind in allowed_lite_kinds)
        and len(listed_paths) <= 2
        and changed_lines <= 80
    )
    if lite_eligible:
        return {"ok": True, "review_depth": "lite", "risk_reasons": ["文档/测试数据/简单配置小变更，运行时行为未变"]}
    if reasons:
        return {"ok": True, "review_depth": "deep", "risk_reasons": sorted(set(reasons))}
    if runtime_changed is False and not lite_paths and change_kind not in allowed_lite_kinds:
        reasons.append("无法证明变更仅限文档/测试数据/简单配置")
    return {
        "ok": True,
        "review_depth": "standard" if not reasons else "deep",
        "risk_reasons": sorted(set(reasons)) or ["普通业务逻辑变更"],
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
        if not isinstance(receipt["file_list"], list) or not receipt["file_list"]:
            return {"ok": False, "reason": "file-list-invalid"}
        if not isinstance(receipt["test_commands"], list) or not receipt["test_commands"]:
            return {"ok": False, "reason": "test-commands-invalid"}
        if not isinstance(receipt["test_exit_codes"], list) or not receipt["test_exit_codes"]:
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
        if receipt["build_recovery"] not in {"resolved", "not-applicable", "blocked"}:
            return {"ok": False, "reason": "build-recovery-invalid"}
        if receipt["build_recovery"] == "not-applicable":
            if build_attempts or build_exit_codes or build_log_paths or final_build_exit_code is not None:
                return {"ok": False, "reason": "non-applicable-build-receipt-not-empty"}
        elif receipt["build_recovery"] == "resolved":
            if not build_attempts or final_build_exit_code != 0 or build_exit_codes[-1] != 0:
                return {"ok": False, "reason": "build-not-resolved"}
        else:
            return {"ok": False, "reason": "build-recovery-blocked"}
        if requires_embedded_build(receipt["file_list"]) and receipt["build_recovery"] != "resolved":
            return {"ok": False, "reason": "embedded-build-required"}
    if phase == "C":
        if not isinstance(receipt["spec_file"], str) or not receipt["spec_file"].strip():
            return {"ok": False, "reason": "spec-file-invalid"}
        if not isinstance(receipt["diff_file"], str) or not receipt["diff_file"].strip():
            return {"ok": False, "reason": "diff-file-invalid"}
        if receipt["review_mode"] not in {"full", "no-spec"}:
            return {"ok": False, "reason": "review-mode-invalid"}
        if receipt["review_depth"] not in {"lite", "standard", "deep"}:
            return {"ok": False, "reason": "review-depth-invalid"}
        for field in ("active_layers", "mandatory_layers", "completed_layers", "failed_layers", "risk_reasons"):
            if not isinstance(receipt[field], list):
                return {"ok": False, "reason": f"{field}-invalid"}
        for field in ("findings", "patches", "deferred", "rejected", "unresolved_high_medium"):
            if not isinstance(receipt[field], int) or isinstance(receipt[field], bool) or receipt[field] < 0:
                return {"ok": False, "reason": f"{field}-invalid"}
        if receipt["outcome"] not in {"clean", "autofixed", "review", "blocked"}:
            return {"ok": False, "reason": "review-outcome-invalid"}
        if not isinstance(receipt["test_command"], str) or not receipt["test_command"].strip():
            return {"ok": False, "reason": "test-command-invalid"}
        if receipt["test_exit_code"] is not None and (
            not isinstance(receipt["test_exit_code"], int)
            or isinstance(receipt["test_exit_code"], bool)
        ):
            return {"ok": False, "reason": "test-exit-code-invalid"}
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
