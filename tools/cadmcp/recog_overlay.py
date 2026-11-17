# -*- utf-8 -*-
"""把识别结果叠加回原图，作为"算法读对了"的可视证据。
灰 = 原始图元；红 = 识别出的母线槽；蓝 = 识别出的断路器单元；
绿点 = 分配到该回路的标注；紫线 = 标注→单元的归属关系。
"""
import json
from PIL import Image, ImageDraw, ImageFont

S = json.load(open(r"C:\temp\sys.json", encoding="utf-8"))
R = json.load(open(r"C:\temp\recog.json", encoding="utf-8"))
x0f, y0f, x1f, y1f = S["frame"]

W = 2200
scale = W / (x1f - x0f)
H = int((y1f - y0f) * scale)
img = Image.new("RGB", (W, H + 34), "white")
dr = ImageDraw.Draw(img)
F = ImageFont.truetype(r"C:\Windows\Fonts\simsun.ttc", 12)
Fs = ImageFont.truetype(r"C:\Windows\Fonts\simsun.ttc", 9)


def mp(x, y):
    return ((x - x0f) * scale, (y1f - y) * scale + 34)


# ---- 原始图元（灰底）----
for l in S["lines"]:
    dr.line([mp(l[0], l[1]), mp(l[2], l[3])], fill=(205, 205, 205))
for p in S["plines"]:
    pts = [mp(q[0], q[1]) for q in p["p"]]
    if p["c"]:
        pts.append(pts[0])
    dr.line(pts, fill=(205, 205, 205))
for c in S["circles"]:
    cx, cy = mp(c[0], c[1])
    r = max(1, c[2] * scale)
    dr.ellipse([cx - r, cy - r, cx + r, cy + r], outline=(215, 215, 215))

# ---- 识别出的母线槽（红）----
for i, b in enumerate(R["buses"]):
    a = mp(b["x0"], b["top"] + 800)
    c = mp(b["x1"], b["top"])
    box = [min(a[0], c[0]), min(a[1], c[1]), max(a[0], c[0]), max(a[1], c[1])]
    dr.rectangle(box, outline=(215, 40, 40), width=2)
    dr.text((box[0], box[1] - 15), "母线%d  %d 回  %s" % (i + 1, b["count"], b["spec"][:22]),
            font=F, fill=(200, 30, 30))

# ---- 断路器单元（蓝）+ 标注归属（绿点/紫线）----
for lo in R["loops"]:
    cx, cy = lo["x"], lo["y"]
    half_w, half_h = 800, 1700          # 单元半宽/半高（识别时实测 1600x3500）
    a = mp(cx - half_w, cy - half_h)
    c = mp(cx + half_w, cy + half_h)
    box = [min(a[0], c[0]), min(a[1], c[1]), max(a[0], c[0]), max(a[1], c[1])]
    dr.rectangle(box, outline=(40, 90, 220), width=1)
    # 归属：容量文字在单元上方，用紫线连到框顶
    if lo["cap"]:
        p = mp(cx + 600, cy + 4200)     # 容量标注实际位置（框右侧上方）
        dr.line([(box[2], box[1]), p], fill=(150, 60, 200), width=1)
        dr.ellipse([p[0] - 2, p[1] - 2, p[0] + 2, p[1] + 2], fill=(150, 60, 200))
    # 标记：容量（绿，紧贴框上方）
    dr.text((box[0] + 1, box[1] - 11), lo["cap"] + "A", font=Fs, fill=(0, 150, 60))
    # 备用（品红）
    if lo.get("spare"):
        dr.text((box[0] + 1, box[3] + 1), "备", font=Fs, fill=(190, 0, 110))

# ---- 图例 ----
lg = ["灰=原始图元", "红=母线槽段", "蓝=断路器单元", "绿=容量标注", "紫=标注归属", "品红=备用"]
for i, s in enumerate(lg):
    dr.text((20 + i * 150, 8), s, font=F, fill=(60, 60, 60))

img.save(r"G:\PyArx\docs\recog_overlay.png")
print("saved", img.size)
