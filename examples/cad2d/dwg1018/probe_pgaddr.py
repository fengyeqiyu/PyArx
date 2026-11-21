# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
c=open_dwg(r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg")
for name in ["AcDb:Handles","AcDb:AcDbObjects"]:
    s=c.sections[name]
    print("===",name,"pages",len(s.pages))
    for pg in s.pages[:4]:
        _,hdr=c.read_data_page(pg["page_id"])
        print(" pid",pg["page_id"],"map_off",pg["start_offset"],"hdr_addr",hdr["start_offset"],"data_size",hdr["data_size"],"page_size",hdr["page_size"])
