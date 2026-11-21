# -*- coding: utf-8 -*-
"""临时探针：1021 样例对象头 + 公共段口径核对。"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream

data = open(r"G:\PyArx\examples\cad2d\dwg1021\obj_full.bin", "rb").read()


def read_h(b):
    c = b.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | b.read_rc()
    return c, sz, v


def read_common(b):
    preview = b.read_b()
    if preview:
        pvsz = b.read_rl(); b.read_tf(pvsz)
    entmode = b.read_bb()
    num_reactors = b.read_bl()
    xdic_missing = b.read_b()
    raw = b.read_bs()
    flag = raw >> 8
    if flag & 0x80: b.read_bl()
    if flag & 0x20: b.read_bl()
    if (flag & 0x41) == 0x41: b.read_tv()
    if (flag & 0x42) == 0x42: b.read_tv()
    b.read_bd()
    ltype_flags = b.read_bb()
    plotstyle_flags = b.read_bb()
    b.read_bs(); b.read_rc()
    return {"color": raw & 0x1ff, "raw": raw, "entmode": entmode,
            "reactors": num_reactors, "xdic": xdic_missing,
            "ltype": ltype_flags, "plot": plotstyle_flags}


samples = [("LINE", 14006808), ("CIRCLE", 53222), ("ARC", 18921),
           ("POINT", 19923), ("INSERT", 19687), ("LWPOLY", 15202984)]

for name, addr in samples:
    b = BitStream(data); b.byte = addr; b.bit = 0
    t = b.read_bs(); bitsize = b.read_rl(); _, _, handle = read_h(b)
    print(f"{name:8s} addr={addr} type={t} bitsize={bitsize} handle={handle} bitpos={b.byte}:{b.bit}")
