import tempfile
import unittest
from pathlib import Path

from unity_doctor.adapters.cmake import CMakeAdapter
from tests.fixtures.catalog import CASES


class ConflictFixtureMatrixTests(unittest.TestCase):
    def test_eight_fixtures_pass_baseline_and_fail_unity(self):
        adapter = CMakeAdapter()
        with tempfile.TemporaryDirectory() as root:
            root_path = Path(root)
            for name, files in CASES.items():
                with self.subTest(case=name):
                    source = root_path / "sources" / name
                    work = root_path / "work" / name
                    source.mkdir(parents=True)
                    (source / "CMakeLists.txt").write_text(
                        "cmake_minimum_required(VERSION 3.18)\n"
                        "project({} LANGUAGES CXX)\n"
                        "add_library(conflict STATIC a.cpp b.cpp)\n".format(name),
                        encoding="utf-8",
                    )
                    for filename, content in files.items():
                        (source / filename).write_text(content, encoding="utf-8")
                    baseline = adapter.configure_and_build(
                        source,
                        work / "baseline",
                        False,
                        "Ninja",
                        "Debug",
                        [],
                        [],
                        60,
                    )
                    unity = adapter.configure_and_build(
                        source,
                        work / "unity",
                        True,
                        "Ninja",
                        "Debug",
                        [],
                        [],
                        60,
                    )
                    self.assertTrue(baseline.succeeded, baseline.log_text())
                    self.assertFalse(unity.succeeded, name)
                    units = adapter.discover_unity_units(work / "unity")
                    self.assertEqual(len(units), 1)
                    self.assertEqual(len(units[0].ordered_sources), 2)


if __name__ == "__main__":
    unittest.main()
