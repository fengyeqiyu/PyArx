# -*- coding: utf-8 -*-
"""追某段文字到底来自哪里：顶层 tx、还是某个块定义的 tx、经哪个实例变换到世界。"""
import json, re, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

X0, Y0, X1, Y1 = 880000, -295000, 960000, -250000   # 视口 #b

def near(x, y):
    return X0 - 5000 <= x <= X1 + 5000 and Y0 - 8000 <= y <= Y1 + 8000

print("=== 顶层 tx 里落在视口内的 ===")
for t in D["tx"]:
    if near(t[0], t[1]):
        print("  %-24s (%.0f,%.0f) h=%.1f ha=%s va=%s ly=%s" % (t[6][:24], t[0], t[1], t[2], t[4], t[8], t[7]))

# 哪些块定义里含这些文字，并且该块有实例落在视口
names = collections.defaultdict(list)
for k, b2 in D["blk"].items():
    for t in (b2.get("tx") or []):
        txt = t[6]
        if any(w in txt for w in ("CH=", "链式清洗", "无菌", "净化夹道")):
            names[txt].append(k)
plain = sorted({k for k, v in D["blk"].items() if v})
print("\n=== 块定义里含 CH=/链式清洗/无菌/净化夹道 的块 (defTx>0 且含这些字) ===")
for txt, ks in sorted(names.items())[:25]:
    print("  %-22s <- %s" % (txt[:22], ", ".join(k[:30] for k in ks[:4])))

# 这些块的实例，是否落在视口附近
print("\n=== 相关块的实例位置 ===")
seen = set()
for e in D["ins"]:
    if e["k"] in {k for ks in names.values() for k in ks}:
        if e["k"] in seen: continue
        seen.add(e["k"])
        b2 = D["blk"].get(e["k"]) or {}
        print("  %-30s ins@(%.0f,%.0f) defTx=%d" % (e["k"][:30], e["x"], e["y"], len(b2.get("tx") or [])))
