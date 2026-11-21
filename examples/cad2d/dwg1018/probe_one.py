# -*- coding: utf-8 -*-
"""单样本位级探针：逐步打印主流字段位置/值，对照 spec 顺序定位错位点。"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()

addr = int(sys.argv[1]) if len(sys.argv) > 1 else 14617392


def read_h(x):
    c = x.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | x.read_rc()
    return c, sz, v


b = BitStream(data); b.byte = addr
ot = b.read_bs(); bitsize = b.read_rl()
c, sz, hv = read_h(b)
print(f"addr={addr} type={ot} bitsize={bitsize} handle=0x{hv:X} afterH {b.byte-addr}.{b.bit}")

# EED
eed_idx = 0
q = None; end = None
while True:
    size = b.read_bs()
    print(f"EED size={size} @{b.byte-addr}.{b.bit}")
    if size == 0 or size > 1000:
        break
    ec, esz, ehv = read_h(b)
    print(f"  EED handle code=0x{ec:X} size={esz} v=0x{ehv:X} @{b.byte-addr}.{b.bit}")
    end = b.byte + size
    q = BitStream(data); q.byte = b.byte; q.bit = b.bit
    while q.byte < end:
        cc = q.read_rc()
        print(f"  [eed{eed_idx}] code={cc} @{q.byte-addr}.{q.bit}")
        if cc == 70: q.read_rs()
        elif cc == 71: q.read_rl()
        elif cc in (3, 5): q.read_rll_be()
        elif cc == 1: q.read_rs()
        elif cc == 2: q.read_rc()
        elif cc == 0:
            l = q.read_rc(); q.read_rs_be(); print("   str len", l, "cp", ); q.read_tf(l)
        elif cc == 4:
            l = q.read_rc(); q.read_tf(l)
        elif cc in (10, 11, 12, 13, 14, 15):
            for _ in range(3): q.read_rd()
        elif cc in (40, 41, 42): q.read_rd()
        else:
            q.byte = end; q.bit = 0
            print("   ?? -> skip to end"); break
    b.byte = q.byte; b.bit = q.bit
    eed_idx += 1
print(f"after EED {b.byte-addr}.{b.bit} (q.bit={q.bit})")

def step(name, fn):
    v = fn()
    print(f"  {name:14s} = {v!r:24s} @{b.byte-addr}.{b.bit}")
    return v

pv = step("preview B", b.read_b)
if pv:
    ps = step("preview_size RL", b.read_rl)
    for _ in range(ps): b.read_rc()
    print("      skipped preview", ps, "bytes @", b.byte-addr, ".", b.bit)
step("entmode BB", b.read_bb)
step("num_reactors BL", b.read_bl)
step("xdic B", b.read_b)
raw = step("color BSx", b.read_bs)
print(f"      flag=0x{raw>>8:X} index={raw&0x1ff}")
step("ltscale BD", b.read_bd)
step("lt_flags BB", b.read_bb)
step("plotstyle BB", b.read_bb)
step("invisible BS", b.read_bs)
step("linewt RC", b.read_rc)
zz = step("z_is_zero B", b.read_b)
sx = step("start.x RD", b.read_rd)
ex = step("end.x DD", lambda: b.read_dd(sx))
sy = step("start.y RD", b.read_rd)
ey = step("end.y DD", lambda: b.read_dd(sy))
print(f"  GEOM ({sx},{sy}) -> ({ex},{ey})")
