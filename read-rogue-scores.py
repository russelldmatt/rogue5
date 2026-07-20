#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys


ENTRY_SIZE = 110
SCORE_SIZE = 80
NICKNAME_SIZE = 30


def decode(data: bytes) -> bytes:
    """Reverse Rogue's xxxx() XOR obfuscation."""
    f = 37
    s = 7
    result = bytearray()

    for byte in data:
        r = ((f * s) + 9337) % 8887
        f = s
        s = r

        # Rogue converts the result to unsigned char before XORing.
        result.append(byte ^ (r & 0xFF))

    return bytes(result)


def c_string(data: bytes) -> str:
    """Decode a null-terminated C string."""
    return data.split(b"\0", 1)[0].decode("utf-8", errors="replace")


def apply_nickname(score: str, nickname: str) -> str:
    """
    Rogue stores the Unix login name in the score line and stores the
    chosen Rogue nickname separately. Reproduce nickize() approximately.
    """
    if not nickname:
        return score

    colon = score.find(":", 15)

    if colon == -1:
        return score

    return score[:15] + nickname + score[colon:]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Read a Rogue 5 rogue.scores file."
    )
    parser.add_argument("scorefile", help="Path to rogue.scores")
    args = parser.parse_args()

    try:
        with open(args.scorefile, "rb") as f:
            encoded = f.read()
    except OSError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if len(encoded) % ENTRY_SIZE != 0:
        print(
            f"error: file is {len(encoded)} bytes; "
            f"expected a multiple of {ENTRY_SIZE}",
            file=sys.stderr,
        )
        return 1

    decoded = decode(encoded)
    count = len(decoded) // ENTRY_SIZE

    print("Top Ten Rogueists")
    print()
    print("Rank   Score   Name")
    print()

    for i in range(count):
        offset = i * ENTRY_SIZE

        score_data = decoded[offset : offset + SCORE_SIZE]
        nickname_data = decoded[
            offset + SCORE_SIZE : offset + ENTRY_SIZE
        ]

        score = c_string(score_data).rstrip()
        nickname = c_string(nickname_data)

        print(apply_nickname(score, nickname))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
