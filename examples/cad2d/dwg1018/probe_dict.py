import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity
from entity import BitStream, read_header, read_h, decode_eed

rows = []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4 and p[2] == "42":
        rows.append((int(p[0]), int(p[1])))

bad = 0; tot_items = 0
for h, a in rows:
    try:
        tc, hh, d = entity.decode_dict(a)
        tot_items += len(d["items"])
    except Exception as e:
        bad += 1
        if bad <= 10: print("ERR", h, repr(e))
print("dict total", len(rows), "items", tot_items, "bad", bad)
