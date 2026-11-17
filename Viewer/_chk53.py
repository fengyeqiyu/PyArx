# -*- coding: utf-8 -*-
"""验证：块定义里的嵌套实例是否被“摊平”成顶层实例（两者都在画 → 同一符号画两遍、错开）。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

ins = D["ins"]
top = collections.Counter((e["k"], round(e["x"]), round(e["y"])) for e in ins)

byk = collections.defaultdict(list)
for e in ins:
    byk[e["k"]].append(e)

hits = []
checked = 0
for k, lst in byk.items():
    d = D["blk"].get(k) or {}
    nest = d.get("ins") or []
    if not nest:
        continue
    checked += len(nest)
    m = lst[0].get("m") or [1, 0, 0, 1, 0, 0]
    for c in nest:
        wx = m[0]*c["x"] + m[2]*c["y"] + m[4]
        wy = m[1]*c["x"] + m[3]*c["y"] + m[5]
        if top.get((c["k"], round(wx), round(wy))):
            hits.append((k, c["k"], round(wx), round(wy)))

print("块定义内嵌套实例 %d 个" % checked)
print("其中与顶层实例同块同坐标的: %d 个" % len(hits))
for h in hits[:10]:
    print("   父块 %-24s 子块 %-24s @(%.0f,%.0f)" % (h[0][:24], h[1][:24], h[2], h[3]))

# 反过来：顶层实例里，有多少“看起来属于某个块的内部” —— 用同块实例对的位置关系粗判
print("\n顶层实例最多的块里，是否存在父子关系：")
for k in ("101-AR-LAY-L1-20260121_t3", "madao111"):
    d = D["blk"].get(k) or {}
    print("   %-30s 嵌套实例数 %d" % (k, len(d.get("ins") or [])))
