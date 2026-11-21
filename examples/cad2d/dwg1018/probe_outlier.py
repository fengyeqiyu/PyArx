import sys, math
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

GEOM_NAMES = set(entity.TYPE_NAME.values()); GEOM_CODES = set(entity.TYPE_NAME.keys())
def isg(p2):
    return p2 in GEOM_NAMES or (lambda c: c in GEOM_CODES)(int(p2)) if p2.lstrip("-").isdigit() else False

bad = []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) < 4 or not isg(p[2]):
        continue
    h = int(p[0]); addr = int(p[1])
    try:
        name, handle, common, g = entity.decode_entity(addr)
    except Exception:
        continue
    if g is None: continue
    pts = []
    if name == "LINE": pts = [g["sp"], g["ep"]]
    elif name in ("CIRCLE", "ARC"): pts = [g["center"]]
    elif name == "LWPOLYLINE": pts = [v["pt"] for v in g["verts"]]
    elif name == "TEXT": pts = [g["pos"]]
    elif name == "MTEXT": pts = [g["loc"]]
    elif name == "POINT": pts = [g["pos"]]
    elif name == "ELLIPSE": pts = [g["center"]]
    elif name == "SPLINE": pts = g["ctrl"] or g["fit"]
    for q in pts:
        if abs(q[0]) > 1e6 or abs(q[1]) > 1e6:
            bad.append((h, name, round(q[0],1), round(q[1],1)))
            break
print("outlier", len(bad))
for b in bad[:20]: print(b)
