# -*- coding: utf-8 -*-
"""R2007 地图探针：头 -> 页地图 -> 节地图，列节名/页数/页 id。"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from compress1021 import decode_rs, decompress_r2007  # noqa: E402
from container1021 import (  # noqa: E402
    read_system_page, parse_pages_map, parse_sections_map, find_page,
    HEADER_FIXED, OVERREAD, PAGES_MAP_BASE)

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
    compr_len, len2 = struct.unpack_from("<ii", pedata, 24)
    out_size = len(HDR_FIELDS) * 8
    hdr = decompress_r2007(out_size, pedata[32:], min(len(pedata) - 32, compr_len)) \
        if compr_len > 0 else bytearray(pedata[32:32 + out_size])
    vals = struct.unpack_from(f"<{len(HDR_FIELDS)}Q", hdr, 0)
    return dict(zip(HDR_FIELDS, vals))


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
    data = path.read_bytes()
    h = read_header(data)
    print(f"num_sections={h['num_sections']} pages_amount={h['pages_amount']}")

    # --- Pages Map ---
    pm_addr = PAGES_MAP_BASE + h["pages_map_offset"]
    print(f"\npages_map @0x{pm_addr:X} comp={h['pages_map_size_comp']} "
          f"uncomp={h['pages_map_size_uncomp']} correction={h['pages_map_correction']}")
    pm_buf = read_system_page(data, pm_addr, h["pages_map_size_comp"],
                              h["pages_map_size_uncomp"], h["pages_map_correction"])
    pages = parse_pages_map(pm_buf, h["pages_amount"])
    ids = sorted(p.page_id for p in pages)
    print(f"pages parsed={len(pages)} id range=[{ids[0]}, {ids[-1]}]")

    # --- Sections Map ---
    sec_page = find_page(pages, h["sections_map_id"])
    if not sec_page:
        print(f"!! sections_map_id={h['sections_map_id']} not in pages map")
        return 1
    print(f"\nsections_map page id={sec_page.page_id} @0x{sec_page.offset:X} "
          f"comp={h['sections_map_size_comp']} uncomp={h['sections_map_size_uncomp']} "
          f"correction={h['sections_map_correction']}")
    sm_buf = read_system_page(data, sec_page.offset, h["sections_map_size_comp"],
                              h["sections_map_size_uncomp"], h["sections_map_correction"])
    sections = parse_sections_map(sm_buf)
    print(f"sections parsed={len(sections)}")
    for s in sections:
        pids = [p.page_id for p in s.pages]
        print(f"  {s.name:28s} data_size={s.data_size:<8} pages={s.num_pages:<3} "
              f"page_ids={pids}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
