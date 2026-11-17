# -*- coding: utf-8 -*-
"""Scan object tail for a short S such that (byte_of_S - S) lands on the truth string start."""
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
    ("LAYER", 32187, "E-CABL-TRAY", 32204),
    ("MTEXT", 10999, "8890.00012", 11668),
]


def rs_at(data: bytes, b: int) -> int:
    return Reader(data, b * 8).rs()


def main() -> int:
    c = open_dwg(DWG)
    obj = c.section_bytes("AcDb:AcDbObjects")
    for label, off, s, truth in TARGETS:
        r = Reader(obj, off * 8)
        ms = r.ms()
        typ = r.bs()
        bitsize = r.rl()
        print(f"\n== {label} off={off} ms={ms} type=0x{typ:X} bitsize={bitsize}")
        print(" hexdump:")
        for row in range(off, off + ms + 4, 16):
            chunk = obj[row:row + 16]
            mark = "".join("." if row + i < truth else ">" for i in range(len(chunk)))
            print(f"  {row:6d} {chunk.hex(' ')}  {mark}")
        print("  RS scan (b, value, b-value) where b-value == truth_byte:")
        for b in range(off + 4, off + ms + 2):
            v = rs_at(obj, b)
            if b - v == truth:
                print(f"   b={b} v={v} -> {b - v}")
        print("  RS values near tail:")
        for b in range(off + ms - 8, off + ms + 2):
            print(f"   b={b} rs=0x{rs_at(obj, b):04X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
