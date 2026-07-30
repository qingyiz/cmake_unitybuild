import json
import tempfile
import unittest
from pathlib import Path

from unity_doctor.cli import main


class CliEndToEndTests(unittest.TestCase):
    def test_diagnose_minimizes_and_reports_static_conflict(self):
        with tempfile.TemporaryDirectory() as root:
            source, work = _create_fixture(Path(root), baseline_error=False)
            exit_code = main(
                [
                    "diagnose",
                    "--source",
                    str(source),
                    "--work-dir",
                    str(work),
                    "--generator",
                    "Ninja",
                    "--timeout",
                    "60",
                ]
            )

            self.assertEqual(exit_code, 0)
            report = _only_report(work)
            self.assertEqual(report["status"], "COMPLETE")
            self.assertEqual(report["cases"][0]["status"], "MINIMIZED")
            self.assertEqual(len(report["cases"][0]["minimal_sources"]), 2)
            self.assertEqual(
                report["cases"][0]["classification"]["category"], "TU_LOCAL_NAME"
            )
            cmake = next((work / "reports").glob("*/recommendations.cmake")).read_text()
            self.assertIn("SKIP_UNITY_BUILD_INCLUSION", cmake)
            self.assertFalse((source / "build").exists())

    def test_baseline_failure_never_creates_probes(self):
        with tempfile.TemporaryDirectory() as root:
            source, work = _create_fixture(Path(root), baseline_error=True)
            exit_code = main(
                [
                    "diagnose",
                    "--source",
                    str(source),
                    "--work-dir",
                    str(work),
                    "--generator",
                    "Ninja",
                    "--timeout",
                    "60",
                ]
            )
            self.assertEqual(exit_code, 2)
            report = _only_report(work)
            self.assertEqual(report["status"], "BASELINE_FAILED")
            self.assertEqual(report["cases"], [])
            self.assertEqual(list(work.rglob("probes")), [])

    def test_verify_runs_both_modes_tests_and_three_benchmarks(self):
        with tempfile.TemporaryDirectory() as root:
            source, work = _create_fixture(Path(root), baseline_error=False)
            exit_code = main(
                [
                    "verify",
                    "--source",
                    str(source),
                    "--work-dir",
                    str(work),
                    "--generator",
                    "Ninja",
                    "--cmake-arg=-DDOCTOR_SKIP=ON",
                    "--test-command",
                    "cmake --version",
                    "--benchmark-runs",
                    "3",
                    "--timeout",
                    "60",
                ]
            )

            self.assertEqual(exit_code, 0)
            report = _only_report(work)
            self.assertEqual(report["status"], "VERIFIED")
            self.assertTrue(report["verification"]["baseline"]["passed"])
            self.assertTrue(report["verification"]["unity"]["passed"])
            self.assertTrue(report["verification"]["tests"]["passed"])
            self.assertEqual(len(report["verification"]["benchmark"]["runs"]), 3)
            self.assertIn("median_seconds", report["verification"]["benchmark"])

    def test_budget_exhausted_session_can_resume_with_cached_probes(self):
        with tempfile.TemporaryDirectory() as root:
            source, work = _create_fixture(Path(root), baseline_error=False)
            first_exit = main(
                [
                    "diagnose",
                    "--source",
                    str(source),
                    "--work-dir",
                    str(work),
                    "--generator",
                    "Ninja",
                    "--max-probes",
                    "1",
                    "--timeout",
                    "60",
                ]
            )
            self.assertEqual(first_exit, 3)
            session_path = next((work / "reports").glob("*/session.json"))
            first = json.loads(session_path.read_text())
            first_keys = {item["key"] for item in first["cases"][0]["probes"]}

            resumed_exit = main(["resume", "--session", str(session_path)])

            self.assertEqual(resumed_exit, 0)
            resumed = json.loads(session_path.read_text())
            self.assertEqual(resumed["status"], "COMPLETE")
            resumed_keys = {item["key"] for item in resumed["cases"][0]["probes"]}
            self.assertTrue(first_keys.issubset(resumed_keys))

    def test_rejects_output_inside_source(self):
        with tempfile.TemporaryDirectory() as root:
            source, _ = _create_fixture(Path(root), baseline_error=False)
            exit_code = main(
                [
                    "diagnose",
                    "--source",
                    str(source),
                    "--work-dir",
                    str(source / "doctor-work"),
                ]
            )
            self.assertEqual(exit_code, 2)


def _create_fixture(root, baseline_error):
    source = root / "source"
    work = root / "work"
    source.mkdir()
    extra = "this is not valid C++\n" if baseline_error else ""
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.18)\n"
        "project(cli_fixture LANGUAGES CXX)\n"
        "option(DOCTOR_SKIP \"skip conflict\" OFF)\n"
        "add_library(conflict STATIC a.cpp b.cpp)\n"
        "if(DOCTOR_SKIP)\n"
        "  set_source_files_properties(a.cpp b.cpp TARGET_DIRECTORY conflict "
        "PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)\n"
        "endif()\n",
        encoding="utf-8",
    )
    (source / "a.cpp").write_text(
        "static int helper() { return 1; }\nint a() { return helper(); }\n" + extra,
        encoding="utf-8",
    )
    (source / "b.cpp").write_text(
        "static int helper() { return 2; }\nint b() { return helper(); }\n",
        encoding="utf-8",
    )
    return source, work


def _only_report(work):
    reports = list((work / "reports").glob("*/report.json"))
    if len(reports) != 1:
        raise AssertionError("expected one report, got {}".format(reports))
    return json.loads(reports[0].read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
