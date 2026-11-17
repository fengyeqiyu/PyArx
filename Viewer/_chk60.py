# -*- coding: utf-8 -*-
"""查 scene 里块定义文字自身同坐标重复（应只剩图框中英并排这种真实双行）。"""
import json

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

dup = 0; equip = 0; samples = []
for name, d in D["blk"].items():
    ts = d.get("tx") or []
    for i in range(len(ts)):
        for j in range(i + 1, len(ts)):
            x, y = ts[i], ts[j]
            if abs(x[0] - y[0]) <= 6 and abs(x[1] - y[1]) <= 6:
                dup += 1
                if name.startswith("$equip"):
                    equip += 1
                if len(samples) < 10:
                    samples.append((name[:26], str(x[6])[:12], str(y[6])[:12], x[0], x[1]))

print("scene 内块定义文字同坐标对 %d（其中 $equip 类 %d）" % (dup, equip))
for s0 in samples:
    print("   %-26s '%s' vs '%s' @(%.0f,%.0f)" % s0)
