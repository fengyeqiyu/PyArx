"""explode_text2.py —— 通用版：按过滤串把图里的 AcDbText 逐个炸成闭合折线（支持旋转）。

配置：G:\\PyArx\\tools\\Text2Line\\job.json
    {"filter": "YJLHV", "face": "SimSun", "simp_px": 1.0, "erase": true}
输出：G:\\PyArx\\tools\\Text2Line\\explode_out.json
"""

import json
import math
import os
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release", r"G:\PyArx\tools\Text2Line"):
    if _p not in sys.path:
        sys.path.insert(0, _p)

JOB = r"G:\PyArx\tools\Text2Line\job.json"
OUT = r"G:\PyArx\tools\Text2Line\explode_out.json"


def xy(p):
    for name in ("x", "y"):
        pass
    x = getattr(p, "x", None)
    y = getattr(p, "y", None)
    if x is not None and y is not None:
        return float(x), float(y)
    return float(p[0]), float(p[1])


def as_bbox(ext):
    if isinstance(ext, (tuple, list)) and len(ext) == 2:
        a, b = ext
        ax, ay = xy(a)
        bx, by = xy(b)
        return min(ax, bx), min(ay, by), max(ax, bx), max(ay, by)
    mn = getattr(ext, "minPoint", None)
    mx = getattr(ext, "maxPoint", None)
    if mn is None:
        mn = getattr(ext, "min", None)
    if mx is None:
        mx = getattr(ext, "max", None)
    if mn is not None and mx is not None:
        ax, ay = xy(mn)
        bx, by = xy(mx)
        return min(ax, bx), min(ay, by), max(ax, bx), max(ay, by)
    raise RuntimeError("unknown extents %r" % (ext,))


def solve_scale(raww, rawh, measw, meash, rot):
    c, s = abs(math.cos(rot)), abs(math.sin(rot))
    det = raww * rawh * (c * c - s * s)
    if abs(det) > 1e-9:
        sx = (measw * c - meash * s) * rawh / det
        sy = (meash * c - measw * s) * raww / det
    else:
        sx = sy = max(measw / (raww * c + rawh * s), meash / (raww * s + rawh * c))
    return sx, sy


def explode_one(tr, tid, text, face, simp_px, erase, layer):
    import PyArxCore as P
    import glyph_trace

    rot = float(text.rotation())
    ext = text.getGeomExtents()
    bx0, by0, bx1, by1 = as_bbox(ext)
    measw, meash = bx1 - bx0, by1 - by0

    loops = glyph_trace.text_contours(face, s_text, em_px=768, simp_px=simp_px)
    if not loops:
        return 0, 0

    rx0 = min(p[0] for l in loops for p in l)
    ry0 = min(p[1] for l in loops for p in l)
    rx1 = max(p[0] for l in loops for p in l)
    ry1 = max(p[1] for l in loops for p in l)
    sx, sy = solve_scale(rx1 - rx0, ry1 - ry0, measw, meash, rot)

    cr, sr = math.cos(rot), math.sin(rot)
    rotated = []
    for loop in loops:
        pts = []
        for lx, ly in loop:
            X, Y = lx * sx, ly * sy
            pts.append((X * cr - Y * sr, X * sr + Y * cr))
        rotated.append(pts)

    ax0 = min(p[0] for l in rotated for p in l)
    ay0 = min(p[1] for l in rotated for p in l)
    dx, dy = bx0 - ax0, by0 - ay0

    made = 0
    for pts in rotated:
        pl = P.Db.PyDbPolyline()
        for i, (x, y) in enumerate(pts):
            pl.addVertexAt(i, P.Ge.AcGePoint2d(x + dx, y + dy))
        pl.setClosed(True)
        try:
            pl.setLayer(layer)
        except Exception:
            pass
        tr.addEntity(pl)
        made += 1

    if erase:
        t2 = tr.openObject(tid, 1)
        t2.erase()
    return made, sum(len(l) for l in rotated)


s_text = ""


def main():
    import PyArxCore as P

    with open(JOB, encoding="utf-8") as f:
        job = json.load(f)
    flt = job.get("filter", "")
    face = job.get("face", "SimSun")
    simp = float(job.get("simp_px", 1.0))
    erase = bool(job.get("erase", True))

    rep = {"filter": flt}
    db = P.Db.curDb()
    rep["file"] = db.getFilename()

    ids = db.objectIds("AcDbText")
    rep["texts"] = len(ids)

    targets = []
    with P.Db.PyDbTrans() as tr:
        for tid in ids:
            t = tr.openObject(tid, 0)
            s = t.textString()
            if flt and flt not in s:
                continue
            targets.append((tid, s, float(t.rotation()), float(t.height()), t.layer()))

    rep["matched"] = len(targets)

    global s_text
    total_pl, total_pt = 0, 0
    for tid, s, rot, h, layer in targets:
        s_text = s
        with P.Db.PyDbTrans() as tr:
            t = tr.openObject(tid, 0)
            made, pts = explode_one(tr, tid, t, face, simp, erase, layer)
        total_pl += made
        total_pt += pts
    rep["polylines"] = total_pl
    rep["vertices"] = total_pt
    rep["texts_done"] = len(targets)
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"exception": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[EXPLODE2]", {k: rep[k] for k in ("matched", "polylines", "texts_done", "file") if k in rep})
