import struct
p = r"G:\PyArx\samples\dwg_versions\v_2000.dwg"
data = open(p, "rb").read()
start = 1053242
endl = 1053676
def bit(g): return (data[g//8] >> (7-(g%8))) & 1
class R:
    def __init__(s, b): s.b = b
    def u(s, n):
        v = 0
        for i in range(n): v = (v<<1) | bit(s.b+i)
        s.b += n; return v
def bd(r):
    c = r.u(2)
    if c == 0: return struct.unpack(">d", bytes(r.u(8) for _ in range(8)))[0]
    return {1:1.0, 2:0.0}.get(c, float("nan"))
def be(r):
    if r.u(1): return (0.0,0.0,1.0)
    return (bd(r), bd(r), bd(r))
def p3(r): return (bd(r), bd(r), bd(r))
def show(name, fn):
    rr = R(start)
    try:
        vals = fn(rr)
        print(f"{name} used={rr.b-start} (avail={endl-start}) vals={vals}")
    except Exception as e:
        print(name, "err", e, "used=", rr.b-start)
show("A(BE)", lambda rr: (p3(rr), p3(rr), be(rr), bd(rr), bd(rr), bd(rr)))
show("B(3BD)", lambda rr: (p3(rr), p3(rr), p3(rr), bd(rr), bd(rr), bd(rr)))
show("C(BT+BE)", lambda rr: (p3(rr), p3(rr), bd(rr), be(rr), bd(rr), bd(rr), bd(rr)))
