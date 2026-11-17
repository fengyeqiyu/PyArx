"""诊断：单张参数表附近到底有哪些水平/垂直线，判断柜框由什么构成。"""
import json

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))
tb = d["tb"][0]["b"]
print("表 bbox:", [round(v) for v in tb])
cx, cy = (tb[0] + tb[2]) / 2, (tb[1] + tb[3]) / 2
print("表中心:", round(cx), round(cy))

L = d["ln"]
H, V = [], []
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if abs(y1 - y2) <= 40 and abs(x1 - x2) >= 3000:
        H.append((round((y1 + y2) / 2), round(min(x1, x2)), round(max(x1, x2), )))
    elif abs(x1 - x2) <= 40 and abs(y1 - y2) >= 3000:
        V.append((round((x1 + x2) / 2), round(min(y1, y2)), round(max(y1, y2))))

print("\n窗口 x∈[%d,%d] y∈[%d,%d] 内的水平线：" % (cx - 30000, cx + 30000, cy - 40000, cy + 40000))
for y, a, b in sorted(H, key=lambda t: -t[0]):
    if cy - 40000 <= y <= cy + 40000 and a <= cx + 30000 and b >= cx - 30000:
        print("   y=%-8d x=%d..%d  len=%d" % (y, a, b, b - a))

print("\n同窗口内的垂直线：")
for x, a, b in sorted(V, key=lambda t: t[0]):
    if cx - 30000 <= x <= cx + 30000 and a <= cy + 40000 and b >= cy - 40000:
        print("   x=%-8d y=%d..%d  len=%d" % (x, a, b, b - a))

print("\n水平线长度分布:", sorted({(t[2] - t[1]) // 5000 * 5000 for t in H})[:12])
print("垂直线长度分布:", sorted({(t[2] - t[1]) // 5000 * 5000 for t in V})[:12])
