# -*- coding: utf-8 -*-
"""Locate the per-object string stream empirically by bit-scanning for known strings."""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

from PyArxDwg.container import open_dwg  # noqa: E402
from PyArxDwg.objects import Reader  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-10KV电缆桥架.dwg")
TARGETS = [
    ("LAYER", 32187, "E-CABL-TRAY"),
    ("MTEXT", 10999, "8890.00012"),
]


def read_tu(r: Reader) -> str | None:
    try:
        n = r.bs()
        if n <= 0 or n > 400:
            return None
        blob = bytes(r.rc() for _ in range(n * 2))
        s = blob.decode("utf-16-le", errors="replace")
        return s.replace("\x00", "")
    except Exception:
        return None


def scan(obj: bytes, off: int, ms: int, needle: str) -> list[tuple[int, int, str]]:
    hits = []
    start = off * 8 + 16
    end = (off + ms) * 8 - 16
    for p in range(start, end):
        r = Reader(obj, p)
        s = read_tu(r)
        if s and needle in s:
            hits.append((p, p - off * 8, s))
    return hits


def main() -> int:
    c = open_dwg(DWG)
    obj = c.section_bytes("AcDb:AcDbObjects")
    for label, off, needle in TARGETS:
        r = Reader(obj, off * 8)
        ms = r.ms()
        typ = r.bs()
        bitsize = r.rl()
        print(f"\n== {label} off={off} ms={ms} type=0x{typ:X} bitsize={bitsize} "
              f"obj_end_bit={(off + ms) * 8} obj_start_bit={off * 8}")
        hits = scan(obj, off, ms, needle)
        for p, rel, s in hits[:6]:
            print(f"  strStart bit={p} rel={rel} (bytes {p / 8:.3f}) text={s!r}")
            print(f"     vs obj_start+ms*8: {p - (off + ms) * 8:+d} bits; "
                  f"vs obj_start+bitsize: {p - (off * 8 + bitsize):+d}; "
                  f"vs obj_start: {p - off * 8:+d}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
