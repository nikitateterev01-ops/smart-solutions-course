"""Smart Solutions Lab 1 station-side letter receiver.

This file deliberately does NOT move the MG400 yet.
Real robot coordinates, pen Z and motion verification belong to the lab session.

Run:
    python -m pip install -r requirements.txt
    python station.py --host 0.0.0.0 --port 5000

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

app = Flask(__name__)


@dataclass(frozen=True)
class Config:
    log_path: Path


CONFIG = Config(log_path=Path("data/letter_events.csv"))
LOG_LOCK = threading.Lock()

# The latest accepted sequence number per Atom boot/session id.
# It prevents a retry from accidentally starting the same robot drawing twice.
LAST_SEQ: dict[str, int] = {}


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

    # IMPORTANT: no robot movement here yet.
    # In the lab this is where a safety-checked queue/worker will:
    #  1. confirm robot is enabled and idle;
    #  2. load a measured letter path;
    #  3. send the first MG400 command;
    #  4. record robot_command_monotonic_ns immediately before that command.
    append_log(
        letter=letter,
        session=session,
        seq=seq,
        atom_sent_ms=atom_sent_ms,
        station_received_iso=received_iso,
        station_received_monotonic_ns=received_mono,
        robot_command_monotonic_ns=None,
        status="accepted_no_robot",
        note="robot integration intentionally pending real MG400 coordinates/test",
    )

    print(
        json.dumps(
            {
                "event": "letter",
                "letter": letter,
                "session": session,
                "seq": seq,
                "station_received_iso": received_iso,
            },
            ensure_ascii=False,
        ),
        flush=True,
    )

    return (
        jsonify(
            ok=True,
            duplicate=False,
            letter=letter,
            seq=seq,
            station_received_iso=received_iso,
            robot_started=False,
        ),
        202,
    )


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="0.0.0.0")
    p.add_argument("--port", type=int, default=5000)
    p.add_argument("--log", type=Path, default=Path("data/letter_events.csv"))
    return p.parse_args()


if __name__ == "__main__":
    args = parse_args()
    CONFIG = Config(log_path=args.log)
    ensure_log()
    # Debug/reloader off: the station must have one process and one event queue.
    app.run(host=args.host, port=args.port, debug=False, use_reloader=False)
