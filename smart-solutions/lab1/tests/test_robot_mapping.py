from __future__ import annotations

import sys
import unittest
from pathlib import Path

LAB1_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(LAB1_ROOT / "src"))

from robot_calibration import (
    CalibrationError,
    IncompleteCalibrationError,
    load_robot_calibration,
    validate_robot_calibration,
)
from robot_mapping import RobotPoint, map_normalized_point
from test_robot_calibration import synthetic_document


class RobotMappingTests(unittest.TestCase):
    def setUp(self) -> None:
        self.calibration = validate_robot_calibration(synthetic_document())

    def test_origin(self) -> None:
        self.assertEqual(
            map_normalized_point(self.calibration, 0.0, 0.0),
            RobotPoint(10.0, 20.0),
        )

    def test_opposite_corner(self) -> None:
        self.assertEqual(
            map_normalized_point(self.calibration, 1.0, 1.0),
            RobotPoint(110.0, 60.0),
        )

    def test_midpoint(self) -> None:
        self.assertEqual(
            map_normalized_point(self.calibration, 0.5, 0.5),
            RobotPoint(60.0, 40.0),
        )

    def test_normalized_out_of_range_is_rejected(self) -> None:
        cases = (
            (-0.01, 0.5),
            (1.01, 0.5),
            (0.5, -0.01),
            (0.5, 1.01),
        )
        for x, y in cases:
            with self.subTest(x=x, y=y):
                with self.assertRaisesRegex(CalibrationError, "vahemikus 0..1"):
                    map_normalized_point(self.calibration, x, y)

    def test_incomplete_calibration_is_rejected(self) -> None:
        calibration = load_robot_calibration()
        with self.assertRaisesRegex(
            IncompleteCalibrationError, "kalibratsioon on puudulik"
        ):
            map_normalized_point(calibration, 0.5, 0.5)


if __name__ == "__main__":
    unittest.main()
