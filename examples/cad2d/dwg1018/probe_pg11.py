# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg)
s=c.sections["AcDb:Handles"]
for i,pg in enumerate(s.pages):
    print(i,"pid",pg["page_id"],"data_size",pg["data_size"],"start",pg["start_offset"])
