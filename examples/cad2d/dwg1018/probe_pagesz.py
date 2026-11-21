# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg, SEC_MASK

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
for name in ("AcDb:AcDbObjects", "AcDb:Handles"):
    s = c.sections[name]
    print(f"== {name} max={s.max_page_size:#x}")
    for pg in s.pages[:4]:
        addr = c._page_addr(pg["page_id"])
        hdr = [v ^ (SEC_MASK ^ addr) for v in struct.unpack_from("<8I", c.data, addr)]
        print(f"  pid={pg['page_id']} data_size={pg['data_size']} "
              f"hdr.data_size={hdr[2]} page_size={hdr[3]} start={hdr[4]:#x}")
