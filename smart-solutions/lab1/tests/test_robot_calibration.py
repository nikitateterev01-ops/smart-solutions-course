from __future__ import annotations

import sys
import unittest
from pathlib import Path

LAB1_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(LAB1_ROOT / "src"))

from robot_calibration import (
    CalibrationError,
    load_robot_calibration,
    validate_robot_calibration,
)


def synthetic_document() -> dict[str, object]:
    return {
        "version": 1,
        "workspace": {
            "origin_x": 10.0,
            "origin_y": 20.0,
            "width": 100.0,
            "height": 40.0,
        },
        "pose": {"r": 0.0, "pen_up_z": 30.0, "pen_down_z": 25.0},
        "motion": {"speed_percent": 20.0},
    }


class RobotCalibrationTests(unittest.TestCase):
    def test_default_calibration_is_incomplete(self) -> None:
        calibration = load_robot_calibration()
        self.assertFalse(calibration.is_complete)

    def test_configured_looking_null_object_is_incomplete(self) -> None:
        document = synthetic_document()
        workspace = document["workspace"]
        assert isinstance(workspace, dict)
        workspace["origin_x"] = None
        calibration = validate_robot_calibration(document)
        self.assertFalse(calibration.is_complete)

    def test_synthetic_calibration_is_complete(self) -> None:
        calibration = validate_robot_calibration(synthetic_document())
        self.assertTrue(calibration.is_complete)

    def test_bool_is_rejected_as_number(self) -> None:
        document = synthetic_document()
        workspace = document["workspace"]
        assert isinstance(workspace, dict)
        workspace["origin_x"] = True
        with self.assertRaisesRegex(CalibrationError, "peab olema arv või null"):
            validate_robot_calibration(document)

    def test_invalid_dimensions_are_rejected(self) -> None:
        for key, value in (("width", 0), ("height", -1)):
            with self.subTest(key=key, value=value):
                document = synthetic_document()
                workspace = document["workspace"]
                assert isinstance(workspace, dict)
                workspace[key] = value
                with self.assertRaisesRegex(CalibrationError, "peab olema positiivne"):
                    validate_robot_calibration(document)


if __name__ == "__main__":
    unittest.main()
