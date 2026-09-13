"""Smart Solutions Lab 1 station-side letter receiver.

This file deliberately does NOT move the MG400 yet.
Real robot coordinates, pen Z and motion verification belong to the lab session.

Run:
    python -m pip install -r requirements.txt
    python station.py --host 0.0.0.0 --port 5000 --dry-run

Test without Atom:
    python mock_atom.py --url http://127.0.0.1:5000 --letter A
"""

from __future__ import annotations

import argparse
import csv
import json
import threading
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

from flask import Flask, jsonify, request
from letter_paths import DEFAULT_LETTERS_PATH, LetterPaths, get_letter_path, load_letter_paths
from motion_plan import build_motion_plan, format_motion_plan


app = Flask(__name__)


@dataclass(frozen=True)
class Config:
    log_path: Path
    letters_path: Path
    dry_run: bool


CONFIG = Config(log_path=Path("data/letter_events.csv"), letters_path=DEFAULT_LETTERS_PATH, dry_run=True)
LOG_LOCK = threading.Lock()

# The latest accepted sequence number per Atom boot/session id.
# It prevents a retry from accidentally starting the same robot drawing twice.
LAST_SEQ: dict[str, int] = {}
LETTER_PATHS: LetterPaths = load_letter_paths(CONFIG.letters_path)


def utc_iso() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def monotonic_ns() -> int:
    return time.monotonic_ns()


def ensure_log() -> None:
    CONFIG.log_path.parent.mkdir(parents=True, exist_ok=True)
    if CONFIG.log_path.exists():
        return
    with CONFIG.log_path.open("w", newline="", encoding="utf-8") as f:
        csv.writer(f).writerow(
            [
                "letter",
                "session",
                "seq",
                "atom_sent_ms",
                "station_received_iso",
                "station_received_monotonic_ns",
                "robot_command_monotonic_ns",
                "status",
                "note",
            ]
        )


def append_log(
    *,
    letter: str,
    session: str,
    seq: int,
    atom_sent_ms: int | None,
    station_received_iso: str,
    station_received_monotonic_ns: int,
    robot_command_monotonic_ns: int | None,
    status: str,
    note: str = "",
) -> None:
    ensure_log()
    with LOG_LOCK, CONFIG.log_path.open("a", newline="", encoding="utf-8") as f:
        csv.writer(f).writerow(
            [
                letter,
                session,
                seq,
                "" if atom_sent_ms is None else atom_sent_ms,
                station_received_iso,
                station_received_monotonic_ns,
                "" if robot_command_monotonic_ns is None else robot_command_monotonic_ns,
                status,
                note,
            ]
        )


def validate_message(payload: object) -> tuple[str, str, int, int | None]:
    if not isinstance(payload, dict):
        raise ValueError("JSON object required")

    letter = payload.get("letter")
    if not isinstance(letter, str):
        raise ValueError("letter must be a string")

    letter = letter.strip().upper()
    if len(letter) != 1 or not ("A" <= letter <= "Z"):
        raise ValueError("letter must be one character A-Z")

    # Session + sequence make retries idempotent.
    session = payload.get("session", "default")
    if not isinstance(session, str) or not session.strip():
        raise ValueError("session must be a non-empty string")
    session = session.strip()[:64]

    seq = payload.get("seq", 0)
    if isinstance(seq, bool) or not isinstance(seq, int) or seq < 0:
        raise ValueError("seq must be a non-negative integer")

    atom_sent_ms = payload.get("atom_sent_ms")
    if atom_sent_ms is not None:
        if isinstance(atom_sent_ms, bool) or not isinstance(atom_sent_ms, int) or atom_sent_ms < 0:
            raise ValueError("atom_sent_ms must be a non-negative integer")

    return letter, session, seq, atom_sent_ms


@app.get("/health")
def health():
    return jsonify(ok=True, service="smart-solutions-station")


@app.post("/api/letter")
def receive_letter():
    # Timestamp as early as possible after Flask has accepted the request.
    received_mono = monotonic_ns()
    received_iso = utc_iso()

    try:
        payload = request.get_json(force=False, silent=False)
        letter, session, seq, atom_sent_ms = validate_message(payload)
    except Exception as exc:
        return jsonify(ok=False, error=str(exc)), 400

    try:
        strokes = get_letter_path(letter, LETTER_PATHS)
    except ValueError as exc:
        append_log(
            letter=letter,
            session=session,
            seq=seq,
            atom_sent_ms=atom_sent_ms,
            station_received_iso=received_iso,
            station_received_monotonic_ns=received_mono,
            robot_command_monotonic_ns=None,
            status="path_not_configured",
            note=str(exc),
        )
        return (
            jsonify(
                ok=False,
                error="letter path not configured",
                letter=letter,
                robot_started=False,
            ),
            422,
        )

    previous = LAST_SEQ.get(session)
    if previous is not None and seq <= previous:
        # Retry / duplicate: acknowledge it, but never move the robot twice.
        append_log(
            letter=letter,
            session=session,
            seq=seq,
            atom_sent_ms=atom_sent_ms,
            station_received_iso=received_iso,
            station_received_monotonic_ns=received_mono,
            robot_command_monotonic_ns=None,
            status="duplicate",
            note="acknowledged; robot command suppressed",
        )
        return (
            jsonify(
                ok=True,
                duplicate=True,
                letter=letter,
                seq=seq,
                station_received_iso=received_iso,
            ),
            200,
        )

    LAST_SEQ[session] = seq

    # This plan contains normalized geometry only. It cannot command an MG400.
    plan = build_motion_plan(strokes)
    append_log(
        letter=letter,
        session=session,
        seq=seq,
        atom_sent_ms=atom_sent_ms,
        station_received_iso=received_iso,
        station_received_monotonic_ns=received_mono,
        robot_command_monotonic_ns=None,
        status="accepted_dry_run",
        note="normalized motion plan only; no robot command sent",
    )

    print(
        json.dumps(
            {
                "event": "letter",
                "letter": letter,
                "session": session,
                "seq": seq,
                "station_received_iso": received_iso,
                "status": "accepted_dry_run",
            },
            ensure_ascii=False,
        ),
        flush=True,
    )
    print(format_motion_plan(plan), flush=True)

    return (
        jsonify(
            ok=True,
            duplicate=False,
            letter=letter,
            seq=seq,
            station_received_iso=received_iso,
            robot_started=False,
            dry_run=True,
            action_count=len(plan),
        ),
        202,
    )


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="0.0.0.0")
    p.add_argument("--port", type=int, default=5000)
    p.add_argument("--log", type=Path, default=Path("data/letter_events.csv"))
    p.add_argument("--letters", type=Path, default=DEFAULT_LETTERS_PATH)
    p.add_argument(
        "--dry-run",
        action="store_true",
        default=True,
        help="build and print normalized plans without sending robot commands (default)",
    )
    return p.parse_args()


if __name__ == "__main__":
    args = parse_args()
    CONFIG = Config(log_path=args.log, letters_path=args.letters, dry_run=args.dry_run)
    LETTER_PATHS = load_letter_paths(CONFIG.letters_path)
    ensure_log()
    # Debug/reloader off: the station must have one process and one event queue.
    app.run(host=args.host, port=args.port, debug=False, use_reloader=False)
