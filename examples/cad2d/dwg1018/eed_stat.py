# -*- coding: utf-8 -*-
"""离线 size=98 对象的 EED 长度/结束位分布，对照 C trace entmode byte。"""
import sys
from collections import Counter
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()


def read_h(x):
    c = x.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | x.read_rc()
    return c, sz, v


def eed_probe(addr):
    b = BitStream(data); b.byte = addr
    b.read_bs(); b.read_rl(); read_h(b)
    size = b.read_bs()
    if not size:
        return 0, b.byte - addr, b.bit, 0
    read_h(b); end = b.byte + size
    q = BitStream(data); q.byte = b.byte; q.bit = b.bit
    ncode = 0
    while q.byte < end:
        cc = q.read_rc(); ncode += 1
        if cc == 70: q.read_rs()
        elif cc == 71: q.read_rl()
        elif cc in (3, 5): q.read_rll_be()
        elif cc == 1: q.read_rs()
        elif cc == 2: q.read_rc()
        elif cc == 0:
            l = q.read_rc(); q.read_tf(l)
        elif cc == 4:
            l = q.read_rc(); q.read_tf(l)
        elif cc in (10, 11, 12, 13, 14, 15):
            for _ in range(3): q.read_rd()
        elif cc in (40, 41, 42): q.read_rd()
        else:
            break
    b.byte = q.byte; b.bit = q.bit
    if b.byte < end - 1:
        size = end - b.byte + 1; end = b.byte + size
    return size, end - addr, q.bit, ncode


rows = []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 9 and p[2] == "LINE":
        rows.append((int(p[0]), int(p[1])))

endbyte = Counter(); eedsize = Counter(); samples = {}
for h, addr in rows:
    body = addr - 2
    bb = BitStream(data); bb.byte = body; ms = bb.read_ms()
    if ms != 98:
        continue
    esize, endrel, qbit, ncode = eed_probe(addr)
    endbyte[endrel] += 1
    eedsize[esize] += 1
    samples.setdefault(endrel, (h, addr, esize, qbit, ncode))

print("size=98 数量累计 endbyte 分布:", dict(endbyte.most_common(12)))
print("EED size 值分布:", dict(eedsize.most_common(12)))
for k, v in endbyte.most_common(8):
    print("  endbyte", k, "->", samples[k])
