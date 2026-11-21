import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity
from entity import BitStream, read_header, read_h, decode_eed

rows = []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4 and p[2] == "79":
        rows.append((int(p[0]), int(p[1])))

bad = 0
for h, a in rows[:8]:
    b = BitStream(entity.DATA); b.byte = a
    tc, bs, hh = read_header(b)
    decode_eed(b)
    numr = b.read_bl(); xm = b.read_b()
    xds = b.read_bl()
    start = b.byte
    end = start + xds
    b.byte = end; b.bit = 0
    cloning = b.read_bs()
    hd = BitStream(entity.DATA); hd.byte = a + bs//8; hd.bit = bs%8
    _, _, owner = read_h(hd)
    react = [read_h(hd)[2] for _ in range(numr)]
    print(h, "bs", bs, "numr", numr, "xm", xm, "xds", xds, "cloning", cloning, "owner", owner, "react", react)
