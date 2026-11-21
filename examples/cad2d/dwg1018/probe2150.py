import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity
from entity import BitStream, read_header, read_common, read_h, _hatch_seg

a = 3201869
b = BitStream(entity.DATA); b.byte = a
tc, bs, h = read_header(b)
print("hdr", tc, bs, h, "at", b.byte, b.bit)
entity.decode_eed(b)
common = read_common(b)
print("after common", b.byte, b.bit)

b.read_bl(); b.read_bl()
b.read_bd(); b.read_bd()
b.read_bl(); b.read_bd()
ncol = b.read_bl()
print("ncol", ncol, b.byte, b.bit)
for _ in range(ncol):
    b.read_bd(); b.read_cmc()
b.read_tv()
b.read_bd()
b.read_3bd()
name = b.read_tv()
solid = b.read_b(); assoc = b.read_b()
npath = b.read_bl()
print("name", name, "solid", solid, "npath", npath, b.byte, b.bit)
for pi in range(npath):
    flag = b.read_bl()
    print("path", pi, "flag", flag, b.byte, b.bit)
    if not (flag & 2):
        nseg = b.read_bl()
        print("  nseg", nseg, b.byte, b.bit)
        for si in range(nseg):
            ct = b.peek_rc() if hasattr(b, "peek_rc") else None
            try:
                s = _hatch_seg(b)
                print("  seg", si, s["type"], b.byte, b.bit)
            except Exception as e:
                print("  seg", si, "ERR", repr(e), b.byte, b.bit)
                break
    nb = b.read_bl()
    print("  nb", nb, b.byte, b.bit)
    for _ in range(nb):
        print("  h", read_h(b), b.byte, b.bit)

style = b.read_bs(); ptype = b.read_bs()
print("style", style, "ptype", ptype, b.byte, b.bit)
if not solid:
    angle = b.read_bd(); spacing = b.read_bd(); double = b.read_b()
    print("angle", angle, "spacing", spacing, "double", double, b.byte, b.bit)
    nd = b.read_bs()
    print("nd", nd, b.byte, b.bit)
    for di in range(nd):
        b.read_bd(); b.read_2bd(); b.read_2bd()
        ndash = b.read_bs()
        print("  defline", di, "ndash", ndash, b.byte, b.bit)
        [b.read_bd() for _ in range(ndash)]
derived = False
if derived:
    b.read_bd()
nseed = b.read_bl()
print("nseed", nseed, b.byte, b.bit)
seeds = [list(b.read_2rd()) for _ in range(nseed)]
print("seeds", seeds, b.byte, b.bit)
