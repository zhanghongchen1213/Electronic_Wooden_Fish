import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "autopilot_state.py"


class AutopilotStateTests(unittest.TestCase):
    def run_helper(self, *args, check=True):
        completed = subprocess.run(
            ["uv", "run", "--no-cache", str(SCRIPT), *map(str, args)],
            text=True,
            capture_output=True,
            check=check,
        )
        return json.loads(completed.stdout)

    def test_snapshot_and_diff_isolate_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            root.mkdir()
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            (root / "one.txt").write_text("old\n", encoding="utf-8")
            subprocess.run(["git", "-C", str(root), "add", "one.txt"], check=True)
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(root),
                    "-c",
                    "user.name=Test",
                    "-c",
                    "user.email=test@example.com",
                    "commit",
                    "-qm",
                    "base",
                ],
                check=True,
            )

            snapshot = Path(directory) / "before"
            result = self.run_helper("snapshot", "--root", root, "--out", snapshot)
            self.assertTrue(result["ok"])
            (root / "one.txt").write_text("new\n", encoding="utf-8")
            (root / "two.txt").write_text("added\n", encoding="utf-8")
            diff_path = Path(directory) / "story.diff"
            result = self.run_helper(
                "diff", "--before", snapshot, "--root", root, "--out", diff_path
            )
            self.assertEqual(result["changed_paths"], ["one.txt", "two.txt"])
            diff = diff_path.read_text(encoding="utf-8")
            self.assertIn("old", diff)
            self.assertIn("added", diff)

    def test_diff_excludes_state_and_metadata_and_keeps_empty_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            root.mkdir()
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            (root / "tracked.txt").write_text("same\n", encoding="utf-8")
            subprocess.run(["git", "-C", str(root), "add", "tracked.txt"], check=True)
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(root),
                    "-c",
                    "user.name=Test",
                    "-c",
                    "user.email=test@example.com",
                    "commit",
                    "-qm",
                    "base",
                ],
                check=True,
            )

            snapshot = Path(directory) / "before"
            self.run_helper("snapshot", "--root", root, "--out", snapshot)
            state = root / ".autopilot" / "story"
            state.mkdir(parents=True)
            (state / "receipt.json").write_text("{}\n", encoding="utf-8")
            (root / "empty.txt").write_text("", encoding="utf-8")
            diff_path = Path(directory) / "story.diff"
            result = self.run_helper(
                "diff",
                "--before",
                snapshot,
                "--root",
                root,
                "--out",
                diff_path,
                "--exclude",
                root / ".autopilot",
            )
            self.assertEqual(result["changed_paths"], ["empty.txt"])
            self.assertIn("/dev/null", diff_path.read_text(encoding="utf-8"))

    def test_diff_marks_missing_trailing_newline_so_patch_applies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            root.mkdir()
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            (root / "no-newline.txt").write_text("tail", encoding="utf-8")
            subprocess.run(["git", "-C", str(root), "add", "no-newline.txt"], check=True)
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(root),
                    "-c",
                    "user.name=Test",
                    "-c",
                    "user.email=test@example.com",
                    "commit",
                    "-qm",
                    "base",
                ],
                check=True,
            )

            snapshot = Path(directory) / "before"
            self.run_helper("snapshot", "--root", root, "--out", snapshot)
            (root / "no-newline.txt").write_text("tail changed", encoding="utf-8")
            (root / "second.txt").write_text("second", encoding="utf-8")
            diff_path = Path(directory) / "story.diff"
            self.run_helper(
                "diff", "--before", snapshot, "--root", root, "--out", diff_path
            )

            diff = diff_path.read_text(encoding="utf-8")
            # 三个无尾换行的内容行各需一个标记：no-newline.txt 的旧行与新行、second.txt 的新行。
            self.assertEqual(diff.count("\\ No newline at end of file"), 3)
            # 缺少标记时，无尾换行的内容行会与随后的 hunk 头/文件头粘连，
            # git apply 会报 corrupt patch。反向检查等价于"这是一份可应用的补丁"。
            completed = subprocess.run(
                ["git", "-C", str(root), "apply", "--check", "--reverse", str(diff_path)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)

    def test_status_validation_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.yaml"
            result = self.run_helper("validate-status", "--file", missing, check=False)
            self.assertEqual(
                result,
                {"ok": False, "path": str(missing.resolve()), "reason": "sprint-status-missing"},
            )
            malformed = Path(directory) / "malformed.yaml"
            malformed.write_text("development_status: [", encoding="utf-8")
            result = self.run_helper("validate-status", "--file", malformed, check=False)
            self.assertEqual(result["reason"], "sprint-status-invalid")
            unknown = Path(directory) / "unknown.yaml"
            unknown.write_text("development_status:\n  1-1-demo: mystery\n", encoding="utf-8")
            result = self.run_helper("validate-status", "--file", unknown, check=False)
            self.assertEqual(result["reason"], "unknown-status")

    def test_risk_classification_is_risk_first(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            diff = root / "story.diff"
            receipt = root / "dev.json"
            receipt.write_text(
                '{"file_list":["docs/notes.md"],"test_commands":["true"],'
                '"test_exit_codes":[0],"runtime_behavior_changed":false}',
                encoding="utf-8",
            )
            diff.write_text("--- /dev/null\n+++ b/docs/notes.md\n@@ -0,0 +1 @@\n+# note\n", encoding="utf-8")
            result = self.run_helper("classify-risk", "--diff", diff, "--dev-receipt", receipt)
            self.assertEqual(result["review_depth"], "lite")

            receipt.write_text(
                '{"file_list":["src/service.py"],"test_commands":["true"],'
                '"test_exit_codes":[0],"runtime_behavior_changed":true}',
                encoding="utf-8",
            )
            diff.write_text("--- a/src/service.py\n+++ b/src/service.py\n@@ -1 +1 @@\n-old\n+new\n", encoding="utf-8")
            result = self.run_helper("classify-risk", "--diff", diff, "--dev-receipt", receipt)
            self.assertEqual(result["review_depth"], "standard")

            receipt.write_text(
                '{"file_list":["src/other.py"],"test_commands":["true"],'
                '"test_exit_codes":[0],"runtime_behavior_changed":true}',
                encoding="utf-8",
            )
            completed = subprocess.run(
                [
                    "uv", "run", "--no-cache", str(SCRIPT), "classify-risk",
                    "--diff", diff, "--dev-receipt", receipt,
                ],
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertIn("diff 与 File List 不一致", completed.stdout)

            receipt.write_text(
                '{"file_list":["src/api.py"],"test_commands":["true"],'
                '"test_exit_codes":[0],"runtime_behavior_changed":true}',
                encoding="utf-8",
            )
            diff.write_text("--- a/src/api.py\n+++ b/src/api.py\n@@ -1 +1 @@\n-old\n+api route\n", encoding="utf-8")
            result = self.run_helper("classify-risk", "--diff", diff, "--dev-receipt", receipt)
            self.assertEqual(result["review_depth"], "deep")

            receipt.write_text(
                '{"file_list":["src/api.py"],"runtime_behavior_changed":true}',
                encoding="utf-8",
            )
            result = self.run_helper("classify-risk", "--diff", diff, "--dev-receipt", receipt)
            self.assertEqual(result["review_depth"], "deep")
            self.assertTrue(result["risk_reasons"])

            receipt.write_text(
                '{"file_list":["src/api.py"],"test_commands":["true"],'
                '"test_exit_codes":[0]}',
                encoding="utf-8",
            )
            result = self.run_helper("classify-risk", "--diff", diff, "--dev-receipt", receipt)
            self.assertEqual(result["review_depth"], "deep")
            self.assertIn("runtime_behavior_changed 缺失或无效", result["risk_reasons"])

    def test_receipt_validation_is_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            receipt = Path(directory) / "dev.json"
            receipt.write_text(
                '{"story_key":"1-1-demo","story_path":"/tmp/1-1-demo.md",'
                '"phase":"B","file_list":["src/a.py"],"test_commands":["true"],'
                '"test_exit_codes":[0],"change_kind":"runtime",'
                '"runtime_behavior_changed":true,"build_attempts":[],'
                '"build_exit_codes":[],"build_log_paths":[],"final_build_exit_code":null,'
                '"build_recovery":"not-applicable","status":"review","ok":true}',
                encoding="utf-8",
            )
            result = self.run_helper(
                "validate-receipt", "--file", receipt, "--phase", "B",
                "--story-key", "1-1-demo", "--expected-status", "review",
            )
            self.assertTrue(result["ok"])

            receipt.write_text(
                '{"story_key":"1-1-demo","story_path":"/tmp/1-1-demo.md",'
                '"phase":"B","file_list":["Embedded/main/main.c"],"test_commands":["idf.py build"],'
                '"test_exit_codes":[0],"change_kind":"runtime",'
                '"runtime_behavior_changed":true,"build_attempts":['
                '{"attempt":1,"log_path":"/tmp/build-1.log","exit_code":127,'
                '"diagnosis":"idf.py 未在激活 shell 中可见","fix":"重新 source runbook 指定激活脚本"},'
                '{"attempt":2,"log_path":"/tmp/build-2.log","exit_code":0,'
                '"diagnosis":"激活后重新构建成功","fix":"无"}],'
                '"build_exit_codes":[127,0],"build_log_paths":["/tmp/build-1.log","/tmp/build-2.log"],'
                '"final_build_exit_code":0,"build_recovery":"resolved",'
                '"status":"review","ok":true}',
                encoding="utf-8",
            )
            result = self.run_helper(
                "validate-receipt", "--file", receipt, "--phase", "B",
                "--story-key", "1-1-demo", "--expected-status", "review",
            )
            self.assertTrue(result["ok"])

            receipt.write_text(
                '{"story_key":"1-1-demo","story_path":"/tmp/1-1-demo.md",'
                '"phase":"B","file_list":["Embedded/main/main.c"],"test_commands":["idf.py build"],'
                '"test_exit_codes":[0],"change_kind":"runtime",'
                '"runtime_behavior_changed":true,"build_attempts":[],"build_exit_codes":[],'
                '"build_log_paths":[],"final_build_exit_code":null,"build_recovery":"not-applicable",'
                '"status":"review","ok":true}',
                encoding="utf-8",
            )
            result = self.run_helper(
                "validate-receipt", "--file", receipt, "--phase", "B",
                "--story-key", "1-1-demo", "--expected-status", "review", check=False,
            )
            self.assertEqual(result["reason"], "embedded-build-required")

            receipt.write_text(
                '{"story_key":"1-1-demo","story_path":"/tmp/1-1-demo.md",'
                '"phase":"B","file_list":["Embedded/main/main.c"],"test_commands":["idf.py build"],'
                '"test_exit_codes":[0],"change_kind":"runtime",'
                '"runtime_behavior_changed":true,"build_attempts":[],"build_exit_codes":[],'
                '"build_log_paths":[],"final_build_exit_code":127,"build_recovery":"resolved",'
                '"status":"review","ok":true}',
                encoding="utf-8",
            )
            result = self.run_helper(
                "validate-receipt", "--file", receipt, "--phase", "B",
                "--story-key", "1-1-demo", "--expected-status", "review", check=False,
            )
            self.assertEqual(result["reason"], "build-not-resolved")
            receipt.write_text('{"story_key":"1-1-demo","phase":"B","ok":true}', encoding="utf-8")
            completed = subprocess.run(
                [
                    "uv", "run", "--no-cache", str(SCRIPT), "validate-receipt",
                    "--file", receipt, "--phase", "B", "--story-key", "1-1-demo",
                ],
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertIn("receipt-fields-missing", completed.stdout)

            receipt.write_text(
                '{"story_key":"1-1-demo","spec_file":"/tmp/1-1-demo.md",'
                '"diff_file":"/tmp/1-1-demo.diff","review_mode":"full",'
                '"phase":"C","review_depth":"lite","risk_reasons":[],'
                '"active_layers":["edge-case-hunter"],"mandatory_layers":["edge-case-hunter"],'
                '"completed_layers":["edge-case-hunter"],"failed_layers":[],"findings":0,'
                '"patches":0,"deferred":0,"rejected":0,"unresolved_high_medium":0,'
                '"test_command":"true","test_exit_code":0,"outcome":"clean",'
                '"failure_reason":null,"status":"done","ok":true}',
                encoding="utf-8",
            )
            result = self.run_helper(
                "validate-receipt", "--file", receipt, "--phase", "C", "--story-key", "1-1-demo"
            )
            self.assertTrue(result["ok"])


if __name__ == "__main__":
    unittest.main()
