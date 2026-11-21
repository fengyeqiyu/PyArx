# -*- coding: utf-8 -*-
"""离线分桶：EED 首 size / 派生触发 / 结束位置 vs 几何对齐，找单 EED 成功样本特征。"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()


def read_h(x):
    c = x.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | x.read_rc()
    return c, sz, v


def probe_eed(addr):
    """返回 (首size, eed结束byte相对addr, 探针读到的条目数, 读完EED后byte)。"""
    b = BitStream(data); b.byte = addr
    b.read_bs(); b.read_rl()
    _, _, _ = read_h(b)
    size = b.read_bs()
    first = size
    end = None; n = 0; last_byte = b.byte
    while size:
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
        n += 1
        size = b.read_bs()
        if size == 0 or size > 1000:
            break
    last_byte = b.byte
    return first, (end - addr if end else None), n, last_byte - addr


def decode_line(addr):
    b = BitStream(data); b.byte = addr
    b.read_bs(); b.read_rl()
    _, _, handle = read_h(b)
    end = None; q = None
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
    b.read_b(); b.read_bb(); b.read_bl(); b.read_b()
    raw = b.read_bs(); b.read_bd(); b.read_bb(); b.read_bb(); b.read_bs()
    b.read_rc(); b.read_b()
    sx = b.read_rd(); ex = b.read_dd(sx)
    sy = b.read_rd(); ey = b.read_dd(sy)
    return handle, sx, sy, ex, ey, raw & 0x1ff


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
        d = dict(zip(hdr, p)); arx[int(d["handle"])] = d

buckets = {}
ok_samples = []
for h in arx:
    if h not in addr_of:
        continue
    addr = addr_of[h]; a = arx[h]
    try:
        first, eend, n, last = probe_eed(addr)
        ph, psx, psy, pex, pey, pcol = decode_line(addr)
        geom = (abs(psx - float(a["sx"])) < 1e-6 and abs(psy - float(a["sy"])) < 1e-6
                and abs(pex - float(a["ex"])) < 1e-6 and abs(pey - float(a["ey"])) < 1e-6)
        key = (first, n)
        bk = buckets.setdefault(key, [0, 0])
        bk[0] += 1
        if geom: bk[1] += 1
        if geom and first and n == 1 and len(ok_samples) < 12:
            ok_samples.append((h, addr, first, eend, last))
    except Exception:
        pass

print("按 (首size, 探针条目数) 分桶: 总数 / 几何对齐")
for k in sorted(buckets):
    t, o = buckets[k]
    print(f"  first={k[0]:5d} n={k[1]}  total={t:6d} ok={o:6d}  {100.0*o/t:.1f}%")
print("\n单 EED 成功样本 (handle, addr, firstsize, eed_end, lastbyte):")
for s in ok_samples:
    print("  ", s)
