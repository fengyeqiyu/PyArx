# -*- coding: utf-8 -*-
"""Trace table records: bitsize, string stream, first strings."""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

from PyArxDwg.container import open_dwg  # noqa: E402
from PyArxDwg import objects  # noqa: E402
from PyArxDwg.objects import Reader, _string_stream  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-10KV电缆桥架.dwg")


def main() -> int:
    c = open_dwg(DWG)
    objs = c.section_bytes("AcDb:AcDbObjects")
    hdls = c.section_bytes("AcDb:Handles")
    entries = sorted(objects.read_handles_map(hdls))
    want = {0x31: "BLOCK_HEADER", 0x33: "LAYER", 0x39: "LTYPE"}
    shown = {0x31: 6, 0x33: 4, 0x39: 0}
    for i, (off, h) in enumerate(entries):
        obj = objs[off:entries[i + 1][0] if i + 1 < len(entries) else len(objs)]
        r = Reader(obj, 0)
        try:
            ms = r.ms()
            typ = r.bs()
            bitsize = r.rl()
        except objects._Bounds:
            continue
        if typ not in want or shown[typ] <= 0:
            continue
        shown[typ] -= 1
        strr = _string_stream(obj, 16 + bitsize)
        strs = []
        if strr is not None:
            for _ in range(4):
                try:
                    strs.append(strr.tv())
                except objects._Bounds:
                    break
        print(f"{want[typ]:12s} h={h:X} off={off} ms={ms} bitsize={bitsize} "
              f"body={bitsize - (r.pos - 0)} str={None if strr is None else (strr.pos, strr.end)} "
              f"strings={strs}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

