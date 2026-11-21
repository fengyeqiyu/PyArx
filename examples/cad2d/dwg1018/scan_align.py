# -*- coding: utf-8 -*-
"""定位离线流错位起始：按 address 排序 LINE，找第一个 handle 不符的对象。"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()


def read_h(b):
    c = b.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | b.read_rc()
    return c, sz, v


def decode_eed(b):
    end = None
    while True:
        size = b.read_bs()
        if size == 0 or size > 1000:
            break
        read_h(b)
        end = b.byte + size
        q = BitStream(data); q.byte = b.byte; q.bit = b.bit
        while q.byte < end:
            c = q.read_rc()
            if c == 70: q.read_rs()
            elif c == 71: q.read_rl()
            elif c in (3, 5): q.read_rll_be()
            elif c == 1: q.read_rs()
            elif c == 2: q.read_rc()
            elif c == 0:
                l = q.read_rc(); q.read_rs_be(); q.read_tf(l)
            elif c == 4:
                l = q.read_rc(); q.read_tf(l)
            elif c in (10, 11, 12, 13, 14, 15):
                for _ in range(3): q.read_rd()
            elif c in (40, 41, 42): q.read_rd()
            else:
                q.byte = end; q.bit = 0
                break
        b.byte = q.byte; b.bit = q.bit


def hdl_of(addr):
    b = BitStream(data); b.byte = addr
    b.read_bs(); b.read_rl()
    _, _, h = read_h(b)
    return h


rows = []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 9 and p[2] == "LINE":
        rows.append((int(p[1]), int(p[0])))
rows.sort()

bad = []
good = 0
for addr, h in rows:
    try:
        ph = hdl_of(addr)
    except Exception as e:
        ph = -1
    if ph != h:
        bad.append((addr, h, ph))
    else:
        good += 1

print("total", len(rows), "good", good, "bad", len(bad))
bad.sort()
print("first 20 bad (addr, auth_h, parsed_h):")
for x in bad[:20]:
    print("  ", x)
print("last 5 bad:")
for x in bad[-5:]:
    print("  ", x)
# 错位段：相邻 bad 连续地址
if bad:
    print("min bad addr", bad[0][0], "max bad addr", bad[-1][0])
