"""以 79 张参数表为锚，用局部内容剖面（x/y 密度直方图）自适应求每台柜的真实边界。"""
import json
import os

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUT = r"G:\PyArx\viewer\web\cuts.html"
YMIN = 500000
BIN = 800          # 剖面分格宽度(世界单位)
EMPTY = 3          # 连续几个空 bin 判为边界
MINPT = 2          # bin 内点数 >= 该值算有内容

d = json.load(open(SCENE, encoding="utf-8"))

pts = []
L = d["ln"]
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    pts.append((x1, y1))
    pts.append((x2, y2))
    pts.append(((x1 + x2) / 2, (y1 + y2) / 2))
for p in d["pl"]:
    raw = p["p"]
    if not raw or max(raw[1::2]) < YMIN:
        continue
    for j in range(0, len(raw) - 1, 2):
        pts.append((raw[j], raw[j + 1]))
for t in d["tx"]:
    if t[1] >= YMIN and (t[6] or "").strip():
        pts.append((t[0], t[1]))
for i in range(0, len(d["cr"]) - 4, 5):
    if d["cr"][i + 1] >= YMIN:
        pts.append((d["cr"][i], d["cr"][i + 1]))
for b in d["ins"]:
    if b.get("y", 0) >= YMIN:
        pts.append((b["x"], b["y"]))
print("内容点:", len(pts))


def scan_dir(win, axis, anchor, lo, hi, step):
    """从 anchor 沿 step 方向扫，找连续 EMPTY 个空 bin 的位置。axis 0=x 1=y。"""
    idx = 0 if axis == 0 else 1
    pos = anchor
    empt = 0
    while lo <= pos <= hi:
        b0, b1 = pos, pos + step * BIN
        if step < 0:
            b0, b1 = pos + step * BIN, pos
        n = 0
        for px, py in win:
            v = px if idx == 0 else py
            if b0 <= v < b1:
                n += 1
                if n >= MINPT:
                    break
        if n >= MINPT:
            empt = 0
        else:
            empt += 1
            if empt >= EMPTY:
                return pos if step > 0 else pos + step * BIN
        pos += step * BIN
    return hi if step > 0 else lo


boxes = []
for ti, tb in enumerate(d["tb"]):
    bb = tb["b"]
    tx0, ty0, tx1, ty1 = bb[0], bb[1], bb[2], bb[3]
    pe = ""
    for row in (tb.get("t") or []):
        cells = [str(c) if c is not None else "" for c in row]
        if len(cells) >= 2 and cells[0].strip() == "Pe=":
            pe = cells[1].strip()
    wx0, wy0 = tx0 - 26000, ty0 - 26000
    wx1, wy1 = tx0 + 48000, ty0 + 66000
    win = [(x, y) for x, y in pts if wx0 <= x <= wx1 and wy0 <= y <= wy1]
    if len(win) < 20:
        boxes.append({"b": [round(tx0), round(ty0), round(tx0) + 21000, round(ty1) + 38000],
                      "pe": pe, "src": "fallback"})
        continue
    left = scan_dir(win, 0, tx0, wx0, tx1, -1)
    right = scan_dir(win, 0, tx1, wx1, tx0, +1)
    bottom = scan_dir(win, 1, ty0, wy0, ty1, -1)
    top = scan_dir(win, 1, ty1, wy1, ty0, +1)
    boxes.append({"b": [round(left), round(bottom), round(right), round(top)],
                  "pe": pe, "src": "profile"})

ok = [b for b in boxes if b["src"] == "profile"]
print("剖面定界:", len(ok), " 回落:", len(boxes) - len(ok))
for name, arr in (("宽", [b["b"][2] - b["b"][0] for b in ok]),
                  ("高", [b["b"][3] - b["b"][1] for b in ok])):
    arr.sort()
    print("%s 中位 %d  [%d ~ %d]  最小5个 %s" % (name, arr[len(arr) // 2], arr[0], arr[-1], arr[:5]))
for b in boxes[:10]:
    print("  Pe=%-9s %s  %dx%d  %s" % (b["pe"], b["b"], b["b"][2] - b["b"][0],
                                      b["b"][3] - b["b"][1], b["src"]))
json.dump({"file": d["f"], "boxes": boxes},
          open(r"G:\PyArx\viewer\export\cabinets.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
