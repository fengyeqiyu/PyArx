# -*- coding: utf-8 -*-
"""从 RTTI 类名还原 CADReader 的绘制分层（只读 .rdata 里的公开类型信息）。"""
import re
from collections import Counter

P = r"G:\PyArx\CAD快速看图\CADReader.exe"
data = open(P, "rb").read()

# MSVC RTTI 名字形如 .?AVClassName@@ / .?AUStructName@@，UTF-8 存储
raw = re.findall(rb"\.\?A[VU]([A-Za-z_][\x20-\x7e]{2,80}?)@@", data)
names = []
for n in raw:
    try:
        s = n.decode("ascii", "ignore")
    except Exception:
        continue
    names.append(s)
uniq = sorted(set(names))
print("RTTI 类名总数 %d（去重前 %d）" % (len(uniq), len(names)))

PREFIX_KEYS = ("Ca", "CCa", "CGe", "Ge", "CA", "TDb", "Od", "Q", "Ex", "Rx",
               "Render", "Draw", "View", "Glyph", "Tess", "Batch", "Hatch",
               "Text", "Font", "Shx", "SHX", "Curve", "Seg", "Pick", "Snap")
group = Counter()
for s in uniq:
    hit = None
    for p in PREFIX_KEYS:
        if s.startswith(p):
            hit = p
            break
    group[hit or "other"] += 1
print("前缀分布:", dict(sorted(group.items(), key=lambda kv: -kv[1])[:12]))

def show(tag, cond, limit=40):
    hit = [s for s in uniq if cond(s)]
    print("\n== %s : %d" % (tag, len(hit)))
    for s in hit[:limit]:
        print("   ", s[:90])

show("自研 CA* 类", lambda s: s.startswith("Ca") or s.startswith("CCa"), 60)
show("几何/实体型", lambda s: s.startswith("TDb") or s.startswith("CGe"), 30)
show("绘制相关", lambda s: any(w in s for w in ("Draw", "Render", "Paint", "View", "GLCanvas", "Canvas")), 40)
show("文字/字体", lambda s: any(w in s for w in ("Text", "Font", "Shx", "SHX", "Glyph", "Char")), 40)
show("填充/曲线细分", lambda s: any(w in s for w in ("Hatch", "Tess", "Seg", "Strok", "Dash")), 40)
