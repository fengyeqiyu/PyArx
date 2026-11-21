# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg, SEC_MASK
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg); s=c.sections["AcDb:Handles"]
for i,pg in enumerate(s.pages[:12]):
    addr=c._page_addr(pg["page_id"])
    raw=struct.unpack_from("<8I", c.data, addr)
    mask=SEC_MASK^addr
    hdr=[v^mask for v in raw]
    print(i, "pid",pg["page_id"],"data",hdr[2],"page",hdr[3],"start",hdr[4],"stored_off",pg["start_offset"])
