import struct, sys

d = open(r"G:\PyArx\samples\dwg_versions\v_2000.dwg", "rb").read()

# 锚点记录：addr=131645 type=35 handle=2702 bitsize=500（r2000_rec.py 已核实）
A = 131645 * 8
def bit(g): return (d[g // 8] >> (7 - (g % 8))) & 1

class R:
    def __init__(s, b): s.b = b
    def u(s, n):
        v = 0
        for i in range(n): v = (v << 1) | bit(s.b + i)
        s.b += n; return v
    def rs(s):
        lo = s.u(8); hi = s.u(8); return (hi << 8) | lo
    def bs(s):
        t = s.u(2)
        if t == 0: return s.rs()
        if t == 1: return s.u(8)
        if t == 2: return 0
        return 256
    def rl(s):
        lo = s.rs(); hi = s.rs(); return lo | (hi << 16)
    def bl(s):
        t = s.u(2)
        if t == 0: return s.u(32)
        if t == 1: return s.u(16)
        if t == 2: return s.u(8)
        return 0
    def rd(s):
        w1 = s.rs(); w2 = s.rs(); w3 = s.rs(); w4 = s.rs()
        lo = w1 | (w2 << 16); hi = w3 | (w4 << 16)
        u = lo | (hi << 32)
        return struct.unpack("<d", struct.pack("<Q", u))[0]
    def bd(s):
        c = s.u(2)
        if c == 0: return s.rd()
        return {1: 1.0, 2: 0.0}.get(c, float("nan"))
    def bt(s):
        if s.u(1): return 0.0
        return s.bd()
    def be(s):
        if s.u(1): return (0.0, 0.0, 1.0)
        return (s.bd(), s.bd(), s.bd())
    def p3(s): return (s.bd(), s.bd(), s.bd())

def parse(kind):
    r = R(A)
    r.rs(); tcode = r.bs(); bitsize = r.rl()
    hc = r.u(8); n = hc & 0xF
    for _ in range(n): r.u(8)
    afterHeader = r.b
    print(f"  hdr type={tcode} bitsize={bitsize} afterHeader={afterHeader}")
    pv = r.u(1)  # preview
    if pv:
        pvsz = r.rl(); r.u(8 * pvsz)
    r.u(2)  # entmode BB
    r.u(2)  # entmode BB
    r.bs()  # numReactors BS
    r.u(1)  # nolinks
    r.bs()  # color
    lts = r.bd()
    r.u(2)  # ltypeFlags
    r.u(2)  # plotstyleFlags
    r.bs()  # invisible
    r.u(8)  # linewt
    commonEnd = r.b
    # 几何段
    g = R(commonEnd)
    if kind == "spec":       # center 3BD, sm_axis 3BD, extrusion 3BD, axisRatio, start, end
        vals = (g.p3(), g.p3(), g.p3(), g.bd(), g.bd(), g.bd())
    elif kind == "be":       # center 3BD, sm_axis 3BD, BE, axisRatio, start, end
        vals = (g.p3(), g.p3(), g.be(), g.bd(), g.bd(), g.bd())
    elif kind == "bt0be":    # thickness BT0, center 3BD, sm_axis 3BD, BE, axisRatio,start,end
        vals = (g.bt(), g.p3(), g.p3(), g.be(), g.bd(), g.bd(), g.bd())
    elif kind == "bt03bd":   # thickness BT0, center 3BD, sm_axis 3BD, 3BD, 3 BD
        vals = (g.bt(), g.p3(), g.p3(), g.p3(), g.bd(), g.bd(), g.bd())
    print(f"{kind}: commonEnd={commonEnd} geomUsed={g.b-commonEnd} vals={vals}")

for k in ["spec", "be", "bt0be", "bt03bd"]:
    parse(k)
