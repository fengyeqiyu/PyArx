# -*- coding: utf-8 -*-
"""探测系统页压缩体尾部：数据段与 RS 校验段的边界。"""
import struct
import sys

p = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
d = open(p, "rb").read()

for sig, name in [(0x41630E3B, "PageMap"), (0x4163003B, "SectionMap")]:
    hit = d.find(struct.pack("<I", sig))
    t, ds, cs, ct, ck = struct.unpack_from("<LLLLL", d, hit)
    body = d[hit + 0x14: hit + 0x14 + cs]
    print(f"{name}: decomp={ds} comp={cs}")
    print(f"  body head: {body[:8].hex()}")
    print(f"  body tail: {body[-24:].hex()}")
    # 统计尾部 0 密度
    for back in (0, 2, 4, 6, 8, 12, 16, 20, 24):
        seg = body[cs - back:cs] if back else b""
        zeros = seg.count(0)
        print(f"  tail {back:2d} bytes zeros={zeros}/{len(seg)}")
    print()
