#!/usr/bin/env python3
"""Check that the project's .agents and .claude skill mirrors are identical."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


IGNORED_PARTS = {"__pycache__"}
IGNORED_NAMES = {".DS_Store"}


def files(root: Path) -> set[Path]:
    return {
        path.relative_to(root)
        for path in root.rglob("*")
        if path.is_file()
        and not (set(path.parts) & IGNORED_PARTS)
        and path.name not in IGNORED_NAMES
    }


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(project_root: Path, skills: list[str]) -> dict:
    mismatches: list[dict] = []
    for skill in skills:
        left = project_root / ".agents" / "skills" / skill
        right = project_root / ".claude" / "skills" / skill
        if not left.is_dir() or not right.is_dir():
            mismatches.append({"skill": skill, "reason": "mirror-missing"})
            continue
        left_files = files(left)
        right_files = files(right)
        if left_files != right_files:
            mismatches.append({
                "skill": skill,
                "reason": "file-set-mismatch",
                "only_agents": sorted(str(item) for item in left_files - right_files),
                "only_claude": sorted(str(item) for item in right_files - left_files),
            })
            continue
        for relative in sorted(left_files):
            if digest(left / relative) != digest(right / relative):
                mismatches.append({"skill": skill, "reason": "content-mismatch", "file": str(relative)})
    return {"ok": not mismatches, "mismatches": mismatches}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, required=True)
    parser.add_argument("skills", nargs="+", help="skill directory names")
    args = parser.parse_args()
    result = check(args.project_root.resolve(), args.skills)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
