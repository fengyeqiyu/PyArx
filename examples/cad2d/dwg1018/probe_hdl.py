import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity
from entity import BitStream, read_header, read_common, read_h

for hh in (2150, 2146):
    a = None
    for l in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
        q = l.split("\t")
        if int(q[0]) == hh:
            a = int(q[1]); break
    b = BitStream(entity.DATA); b.byte = a
    tc, bs, h = read_header(b)
    entity.decode_eed(b)
    common = read_common(b)
    hds = BitStream(entity.DATA); hds.byte = a + bs // 8; hds.bit = bs % 8
    print(hh, "bs", bs, "entmode", common["entmode"], "numr", common["num_reactors"],
          "xm", common["xdic_missing"], "ltf", common["ltype_flags"], "psf", common["plotstyle_flags"])
    if common["entmode"] == 0:
        print("  owner", read_h(hds)[2])
    for _ in range(common["num_reactors"]):
        print("  react", read_h(hds)[2])
    if not common["xdic_missing"]:
        print("  xdic", read_h(hds)[2])
    print("  layer", read_h(hds)[2])
    if common["ltype_flags"] == 3:
        print("  ltype", read_h(hds)[2])
    if common["plotstyle_flags"] == 3:
        print("  ps", read_h(hds)[2])
    print("  hdl pos", hds.byte, hds.bit, "next H", read_h(hds))
