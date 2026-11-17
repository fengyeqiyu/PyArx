"""把导出的轮廓渲染成 PNG，按字切块，便于肉眼核对笔画完整性。"""

import json

SRC = r"G:\PyArx\tools\ShxProbe\ttf_probe.json"
OUT = r"G:\PyArx\tools\ShxProbe\ttf_probe.png"

data = json.load(open(SRC, encoding="utf-8"))
samples = data["samples"]

from PIL import Image, ImageDraw

CELL = 240
PAD = 20
cols = min(4, len(samples))
rows = (len(samples) + cols - 1) // cols
W = cols * CELL + PAD * 2
H = rows * CELL + PAD * 2 + 20 * rows

img = Image.new("RGB", (W, H), (255, 255, 255))
dr = ImageDraw.Draw(img)

for idx, (text, s) in enumerate(samples.items()):
    counts, xs, ys = s["counts"], s["xs"], s["ys"]
    # local bbox
    if xs:
        minx, maxx = min(xs), max(xs)
        miny, maxy = min(ys), max(ys)
    else:
        minx = maxx = miny = maxy = 0.0
    span = max(maxx - minx, maxy - miny, 1e-9)
    col, row = idx % cols, idx // cols
    ox = PAD + col * CELL
    oy = PAD + row * (CELL + 20)

    scale = (CELL - 70) / span
    def px(x, y):
        return (ox + 30 + (x - minx) * scale, oy + 10 + (maxy - y) * scale)

    off = 0
    for ci, n in enumerate(counts):
        pts = [px(xs[off + i], ys[off + i]) for i in range(n)]
        if len(pts) > 1:
            pts.append(pts[0])
            dr.line(pts, fill=(20, 20, 20), width=2)
        off += n

    dr.text((ox + 4, oy + CELL - 10), f"{text}  contours={len(counts)}", fill=(180, 0, 0))

img.save(OUT)
print("saved", OUT, img.size)
for text, s in samples.items():
    print(text, "contours=", len(s["counts"]), "pts=", sum(s["counts"]))
