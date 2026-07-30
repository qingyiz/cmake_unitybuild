import json
import tempfile
import unittest
from pathlib import Path

from unity_doctor.adapters.reporting import ArtifactReporter, JsonSessionStore
from unity_doctor.domain.models import (
    Classification,
    ConflictCase,
    FailureFingerprint,
    Session,
    SessionStatus,
    Suggestion,
)


class ReportingIntegrationTests(unittest.TestCase):
    def test_atomic_store_round_trip_and_artifact_ids(self):
        with tempfile.TemporaryDirectory() as root:
            report_root = Path(root) / "reports"
            source = Path(root) / "source"
            case = ConflictCase(
                "CASE-001",
                "app",
                "CXX",
                str(Path(root) / "work/unity.cxx"),
                [str(source / "a.cpp"), str(source / "b.cpp")],
                fingerprint=FailureFingerprint(
                    "clang", "compile", "redefinition", "value", "redefinition"
                ),
                minimal_sources=[str(source / "a.cpp"), str(source / "b.cpp")],
                classification=Classification("TU_LOCAL_NAME", 0.95, "summary"),
                suggestions=[
                    Suggestion(
                        "SOURCE_EXCLUSION",
                        "low",
                        "summary",
                        "3.16",
                        True,
                        "set_property(TARGET app PROPERTY UNITY_BUILD OFF)",
                    )
                ],
            )
            session = Session(
                "session-1",
                "now",
                "now",
                SessionStatus.COMPLETE,
                {
                    "source": str(source),
                    "work_dir": str(Path(root) / "work"),
                    "report_dir": str(report_root),
                },
                cases=[case],
            )
            store = JsonSessionStore(report_root)
            path = store.save(session)
            restored = store.load(path)
            artifacts = ArtifactReporter(report_root).render(restored)

            self.assertEqual(restored.cases[0].case_id, "CASE-001")
            report = json.loads(artifacts["json"].read_text())
            self.assertEqual(report["cases"][0]["case_id"], "CASE-001")
            self.assertIn("CASE-001", artifacts["markdown"].read_text())
            self.assertIn("CASE-001", artifacts["cmake"].read_text())
            self.assertNotIn(str(source), artifacts["json"].read_text())


if __name__ == "__main__":
    unittest.main()
