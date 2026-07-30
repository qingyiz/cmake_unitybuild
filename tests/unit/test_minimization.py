import unittest

from unity_doctor.domain.minimization import minimize_ordered
from unity_doctor.domain.models import CommandRecord, ProbeRecord


def fake_record(sources, reproduced):
    command = CommandRecord([], "/tmp", int(not reproduced), "now", 0, "log")
    return ProbeRecord("|".join(sources), list(sources), command, reproduced, None)


class OrderedMinimizationTests(unittest.TestCase):
    def test_finds_three_file_ordered_interaction(self):
        required = ["macro.cpp", "bridge.cpp", "consumer.cpp"]

        def probe(sources):
            positions = [sources.index(item) for item in required if item in sources]
            reproduced = len(positions) == 3 and positions == sorted(positions)
            return fake_record(sources, reproduced)

        result = minimize_ordered(["noise.cpp"] + required + ["tail.cpp"], probe)

        self.assertEqual(result.status, "MINIMIZED")
        self.assertEqual(result.sources, required)
        for index in range(len(result.sources)):
            subset = result.sources[:index] + result.sources[index + 1 :]
            self.assertFalse(probe(subset).reproduced)

    def test_does_not_treat_different_failure_as_reproduction(self):
        def probe(sources):
            return fake_record(sources, False)

        result = minimize_ordered(["a.cpp", "b.cpp"], probe)
        self.assertEqual(result.status, "NON_REPLAYABLE")

    def test_budget_returns_partial_candidate(self):
        def probe(sources):
            return fake_record(sources, len(sources) >= 2)

        result = minimize_ordered(["a.cpp", "b.cpp", "c.cpp"], probe, max_probes=1)
        self.assertEqual(result.status, "BUDGET_EXHAUSTED")


if __name__ == "__main__":
    unittest.main()
