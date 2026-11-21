# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

def read_ms(b, i):
    result = 0
    for k in range(1, -1, -1):
        lo, hi = b[i], b[i+1]; i += 2
        word = (hi << 8) | lo
        if not (word & 0x8000):
            return result | word << (k*15), i
        result |= (word & 0x7FFF) << (k*15)
    return result, i

# 合法 type 大致范围 0..130；扫描每个偏移
for a in range(0, 300):
    try:
        size, j = read_ms(obj, a)
        if 1 <= size <= 6000:
            bs = struct.unpack_from("<h", obj, j)[0]
            if 0 <= bs <= 130:
                print(f"candidate @{a}: MS={size} BS={bs} nextbyte={j}")
    except Exception:
        pass
