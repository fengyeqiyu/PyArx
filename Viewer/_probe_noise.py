"""查干扰线（超长斜线）的图层与颜色归属，决定用图层开关还是几何过滤。"""
import json
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))

print("ln 6 元组示例:", d["ln"][:12])
L = d["ln"]
print("ln 长度:", len(L), " = ", len(L) / 6, "条")
ly = d.get("ly", [])
print("图层数:", len(ly), "前 6:", ly[:6] if isinstance(ly, list) else ly)

# 第 5、6 位分布
p4 = Counter(L[4::6])
p5 = Counter(L[5::6])
print("第5位 top8:", p4.most_common(8))
print("第6位 范围:", min(p5) if p5 else None, max(p5) if p5 else None, " top8:", p5.most_common(8))

long_slash = []
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    ln = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    if ln > 50000 and abs(y2 - y1) > 1000:
        long_slash.append((ln, L[i + 4], L[i + 5]))

print("\n超长斜线（长>5万且明显倾斜）:", len(long_slash))
if long_slash:
    print("  按第5位:", Counter(v[1] for v in long_slash).most_common(6))
    print("  按第6位:", Counter(v[2] for v in long_slash).most_common(6))
    ln_s = sorted(v[0] for v in long_slash)
    print("  长度 中位 %d 最大 %d 最小 %d" % (ln_s[len(ln_s) // 2], ln_s[-1], ln_s[0]))

# 对照：正常柜内线的第 5/6 位
norm = Counter()
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if 500000 <= y1 <= 800000 and y1 == y2 and 8000 <= abs(x2 - x1) <= 32000:
        norm[(L[i + 4], L[i + 5])] += 1
print("\n柜内水平线（正常）的第5/6位 top6:", norm.most_common(6))
