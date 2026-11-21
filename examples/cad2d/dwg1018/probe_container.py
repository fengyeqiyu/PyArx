# -*- coding: utf-8 -*-
"""探针：打开容器，打印页地图与节地图，解压各数据节核对大小。"""
import sys
import os

sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg


def main(path):
    c = open_dwg(path)
    print("version", c.version, "maint", c.maintenance, "codepage", c.codepage,
          "security", hex(c.security))
    print("header:", {k: (hex(v) if isinstance(v, int) and v > 9 else v)
                      for k, v in c.header.items()})
    print(f"\npage map: {len(c.page_map)} entries")
    gaps = [p for p in c.page_map.values() if p.is_gap]
    print(f"  gaps: {len(gaps)}")

    print(f"\nsection map: {len(c.sections)} sections")
    for name, d in c.sections.items():
        print(f"  {name:24s} id={d.section_id:3d} size={d.size:9d} pages={d.page_count:3d} "
              f"max={d.max_page_size:#x} comp={d.compressed} enc={d.encrypted}")

    # 尝试解压每个节的第一页，核对解压大小
    print("\n-- first page decompress check --")
    for name, d in c.sections.items():
        if not d.pages:
            continue
        p = d.pages[0]
        try:
            raw = c.read_data_page(p["page_id"], p["data_size"],
                                   min(d.max_page_size, d.size), p["start_offset"])
            print(f"  {name:24s} page {p['page_id']:4d} -> {len(raw)} bytes "
                  f"(expect ~{min(d.max_page_size, d.size)})")
        except Exception as e:
            print(f"  {name:24s} page {p['page_id']:4d} FAIL: {type(e).__name__}: {e}")


if __name__ == "__main__":
    main(sys.argv[1])
