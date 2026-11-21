# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress, DecompressError
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:Handles"]
buf = bytearray(s.max_page_size * s.page_count)
for i, pg in enumerate(s.pages):
    payload, hdr = c.read_data_page(pg["page_id"])
    off = pg["start_offset"]
    try:
        decompress(payload, len(buf), 0, out=buf, out_pos=off)
        print(f"page {i} pid={pg['page_id']} off={off} ok data={hdr['data_size']} page={hdr['page_size']}")
    except DecompressError as e:
        print(f"page {i} pid={pg['page_id']} off={off} FAIL {e}")
        break
open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","wb").write(buf)
print("written", len(buf))
