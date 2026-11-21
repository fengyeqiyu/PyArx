# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

# 从 @8.0 起：RL size, BS type，然后跳过 size 字节看下一对象
def try_parse(start, n=6):
    bs = BitStream(obj)
    bs.byte = start; bs.bit = 0
    for k in range(n):
        p0 = bs.byte
        size = bs.read_rl()
        typ = bs.read_bs()
        print(f"  obj{k} @{p0}: RLsize={size} BStype={typ} end={bs.byte}.{bs.bit}")
        if size < 0 or size > 200000:
            print("  size 异常，停"); break
        # 跳到下一个对象：dat->size=size, chain+=pos；简化：下一字节边界 + size
        bs.byte = p0 + size
        bs.bit = 0

print("=== @8.0 RL/BS ===")
try_parse(8)
