# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress, DecompressError
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg); s=c.sections["AcDb:Handles"]
pg=s.pages[10]
payload,hdr=c.read_data_page(pg["page_id"])
buf=bytearray(hdr["page_size"])
try:
    decompress(payload, len(buf), 0, out=buf, out_pos=0)
    print("decompressed OK, nonzero:", sum(1 for x in buf if x))
except DecompressError as e:
    print("ERR", e)
print("first 16:", bytes(buf[:16]).hex(' '))
