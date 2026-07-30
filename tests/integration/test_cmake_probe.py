import hashlib
import tempfile
import unittest
from pathlib import Path

from unity_doctor.adapters.cmake import CMakeAdapter
from unity_doctor.adapters.compiler import CompilerProbeRunner, parse_failure_fingerprint


class CMakeProbeIntegrationTests(unittest.TestCase):
    def test_baseline_passes_unity_fails_and_probe_replays(self):
        with tempfile.TemporaryDirectory() as root:
            root_path = Path(root)
            source = root_path / "source"
            work = root_path / "work"
            source.mkdir()
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.18)\n"
                "project(fixture LANGUAGES CXX)\n"
                "add_library(conflict STATIC a.cpp b.cpp)\n",
                encoding="utf-8",
            )
            (source / "a.cpp").write_text(
                "static int helper() { return 1; }\nint a() { return helper(); }\n",
                encoding="utf-8",
            )
            (source / "b.cpp").write_text(
                "static int helper() { return 2; }\nint b() { return helper(); }\n",
                encoding="utf-8",
            )
            before = _tree_digest(source)
            adapter = CMakeAdapter()
            baseline = adapter.configure_and_build(
                source, work / "baseline", False, "Ninja", "Debug", [], [], 60
            )
            unity = adapter.configure_and_build(
                source, work / "unity", True, "Ninja", "Debug", [], [], 60
            )

            self.assertTrue(baseline.succeeded, baseline.log_text())
            self.assertFalse(unity.succeeded)
            units = adapter.discover_unity_units(work / "unity")
            self.assertEqual(len(units), 1)
            self.assertEqual([item.name for item in units[0].ordered_sources], ["a.cpp", "b.cpp"])
            expected = parse_failure_fingerprint(unity.log_text(), "clang")
            self.assertIsNotNone(expected)
            probe = CompilerProbeRunner(work / "probes", 60).run(
                [str(item) for item in units[0].ordered_sources],
                units[0].compile_entry,
                expected,
            )
            self.assertTrue(probe.reproduced, Path(probe.command.log_path).read_text())
            self.assertEqual(before, _tree_digest(source))


def _tree_digest(root):
    digest = hashlib.sha256()
    for path in sorted(root.rglob("*")):
        if path.is_file():
            stat = path.stat()
            digest.update(str(path.relative_to(root)).encode())
            digest.update(path.read_bytes())
            digest.update(str(stat.st_mode).encode())
            digest.update(str(stat.st_mtime_ns).encode())
    return digest.hexdigest()


if __name__ == "__main__":
    unittest.main()
