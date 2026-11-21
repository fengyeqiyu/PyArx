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

# 按字节边界扫描：MS 后 BS（BS 紧凑：BB 2bit 不便于字节对齐，先试首字段为 RL/RC）
hits = 0
for a in range(0, 400):
    # 试 RL: 两个 RS
    w1 = struct.unpack_from("<H", obj, a)[0]
    w2 = struct.unpack_from("<H", obj, a+2)[0]
    rl = (w2 << 16) | w1
    if 4 <= rl <= 2000:
        # 后两字节当 RC 类型
        t = obj[a+4]
        if t < 200:
            print(f"RL candidate @{a}: RL={rl} nextRC={t:#x} bytes={obj[a:a+8].hex(' ')}")
            hits += 1
            if hits > 15: break
print("hits", hits)
