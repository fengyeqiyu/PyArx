# -*- coding: utf-8 -*-
"""在真机视野附近取样，看该处图元是否本身就有两份（同坐标重复线/同块多实例重叠）。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

CX, CY, R = 437493.0, 13237.0, 12.0     # 真机状态栏坐标，半径按 5007% 下 540px 估

def inbox(x, y):
    return abs(x - CX) <= R and abs(y - CY) <= R

def sig(x1, y1, x2, y2):
    p1 = (round(x1, 2), round(y1, 2)); p2 = (round(x2, 2), round(y2, 2))
    return (p1, p2) if p1 <= p2 else (p2, p1)

lines = collections.Counter()
M = D.get("ln") or []
n = 0
for i in range(0, len(M), 6):
    x1, y1, x2, y2 = M[i], M[i+1], M[i+2], M[i+3]
    if inbox(x1, y1) or inbox(x2, y2):
        lines[sig(x1, y1, x2, y2)] += 1
        n += 1
print("视野内顶层线 %d 条，去重后 %d 条 → 重复 %d 组" % (n, len(lines), sum(1 for v in lines.values() if v > 1)))
for k, v in lines.most_common(6):
    if v > 1:
        print("   x%d  (%s)-(%s)" % (v, k[0], k[1]))

ins = []
for e in D["ins"]:
    if inbox(e["x"], e["y"]):
        ins.append(e)
print("\n视野内块实例 %d 个" % len(ins))
cnt = collections.Counter((e["k"], round(e["x"], 1), round(e["y"], 1)) for e in ins)
for k, v in cnt.most_common(8):
    print("   %-34s @(%.1f,%.1f)  x%d" % (k[0][:34], k[1], k[2], v))

tx = [t for t in (D.get("tx") or []) if inbox(t[0], t[1])]
print("\n视野内顶层文字 %d 条" % len(tx))
for t in tx[:8]:
    print("   %-20s (%.0f,%.0f)" % (t[6][:20], t[0], t[1]))
