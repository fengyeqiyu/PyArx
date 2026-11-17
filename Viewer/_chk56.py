# -*- coding: utf-8 -*-
"""看大块实例的 bbox 是否正常（决定它会不会被 visible() 判掉）。"""
import json

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

names = ("101-AR-LAY-L1-20260121_t3", "101-AR-LAY-L1-吊顶用图_t3", "madao111")
for e in D["ins"]:
    if e["k"] in names:
        bb = e.get("b")
        print("%-30s x=%.0f y=%.0f sx=%.2f sy=%.2f" % (e["k"][:30], e["x"], e["y"], e.get("sx",1), e.get("sy",1)))
        if bb:
            print("      b=[%.0f,%.0f,%.0f,%.0f]  w=%.0f h=%.0f  x0>x1=%s y0>y1=%s" %
                  (bb[0], bb[1], bb[2], bb[3], bb[2]-bb[0], bb[3]-bb[1], bb[0]>bb[2], bb[1]>bb[3]))
        else:
            print("      b=None")

print("\n图纸总框 D.b =", D["b"])
print("视图 D.v =", D["v"])
