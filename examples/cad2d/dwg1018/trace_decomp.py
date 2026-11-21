# -*- coding: utf-8 -*-
"""跟踪 R2004 解压首段，定位 opcode 分歧。"""
import struct
import sys

sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")

p = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
d = open(p, "rb").read()

sig = struct.pack("<I", 0x41630E3B)
hit = d.find(sig)
t, ds, cs, ct, ck = struct.unpack_from("<LLLLL", d, hit)
payload = d[hit + 0x14: hit + 0x14 + cs]
print("decomp", ds, "comp", cs, "ctype", ct)
print("payload:", payload[:32].hex())

# 手工解析 Literal Length
def litlen(src, pos):
    op = src[pos]; pos += 1
    if op == 0x00:
        total = 0x0F
        while True:
            b = src[pos]; pos += 1
            if b == 0x00:
                total += 0xFF
            else:
                total += b
                break
        return total + 3, pos
    if 0x01 <= op <= 0x0E:
        return op - 0x12 + 3, pos
    if op == 0xF0:
        return 0, pos - 1
    raise ValueError(f"bad lit op {op:02X}")

n, pos = litlen(payload, 0)
print("first literal length =", n, "pos ->", pos)
print("literal bytes:", payload[pos:pos+n].hex())
