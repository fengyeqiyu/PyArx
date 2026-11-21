# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:AcDbObjects"]
pg = s.pages[0]
payload, hdr = c.read_data_page(pg["page_id"])
# 头部连续零
z = 0
for b in payload:
    if b == 0: z += 1
    else: break
print("leading zeros:", z, "data_size", hdr["data_size"], "page_size", hdr["page_size"])
print("payload @0:", payload[:24].hex(' '))
# 源码 read_literal_length: opcode1=0x00 -> low=0, 循环读字节累加0xFF直到非零
# 前 z 个零：每读到 0 累加 0xFF；第 z+1 非零退出
print("first nonzero byte @", z, "=", hex(payload[z]) if z<len(payload) else "EOF")
