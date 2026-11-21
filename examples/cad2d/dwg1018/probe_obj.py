import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity
from entity import BitStream, read_header, read_common, read_h, decode_eed

samples = {"42": [], "79": [], "503": [], "818": []}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4 and p[2] in samples and len(samples[p[2]]) < 3:
        samples[p[2]].append((int(p[0]), int(p[1]), p[3]))

for code, lst in samples.items():
    print("=== code", code)
    for h, a, n in lst:
        b = BitStream(entity.DATA); b.byte = a
        tc, bs, hh = read_header(b)
        decode_eed(b)
        numr = b.read_bl(); xm = b.read_b()
        if code == "42":
            ni = b.read_bl(); cloning = b.read_bs(); ho = b.read_rc()
            texts = [b.read_tv() for _ in range(ni)]
            hd = BitStream(entity.DATA); hd.byte = a + bs//8; hd.bit = bs%8
            read_h(hd)  # owner
            for _ in range(numr): read_h(hd)
            if not xm: read_h(hd)
            ih = [read_h(hd)[2] for _ in range(ni)]
            print(" ", h, "ni", ni, "cloning", cloning, "ho", ho, "texts", texts, "handles", ih)
        elif code == "503":
            schema = b.read_rc(); val = b.read_tv()
            print(" ", h, "schema", schema, "val", val)
        else:
            print(" ", h, "addr", a, "type", tc, "bs", bs, "numr", numr, "xm", xm, "name", n, "at", b.byte, b.bit)
