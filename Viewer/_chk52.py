# -*- coding: utf-8 -*-
"""按真机截图上的文字反查世界坐标，用来定位用户截图的区域。"""
import json

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

want = ("MAU", "WL25", "WL31", "1AL9", "FJM523", "FJM3535", "卷帘")
for t in (D.get("tx") or []):
    txt = t[6]
    if any(w in txt for w in want):
        print("%-22s (%.0f,%.0f) h=%.0f" % (txt[:22], t[0], t[1], t[2]))

# 块的 attrib 也可能承载
print("\n--- 块内文字命中 ---")
for k, v in D["blk"].items():
    for t in (v.get("tx") or []):
        if any(w in t[6] for w in want):
            print("%-28s %-18s (%.1f,%.1f)" % (k[:28], t[6][:18], t[0], t[1]))
