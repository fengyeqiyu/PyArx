# -*- coding: utf-8 -*-
"""查图框块实例与重复文字：谁和谁在同坐标、是块内文字还是本体文字。"""
import json, re, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"')
a = s.find(">", a) + 1
b = s.find("</script>", a)
D = json.loads(s[a:b])
print("tx 顶层", len(D.get("tx") or []), "ins", len(D.get("ins") or []), "blk", len(D.get("blk") or {}))

ins = D["ins"]
g = collections.defaultdict(list)
for e in ins:
    g[e["k"]].append(e)
top = sorted(g.items(), key=lambda kv: -len(kv[1]))[:15]
print("\n=== 实例最多的块 ===")
for k, lst in top:
    b2 = D["blk"].get(k) or {}
    print("%-44s ins=%3d defTx=%4d" % (k[:44], len(lst), len(b2.get("tx") or [])))

print("\n=== 疑似图框块 ===")
for k in g:
    if "LAY" in k.upper() or "-AR-" in k.upper():
        b2 = D["blk"].get(k) or {}
        print("%-44s ins=%3d defTx=%4d defLn=%5d" % (k[:44], len(g[k]), len(b2.get("tx") or []), len(b2.get("ln") or [])))
        for e in g[k]:
            print("     x=%.0f y=%.0f r=%.2f sx=%.3f sy=%.3f" % (e["x"], e["y"], e.get("r", 0), e.get("sx", 1), e.get("sy", 1)))

dup = collections.Counter()
for t in (D.get("tx") or []):
    dup[(t[6], round(t[0]), round(t[1]))] += 1
d2 = sorted(((v, k) for k, v in dup.items() if v > 1), reverse=True)
print("\n=== 顶层文字同坐标重复 %d 组 ===" % len(d2))
for v, k in d2[:12]:
    print("  x%d %s @ (%d,%d)" % (v, k[0][:26], k[1], k[2]))
