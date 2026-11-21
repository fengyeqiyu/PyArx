# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")
print("head RL", hex(struct.unpack_from("<I", obj, 0)[0]))

def h_read(bs):
    # H handle: 通常为 UMC/RS；先按 UMC
    return bs.read_umc()

for start in range(0, 16):
    for lead in ("RL", "MS"):
        bs = BitStream(obj); bs.byte = start; bs.bit = 0
        try:
            size = bs.read_rl() if lead == "RL" else bs.read_ms()
            typ = bs.read_bs()
            h = h_read(bs)
            ext = bs.read_bs()
            ok_type = 0 <= typ <= 0x1f3
            ok_h = 1 <= h <= 0x100000
            if ok_type and ok_h and 0 <= ext < 4000:
                print(f"@{start} {lead}={size} type={typ} H={h:#x} ext={ext} end={bs.byte}.{bs.bit}")
        except Exception:
            pass
