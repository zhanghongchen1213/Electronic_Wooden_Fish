import unittest
from pathlib import Path


ROOT = Path(__file__).parents[2]


class ContractTests(unittest.TestCase):
    def read(self, relative):
        return (ROOT / relative).read_text(encoding="utf-8")

    def test_autopilot_contract_has_single_pass_and_receipts(self):
        text = self.read("SKILL.md")
        step = self.read("steps/step-02-run-one-story.md")
        phase_b = self.read("subagent-prompts/phase-b-dev-story.md")
        prompt = self.read("subagent-prompts/phase-c-code-review.md")
        self.assertIn("review_depth", text)
        self.assertIn("普通失败进入恢复层", text)
        self.assertIn("story-local", step)
        self.assertIn("receipt", step)
        self.assertIn("classify-risk", step)
        self.assertIn("build_recovery", phase_b)
        self.assertIn("macos_esp_idf_hardware_test_runbook.md", phase_b)
        self.assertIn("退出码 `127`", phase_b)
        self.assertIn("最多允许 5 次尝试", phase_b)
        self.assertIn("command -v idf.py", phase_b)
        for field in ("story_key", "spec_file", "diff_file", "review_mode", "receipt_file", "test_command", "sprint_status"):
            self.assertIn(field, step)
        self.assertIn("不得内部 review→fix 循环", prompt)
        self.assertIn("risk_reasons", prompt)
        self.assertIn("continue_on_quality_failure", text)
        self.assertIn("repairing-evidence", step)
        self.assertIn("quality_debt", phase_b)
        self.assertIn("conservative_degrade", prompt)
        self.assertIn("quality_debt_file", prompt)
        self.assertIn("repair_attempt", prompt)
        self.assertIn("no_code_change", prompt)

    def test_code_review_depth_and_failure_gate(self):
        customize = self.read("../bmad-code-review/customize.toml")
        step = self.read("../bmad-code-review/steps/step-02-review.md")
        present = self.read("../bmad-code-review/steps/step-04-present.md")
        gather = self.read("../bmad-code-review/steps/step-01-gather-context.md")
        self.assertIn('when = \'Only when {review_depth} = "deep".\'', customize)
        self.assertIn("Zero findings is a valid result", customize)
        self.assertIn("quality_debt", step)
        self.assertIn("unverified", present)
        self.assertIn("unresolved_catastrophic", present)
        self.assertIn("unattended=true", gather)
        self.assertIn("skip the interactive target-selection cascade", gather)

    def test_dev_story_has_unattended_quality_override(self):
        dev = self.read("../bmad-dev-story/SKILL.md")
        self.assertIn("completion_policy=continue_on_quality_failure", dev)
        self.assertIn("test-only subtasks may remain unchecked", dev)
        self.assertIn("repairing-implementation", dev)

    def test_mirrored_skills_are_identical_except_generated_cache(self):
        import filecmp

        repo = ROOT.parents[2]
        pairs = (
            (ROOT, repo / ".claude/skills/bmad-epic-autopilot"),
            (ROOT.parent / "bmad-code-review", repo / ".claude/skills/bmad-code-review"),
            (ROOT.parent / "bmad-dev-story", repo / ".claude/skills/bmad-dev-story"),
            (ROOT.parent / "bmad-create-story", repo / ".claude/skills/bmad-create-story"),
        )
        for left, right in pairs:
            self.assertTrue(right.is_dir(), right)
            left_files = {
                path.relative_to(left)
                for path in left.rglob("*")
                if path.is_file() and "__pycache__" not in path.parts
            }
            right_files = {
                path.relative_to(right)
                for path in right.rglob("*")
                if path.is_file() and "__pycache__" not in path.parts
            }
            self.assertEqual(left_files, right_files)
            for relative in left_files:
                self.assertTrue(filecmp.cmp(left / relative, right / relative, shallow=False), relative)


if __name__ == "__main__":
    unittest.main()
