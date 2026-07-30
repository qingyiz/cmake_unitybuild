import unittest
from pathlib import Path

from unity_doctor.domain.diagnostics import build_suggestions, classify_case
from unity_doctor.domain.models import ConflictCase, FailureFingerprint


def case(category="redefinition", symbol="value", sources=None):
    sources = sources or ["/src/a.cpp", "/src/b.cpp"]
    return ConflictCase(
        "CASE-001",
        "app",
        "CXX",
        "/build/unity.cxx",
        sources,
        fingerprint=FailureFingerprint("clang", "compile", category, symbol, category),
        minimal_sources=sources,
    )


class DiagnosticRuleTests(unittest.TestCase):
    def test_nine_categories_have_explicit_outcomes(self):
        scenarios = [
            (
                "TU_LOCAL_NAME",
                case(),
                {"/src/a.cpp": "static int value;", "/src/b.cpp": "static int value;"},
                {},
                {},
            ),
            (
                "ANONYMOUS_NAMESPACE",
                case(),
                {"/src/a.cpp": "namespace { int value; }", "/src/b.cpp": "namespace { int value; }"},
                {},
                {},
            ),
            (
                "MACRO_LEAK",
                case("preprocessor", ""),
                {"/src/a.cpp": "#define FLAG 1", "/src/b.cpp": "#ifdef FLAG\n#error leak\n#endif"},
                {},
                {},
            ),
            (
                "INCLUDE_ORDER",
                case("missing_declaration", "Type"),
                {"/src/a.cpp": "", "/src/b.cpp": "Type value;"},
                {},
                {},
            ),
            (
                "HEADER_DEFINITION",
                case(),
                {"/src/a.cpp": "", "/src/b.cpp": ""},
                {},
                {"header_without_guard": "bad.h"},
            ),
            (
                "PER_SOURCE_OPTIONS",
                case(),
                {"/src/a.cpp": "", "/src/b.cpp": ""},
                {"/src/a.cpp": "-DA", "/src/b.cpp": "-DB"},
                {},
            ),
            (
                "QT_GENERATED_SOURCE",
                case(sources=["/build/moc_a.cpp", "/src/b.cpp"]),
                {"/build/moc_a.cpp": "", "/src/b.cpp": ""},
                {},
                {},
            ),
            (
                "SINGLE_SOURCE",
                case(sources=["/src/a.cpp"]),
                {"/src/a.cpp": "broken"},
                {},
                {},
            ),
            (
                "UNKNOWN",
                case("compiler_error", ""),
                {"/src/a.cpp": "int a();", "/src/b.cpp": "int b();"},
                {},
                {},
            ),
        ]
        for expected, target_case, texts, signatures, hints in scenarios:
            with self.subTest(expected=expected):
                result = classify_case(target_case, texts, signatures, hints)
                self.assertEqual(result.category, expected)

    def test_suggestions_are_version_gated_and_scoped(self):
        target = case()
        target.classification = classify_case(
            target,
            {"/src/a.cpp": "static int value;", "/src/b.cpp": "static int value;"},
        )
        old = build_suggestions(target, (3, 16, 0), Path("/src"))
        modern = build_suggestions(target, (3, 27, 1), Path("/src"))

        self.assertFalse(next(item for item in old if item.kind == "EXPLICIT_GROUP").available)
        exclusion = next(item for item in modern if item.kind == "SOURCE_EXCLUSION")
        self.assertIn('TARGET_DIRECTORY "app"', exclusion.cmake)
        self.assertIn('"a.cpp"', exclusion.cmake)


if __name__ == "__main__":
    unittest.main()
