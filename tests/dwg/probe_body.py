# -*- coding: utf-8 -*-
"""Field-level probe for ELLIPSE / SPLINE / POLYLINE bodies."""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

from PyArxDwg.container import open_dwg  # noqa: E402
from PyArxDwg import objects  # noqa: E402
from PyArxDwg.objects import Reader, _string_stream  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-强电.dwg")


def body_of(handle: int):
    c = open_dwg(DWG)
    sec = c.section_bytes("AcDb:AcDbObjects")
    entries = sorted(objects.read_handles_map(c.section_bytes("AcDb:Handles")))
    # entries are (offset, handle)
    for i, (off, h) in enumerate(entries):
        if h != handle:
            continue
        body = sec[off:entries[i + 1][0] if i + 1 < len(entries) else len(sec)]
        r = Reader(body, 0)
        r.ms()
        typ = r.bs()
        bitsize = r.rl()
        strr = _string_stream(body, 16 + bitsize)
        main = Reader(body, r.pos, 16 + bitsize)
        main.h()
        objects._read_eed(main, None)  # type: ignore[arg-type]
        objects._common_entity(main, Reader(body, 16 + bitsize, len(body) * 8), strr, handle)
        return typ, strr, main
    return None, None, None


def main() -> int:
    for handle, kind in ((0x2AA, "ELLIPSE"), (0x5E4, "SPLINE"), (0x6E7, "POLYLINE")):
        typ, strr, r = body_of(handle)
        print(f"\n== {kind} h={handle:X} type={typ:X} pos={r.pos}")
        try:
            if kind == "ELLIPSE":
                print("  center", [round(r.bd(), 6) for _ in range(3)])
                print("  axis", [round(r.bd(), 6) for _ in range(3)])
                print("  next-bit/pos", r.pos, "bit?", )
                p = r.pos
                print("  raw3bd", [round(r.bd(), 6) for _ in range(3)])
                print("  after raw3bd ratio", round(r.bd(), 6), "start", round(r.bd(), 6),
                      "end", round(r.bd(), 6), "pos", r.pos)
            elif kind == "SPLINE":
                print("  scenario/degree bs:", r.bs(), r.bs())
                print("  bits:", r.bit(), r.bit(), r.bit())
                print("  tolerances:", round(r.bd(), 6), round(r.bd(), 6))
                nk, nc = r.bl(), r.bl()
                w = r.bit()
                print("  numknots:", nk, "numctrl:", nc, "weight bit:", w)
                for _ in range(nk):
                    r.bd()
                print("  after knots pos", r.pos)
                for _ in range(nc):
                    r.bd(), r.bd(), r.bd()
                print("  after ctrl pos", r.pos, "end", r.end)
                print("  remaining bits", r.end - r.pos)
            else:
                print("  rc1:", r.rc(), "rc2:", r.rc())
                print("  pos", r.pos, "end", r.end)
                try:
                    print("  bl:", r.bl())
                except objects._Bounds as exc:
                    print("  bl bounds:", exc)
        except objects._Bounds as exc:
            print("  bounds:", exc)
    return 0


if __name__ == "__main__":
    sys.exit(main())
