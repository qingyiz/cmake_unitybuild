import ast
import unittest
from pathlib import Path


class ImportBoundaryTests(unittest.TestCase):
    def test_domain_and_application_do_not_import_adapters(self):
        source_root = Path(__file__).parents[2] / "src" / "unity_doctor"
        violations = []
        for layer in ("domain", "application"):
            for path in (source_root / layer).rglob("*.py"):
                tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
                for node in ast.walk(tree):
                    if isinstance(node, ast.ImportFrom) and (
                        node.module or ""
                    ).startswith("unity_doctor.adapters"):
                        violations.append(
                            "{}:{}".format(path.relative_to(source_root), node.lineno)
                        )
        self.assertEqual(violations, [])

    def test_composition_root_owns_concrete_adapter_wiring(self):
        composition = (
            Path(__file__).parents[2] / "src" / "unity_doctor" / "composition.py"
        ).read_text(encoding="utf-8")
        self.assertIn("CMakeAdapter()", composition)
        self.assertIn("CompilerProbeRunner", composition)
        self.assertIn("JsonSessionStore", composition)


if __name__ == "__main__":
    unittest.main()
