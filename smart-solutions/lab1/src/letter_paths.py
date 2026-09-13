"""Load and validate normalized letter trajectories."""

from __future__ import annotations

import json
from pathlib import Path

Point = tuple[float, float]
Stroke = list[Point]
LetterPath = list[Stroke]
LetterPaths = dict[str, LetterPath]

DEFAULT_LETTERS_PATH = Path(__file__).resolve().parents[1] / "config" / "letters.json"


def _error(location: str, message: str) -> ValueError:
    return ValueError(f"{location}: {message}")


def _validate_point(point: object, location: str) -> Point:
    if not isinstance(point, list) or len(point) != 2:
        raise _error(location, "point must be [x, y]")

    values: list[float] = []
    for axis, value in zip(("x", "y"), point):
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise _error(location, f"{axis} must be numeric (bool is not allowed)")
        numeric = float(value)
        if not 0.0 <= numeric <= 1.0:
            raise _error(location, f"{axis} must be between 0.0 and 1.0")
        values.append(numeric)
    return values[0], values[1]


def _validate_strokes(strokes: object, letter: str) -> LetterPath:
    if not isinstance(strokes, list) or not strokes:
        raise _error(f"letters.{letter}.strokes", "must be a non-empty list")

    validated: LetterPath = []
    for stroke_index, stroke in enumerate(strokes):
        location = f"letters.{letter}.strokes[{stroke_index}]"
        if not isinstance(stroke, list):
            raise _error(location, "stroke must be a list")
        if len(stroke) < 2:
            raise _error(location, "stroke must contain at least 2 points")
        validated.append(
            [
                _validate_point(point, f"{location}[{point_index}]")
                for point_index, point in enumerate(stroke)
            ]
        )
    return validated


def load_letter_paths(path: str | Path = DEFAULT_LETTERS_PATH) -> LetterPaths:
    """Load a version 1 normalized trajectory file and validate every letter."""
    source = Path(path)
    try:
        document = json.loads(source.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ValueError(f"cannot read letter path file {source}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"invalid JSON in letter path file {source}: {exc}") from exc

    if not isinstance(document, dict):
        raise ValueError("letter path document must be a JSON object")
    if document.get("version") != 1:
        raise ValueError("letter path version must be 1")
    if document.get("coordinate_system") != "normalized_0_1":
        raise ValueError("coordinate_system must be normalized_0_1")

    letters = document.get("letters")
    if not isinstance(letters, dict):
        raise ValueError("letters must be a JSON object")

    validated: LetterPaths = {}
    for letter, definition in letters.items():
        if not isinstance(letter, str) or len(letter) != 1 or not ("A" <= letter <= "Z"):
            raise ValueError(f"invalid letter key {letter!r}; expected one character A-Z")
        if not isinstance(definition, dict):
            raise _error(f"letters.{letter}", "definition must be a JSON object")
        validated[letter] = _validate_strokes(definition.get("strokes"), letter)
    return validated


def get_letter_path(letter: str, paths: LetterPaths | None = None) -> LetterPath:
    """Return one configured trajectory or raise a clear error."""
    normalized = letter.strip().upper()
    available = paths if paths is not None else load_letter_paths()
    try:
        return available[normalized]
    except KeyError as exc:
        raise ValueError(f"letter path not configured: {normalized}") from exc
