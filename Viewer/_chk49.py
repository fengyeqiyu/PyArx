# -*- coding: utf-8 -*-
"""找块内 ln 的字段步长，并做顶层/块内线重合统计。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

STEP = None
for k, v in D["blk"].items():
    ln = v.get("ln") or []
    if ln:
        print("样例块", k, "len", len(ln))
        print("  ", ln[:12])
        for st in (6, 7, 8):
            if len(ln) % st == 0:
                print("   可被 %d 整除" % st)
        STEP = 6 if len(ln) % 6 == 0 else (7 if len(ln) % 7 == 0 else 8)
        break
print("推定步长", STEP)
