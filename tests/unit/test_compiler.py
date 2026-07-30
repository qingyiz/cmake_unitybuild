import unittest

from unity_doctor.adapters.compiler import parse_failure_fingerprint


class DiagnosticParserTests(unittest.TestCase):
    def test_clang_redefinition_is_stable(self):
        output = "/src/b.cpp:3:12: error: redefinition of 'helper'\n"
        fingerprint = parse_failure_fingerprint(output, "clang")
        self.assertIsNotNone(fingerprint)
        self.assertEqual(fingerprint.category, "redefinition")
        self.assertEqual(fingerprint.symbol, "helper")

    def test_different_errors_have_different_keys(self):
        first = parse_failure_fingerprint(
            "/src/b.cpp:3:12: error: redefinition of 'helper'\n", "clang"
        )
        second = parse_failure_fingerprint(
            "/src/b.cpp:3:12: error: use of undeclared identifier 'helper'\n",
            "clang",
        )
        self.assertNotEqual(first.key, second.key)


if __name__ == "__main__":
    unittest.main()
