# -*- coding: utf-8 -*-
"""Parse LAYER data after common-object fields; report candidates vs truth colour."""
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


def cmc(r: Reader):
    index = r.bs()
    rgb = r.bl()
    flags = r.rc()
    if rgb == 0xC0000000:
        return 0, rgb
    if rgb & 0x100:
        return rgb & 0xFF, rgb
    if rgb:
        return -1, rgb
    return (256 if index == 256 else index), rgb


def main() -> int:
    c = open_dwg(DWG)
    sec = c.section_bytes("AcDb:AcDbObjects")
    entries = sorted(objects.read_handles_map(c.section_bytes("AcDb:Handles")))
    for i, (off, h) in enumerate(entries):
        body = sec[off:entries[i + 1][0] if i + 1 < len(entries) else len(sec)]
        r = Reader(body, 0)
        ms = r.ms()
        typ = r.bs()
        if typ != 0x33:
            continue
        bitsize = r.rl()
        strr = _string_stream(body, 16 + bitsize)
        name = strr.tv() if strr is not None else ""
        # common object data: own handle, eed, then num reactors + xdic flag
        main = Reader(body, r.pos, 16 + bitsize)
        main.h()
        objects._read_eed(main, None)  # type: ignore[arg-type]
        nr = main.bl()
        xdic = main.bit()
        print(f"\n== LAYER {name!r} h={h:X} numreactors={nr} xdic={xdic} pos={main.pos}")
        for label, pre in (
            ("ACadSharp bs+values", ""),
            ("spec bit+bs+bit+values", "bib"),
            ("bit+bs+values", "ib"),
        ):
            rr = Reader(body, main.pos)
            try:
                if pre == "bib":
                    rr.bit()
                    rr.bs()
                    rr.bit()
                elif pre == "ib":
                    rr.bit()
                    rr.bs()
                elif pre == "":
                    rr.bs()
                values = rr.bs()
                color, rgb = cmc(rr)
            except objects._Bounds:
                print(f"  {label}: overrun")
                continue
            print(f"  {label}: values={values} color={color} rgb={hex(rgb)} end={rr.pos} "
                  f"(str_start={None if strr is None else strr.pos})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
