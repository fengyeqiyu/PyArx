# -*- coding: utf-8 -*-
"""查同一图元是否被声明两次：顶层 ln 与块内 ln 经实例变换后是否重合。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

# 顶层线段签名（量化到 1 单位，方向无关）
def sig(x1, y1, x2, y2):
    p1 = (round(x1), round(y1)); p2 = (round(x2), round(y2))
    return (p1, p2) if p1 <= p2 else (p2, p1)

top = collections.Counter()
ln = D.get("ln") or []
print("顶层 ln 条数", len(ln))
for i in range(0, len(ln), 6):
    top[sig(ln[i], ln[i+1], ln[i+2], ln[i+3])] += 1

dupTop = sum(1 for v in top.values() if v > 1)
print("顶层 ln 自身重复 %d 组（共 %d 条参与）" % (dupTop, len(ln)//6))

# 块内 ln 变换到世界，看是否与顶层重复
ins = {}
for e in D["ins"]:
    ins.setdefault(e["k"], []).append(e)

hit = 0; total = 0; samples = []
for k, lst in ins.items():
    d = D["blk"].get(k) or {}
    bl = d.get("ln") or []
    if not bl: continue
    m = lst[0].get("m") or [1, 0, 0, 1, 0, 0]
    for i in range(0, len(bl), 6):
        x1 = m[0]*bl[i] + m[2]*bl[i+1] + m[4]
        y1 = m[1]*bl[i] + m[3]*bl[i+1] + m[5]
        x2 = m[0]*bl[i+2] + m[2]*bl[i+3] + m[4]
        y2 = m[1]*bl[i+2] + m[3]*bl[i+3] + m[5]
        total += 1
        if top.get(sig(x1, y1, x2, y2)):
            hit += 1
            if len(samples) < 6:
                samples.append((k, round(x1), round(y1), round(x2), round(y2)))
print("块内 ln（按首实例变换）%d 条，其中与顶层线重合 %d 条" % (total, hit))
for s2 in samples:
    print("   %-28s (%.0f,%.0f)-(%.0f,%.0f)" % s2)
