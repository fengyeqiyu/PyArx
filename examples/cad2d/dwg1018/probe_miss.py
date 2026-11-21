import sys, json
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")

addr_of = {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4:
        addr_of[int(p[0])] = (int(p[1]), p[2], p[3])

arx = {}
with open(r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv", encoding="utf-8") as f:
    hdr = f.readline().rstrip("\n").split("\t")
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < len(hdr): continue
        d = dict(zip(hdr, p))
        arx[int(d["handle"])] = d

from collections import Counter
miss_cls = Counter(); miss_layer = Counter(); samples = {}
for h, a in arx.items():
    if h not in addr_of:
        miss_cls[a["class"]] += 1
        miss_layer[a["layer"]] += 1
        samples.setdefault(a["class"], []).append(h)
print("miss by class:", dict(miss_cls))
print("miss by layer:", dict(miss_layer))
miss_h = [h for h, a in arx.items() if h not in addr_of]
print("miss total", len(miss_h), "min", min(miss_h), "max", max(miss_h))
ks = sorted(miss_h)
print("first", ks[:5], "last", ks[-5:])
import collections
# ARX 全库 handle 范围
print("arx min/max", min(arx), max(arx))
