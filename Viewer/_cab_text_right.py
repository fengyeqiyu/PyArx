"""第三角度：柜内文字的右界，用来确认真实柜宽（区别于固定框宽 26000）。"""
import json

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"
YMIN = 500000

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]
texts = [(t[0], t[1], t[2], t[6]) for t in d["tx"] if t[1] >= YMIN and (t[6] or "").strip()]

print("台  柜名                表x0     框宽  文字右界距表x0  文本末字长度估计")
rows = []
for c in cabs:
    b = c["box"]
    tx0 = b[0] + 2000
    inw = [t for t in texts if b[0] <= t[0] <= b[2] and b[1] <= t[1] <= b[3]]
    if not inw:
        rows.append((c, 0, 0))
        continue
    # 文字的右端点 = x + 字高 * 字数（中文约 1 字宽 = 字高）
    right = max(t[0] + t[2] * min(len(t[3]), 40) for t in inw)
    right_near = max(t[0] for t in inw)
    rows.append((c, right - tx0, right_near - tx0, len(inw)))

for c, w1, w2, n in rows[:22]:
    print("#%-2d %-18s %-9d %-6d %-16d 近 %d  字%d"
          % (c["i"] + 1, (c["name"] or "—")[:18], c["box"][0] + 2000, c["box"][2] - c["box"][0], w1, w2, n))

ws = sorted(r[1] for r in rows if r[1])
print("\n文字右界距表x0: 中位 %d  四分位 %d~%d  最大 %d"
      % (ws[len(ws) // 2], ws[len(ws) // 4], ws[3 * len(ws) // 4], ws[-1]))
near = sorted(r[2] for r in rows if r[2])
print("仅按文字起点:   中位 %d  四分位 %d~%d  最大 %d"
      % (near[len(near) // 2], near[len(near) // 4], near[3 * len(near) // 4], near[-1]))
print("框宽 26000 vs 文字右界>24000 的台数: %d" % sum(1 for r in rows if r[1] > 24000))
