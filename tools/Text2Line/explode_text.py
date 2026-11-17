"""explode_text.py —— 纯 PyArx 路线：读文字 → 栅格追踪墨迹 → PyDbTrans 建闭合折线 → 删原文字。

在 acad.exe 内由 (command "PYRUNFILE" ...) 执行。
"""

import json
import math
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release", r"G:\PyArx\tools\Text2Line"):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\explode_out.json"


def num(p, i, name):
    v = getattr(p, name, None)
    if v is not None:
        return float(v)
    return float(p[i])


def as_bbox(ext):
    """把 getGeomExtents / bounds 的任意返回形态规整成 (minx,miny,maxx,maxy)。"""
    if isinstance(ext, (tuple, list)) and len(ext) == 2:
        a, b = ext
        return (num(a, 0, "x"), num(a, 1, "y"), num(b, 0, "x"), num(b, 1, "y"))
    mn = getattr(ext, "minPoint", None) or getattr(ext, "min", None)
    mx = getattr(ext, "maxPoint", None) or getattr(ext, "max", None)
    if mn is not None and mx is not None:
        return (num(mn, 0, "x"), num(mn, 1, "y"), num(mx, 0, "x"), num(mx, 1, "y"))
    raise RuntimeError("unknown extents type: %r" % (ext,))


def main():
    import PyArxCore as P
    import glyph_trace

    rep = {"step": "start"}
    db = P.Db.curDb()
    rep["file"] = db.getFilename()

    ids = db.objectIds("AcDbText")
    rep["textIds"] = len(ids)
    if not ids:
        rep["step"] = "no text"
        return rep

    with P.Db.PyDbTrans() as tr:
        txt = tr.openObject(ids[0], 0)
        s = txt.textString()
        h = txt.height()
        rot = txt.rotation()
        wf = txt.widthFactor()
        obl = txt.oblique()
        pos = txt.position()
        layer = txt.layer()
        ext = txt.getGeomExtents()
        rep["text"] = s
        rep["height"] = h
        rep["rotation"] = rot
        rep["widthFactor"] = wf
        rep["oblique"] = obl
        rep["extentsRaw"] = repr(ext)[:200]

    px0 = num(pos, 0, "x")
    py0 = num(pos, 1, "y")
    bx0, by0, bx1, by1 = as_bbox(ext)
    rep["bbox"] = [bx0, by0, bx1, by1]

    loops = glyph_trace.text_contours("SimSun", s, em_px=768, simp_px=1.0)
    rep["loops"] = len(loops)
    rep["pts"] = sum(len(l) for l in loops)
    if not loops:
        rep["step"] = "no loops"
        return rep

    lx0 = min(p[0] for l in loops for p in l)
    ly0 = min(p[1] for l in loops for p in l)
    lx1 = max(p[0] for l in loops for p in l)
    ly1 = max(p[1] for l in loops for p in l)
    sx = (bx1 - bx0) / (lx1 - lx0)
    sy = (by1 - by0) / (ly1 - ly0)
    rep["scale"] = [sx, sy]

    cos_r, sin_r = math.cos(rot), math.sin(rot)
    made = 0
    with P.Db.PyDbTrans() as tr:
        for loop in loops:
            pl = P.Db.PyDbPolyline()
            for i, (lx, ly) in enumerate(loop):
                dx = (lx - lx0) * sx
                dy = (ly - ly0) * sy
                x = px0 + dx * cos_r - dy * sin_r
                y = py0 + dx * sin_r + dy * cos_r
                pl.addVertexAt(i, P.Ge.AcGePoint2d(x, y))
            pl.setClosed(True)
            try:
                pl.setLayer(layer)
            except Exception:
                pass
            tr.addEntity(pl)
            made += 1

        t2 = tr.openObject(ids[0], 1)
        t2.erase()
    rep["made"] = made
    rep["step"] = "done"
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"step": "exception", "tb": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[EXPLODE]", {k: rep[k] for k in ("step", "made", "loops", "text") if k in rep})
