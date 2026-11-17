# -*- coding: utf-8 -*-
"""Replay LibreDWG obj_string_stream variants and compare with empirical starts."""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

from PyArxDwg.container import open_dwg  # noqa: E402
from PyArxDwg.objects import Reader  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-10KV电缆桥架.dwg")
# label, obj byte, empirical string-stream prefix bit (relative to obj start)
TARGETS = [("LAYER", 32187, 143), ("MTEXT", 10999, 1311)]


def rs_at_bit(data: bytes, base_bit: int, pos: int) -> int:
    return Reader(data, base_bit + pos).rs()


def variant(data: bytes, off: int, bitsize: int, origin: int, back: int,
            rs_back: int) -> dict:
    """origin: bit offset in object where bitsize counting starts.
    back: bits after reading B before the -2 byte step.
    rs_back: byte offset applied before reading RS."""
    base = off * 8 + origin
    start = origin + bitsize - 1
    flag = Reader(data, off * 8 + start).bit()
    pos = bitsize - 1 - back
    pos = ((pos // 8) - rs_back) * 8 + pos % 8
    size = rs_at_bit(data, off * 8, pos)
    if size & 0x8000:
        pos2 = ((pos // 8) - 4) * 8 + pos % 8
        hi = rs_at_bit(data, off * 8, pos2)
        size = (size & 0x7FFF) | (hi << 15)
        pos = pos2
    pos_after = pos + 16
    pos_after = ((pos_after // 8) - rs_back) * 8 + pos_after % 8
    sstart = pos_after - size * 8
    return {"flag": flag, "size": size, "sstart": sstart, "start_b": start}


def main() -> int:
    c = open_dwg(DWG)
    obj = c.section_bytes("AcDb:AcDbObjects")
    for label, off, truth in TARGETS:
        r = Reader(obj, off * 8)
        ms = r.ms()
        typ = r.bs()
        bitsize = r.rl()
        print(f"\n== {label} off={off} ms={ms} bitsize={bitsize} truth_rel={truth}")
        for origin, oname in ((0, "objstart"), (16, "afterMS"), (26, "afterType"),
                              (58, "afterRL")):
            for back in (1, 0):
                for rs_back in (2, 1, 0):
                    v = variant(obj, off, bitsize, origin, back, rs_back)
                    mark = "OK" if v["sstart"] == truth else ""
                    print(f" origin={oname:9s} back={back} rsb={rs_back}: "
                          f"flag={v['flag']} size={v['size']} sstart={v['sstart']} "
                          f"(truth {truth}, d={v['sstart'] - truth:+d}) {mark}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
