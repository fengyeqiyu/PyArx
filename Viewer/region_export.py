"""区域导出（命令行版）：给定 scene.json，按坐标 / 文字锚点 / 视图号切出一块，
产出 .region.json（给 AI 读）与 .region.html（图表清单）。

用法：
  region_export.py <scene.json> --box x0,y0,x1,y1
  region_export.py <scene.json> --find "101-1-1RCUAP1" [--radius 25000]
  region_export.py <scene.json> --find "101-1-1RCUAP1" --list
  region_export.py <scene.json> --view 1
  region_export.py <scene.json> --kv 1        # 用第 1 个视图（同 --view）
可选：--out <前缀>（默认 export\\<锚点或坐标>），--pad 0.05
"""
import argparse
import html
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def load(path):
    return json.load(open(path, encoding="utf-8"))


def R(v):
    return round(float(v), 1)


def lay_of(d, i):
    ly = d.get("ly", [])
    return ly[i] if isinstance(i, int) and 0 <= i < len(ly) else "layer_%s" % i


def points_of(d):
    """把全图压成一串 (x, y, kind, ref)，用于按半径收集锚点附近的东西。"""
    pts = []
    for t in d.get("tx", []):
        pts.append((t[0], t[1], "text", t))
    for b in d.get("ins", []):
        pts.append((b.get("x", 0), b.get("y", 0), "block", b))
    L = d.get("ln", [])
    for i in range(0, len(L) - 5, 6):
        pts.append(((L[i] + L[i + 2]) / 2.0, (L[i + 1] + L[i + 3]) / 2.0, "line", L))
    for p in d.get("pl", []):
        raw = p.get("p") or []
        if not raw:
            continue
        if isinstance(raw[0], (int, float)):
            n = len(raw) // 2
            cx = sum(raw[2 * i] for i in range(n)) / n
            cy = sum(raw[2 * i + 1] for i in range(n)) / n
            pts.append((cx, cy, "pl", p))
    return pts


def bbox_around(d, anchor_x, anchor_y, radius):
    """只累计落在半径内的"点"（文字锚点/块插入点/线端点/多段线顶点），
    不用图元整体外框，否则一条长线会把区域拉到整幅图那么大。"""
    xs, ys = [], []

    def add(x, y):
        if abs(x - anchor_x) <= radius and abs(y - anchor_y) <= radius:
            xs.append(x)
            ys.append(y)

    for t in d.get("tx", []):
        add(t[0], t[1])
        h = t[2] if isinstance(t[2], (int, float)) else 0
        add(t[0] + h * max(len(str(t[6])), 1) * 0.6, t[1] - h)

    for b in d.get("ins", []):
        add(b.get("x", 0), b.get("y", 0))

    L = d.get("ln", [])
    for i in range(0, len(L) - 5, 6):
        add(L[i], L[i + 1])
        add(L[i + 2], L[i + 3])

    for p in d.get("pl", []):
        raw = p.get("p") or []
        if not raw:
            continue
        if isinstance(raw[0], (int, float)):
            for i in range(0, len(raw) - 1, 2):
                add(raw[i], raw[i + 1])
        else:
            for q in raw:
                add(q[0], q[1])

    if not xs:
        return None
    return [min(xs), min(ys), max(xs), max(ys)]


