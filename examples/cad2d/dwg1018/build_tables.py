# -*- coding: utf-8 -*-
"""解析表记录（LAYER/LTYPE/STYLE/DIMSTYLE）handle->name，落盘 tables.tsv。"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

TABLE_CODE = {"51": "LAYER", "57": "LTYPE", "53": "STYLE", "69": "DIMSTYLE"}

rows = []
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) < 4 or p[2] not in TABLE_CODE:
        continue
    addr = int(p[1])
    try:
        tc, h, name, hd = entity.decode_table(addr)
        nm = name.decode("gbk", "replace").rstrip("\x00")
        rows.append((h, TABLE_CODE[p[2]], nm, hd["owner"]))
    except Exception as e:
        rows.append((p[0], TABLE_CODE[p[2]], "ERR:%r" % e, 0))

with open(r"G:\PyArx\examples\cad2d\dwg1018\tables.tsv", "w", encoding="utf-8") as f:
    f.write("handle\ttype\tname\towner\n")
    for h, t, nm, own in rows:
        f.write("%s\t%s\t%s\t%s\n" % (h, t, nm, own))

for t in ("LAYER", "LTYPE", "STYLE", "DIMSTYLE"):
    sub = [r for r in rows if r[1] == t]
    print(t, len(sub))
    for r in sub[:6]:
        print("  ", r[0], r[2])
