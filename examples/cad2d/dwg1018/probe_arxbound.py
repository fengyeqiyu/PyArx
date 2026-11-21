import json, sys
xs, ys = [], []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) < 13: continue
    try: g = json.loads(p[12])
    except Exception: continue
    def add(x, y):
        if x is not None and y is not None: xs.append(x); ys.append(y)
    if "sp" in g: add(g["sp"][0], g["sp"][1]); add(g["ep"][0], g["ep"][1])
    elif "center" in g: add(g["center"][0], g["center"][1])
    elif "pos" in g: add(g["pos"][0], g["pos"][1])
    elif "loc" in g: add(g["loc"][0], g["loc"][1])
    elif "verts" in g:
        for v in g["verts"]: add(v["pt"][0], v["pt"][1])
xs.sort(); ys.sort()
n = len(xs)
print("n", n)
for q in (0, 0.01, 0.05, 0.5, 0.95, 0.99, 1):
    i = min(n-1, int(q*n))
    print(f"x {q}: {xs[i]:.1f}  y {q}: {ys[i]:.1f}")
