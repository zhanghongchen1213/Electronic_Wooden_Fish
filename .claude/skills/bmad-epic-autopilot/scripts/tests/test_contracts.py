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
        self.assertIn("单阶段最多 2 次尝试", text)
        self.assertIn("story-local", step)
        self.assertIn("receipt", step)
        self.assertIn("classify-risk", step)
        self.assertIn("build_recovery", step)
        self.assertIn("macos_esp_idf_hardware_test_runbook.md", phase_b)
        self.assertIn("退出码 `127`", phase_b)
        self.assertIn("最多允许 5 次尝试", phase_b)
        self.assertIn("command -v idf.py", phase_b)
        for field in ("story_key", "spec_file", "diff_file", "review_mode", "receipt_file", "test_command", "sprint_status"):
            self.assertIn(field, step)
        self.assertIn("不得内部 review→fix 循环", prompt)
        self.assertIn("risk_reasons", prompt)

    def test_code_review_depth_and_failure_gate(self):
        customize = self.read("../bmad-code-review/customize.toml")
        step = self.read("../bmad-code-review/steps/step-02-review.md")
        present = self.read("../bmad-code-review/steps/step-04-present.md")
        gather = self.read("../bmad-code-review/steps/step-01-gather-context.md")
        self.assertIn('when = \'Only when {review_depth} = "deep".\'', customize)
        self.assertIn("Zero findings is a valid result", customize)
        self.assertIn("mandatory-review-failed", step)
        self.assertIn("Never convert a failed or incomplete review into `done`", present)
        self.assertIn("unattended=true", gather)
        self.assertIn("skip the interactive target-selection cascade", gather)


if __name__ == "__main__":
    unittest.main()
