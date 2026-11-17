# -*- coding: utf-8 -*-
"""把 CH=5 与 链式清洗 系列在块内坐标算到世界坐标，看是真重复还是我们画错位。"""
import json

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

ins = {}
for e in D["ins"]:
    ins.setdefault(e["k"], []).append(e)

def world(inst, x, y):
    m = inst.get("m") or [1, 0, 0, 1, 0, 0]
    if len(m) == 6:
        return (m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5])
    return (x, y)

for blk, wants in (("madao111", ("CH=5.0", "CH=5.00", "CH=5.5", "CH=5.50")),
                   ("101-AR-LAY-L1-20260121_t3", ("链式清洗", "链式清洗1", "链式清洗2")),
                   ("101-AR-LAY-L1-吊顶用图_t3", ("链式清洗", "链式清洗1", "链式清洗2"))):
    b2 = D["blk"].get(blk)
    if not b2:
        print("no blk", blk); continue
    print("\n=== %s  (defTx=%d, ins=%d) ===" % (blk, len(b2.get("tx") or []), len(ins.get(blk, []))))
    for t in (b2.get("tx") or []):
        if t[6] in wants:
            print("   块内 %-10s (%.1f,%.1f) h=%.1f rot=%s ha=%s" % (t[6], t[0], t[1], t[2], t[3], t[4]))
            for e in ins.get(blk, []):
                wx, wy = world(e, t[0], t[1])
                print("        -> 实例(%.0f,%.0f) 世界 (%.0f,%.0f)" % (e["x"], e["y"], wx, wy))
