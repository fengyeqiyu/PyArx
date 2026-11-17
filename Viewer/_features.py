"""多角度特征探查：用 79 个已识别框当标注样本，反查哪些信号能唯一定位一台柜。"""
import json
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]

print("=== 0. 顶层规模 ===")
for k in d:
    v = d[k]
    print("  %-4s %s" % (k, (("len=%d  %s" % (len(v), repr(v[:2])[:110])) if isinstance(v, list) else repr(v)[:80])))

L = d["ln"]
print("\n=== 1. ln 六元含义探查（前 6 条）===")
for i in range(0, 24, 6):
    print("  ", L[i:i + 6])
pe5 = Counter()
pe6 = Counter()
for i in range(0, len(L) - 5, 6):
    pe5[L[i + 4]] += 1
    pe6[L[i + 5]] += 1
print("  第5元取值数 %d，前8 %s" % (len(pe5), pe5.most_common(8)))
print("  第6元取值数 %d，前8 %s" % (len(pe6), pe6.most_common(8)))
print("  ly 项数 %d，样例 %s" % (len(d["ly"]), d["ly"][:6]))
print("  lyc 项数 %d，样例 %s" % (len(d["lyc"]), d["lyc"][:6]))

YMIN = 500000
BOX = [c["box"] for c in cabs]


def inb(x, y, b):
    return b[0] <= x <= b[2] and b[1] <= y <= b[3]


def boxed(x, y):
    for b in BOX:
        if inb(x, y, b):
            return True
    return False


print("\n=== 2. 图层视角：各层图元落在 79 框内的比例 ===")
lay_cnt = Counter()
lay_in = Counter()
for i in range(0, len(L) - 5, 6):
    y1, y2 = L[i + 1], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    li = L[i + 4]
    lay_cnt[li] += 1
    if boxed((L[i] + L[i + 2]) / 2, (y1 + y2) / 2):
        lay_in[li] += 1
for li, n in lay_cnt.most_common(18):
    nm = d["ly"][li] if isinstance(li, int) and 0 <= li < len(d["ly"]) else "?"
    print("  li=%-4s %-22s 总 %5d  框内 %5d  %5.1f%%" % (li, str(nm)[:22], n, lay_in[li], 100.0 * lay_in[li] / n))

print("\n=== 3. 柜名视角：柜名相对参数表的偏移一致性 ===")
import re
offs = []
for c in cabs:
    b = c["box"]
    tb = [t for t in c["texts"] if re.match(r"\d{3}-\d+-\d{0,2}[A-Z]{2,}\d{0,2}", t[3])]
    if not tb:
        continue
    t0 = tb[0]
    offs.append((c["i"] + 1, c["name"], t0[0] - b[0], t0[1] - b[1], t0[2]))
if offs:
    dx = sorted(o[2] for o in offs)
    dy = sorted(o[3] for o in offs)
    hh = sorted(o[4] for o in offs)
    print("  样本 %d 条；柜名 dx 中位 %d（%d~%d）；dy 中位 %d（%d~%d）；字高中位 %d（%d~%d）"
          % (len(offs), dx[len(dx) // 2], dx[0], dx[-1], dy[len(dy) // 2], dy[0], dy[-1],
             hh[len(hh) // 2], hh[0], hh[-1]))
    for o in offs[:8]:
        print("   #%-3d %-16s dx=%-6d dy=%-6d h=%d" % o)

print("\n=== 4. 几何视角：框内水平行线间距 ===")
for c in cabs[:8]:
    b = c["box"]
    ys = sorted({round(L[i + 1]) for i in range(0, len(L) - 5, 6)
                 if L[i + 1] == L[i + 3] and inb((L[i] + L[i + 2]) / 2, L[i + 1], b)
                 and abs(L[i + 2] - L[i]) > 8000})
    gaps = sorted({ys[j + 1] - ys[j] for j in range(len(ys) - 1)}) if len(ys) > 1 else []
    print("   #%-3d %-16s 长横线 %2d 条  主间距 %s" % (c["i"] + 1, c["name"] or "—", len(ys), gaps[:4]))

print("\n=== 5. 块视角：框内块名分布 ===")
ins = d["ins"]
blk_in = Counter()
for b in ins:
    if b.get("y", 0) >= YMIN and boxed(b["x"], b["y"]):
        blk_in[b.get("k", "?")] += 1
print("  框内块参照共 %d 个，前 10 名 %s" % (sum(blk_in.values()), blk_in.most_common(10)))
