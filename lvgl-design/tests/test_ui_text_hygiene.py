import tempfile
import unittest
from pathlib import Path

from ui_text_hygiene import validate


class UiTextHygieneTest(unittest.TestCase):
    def test_rejects_internal_copy_and_duplicate_progress(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "page.html"
            path.write_text('<div>心经进度 1 / N 字 · 0.4%</div><div>心经进度 1 / N 字 · 0.4%</div><div>qljP7</div>', encoding="utf-8")
            result = validate(path, "miniapp", "production")
            codes = {error["code"] for error in result["errors"]}
            self.assertIn("forbidden_visible_text", codes)
            self.assertIn("duplicate_progress_text", codes)

    def test_accepts_product_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "page.html"
            path.write_text('<main><p>等待设备诵读</p><p>心经进度 42 / N 字 · 16.2%</p></main>', encoding="utf-8")
            result = validate(path, "miniapp", "candidates")
            self.assertTrue(result["ok"], result)

    def test_scans_vue_template_but_ignores_script_and_style(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "reading.vue"
            path.write_text(
                "<template><view>等待设备诵读</view></template><script>const qljP7 = 1</script><style>.draft{}</style>",
                encoding="utf-8",
            )
            result = validate(path, "miniapp", "production")
            self.assertTrue(result["ok"], result)

    def test_rejects_forbidden_vue_template_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "reading.vue"
            path.write_text("<template><view>Node ID: qljP7</view></template>", encoding="utf-8")
            result = validate(path, "miniapp", "production")
            self.assertFalse(result["ok"])


if __name__ == "__main__":
    unittest.main()
