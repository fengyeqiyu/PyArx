# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

REF = r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"
m = {}
for line in open(REF, encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4:
        m[int(p[0])] = (int(p[1]), p[2], p[3])

blocks = {}
n_ins = 0
for line in open(REF, encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) < 4: continue
    if p[3] != "INSERT": continue
    addr = int(p[1])
    try:
        name, h, common, g = entity.decode_entity(addr)
    except Exception as e:
        print("ERR", h, e); continue
    n_ins += 1
    bh = g["block"]
    blocks[bh] = blocks.get(bh, 0) + 1

print("inserts", n_ins, "distinct blocks", len(blocks))
from collections import Counter
cc = Counter()
missing = 0
for bh in blocks:
    if bh in m: cc[m[bh][2]] += blocks[bh]
    else: missing += blocks[bh]
print("missing", missing)
for k, v in cc.most_common(20):
    print(k, v)
for bh in list(blocks)[:8]:
    if bh in m:
        addr, code, nm = m[bh]
        try:
            t, hh, common, gg = entity.decode_entity(addr)
            print("block", bh, "code", code, nm, "type", t, "geom", gg, "owner", common["handles"]["owner"])
        except Exception as e:
            print("block", bh, "code", code, nm, "ERR", e)
