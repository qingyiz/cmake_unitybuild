import hashlib
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from unity_doctor.adapters.process import SubprocessExecutor
from unity_doctor.adapters.reporting import ArtifactReporter, JsonSessionStore
from unity_doctor.application.diagnosis import DiagnosisService
from unity_doctor.application.requests import DiagnosisRequest
from unity_doctor.domain.models import (
    CommandRecord,
    FailureFingerprint,
    ProbeRecord,
)


class RecoveryIntegrationTests(unittest.TestCase):
    def test_interrupted_probe_is_atomically_saved_without_source_changes(self):
        with tempfile.TemporaryDirectory() as root:
            root_path = Path(root)
            source = root_path / "source"
            work = root_path / "work"
            source.mkdir()
            (source / "CMakeLists.txt").write_text("project(fake)\n")
            (source / "a.cpp").write_text("int a;\n")
            (source / "b.cpp").write_text("int b;\n")
            before = _digest(source)
            cmake = _FakeCMake(source)
            probe = _InterruptingProbe()
            service = DiagnosisService(
                cmake,
                JsonSessionStore,
                ArtifactReporter,
                lambda _root, _timeout: probe,
                lambda _log, _entry: _fingerprint(),
                lambda _request, _version: {},
            )

            with self.assertRaises(KeyboardInterrupt):
                service.diagnose(
                    DiagnosisRequest(str(source), str(work), timeout=10)
                )

            session_path = next((work / "reports").glob("*/session.json"))
            restored = JsonSessionStore(work / "reports").load(session_path)
            self.assertEqual(restored.status.value, "MINIMIZING")
            self.assertEqual(len(restored.cases), 1)
            self.assertEqual(len(restored.cases[0].probes), 1)
            self.assertEqual(before, _digest(source))

    def test_timed_out_command_does_not_modify_source(self):
        with tempfile.TemporaryDirectory() as root:
            root_path = Path(root)
            source = root_path / "source"
            output = root_path / "output"
            source.mkdir()
            (source / "keep.cpp").write_text("int keep;\n")
            before = _digest(source)

            record = SubprocessExecutor().run(
                [
                    __import__("sys").executable,
                    "-c",
                    "import time; time.sleep(2)",
                ],
                source,
                output / "timeout.log",
                0.05,
            )

            self.assertTrue(record.timed_out)
            self.assertEqual(record.exit_code, 124)
            self.assertEqual(before, _digest(source))


class _FakeCMake:
    def __init__(self, source):
        self.source = source
        self.calls = 0

    def version(self):
        return (3, 27, 1)

    def configure_and_build(self, _source, build_dir, unity, *_args):
        self.calls += 1
        command = CommandRecord(
            ["cmake"],
            str(self.source),
            1 if unity else 0,
            "now",
            0,
            str(build_dir / "log"),
        )
        return _Outcome(build_dir, command, unity)

    def discover_unity_units(self, _build_dir):
        return [
            SimpleNamespace(
                target="fake",
                language="CXX",
                unity_source=Path("/tmp/Unity/unity_0_cxx.cxx"),
                ordered_sources=[self.source / "a.cpp", self.source / "b.cpp"],
                compile_entry={"file": "unity.cxx", "directory": "/tmp"},
            )
        ]


class _Outcome:
    def __init__(self, build_dir, command, failed):
        self.build_dir = Path(build_dir)
        self.configure = CommandRecord(
            ["cmake"], command.cwd, 0, "now", 0, command.log_path + ".configure"
        )
        self.build = command
        self.succeeded = not failed
        self.failed_stage = "build" if failed else ""

    def log_text(self):
        return "/tmp/b.cpp:1:1: error: redefinition of 'value'\n"


class _InterruptingProbe:
    def __init__(self):
        self.calls = 0

    def run(self, sources, _entry, _expected, _cache):
        self.calls += 1
        if self.calls > 1:
            raise KeyboardInterrupt
        command = CommandRecord([], "/tmp", 1, "now", 0, "/tmp/probe.log")
        return ProbeRecord("first", list(sources), command, True, _fingerprint())


def _fingerprint():
    return FailureFingerprint(
        "clang", "compile", "redefinition", "value", "redefinition"
    )


def _digest(root):
    digest = hashlib.sha256()
    for path in sorted(root.rglob("*")):
        if path.is_file():
            stat = path.stat()
            digest.update(path.read_bytes())
            digest.update(str(stat.st_mode).encode())
            digest.update(str(stat.st_mtime_ns).encode())
    return digest.hexdigest()


if __name__ == "__main__":
    unittest.main()
