# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
c=open_dwg(r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg")
s=c.sections["AcDb:AcDbObjects"]
for idx in [0,1]:
    pg=s.pages[idx]
    payload,hdr=c.read_data_page(pg["page_id"])
    open(rf"G:\PyArx\examples\cad2d\dwg1018\obj{idx}_comp.bin","wb").write(payload)
    print("page",idx,"data_size",hdr["data_size"],"page_size",hdr["page_size"])
