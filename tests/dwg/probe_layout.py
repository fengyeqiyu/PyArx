# -*- coding: utf-8 -*-
"""Probe R2007 object header/string-stream layout against spec 5.x text."""
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


def bits_at(data: bytes, bit: int, n: int = 16) -> int:
    r = Reader(data, bit)
    return r.rs() if n == 16 else r.rc()


def probe(data: bytes, label: str, off: int, truth_str: str, truth_byte: int) -> None:
    r = Reader(data, off * 8)
    ms = r.ms()
    pos_ms = r.pos
    typ = r.bs()
    pos_typ = r.pos
    bitsize = r.rl()
    pos_rl = r.pos
    print(f"\n== {label} @byte {off} truth_str={truth_str!r} truth_str_byte={truth_byte}")
    print(f" MS size={ms} bytes (pos after={pos_ms // 8}.{pos_ms % 8}) type=0x{typ:X} "
          f"bitsize={bitsize} pos_after_rl={pos_rl // 8}.{pos_rl % 8}")
    end_obj = off + ms + 2  # MS size excludes CRC(2)
    cand = {
        "obj_start+bitsize": off * 8 + bitsize,
        "after_ms+bitsize": pos_ms + bitsize,
        "after_rl+bitsize": pos_rl + bitsize,
    }
    for k, v in cand.items():
        flag = Reader(data, v - 1).bit()
        print(f" hdlStart[{k}] = byte {v // 8}.{v % 8} hasStrBit={flag}")
    for k, v in cand.items():
        hdl = v
        for endname, endbit in (("off+ms+2", end_obj * 8), ("off+ms", (off + ms) * 8)):
            e = endbit - 128
            lo = bits_at(data, e)
            size = lo
            e2 = e
            hi = 0
            if lo & 0x8000:
                e2 = e - 128
                hi = bits_at(data, e2)
                size = (lo & 0x7FFF) | (hi << 15)
            sstart = (e2 - size * 8) // 8
            delta = sstart - truth_byte
            print(f"  [{k}][{endname}] strSize={size} (lo=0x{lo:04X} hi={hi}) "
                  f"strStartByte={sstart} delta={delta:+d}")
    # raw tail
    print(" tail bytes:", data[off + ms - 8: off + ms + 10].hex())


def main() -> int:
    c = open_dwg(DWG)
    obj = c.section_bytes("AcDb:AcDbObjects")
    print("objects section bytes:", len(obj))
    for t in TARGETS:
        probe(obj, *t)
    return 0


if __name__ == "__main__":
    sys.exit(main())
