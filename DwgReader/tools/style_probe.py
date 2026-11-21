import struct, sys
p = r"G:\PyArx\samples\dwg_versions\v_2000.dwg"
data = open(p, "rb").read()
start_bit = 14395432*8 + 58
buf = []
class R:
    def __init__(s, bit): s.bit = bit
    def u(s, n):
        v = 0
        for i in range(n):
            g = s.bit + i
            v = (v << 1) | ((data[g//8] >> (7-(g%8))) & 1)
        s.bit += n
        return v
r = R(start_bit)
def bs():
    c = r.u(2)
    return r.u({0:6,1:4,2:8,3:16}[c])
def tv():
    n = bs(); s = []
    for i in range(n): s.append(chr(r.u(8)))
    return "".join(s)
def bd():
    c = r.u(2)
    if c == 0:
        v = struct.unpack(">d", bytes(r.u(8) for _ in range(8)))[0]; return v
    return {1:1.0, 2:0.0}.get(c, float("nan"))
def b(): return r.u(1)
def h():
    code = r.u(4); sz = r.u(4)
    v = 0
    for i in range(sz): v = (v<<8) | r.u(8)
    return (code, sz, v)
def log(name, val):
    print(f"{name:16s} {val!r:28s} bit={r.bit}")
    buf.append((name, val))
log("name", tv())
log("is_xref_ref", b())
log("is_xref_resolved", bs())
log("is_xref_dep", b())
log("xref", h())
log("is_shape", b())
log("is_vertical", b())
log("text_size", bd())
log("width_factor", bd())
log("oblique", bd())
log("generation", r.u(8))
log("last_height", bd())
log("font_file", tv())
log("bigfont_file", tv())
print("end bit rel-to-datastart =", r.bit - start_bit, " abs =", r.bit)
print("hdl start expect =", 14395432*8 + 16 + 370)
