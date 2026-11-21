# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:Handles"]
pg = s.pages[0]
payload, hdr = c.read_data_page(pg["page_id"])
print("data_size", hdr["data_size"], "page_size", hdr["page_size"])
print("payload head:", payload[:16].hex(' '))
buf = bytearray(hdr["page_size"])
decompress(payload, hdr["page_size"], 0, out=buf, out_pos=0)
last = max(i for i,b in enumerate(buf) if b)
print("decomp last nonzero", last, "of", len(buf))
print("decomp head:", bytes(buf[:24]).hex(' '))
