# -*- coding: utf-8 -*-
"""找门窗类块的实例位置，用来在离屏图里框住一个符号做重影取证。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

print("块名里含 门/窗/FJM/FC/MC 的：")
for k in D["blk"]:
    if any(w in k for w in ("门", "窗", "FJM", "FC", "MC", "窗")):
        d = D["blk"][k]
        print("  %-30s ln=%d pl=%d tx=%d ins=%d" % (k[:30], len(d.get("ln") or []), len(d.get("pl") or []),
                                                    len(d.get("tx") or []), len(d.get("ins") or [])))

# 顶层实例里块名命中上述的，取前几个坐标
print("\n这些块的顶层实例（前 12）：")
n = 0
for e in D["ins"]:
    if any(w in e["k"] for w in ("门", "窗", "FJM")):
        print("  %-28s @(%.0f,%.0f) r=%.1f sx=%.2f sy=%.2f" % (e["k"][:28], e["x"], e["y"], e.get("r", 0), e.get("sx", 1), e.get("sy", 1)))
        n += 1
        if n >= 12:
            break
