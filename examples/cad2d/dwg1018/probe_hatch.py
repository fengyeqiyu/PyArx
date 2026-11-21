import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

addr = {}
for l in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    q = l.split("\t")
    if len(q) >= 3 and q[2] in ("78", "HATCH"):
        addr[int(q[0])] = int(q[1])

for i, (h, a) in enumerate(list(addr.items())[:6]):
    try:
        name, handle, common, g = entity.decode_entity(a)
        loops = g["loops"]
        print(h, "npath", len(loops),
              [(len(x.get("segs", x.get("pts", []))), x.get("boundaryHandles", [])) for x in loops],
              g["patternName"], g["hatchStyle"], g["patternType"], g["patternSpace"])
    except Exception as e:
        print(h, "ERR", repr(e))
