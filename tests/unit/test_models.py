import unittest

from unity_doctor.domain.models import (
    Classification,
    CommandRecord,
    ConflictCase,
    FailureFingerprint,
    ProbeRecord,
    Session,
    SessionStatus,
    Suggestion,
)


class SessionModelTests(unittest.TestCase):
    def test_round_trip_keeps_nested_ids_and_enums(self):
        command = CommandRecord(["c++"], "/tmp", 1, "now", 0.1, "log")
        fingerprint = FailureFingerprint("clang", "compile", "redefinition", "x", "msg")
        probe = ProbeRecord("key", ["a.cpp", "b.cpp"], command, True, fingerprint)
        case = ConflictCase(
            "CASE-001",
            "app",
            "CXX",
            "unity.cxx",
            ["a.cpp", "b.cpp"],
            fingerprint=fingerprint,
            probes=[probe],
            classification=Classification("TU_LOCAL_NAME", 0.9, "summary"),
            suggestions=[
                Suggestion("exclude", "low", "summary", "3.16", True)
            ],
        )
        session = Session(
            "session",
            "now",
            "now",
            SessionStatus.MINIMIZING,
            {"source": "/src"},
            cases=[case],
        )

        restored = Session.from_dict(session.to_dict())

        self.assertEqual(restored.status, SessionStatus.MINIMIZING)
        self.assertEqual(restored.cases[0].fingerprint.key, fingerprint.key)
        self.assertEqual(restored.cases[0].probes[0].sources, ["a.cpp", "b.cpp"])


if __name__ == "__main__":
    unittest.main()
