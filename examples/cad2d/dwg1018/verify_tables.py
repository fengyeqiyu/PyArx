# -*- coding: utf-8 -*-
"""比对：离线实体 layer/ltype handle 查表名 vs ARX 权威名字。"""
import sys, json
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

addr_of = {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4:
        addr_of[int(p[0])] = int(p[1])

layer_map, ltype_map = {}, {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\tables.tsv", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if p[1] == "LAYER": layer_map[int(p[0])] = p[2]
    elif p[1] == "LTYPE": ltype_map[int(p[0])] = p[2]

arx = {}
with open(r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv", encoding="utf-8") as f:
    hdr = f.readline().rstrip("\n").split("\t")
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) >= len(hdr):
            arx[int(p[0])] = dict(zip(hdr, p))

def ltype_name(hd, ltf):
    if ltf == 0: return "ByLayer"
    if ltf == 1: return "ByBlock"
    if ltf == 2: return "Continuous"
    return ltype_map.get(hd, "?%d" % hd)

lay_ok = lay_bad = lty_ok = lty_bad = miss = 0
bad = []
for h, a in arx.items():
    if h not in addr_of:
        miss += 1; continue
    try:
        _, _, c, _ = entity.decode_entity(addr_of[h])
    except Exception as e:
        continue
    hd = c["handles"]
    ln = layer_map.get(hd["layer"], "?%d" % hd["layer"])
    if ln == a["layer"]: lay_ok += 1
    else:
        lay_bad += 1; bad.append((h, "layer", ln, a["layer"]))
    tn = ltype_name(hd["ltype"], c["ltype_flags"])
    if tn == a["ltype"]: lty_ok += 1
    else:
        lty_bad += 1; bad.append((h, "ltype", tn, a["ltype"]))

print("layer ok=%d bad=%d | ltype ok=%d bad=%d | miss=%d" % (lay_ok, lay_bad, lty_ok, lty_bad, miss))
for x in bad[:15]:
    print(" ", x)
