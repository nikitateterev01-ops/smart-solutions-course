"""Roboti laborikalibratsiooni laadimine ja valideerimine."""

from __future__ import annotations

import json
import math
from dataclasses import dataclass
from pathlib import Path

DEFAULT_CALIBRATION_PATH = (
    Path(__file__).resolve().parents[1] / "config" / "robot_calibration.json"
)


class CalibrationError(ValueError):
    """Kalibratsioonifaili struktuur või väärtus ei ole lubatud."""


class IncompleteCalibrationError(CalibrationError):
    """Kalibratsioon vajab enne kasutamist laborimõõtmisi."""


@dataclass(frozen=True)
class WorkspaceCalibration:
    origin_x: float | None
    origin_y: float | None
    width: float | None
    height: float | None


@dataclass(frozen=True)
class PoseCalibration:
    r: float | None
    pen_up_z: float | None
    pen_down_z: float | None


@dataclass(frozen=True)
class MotionCalibration:
    speed_percent: float | None


@dataclass(frozen=True)
class RobotCalibration:
    version: int
    workspace: WorkspaceCalibration
    pose: PoseCalibration
    motion: MotionCalibration

    @property
    def is_complete(self) -> bool:
        """Tagasta tõene ainult siis, kui kõik laboriväärtused on olemas."""
        values = (
            self.workspace.origin_x,
            self.workspace.origin_y,
            self.workspace.width,
            self.workspace.height,
            self.pose.r,
            self.pose.pen_up_z,
            self.pose.pen_down_z,
            self.motion.speed_percent,
        )
        return all(value is not None for value in values)

    def require_complete(self) -> None:
        """Peata ohutult tegevus, mis vajab täielikku kalibratsiooni."""
        if not self.is_complete:
            raise IncompleteCalibrationError(
                "roboti kalibratsioon on puudulik; mõõda väärtused laboris"
            )


def _object(document: object, key: str) -> dict[str, object]:
    if not isinstance(document, dict):
        raise CalibrationError("kalibratsiooni juur peab olema JSON objekt")
    value = document.get(key)
    if not isinstance(value, dict):
        raise CalibrationError(f"{key} peab olema JSON objekt")
    return value


def _nullable_number(section: dict[str, object], key: str, location: str) -> float | None:
    if key not in section:
        raise CalibrationError(f"{location}.{key} puudub")
    value = section[key]
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise CalibrationError(f"{location}.{key} peab olema arv või null")
    numeric = float(value)
    if not math.isfinite(numeric):
        raise CalibrationError(f"{location}.{key} peab olema lõplik arv")
    return numeric


def validate_robot_calibration(document: object) -> RobotCalibration:
    """Valideeri versiooni 1 kalibratsioon; null-väärtused on lubatud."""
    if not isinstance(document, dict):
        raise CalibrationError("kalibratsiooni juur peab olema JSON objekt")
    version = document.get("version")
    if isinstance(version, bool) or not isinstance(version, int) or version != 1:
        raise CalibrationError("kalibratsiooni version peab olema täisarv 1")

    workspace_data = _object(document, "workspace")
    pose_data = _object(document, "pose")
    motion_data = _object(document, "motion")

    workspace = WorkspaceCalibration(
        origin_x=_nullable_number(workspace_data, "origin_x", "workspace"),
        origin_y=_nullable_number(workspace_data, "origin_y", "workspace"),
        width=_nullable_number(workspace_data, "width", "workspace"),
        height=_nullable_number(workspace_data, "height", "workspace"),
    )
    if workspace.width is not None and workspace.width <= 0:
        raise CalibrationError("workspace.width peab olema positiivne")
    if workspace.height is not None and workspace.height <= 0:
        raise CalibrationError("workspace.height peab olema positiivne")

    pose = PoseCalibration(
        r=_nullable_number(pose_data, "r", "pose"),
        pen_up_z=_nullable_number(pose_data, "pen_up_z", "pose"),
        pen_down_z=_nullable_number(pose_data, "pen_down_z", "pose"),
    )
    motion = MotionCalibration(
        speed_percent=_nullable_number(motion_data, "speed_percent", "motion")
    )
    if motion.speed_percent is not None and not 0 < motion.speed_percent <= 100:
        raise CalibrationError("motion.speed_percent peab olema vahemikus 0 < väärtus <= 100")

    return RobotCalibration(version, workspace, pose, motion)


def load_robot_calibration(
    path: str | Path = DEFAULT_CALIBRATION_PATH,
) -> RobotCalibration:
    """Laadi JSON-fail ja tagasta valideeritud kalibratsioon."""
    source = Path(path)
    try:
        document = json.loads(source.read_text(encoding="utf-8"))
    except OSError as exc:
        raise CalibrationError(f"kalibratsioonifaili ei saa lugeda: {source}") from exc
    except json.JSONDecodeError as exc:
        raise CalibrationError(f"vigane JSON kalibratsioonifailis: {source}") from exc
    return validate_robot_calibration(document)
