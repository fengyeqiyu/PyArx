# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg); s=c.sections["AcDb:Handles"]; pg=s.pages[0]
payload,hdr=c.read_data_page(pg["page_id"])
print("payload len", len(payload))
print("src 720..736:", payload[720:736].hex(' '))
# op=0x20 @724, long offset 从725
print("byte@725", hex(payload[725]))