def slice_region(d, box, pad):
    x0, y0, x1, y1 = box
    if pad:
        dx, dy = (x1 - x0) * pad, (y1 - y0) * pad
        x0, y0, x1, y1 = x0 - dx, y0 - dy, x1 + dx, y1 + dy
    lines, circles, arcs, plines, texts, blocks, tables = [], [], [], [], [], [], []

    L = d.get("ln", [])
    for i in range(0, len(L) - 5, 6):
        ax, ay, bx, by = L[i], L[i + 1], L[i + 2], L[i + 3]
        if min(ax, bx) <= x1 and max(ax, bx) >= x0 and min(ay, by) <= y1 and max(ay, by) >= y0:
            lines.append({"x1": R(ax), "y1": R(ay), "x2": R(bx), "y2": R(by), "layer": lay_of(d, L[i + 5])})

    C = d.get("cr", [])
    for i in range(0, len(C) - 4, 5):
        cx, cy, r = C[i], C[i + 1], C[i + 2]
        if cx + r >= x0 and cx - r <= x1 and cy + r >= y0 and cy - r <= y1:
            circles.append({"x": R(cx), "y": R(cy), "r": R(r), "layer": lay_of(d, C[i + 4])})

    A = d.get("ar", [])
    for i in range(0, len(A) - 6, 7):
        cx, cy, r = A[i], A[i + 1], A[i + 2]
        if cx + r >= x0 and cx - r <= x1 and cy + r >= y0 and cy - r <= y1:
            arcs.append({"x": R(cx), "y": R(cy), "r": R(r), "a0": R(A[i + 3]), "a1": R(A[i + 4]),
                         "layer": lay_of(d, A[i + 6])})

    for p in d.get("pl", []):
        raw = p.get("p") or []
        pts = ([(raw[2 * i], raw[2 * i + 1]) for i in range(len(raw) // 2)]
               if raw and isinstance(raw[0], (int, float)) else [tuple(q) for q in raw])
        if any(q[0] >= x0 and q[0] <= x1 and q[1] >= y0 and q[1] <= y1 for q in pts):
            plines.append({"points": [[R(q[0]), R(q[1])] for q in pts],
                           "closed": bool(p.get("z")), "layer": lay_of(d, p.get("li"))})

    for t in d.get("tx", []):
        x, y = t[0], t[1]
        if x >= x0 and x <= x1 and y >= y0 and y <= y1:
            texts.append({"text": t[6], "x": R(x), "y": R(y), "h": R(t[2]),
                          "rot": R(t[3] or 0), "layer": lay_of(d, t[7])})
    texts.sort(key=lambda t: (-t["y"], t["x"]))

    for b in d.get("ins", []):
        if b.get("x", 0) >= x0 and b.get("x", 0) <= x1 and b.get("y", 0) >= y0 and b.get("y", 0) <= y1:
            k = b.get("k")
            blocks.append({"name": k if isinstance(k, str) else "", "x": R(b["x"]), "y": R(b["y"]),
                           "rot": R(b.get("r") or 0), "sx": R(b.get("sx") or 1), "sy": R(b.get("sy") or 1),
                           "layer": lay_of(d, b.get("li"))})

    for tb in d.get("tb", []):
        bb = tb.get("b") or []
        if len(bb) == 4 and not (bb[2] >= x0 and bb[0] <= x1 and bb[3] >= y0 and bb[1] <= y1):
            continue
        rows, cols = tb.get("r") or 0, tb.get("c") or 0
        raw = tb.get("t") or []
        grid = [[(str(raw[r][c]) if r < len(raw) and c < len(raw[r]) and raw[r][c] is not None else "")
                 for c in range(cols)] for r in range(rows)]
        if not any(v.strip() for row in grid for v in row):
            continue
        tables.append({"box": [R(v) for v in bb], "rows": rows, "cols": cols, "grid": grid,
                       "layer": lay_of(d, tb.get("li"))})

    return {
        "file": d.get("f", ""),
        "box": [R(v) for v in (x0, y0, x1, y1)],
        "unit": "drawing unit",
        "counts": {"lines": len(lines), "circles": len(circles), "arcs": len(arcs),
                   "polylines": len(plines), "texts": len(texts), "blocks": len(blocks),
                   "tables": len(tables)},
        "layers": sorted({t["layer"] for t in texts} | {b["layer"] for b in blocks}),
        "texts": texts, "blocks": blocks, "tables": tables,
        "lines": lines, "circles": circles, "arcs": arcs, "polylines": plines,
    }


def build_html(r, img_url=""):
    c = r["counts"]
    e = html.escape

    def rows(items, keys, heads):
        th = "".join("<th>%s</th>" % e(h) for h in heads)
        body = "".join("<tr>%s</tr>" % "".join("<td>%s</td>" % e(str(it[k])) for k in keys) for it in items)
        return "<table><tr>%s</tr>%s</table>" % (th, body)

    kv = "".join("<tr><td>%s</td><td>%d</td></tr>" % (e(k), v) for k, v in [
        ("文字", c["texts"]), ("块参照", c["blocks"]), ("表格", c["tables"]),
        ("直线", c["lines"]), ("圆", c["circles"]), ("圆弧", c["arcs"]), ("多段线", c["polylines"])])

    tb_html = ""
    for i, tb in enumerate(r["tables"], 1):
        trs = "".join("<tr>%s</tr>" % "".join("<td>%s</td>" % e(v) for v in row) for row in tb["grid"])
        tb_html += "<h3>表格 %d · %d 行 × %d 列 · 图层 %s</h3><table class='grid'>%s</table>" % (
            i, tb["rows"], tb["cols"], e(tb["layer"]), trs)

    img = ("<img src='%s' style='max-width:100%%;border:1px solid #ccd'>" % img_url) if img_url else \
          "<p class='meta'>（此文件由命令行生成，未附渲染图；应用内导出会自动嵌入该区域图像）</p>"

    body = [
        "<!doctype html><html lang='zh-CN'><head><meta charset='utf-8'><title>%s 区域 %s</title>" % (
            e(r["file"] or "未命名"), r["box"][0]),
        "<style>body{font:14px/1.6 'Microsoft YaHei',system-ui,sans-serif;margin:0;padding:24px;background:#fff;"
        "color:#1b1f23;max-width:1500px}h1{font-size:20px;margin:0 0 6px}h2{font-size:16px;margin:26px 0 8px;"
        "border-left:3px solid #2f6fdf;padding-left:8px}h3{font-size:13px;margin:16px 0 6px;color:#444}"
        ".meta{color:#667;font-size:12px;margin:0 0 14px}table{border-collapse:collapse;font-size:12.5px;"
        "min-width:340px}td,th{border:1px solid #d8dce2;padding:3px 8px;vertical-align:top}"
        "th{background:#f2f4f7;text-align:left}table.grid td{white-space:pre-wrap}"
        "table.kv td:first-child{background:#f7f8fa;white-space:nowrap}</style></head><body>",
        "<h1>%s</h1>" % e(r["file"] or "未命名图纸"),
        "<div class='meta'>区域 X %s ~ %s ，Y %s ~ %s （%s） · 图层：%s</div>" % (
            r["box"][0], r["box"][2], r["box"][1], r["box"][3], r["unit"], e(" / ".join(r["layers"]))),
        img,
        "<h2>图元统计</h2><table class='kv'>%s</table>" % kv,
        "<h2>表格参数（%d）</h2>%s" % (len(r["tables"]), tb_html or "<p class='meta'>无</p>"),
        "<h2>文字（%d）</h2>%s" % (len(r["texts"]),
            rows(r["texts"], ["text", "layer", "x", "y", "h"], ["文字", "图层", "x", "y", "字高"])),
        "<h2>块参照（%d）</h2>%s" % (len(r["blocks"]),
            rows(r["blocks"], ["name", "layer", "x", "y", "rot", "sx", "sy"],
                 ["块名", "图层", "x", "y", "旋转", "sx", "sy"])),
        "</body></html>",
    ]
    return "\n".join(body)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scene")
    ap.add_argument("--box")
    ap.add_argument("--find")
    ap.add_argument("--near")
    ap.add_argument("--view", type=int)
    ap.add_argument("--radius", type=float, default=25000)
    ap.add_argument("--size")
    ap.add_argument("--anchor")
    ap.add_argument("--pad", type=float, default=0.02)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()

    d = load(a.scene)

    if a.list:
        if not a.find:
            sys.exit("--list 需要配合 --find")
        key = a.find
        hits = [t for t in d.get("tx", []) if key in str(t[6])]
        for t in hits[:60]:
            print("  %-40s x=%10.1f y=%10.1f h=%s" % (str(t[6])[:40], t[0], t[1], t[2]))
        print("命中 %d 条（最多列 60）" % len(hits))
        return

    if a.box:
        box = [float(v) for v in a.box.split(",")]
    elif a.view:
        v = d.get("v") or []
        if a.view < 1 or a.view > len(v):
            sys.exit("视图号超出范围 1..%d" % len(v))
        box = [float(x) for x in v[a.view - 1]]
    elif a.find:
        # 同名锚点优先：完全相等 > 前缀 > 包含；同一档位里多于一处时要求 --near
        cands = []
        for t in d.get("tx", []):
            s = str(t[6]).strip()
            if s == a.find:
                cands.append((0, t))
            elif s.startswith(a.find):
                cands.append((1, t))
            elif a.find in s:
                cands.append((2, t))
        if not cands:
            alt = [b for b in d.get("ins", []) if a.find == b.get("k")]
            if not alt:
                sys.exit("没找到 %r，先用 --list 看看有哪些" % a.find)
            cands = [(0, [b.get("x", 0), b.get("y", 0), 0, 0, 0, 0, a.find, 0]) for b in alt]
        cands.sort(key=lambda p: p[0])
        tier = cands[0][0]
        best = [t for r0, t in cands if r0 == tier]
        if a.near:
            nx, ny = [float(v) for v in a.near.split(",")]
            best.sort(key=lambda t: (t[0] - nx) ** 2 + (t[1] - ny) ** 2)
            hit = best[0]
        elif len(best) == 1:
            hit = best[0]
        else:
            print("「%s」有 %d 处同级锚点，用 --near x,y 指定靠近哪一处：" % (a.find, len(best)))
            for t in best[:20]:
                print("   x=%10.1f  y=%10.1f  h=%s" % (t[0], t[1], t[2]))
            sys.exit(1)
        ax, ay = hit[0], hit[1]
        if a.size:
            w, h = [float(v) for v in a.size.split(",")]
            mode = (a.anchor or "center").lower()
            if mode == "center":
                box = [ax - w / 2, ay - h / 2, ax + w / 2, ay + h / 2]
            elif mode == "left-top":       # 锚点作为左上角，柜体向右、向下（y 减小）展开
                box = [ax, ay - h, ax + w, ay]
            elif mode == "left-bottom":
                box = [ax, ay, ax + w, ay + h]
            elif mode == "top-center":
                box = [ax - w / 2, ay - h, ax + w / 2, ay]
            else:
                sys.exit("--anchor 支持 center / left-top / left-bottom / top-center")
        else:
            box = bbox_around(d, ax, ay, a.radius)
        if box is None:
            sys.exit("锚点附近没有图元，试试 --size 或调大 --radius")
        print("锚点 %s @ (%.1f, %.1f)  %s" % (
            a.find, ax, ay,
            ("size=%s anchor=%s" % (a.size, a.anchor or "center")) if a.size
            else ("radius=%s" % a.radius)))
    else:
        sys.exit("需要 --box / --find / --view 之一")

    r = slice_region(d, box, a.pad)

    tag = a.find.replace(" ", "_") if a.find else ("v%d" % a.view if a.view else "box")
    out_dir = os.path.join(HERE, "export")
    os.makedirs(out_dir, exist_ok=True)
    prefix = a.out or os.path.join(out_dir, tag)

    js = prefix + ".region.json"
    ht = prefix + ".region.html"
    json.dump(r, open(js, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    open(ht, "w", encoding="utf-8").write(build_html(r))

    print("区域 %.1f,%.1f → %.1f,%.1f （%.0f × %.0f）" % (
        r["box"][0], r["box"][1], r["box"][2], r["box"][3],
        r["box"][2] - r["box"][0], r["box"][3] - r["box"][1]))
    print("counts:", r["counts"])
    print("layers:", r["layers"])
    print("→", js, round(os.path.getsize(js) / 1024), "KB")
    print("→", ht, round(os.path.getsize(ht) / 1024), "KB")


if __name__ == "__main__":
    main()
