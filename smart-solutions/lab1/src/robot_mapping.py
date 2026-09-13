"""Normaliseeritud punktide teisendus abstraktseteks roboti XY-punktideks."""

from __future__ import annotations

from dataclasses import dataclass

from robot_calibration import CalibrationError, RobotCalibration


@dataclass(frozen=True)
class RobotPoint:
    x: float
    y: float


def _normalized(value: object, axis: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise CalibrationError(f"normaliseeritud {axis} peab olema arv")
    numeric = float(value)
    if not 0.0 <= numeric <= 1.0:
        raise CalibrationError(f"normaliseeritud {axis} peab olema vahemikus 0..1")
    return numeric


def map_normalized_point(
    calibration: RobotCalibration,
    normalized_x: object,
    normalized_y: object,
) -> RobotPoint:
    """Teisenda üks punkt ainult täieliku ja valideeritud kalibratsiooniga."""
    calibration.require_complete()
    x = _normalized(normalized_x, "x")
    y = _normalized(normalized_y, "y")

    workspace = calibration.workspace
    if workspace.width is None or workspace.height is None:
        raise CalibrationError("joonistusala mõõtmed puuduvad")
    if workspace.width <= 0 or workspace.height <= 0:
        raise CalibrationError("joonistusala mõõtmed peavad olema positiivsed")
    if workspace.origin_x is None or workspace.origin_y is None:
        raise CalibrationError("joonistusala alguspunkt puudub")

    return RobotPoint(
        x=workspace.origin_x + x * workspace.width,
        y=workspace.origin_y + y * workspace.height,
    )
