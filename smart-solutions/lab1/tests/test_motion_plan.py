from __future__ import annotations

import sys
import unittest
from pathlib import Path

LAB1_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(LAB1_ROOT / "src"))

from letter_paths import DEFAULT_LETTERS_PATH, get_letter_path, load_letter_paths
from motion_plan import build_motion_plan


class MotionPlanTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.paths = load_letter_paths(DEFAULT_LETTERS_PATH)

    def test_plan_starts_and_ends_pen_up(self) -> None:
        plan = build_motion_plan(get_letter_path("A", self.paths))
        self.assertEqual(plan[0], {"action": "PEN_UP"})
        self.assertEqual(plan[-1], {"action": "PEN_UP"})

    def test_pen_goes_down_before_drawing_move(self) -> None:
        plan = build_motion_plan(get_letter_path("L", self.paths))
        actions = [item["action"] for item in plan]
        first_pen_down = actions.index("PEN_DOWN")
        self.assertEqual(actions[first_pen_down + 1], "MOVE_NORMALIZED")

    def test_separate_strokes_raise_pen_between_lines(self) -> None:
        plan = build_motion_plan(get_letter_path("A", self.paths))
        actions = [item["action"] for item in plan]
        pen_down_indices = [i for i, action in enumerate(actions) if action == "PEN_DOWN"]
        self.assertEqual(len(pen_down_indices), 2)
        between = actions[pen_down_indices[0] + 1 : pen_down_indices[1]]
        self.assertIn("PEN_UP", between)

    def test_plan_has_no_consecutive_pen_up_actions(self) -> None:
        plan = build_motion_plan(get_letter_path("A", self.paths))
        actions = [item["action"] for item in plan]
        for previous, current in zip(actions, actions[1:]):
            with self.subTest(previous=previous, current=current):
                self.assertFalse(previous == current == "PEN_UP")

    def test_moves_remain_normalized(self) -> None:
        plan = build_motion_plan(get_letter_path("N", self.paths))
        for action in plan:
            if action["action"] == "MOVE_NORMALIZED":
                self.assertGreaterEqual(action["x"], 0.0)
                self.assertLessEqual(action["x"], 1.0)
                self.assertGreaterEqual(action["y"], 0.0)
                self.assertLessEqual(action["y"], 1.0)


if __name__ == "__main__":
    unittest.main()
