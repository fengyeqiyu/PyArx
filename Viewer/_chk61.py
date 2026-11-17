# -*- coding: utf-8 -*-
import json
p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])
print("b =", D.get("b"))
print("v len =", len(D.get("v") or []))
for i, v in enumerate((D.get("v") or [])[:3]):
    print(" v%d = %s" % (i, v))
print("vtype:", type(D.get("b")), [type(x).__name__ for x in (D.get("b") or [])])
