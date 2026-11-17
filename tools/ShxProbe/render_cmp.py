"""单字放大渲染：左=我们提取的轮廓，右=PIL 直接渲染色块，用于对比笔画是否缺失。"""

import json

SRC = r"G:\PyArx\tools\ShxProbe\ttf_probe.json"
OUT = r"G:\PyArx\tools\ShxProbe\ttf_cmp.png"
data = json.load(open(SRC, encoding="utf-8"))["samples"]
from PIL import Image, ImageDraw, ImageFont

TEST = ["工", "配", "电", "桥", "艺"]
S = 400
W = len(TEST) * S + 40
H = S * 2 + 80
img = Image.new("RGB", (W, H), (255, 255, 255))
dr = ImageDraw.Draw(img)

font = ImageFont.truetype(r"C:\Windows\Fonts\simsun.ttc", 300)

for i, ch in enumerate(TEST):
    s = data[ch]
    counts, xs, ys = s["counts"], s["xs"], s["ys"]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    span = max(maxx - minx, maxy - miny)
    ox = i * S + 20
    k = (S - 40) / span

    off = 0
    for n in counts:
        pts = [(ox + 10 + (xs[off + j] - minx) * k, 10 + (maxy - ys[off + j]) * k) for j in range(n)]
        pts.append(pts[0])
        dr.line(pts, fill=(20, 20, 20), width=3)
        off += n
    dr.text((ox + 8, S - 6), f"{ch} n={len(counts)}", fill=(200, 0, 0))

    # PIL reference: filled glyph
    tmp = Image.new("L", (S + 60, S + 60), 0)
    d2 = ImageDraw.Draw(tmp)
    d2.text((10, 10), ch, font=font, fill=255)
    bbox = tmp.getbbox()
    ref = img.crop((ox, S + 40, ox + S, S + 40 + S))
    ref.paste((235, 235, 235), (0, 0, S, S))
    glyph = tmp.crop(bbox).resize((S - 40, S - 40))
    ref.paste((60, 60, 60), (10, 10), glyph)
    img.paste(ref, (ox, S + 40))

img.save(OUT)
print("saved", OUT)
