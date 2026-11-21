# -*- coding: utf-8 -*-
"""用实体 ownerhandle 反查块记录归属，与 C 权威 bh_ref.tsv 比对。"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

REF = r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"
BH = r"G:\PyArx\examples\cad2d\dwg1018\bh_ref.tsv"

h2a, h2code = {}, {}
for line in open(REF, encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 3:
        try:
            h2a[int(p[0])] = int(p[1]); h2code[int(p[0])] = p[2]
        except ValueError: pass

# 权威块记录 -> 实体集合
auth = {}
for line in open(BH, encoding="gbk", errors="replace"):
    if "BLOCK_HEADER" not in line: continue
    p = line.rstrip("\n").split("\t")
    bh = int(p[0])
    ents = {int(x[1:]) for x in p if x[:1] == "E" and x[1:].isdigit()}
    auth[bh] = ents

# 离线 owner 反查
by_owner = {}
own_miss = 0; decoded = 0; err = 0
for h, addr in h2a.items():
    if h2code[h] == "49": continue
    try:
        name, handle, common, g = entity.decode_entity(addr)
    except Exception:
        err += 1; continue
    own = common.get("handles", {}).get("owner", 0)
    if own:
        by_owner.setdefault(own, []).append(handle)
    decoded += 1

print("解码", decoded, "err", err, "有 owner 的实体", sum(len(v) for v in by_owner.values()))

# 比对权威块记录
same = 0; diff = 0; missing = 0
for bh, ents in auth.items():
    off = set(by_owner.get(bh, []))
    if not ents:
        if not off: same += 1
        else: diff += 1
        continue
    if off == ents: same += 1
    else:
        diff += 1
        if diff <= 6:
            print("bh", bh, "权威", len(ents), "离线", len(off),
                  "缺", list(ents - off)[:5], "多", list(off - ents)[:5])
print("块记录一致", same, "不一致", diff)
