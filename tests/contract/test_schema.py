import json
import unittest
from pathlib import Path

from unity_doctor.domain.models import Session, SessionStatus


class SessionSchemaContractTests(unittest.TestCase):
    def test_model_contains_required_schema_fields(self):
        schema = json.loads(
            (Path(__file__).parents[2] / "schemas/session-v1.json").read_text()
        )
        data = Session(
            "session",
            "now",
            "now",
            SessionStatus.INSPECTING,
            {},
        ).to_dict()

        self.assertEqual(data["schema_version"], schema["properties"]["schema_version"]["const"])
        self.assertTrue(set(schema["required"]).issubset(data))
        self.assertFalse(set(data) - set(schema["properties"]))


if __name__ == "__main__":
    unittest.main()
