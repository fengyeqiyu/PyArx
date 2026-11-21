# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

def parse(start, label):
    bs = BitStream(obj); bs.byte = start; bs.bit = 0
    size = bs.read_ms()
    typ = bs.read_bs()
    bitsize = bs.read_rl()
    h = bs.read_umc()
    ext = bs.read_bs()
    print(f"{label} @{start}: MSsize={size} BStype={typ} RLbitsize={bitsize} H={h:#x} ext={ext} end={bs.byte}.{bs.bit}")

parse(0, "MS/BS/RL/H")
parse(4, "@4")
