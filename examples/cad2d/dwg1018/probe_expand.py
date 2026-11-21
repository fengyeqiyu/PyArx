# -*- coding: utf-8 -*-
import sys, re
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

REF = r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"
BH = r"G:\PyArx\examples\cad2d\dwg1018\bh_ref.tsv"

# handle -> addr, code
h2a = {}
for line in open(REF, encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 3:
        try: h2a[int(p[0])] = (int(p[1]), p[2])
        except ValueError: pass

# 块记录 -> [实体句柄]
block_ents = {}
all_block_ent = set()
n_bh = 0
for line in open(BH, encoding="gbk", errors="replace"):
    if "BLOCK_HEADER" not in line: continue
    p = line.rstrip("\n").split("\t")
    bh = int(p[0])
    ents = [int(x[1:]) for x in p if x.startswith("E")]
    block_ents[bh] = ents
    all_block_ent.update(ents)
    n_bh += 1

hit = sum(1 for h in all_block_ent if h in h2a)
print("块记录数", n_bh, "块内实体句柄", len(all_block_ent), "ref_map 命中", hit)

# 遍历 INSERT
ins = []
miss_block = 0; miss_ent = 0; ok = 0
for line in open(REF, encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) < 4 or p[2] != "7": continue
    addr = int(p[1])
    try:
        name, handle, common, g = entity.decode_entity(addr)
    except Exception:
        continue
    if not g: continue
    blk = g["block"]
    if blk not in block_ents:
        miss_block += 1; continue
    ents = block_ents[blk]
    if not ents: ok += 1; continue
    if all(e in h2a for e in ents): ok += 1
    else:
        miss_ent += 1
        if miss_ent <= 5:
            print("块内缺句柄 bh", blk, [e for e in ents if e not in h2a][:5])
    ins.append((handle, blk, len(ents)))

print("INSERT 数", len(ins), "块记录命中", ok, "缺块记录", miss_block, "块内缺实体", miss_ent)
