"""Mock Atom sender for Smart Solutions Lab 1.

Lets us test the final HTTP contract without AtomS3 or MG400 hardware.
"""

from __future__ import annotations

import argparse
import time
import uuid

import requests


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--url", default="http://127.0.0.1:5000")
    p.add_argument("--letter", default="A")
    p.add_argument("--seq", type=int, default=1)
    p.add_argument("--session", default=None)
    return p.parse_args()


if __name__ == "__main__":
    args = parse_args()
    session = args.session or f"mock-{uuid.uuid4().hex[:8]}"
    payload = {
        "letter": args.letter.upper(),
        "session": session,
        "seq": args.seq,
        # Same semantics planned for Atom: milliseconds since that Atom boot.
        # This value is NOT directly subtracted from the PC clock.
        "atom_sent_ms": int(time.monotonic() * 1000),
    }
    r = requests.post(f"{args.url.rstrip('/')}/api/letter", json=payload, timeout=2)
    print("HTTP", r.status_code)
    print(r.json())
