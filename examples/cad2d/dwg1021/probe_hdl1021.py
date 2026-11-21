# -*- coding: utf-8 -*-
"""R2007 句柄流校验：解析 hdl_full.bin 的 UMC/MC 偏移序列，与 arx_all1021.tsv 地址列比对。"""
from __future__ import annotations

import csv
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "dwg1018"))
from bitio import BitStream  # noqa: E402


# 对象地址基准：obj 流前 2 字节为头，address = last_offset + ADDR_BASE（实测 224,906 条差 2，6 条跨段特殊对象差 4）。
ADDR_BASE = 2


def parse_hdl(buf: bytes):
    bs = BitStream(buf)
    endpos = len(buf)
    offsets: list[int] = []
    sections = 0
    while True:
        startpos = bs.byte
        section_size = bs.read_rs_be()
        if section_size > 2050:
            raise ValueError(f"section_size={section_size} > 2050")
        end = startpos + section_size
        last_offset = 0
        sections += 1
        while bs.byte < end:
            oldpos = bs.byte
            handleoff = bs.read_umc()
            offset = bs.read_mc()
            if bs.byte == oldpos:
                break
            last_offset += offset
            offsets.append(last_offset)
        _crc2 = bs.read_rs_be()
        if bs.byte >= endpos:
            break
        if section_size <= 2:
            break
    return offsets, sections


def main() -> int:
    buf = (HERE / "hdl_full.bin").read_bytes()
    offsets, sections = parse_hdl(buf)
    print(f"parsed objects={len(offsets)} sections={sections} hdl_size={len(buf)}")

    ref = []
    with open(HERE / "arx_all1021.tsv", encoding="latin-1") as f:
        for row in csv.reader(f, delimiter="\t"):
            if len(row) >= 2:
                try:
                    ref.append(int(row[1]))
                except ValueError:
                    pass
    print(f"reference objects={len(ref)}")

    n = min(len(offsets), len(ref))
    mismatch = [(i, offsets[i] + ADDR_BASE, ref[i]) for i in range(n)
                if offsets[i] + ADDR_BASE != ref[i]]
    print(f"compared={n} mismatch={len(mismatch)} (已知 6 条跨段对象差 2)")
    for i, o, r in mismatch[:10]:
        print(f"  #{i} got={o} ref={r}")
    print("first5 offsets+base:", [o + ADDR_BASE for o in offsets[:5]])
    print("ref  first5:", ref[:5])
    return 0 if len(offsets) == len(ref) else 1


if __name__ == "__main__":
    raise SystemExit(main())
