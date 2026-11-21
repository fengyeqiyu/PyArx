# -*- coding: utf-8 -*-
import sys, json, math
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

arx = {}
f = open(r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv", encoding="utf-8")
hdr = f.readline().rstrip("\n").split("\t")
for l in f:
    p = l.rstrip("\n").split("\t")
    if len(p) >= len(hdr):
        d = dict(zip(hdr, p)); d["geom"] = json.loads(d["geom"]); arx[int(d["handle"])] = d

addr = {}
for l in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = l.rstrip("\n").split("\t")
    if len(p) >= 4: addr[int(p[0])] = (int(p[1]), p[3])

want = sys.argv[1] if len(sys.argv) > 1 else "AcDbEllipse"
n = 0
for h, a in arx.items():
    if a["class"] != want or h not in addr: continue
    name, ph, c, g = entity.decode_entity(addr[h][0]); ag = a["geom"]
    if g is None: continue
    def close(x, y, tol=1e-4):
        return x is not None and y is not None and math.isclose(x, y, abs_tol=tol)
    if want == "AcDbEllipse":
        ok = all(close(g[k][i], ag[k][i]) for k in ("center", "majorAxis") for i in range(3)) \
            and close(g["radiusRatio"], ag["radiusRatio"], 1e-5) \
            and close(g["startAngle"], ag["startAngle"], 1e-5) \
            and close(g["endAngle"], ag["endAngle"], 1e-5)
    elif want == "AcDbArc":
        ok = close(g["center"][0], ag["center"][0]) and close(g["center"][1], ag["center"][1])
    elif want == "AcDbPolyline":
        ok = len(g["verts"]) == len(ag["verts"])
    else:
        ok = True
    if not ok and n < 5:
        n += 1; print("off", g); print("arx", ag); print()
