# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg, SEC_MASK
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:Handles"]
for pg in s.pages[:4]:
    addr = c._page_addr(pg["page_id"])
    raw = struct.unpack_from("<8I", c.data, addr)
    mask = SEC_MASK ^ addr
    hdr = [v ^ mask for v in raw]
    print("pid", pg["page_id"], "addr", hex(addr),
          "type", hex(hdr[0]), "sec", hdr[1], "data", hdr[2],
          "page", hdr[3], "off", hdr[4], "unk", hdr[5],
          "hcrc", hex(hdr[6]), "dcrc", hex(hdr[7]))
