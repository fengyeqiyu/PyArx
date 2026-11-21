# -*- coding: utf-8 -*-
"""失败样本分类：按 size / has_eed / ARX 颜色类别 分桶，抽取代表性样例定位根因。"""
import sys
from collections import Counter
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
        read_h(b); end = b.byte + size
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
        b.byte = end; b.bit = q.bit
    return 1 if end else 0


def decode(addr):
    b = BitStream(data); b.byte = addr
    b.read_bs(); b.read_rl(); read_h(b)
    has = decode_eed(b)
    b.read_b(); b.read_bb(); b.read_bl(); b.read_b()
    raw = b.read_bs()
    flag = raw >> 8
    if flag & 0x80: b.read_bl()
    if flag & 0x20: b.read_bl()
    b.read_bd(); b.read_bb(); b.read_bb(); b.read_bs(); b.read_rc()
    zz = b.read_b()
    sx = b.read_rd(); ex = b.read_dd(sx); sy = b.read_rd(); ey = b.read_dd(sy)
    return sx, sy, ex, ey, has, raw


addr_of = {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 9 and p[2] == "LINE":
        addr_of[int(p[0])] = int(p[1])

arx = {}
with open(r"G:\PyArx\examples\cad2d\dwg1018\arx_lines.tsv", encoding="utf-8") as f:
    hdr = f.readline().rstrip("\n").split("\t")
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < 18:
            continue
        d = dict(zip(hdr, p))
        arx[int(d["handle"])] = d

buckets = Counter()
samples = {}
for h, addr in addr_of.items():
    a = arx.get(h)
    if a is None:
        continue
    body = addr - 2
    bb = BitStream(data); bb.byte = body; ms = bb.read_ms()
    try:
        sx, sy, ex, ey, has, raw = decode(addr)
        geom = (abs(sx - float(a["sx"])) < 1e-6 and abs(sy - float(a["sy"])) < 1e-6
                and abs(ex - float(a["ex"])) < 1e-6 and abs(ey - float(a["ey"])) < 1e-6)
    except Exception as e:
        geom = False; raw = -1; sx = sy = ex = ey = 0
    if geom:
        continue
    r, g, b_ = int(a["r"]), int(a["g"]), int(a["b"])
    rgb = "RGB" if (r or g or b_) else "-"
    ci = int(a["colorIndex"])
    cat = "BYLAYER" if ci == 256 else ("BYBLOCK" if ci in (0, 257) else "ACI%d" % ci)
    key = (ms, has, cat, rgb)
    buckets[key] += 1
    samples.setdefault(key, (h, addr, round(sx, 2), round(sy, 2), round(ex, 2), round(ey, 2),
                             round(float(a["sx"]), 2), round(float(a["sy"]), 2),
                             round(float(a["ex"]), 2), round(float(a["ey"]), 2),
                             ci, r, g, b_, a["layer"], hex(raw)))

print("失败桶 (size, has_eed, 颜色类别, RGB) : 数量")
for k, v in buckets.most_common(25):
    print("  ", k, v)
print("\n样例：")
for k, v in buckets.most_common(12):
    print("  ", k, "->", samples[k])
