# -*- coding: utf-8 -*-
"""Dump the handle stream + data fields of LAYER / BLOCK_HEADER records."""
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


def dump_handles(hdl: Reader, n: int) -> str:
    out = []
    for _ in range(n):
        try:
            code, val = hdl.h()
            out.append(f"({code},{val:X})")
        except objects._Bounds:
            out.append("ovf")
            break
    return " ".join(out)


def main() -> int:
    c = open_dwg(DWG)
    sec = c.section_bytes("AcDb:AcDbObjects")
    entries = sorted(objects.read_handles_map(c.section_bytes("AcDb:Handles")))
    for i, (off, h) in enumerate(entries):
        body = sec[off:entries[i + 1][0] if i + 1 < len(entries) else len(sec)]
        r = Reader(body, 0)
        r.ms()
        typ = r.bs()
        if typ not in (0x31, 0x33, 0x39):
            continue
        bitsize = r.rl()
        strr = _string_stream(body, 16 + bitsize)
        name = strr.tv() if strr is not None else ""
        main_r = Reader(body, r.pos, 16 + bitsize)
        main_r.h()
        objects._read_eed(main_r, None)  # type: ignore[arg-type]
        nr = main_r.bl()
        xdic = main_r.bit()
        hdl = Reader(body, 16 + bitsize, len(body) * 8)
        hdl.h()  # owner
        print(f"\n== type={typ:X} h={h:X} name={name!r} reactors={nr} xdic={xdic}")
        print("   owner+hdl:", dump_handles(Reader(body, 16 + bitsize - 8, len(body) * 8), 1)[:0] or "")
        try:
            if typ == 0x33:
                xref = main_r.bs()
                values = main_r.bs()
                idx = main_r.bs()
                rgb = main_r.bl()
                flags = main_r.rc()
                print(f"   xref={xref} values={values:X} cmc=(idx={idx},rgb={rgb:X},flags={flags:X})")
                print("   handles:", dump_handles(hdl, 8))
            elif typ == 0x31:
                xref = main_r.bs()
                bits = [main_r.bit() for _ in range(5)]
                owned = main_r.bl()
                base = [main_r.bd(), main_r.bd(), main_r.bd()]
                xrefpath = strr.tv() if strr.end > strr.pos else ""
                ins = []
                while True:
                    v = main_r.rc()
                    if not v:
                        break
                    ins.append(v)
                desc = strr.tv() if strr.end > strr.pos else ""
                print(f"   xref={xref} bits={bits} owned={owned} base={base} "
                      f"xrefpath={xrefpath!r} inserts={ins} desc={desc!r}")
                print("   handles:", dump_handles(hdl, 6))
        except objects._Bounds as exc:
            print("   Bounds:", exc)
    return 0


if __name__ == "__main__":
    sys.exit(main())
