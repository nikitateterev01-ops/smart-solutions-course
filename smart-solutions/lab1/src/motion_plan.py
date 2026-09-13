"""Build hardware-independent actions from normalized letter strokes."""

from __future__ import annotations

from typing import TypedDict

from letter_paths import LetterPath


class MotionAction(TypedDict, total=False):
    action: str
    x: float
    y: float


def build_motion_plan(strokes: LetterPath) -> list[MotionAction]:
    """Convert each stroke into pen-state and normalized movement actions."""
    actions: list[MotionAction] = []
    for stroke in strokes:
        first_x, first_y = stroke[0]
        if not actions:
            actions.append({"action": "PEN_UP"})
        actions.append({"action": "MOVE_NORMALIZED", "x": first_x, "y": first_y})
        actions.append({"action": "PEN_DOWN"})
        for x, y in stroke[1:]:
            actions.append({"action": "MOVE_NORMALIZED", "x": x, "y": y})
        actions.append({"action": "PEN_UP"})
    return actions


def format_motion_plan(actions: list[MotionAction]) -> str:
    """Format a plan for dry-run stdout without robot coordinates."""
    lines: list[str] = []
    for action in actions:
        if action["action"] == "MOVE_NORMALIZED":
            lines.append(
                f"MOVE_NORMALIZED {action['x']:.3f} {action['y']:.3f}"
            )
        else:
            lines.append(action["action"])
    return "\n".join(lines)
