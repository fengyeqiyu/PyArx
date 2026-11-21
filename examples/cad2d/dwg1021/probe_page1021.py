# -*- coding: utf-8 -*-
"""单页调试：AcDbObjects 首页(pid=7) RS/解压路径对比。"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from compress1021 import decode_rs, decompress_r2007, DecompressError  # noqa: E402
from container1021 import (  # noqa: E402
    read_system_page, parse_pages_map, parse_sections_map, find_page,
    HEADER_FIXED, PAGES_MAP_BASE, RS_K, RS_N)
from probe_dump1021 import read_header, HDR_FIELDS  # noqa: E402

data = (HERE / "test2007.dwg").read_bytes()
h = read_header(data)
pm = read_system_page(data, PAGES_MAP_BASE + h["pages_map_offset"],
                      h["pages_map_size_comp"], h["pages_map_size_uncomp"],
                      h["pages_map_correction"])
pages = parse_pages_map(pm, h["pages_amount"])
sec_page = find_page(pages, h["sections_map_id"])
sm = read_system_page(data, sec_page.offset, h["sections_map_size_comp"],
                      h["sections_map_size_uncomp"], h["sections_map_correction"])
secs = [s for s in parse_sections_map(sm) if s.data_size > 0]
obj = next(s for s in secs if s.name == "AcDb:AcDbObjects")
sp = obj.pages[0]
page = find_page(pages, sp.page_id)
print(f"pid={sp.page_id} page.off=0x{page.offset:X} page.size={page.size}")
print(f"sp: off={sp.offset} comp={sp.comp_size} uncomp={sp.uncomp_size}")

disk = data[page.offset:page.offset + page.size]
print(f"disk first16={disk[:16].hex()}")

pesize = (sp.comp_size + 7) & ~7
bc = (pesize + RS_K - 1) // RS_K
print(f"block_count(comp)={bc} pedata_size={bc*RS_K} disk={page.size} bc*255={bc*RS_N}")

# 路径 A：decode_rs 后解压
try:
    pedata = decode_rs(disk, bc, RS_K)
    print(f"\n[A] rs pedata first16={bytes(pedata[:16]).hex()}")
    out = decompress_r2007(sp.uncomp_size, pedata, min(bc * RS_K, sp.comp_size), trace=True)
    nz = sum(1 for b in out if b)
    print(f"[A] decompressed ok nonzero={nz}/{len(out)} first16={bytes(out[:16]).hex()}")
except DecompressError as e:
    print(f"[A] fail: {e}")

# 路径 A2：块数按磁盘大小反推（page.size/255）
bc2 = page.size // RS_N
print(f"\n[A2] block_count(disk/255)={bc2}")
try:
    pedata2 = decode_rs(disk, bc2, RS_K)
    print(f"[A2] rs pedata first16={bytes(pedata2[:16]).hex()}")
    out2 = decompress_r2007(sp.uncomp_size, pedata2, min(bc2 * RS_K, sp.comp_size))
    nz = sum(1 for b in out2 if b)
    print(f"[A2] decompressed ok nonzero={nz}/{len(out2)} first16={bytes(out2[:16]).hex()}")
except DecompressError as e:
    print(f"[A2] fail: {e}")

# 路径 B：不经 RS，直接解压磁盘 comp 字节
try:
    out = decompress_r2007(sp.uncomp_size, disk, sp.comp_size)
    nz = sum(1 for b in out if b)
    print(f"[B] decompressed ok nonzero={nz}/{len(out)} first16={bytes(out[:16]).hex()}")
except DecompressError as e:
    print(f"[B] fail: {e}")
