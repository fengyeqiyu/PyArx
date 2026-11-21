# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg, SEC_MASK
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:Handles"]
for pg in s.pages[:2]:
    addr = c._page_addr(pg["page_id"])
    raw = struct.unpack_from("<8I", c.data, addr)
    mask = SEC_MASK ^ addr
    hdr = [v ^ mask for v in raw]
    labels = ["type","sec","data_size(comp)","page_size(decomp)","address","unk","hdr_crc","data_crc"]
    print(f"pid={pg['page_id']} addr=0x{addr:X}")
    for l, v in zip(labels, hdr):
        print(f"   {l:18s} {v}")
