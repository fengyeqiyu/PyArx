# -*- coding: utf-8 -*-
"""R2007 dump：解压 AcDb:AcDbObjects / AcDb:Handles 全局缓冲，落盘并算 SHA256 基准。"""
from __future__ import annotations

import hashlib
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from compress1021 import decode_rs, decompress_r2007  # noqa: E402
from container1021 import (  # noqa: E402
    read_system_page, parse_pages_map, parse_sections_map, find_page, read_section,
    HEADER_FIXED, PAGES_MAP_BASE)

DEFAULT = HERE / "测试图-V2007.dwg"

HDR_FIELDS = [
    "header_size", "file_size", "pages_map_crc_compressed", "pages_map_correction",
    "pages_map_crc_seed", "pages_map2_offset", "pages_map2_id", "pages_map_offset",
    "pages_map_id", "header2_offset", "pages_map_size_comp", "pages_map_size_uncomp",
    "pages_amount", "pages_maxid", "unknown1", "unknown2", "pages_map_crc_uncomp",
    "unknown3", "unknown4", "unknown5", "num_sections", "sections_map_crc_uncomp",
    "sections_map_size_comp", "sections_map2_id", "sections_map_id",
    "sections_map_size_uncomp", "sections_map_crc_comp", "sections_map_correction",
    "sections_map_crc_seed", "stream_version", "crc_seed", "crc_seed_encoded",
    "random_seed", "header_crc",
]


def read_header(data: bytes) -> dict:
    pedata = decode_rs(data[0x80:0x80 + HEADER_FIXED], 3, 239)
    compr_len, _ = struct.unpack_from("<ii", pedata, 24)
    out_size = len(HDR_FIELDS) * 8
    hdr = decompress_r2007(out_size, pedata[32:], min(len(pedata) - 32, compr_len)) \
        if compr_len > 0 else bytearray(pedata[32:32 + out_size])
    return dict(zip(HDR_FIELDS, struct.unpack_from(f"<{len(HDR_FIELDS)}Q", hdr, 0)))


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
    data = path.read_bytes()
    h = read_header(data)

    pm_addr = PAGES_MAP_BASE + h["pages_map_offset"]
    pm_buf = read_system_page(data, pm_addr, h["pages_map_size_comp"],
                              h["pages_map_size_uncomp"], h["pages_map_correction"])
    pages = parse_pages_map(pm_buf, h["pages_amount"])

    sec_page = find_page(pages, h["sections_map_id"])
    sm_buf = read_system_page(data, sec_page.offset, h["sections_map_size_comp"],
                              h["sections_map_size_uncomp"], h["sections_map_correction"])
    sections = [s for s in parse_sections_map(sm_buf) if s.data_size > 0]
    by_name = {s.name: s for s in sections}

    for key, out_name in (("AcDb:AcDbObjects", "obj_full.bin"),
                          ("AcDb:Handles", "hdl_full.bin")):
        sec = by_name.get(key)
        if not sec:
            print(f"!! section {key} not found")
            return 1
        buf = read_section(data, sec, pages)
        (HERE / out_name).write_bytes(buf)
        sha = hashlib.sha256(buf).hexdigest().upper()
        print(f"{key:22s} -> {out_name}  size={len(buf)}  SHA256={sha}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
