from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

LAB1_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(LAB1_ROOT / "src"))

from letter_paths import DEFAULT_LETTERS_PATH, get_letter_path, load_letter_paths


class LetterPathTests(unittest.TestCase):
    def setUp(self) -> None:
        self.paths = load_letter_paths(DEFAULT_LETTERS_PATH)

    def test_configured_letters_load(self) -> None:
        for letter in ("A", "L", "N"):
            with self.subTest(letter=letter):
                self.assertTrue(get_letter_path(letter, self.paths))

    def test_unknown_letter_is_not_found(self) -> None:
        with self.assertRaisesRegex(ValueError, "letter path not configured: Z"):
            get_letter_path("Z", self.paths)

    def test_x_below_zero_is_rejected(self) -> None:
        self.assert_invalid_point([-0.01, 0.5], "x must be between")

    def test_x_above_one_is_rejected(self) -> None:
        self.assert_invalid_point([1.01, 0.5], "x must be between")

    def test_malformed_point_is_rejected(self) -> None:
        self.assert_invalid_point([0.5], "point must be \[x, y\]")

    def test_one_point_stroke_is_rejected(self) -> None:
        document = self.document([[[0.5, 0.5]]])
        with self.assertRaisesRegex(ValueError, "at least 2 points"):
            self.load_document(document)

    def test_bool_coordinate_is_rejected(self) -> None:
        self.assert_invalid_point([True, 0.5], "bool is not allowed")

    def assert_invalid_point(self, point: list[object], message: str) -> None:
        document = self.document([[point, [0.5, 0.5]]])
        with self.assertRaisesRegex(ValueError, message):
            self.load_document(document)

    @staticmethod
    def document(strokes: list[object]) -> dict[str, object]:
        return {
            "version": 1,
            "coordinate_system": "normalized_0_1",
            "letters": {"A": {"strokes": strokes}},
        }

    @staticmethod
    def load_document(document: dict[str, object]):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "letters.json"
            path.write_text(json.dumps(document), encoding="utf-8")
            return load_letter_paths(path)


if __name__ == "__main__":
    unittest.main()
