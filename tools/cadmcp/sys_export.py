# -*- coding: utf-8 -*-
"""按 A1 图框(块 A$Cdee77b06) 提取 母线干线系统图 → sys.json
图框 bbox: 411751.2,-307881.1 .. 495851.2,-248481.1  (84100 x 59400)
不按图层过滤；块内实体先变换到世界坐标再判断归属；跨框的外来实体剔除。
"""
import json
import PyArxCore as P

WX0, WY0, WX1, WY1 = 411751.2, -307881.1, 495851.2, -248481.1
FW, FH = WX1 - WX0, WY1 - WY0
db = P.Db.curDb()

out = {"frame": [WX0, WY0, WX1, WY1], "lines": [], "plines": [], "circles": [], "arcs": [],
       "texts": [], "stats": {}, "layers": {}, "err": [], "skipped": 0}
CLS = ("AcDbLine", "AcDbPolyline", "AcDb2dPolyline", "AcDbCircle", "AcDbArc", "AcDbText", "AcDbMText")


def wt(x, y, mat):
    """块坐标 → 世界坐标"""
    if mat is None:
        return x, y
    try:
        q = P.Ge.AcGePoint3d(x, y, 0.0).transformBy(mat)
        return q.x, q.y
    except Exception:
        return x, y


def ln_of(o):
    try:
        return str(o.layer())
    except Exception:
        return "0"


def take(b, ln):
    """返回值: True 收 / False 丢"""
    cx, cy = (b[0] + b[2]) / 2.0, (b[1] + b[3]) / 2.0
    if not (WX0 <= cx <= WX1 and WY0 <= cy <= WY1):
        return False
    # 跨框实体（伸出图框）丢弃
    if b[0] < WX0 - 2000 or b[2] > WX1 + 2000 or b[1] < WY0 - 2000 or b[3] > WY1 + 2000:
        return False
    return True


def poly_pts(o, mat):
    pts = []
    try:
        n = o.numVerts()
    except Exception:
        return pts
    for i in range(n):
        p = None
        for fn in ("getPointAt", "getPointAtParam", "getPoint2dAt"):
            if hasattr(o, fn):
                try:
                    q = getattr(o, fn)(i)
                    if q is not None:
                        p = q
                        break
                except Exception:
                    pass
        if p is None:
            continue
        x, y = wt(p.x, p.y, mat)
        pts.append([round(x, 2), round(y, 2)])
    return pts


def obj_pts(o, names, mat):
    for n in names:
        if hasattr(o, n):
            try:
                p = getattr(o, n)()
                if p is not None:
                    return wt(p.x, p.y, mat)
            except Exception:
                pass
    return None


def clean_txt(s):
    for a, b in (("%%c", "φ"), ("%%C", "φ"), ("%%d", "°"), ("%%D", "°"),
                 ("%%p", "±"), ("%%P", "±"), ("%%%", "%")):
        s = s.replace(a, b)
    return s


def emit(o, cls, mat, scale=1.0):
    ln = ln_of(o)

    if cls == "AcDbLine":
        s = obj_pts(o, ("startPoint", "getStartPoint"), mat)
        e = obj_pts(o, ("endPoint", "getEndPoint"), mat)
        if not s or not e:
            return
        b = (min(s[0], e[0]), min(s[1], e[1]), max(s[0], e[0]), max(s[1], e[1]))
        if not take(b, ln):
            out["skipped"] += 1
            return
        out["lines"].append([round(s[0], 2), round(s[1], 2), round(e[0], 2), round(e[1], 2), ln])

    elif cls in ("AcDbPolyline", "AcDb2dPolyline"):
        pts = poly_pts(o, mat)
        if len(pts) < 2:
            out["skipped"] += 1
            return
        b = (min(p[0] for p in pts), min(p[1] for p in pts), max(p[0] for p in pts), max(p[1] for p in pts))
        if not take(b, ln):
            out["skipped"] += 1
            return
        closed = False
        try:
            closed = bool(o.isClosed())
        except Exception:
            pass
        out["plines"].append({"p": pts, "c": closed, "l": ln})

    elif cls == "AcDbCircle":
        c = obj_pts(o, ("center", "getCenter"), mat)
        if not c:
            return
        try:
            r = float(o.radius())
        except Exception:
            return
        b = (c[0] - r, c[1] - r, c[0] + r, c[1] + r)
        if not take(b, ln):
            out["skipped"] += 1
            return
        out["circles"].append([round(c[0], 2), round(c[1], 2), round(r, 2), ln])

    elif cls == "AcDbArc":
        c = obj_pts(o, ("center", "getCenter"), mat)
        if not c:
            return
        try:
            r = float(o.radius())
            a0 = float(o.startAngle())
            a1 = float(o.endAngle())
        except Exception:
            return
        if not take((c[0] - r, c[1] - r, c[0] + r, c[1] + r), ln):
            out["skipped"] += 1
            return
        out["arcs"].append({"c": [round(c[0], 2), round(c[1], 2)], "r": round(r, 2),
                            "a0": round(a0, 4), "a1": round(a1, 4), "l": ln})

    elif cls in ("AcDbText", "AcDbMText"):
        s = ""
        for fn in ("textString", "contents", "text"):
            if hasattr(o, fn):
                try:
                    s = str(getattr(o, fn)())
                    break
                except Exception:
                    pass
        s = clean_txt(s)
        if not s.strip():
            return

        def gv(names, default=0):
            for n in names:
                if hasattr(o, n):
                    try:
                        return getattr(o, n)()
                    except Exception:
                        pass
            return default

        ha = int(gv(("horizontalMode", "getHorizontalMode")))
        va = int(gv(("verticalMode", "getVerticalMode")))
        ap = obj_pts(o, ("alignmentPoint", "getAlignmentPoint"), mat)
        pos = obj_pts(o, ("position", "getPosition", "location"), mat)
        # 非默认对齐时，真正落点是 alignmentPoint
        if (ha != 0 or va != 0) and ap and (abs(ap[0]) + abs(ap[1]) > 1e-6):
            pos = ap
        if not pos:
            return
        if cls == "AcDbMText":
            at = gv(("attachment", "attachmentPoint"), 1)
            try:
                at = int(at)
            except Exception:
                at = 1
            H = {1: (0, 3), 2: (1, 3), 3: (2, 3), 4: (0, 2), 5: (1, 2),
                 6: (2, 2), 7: (0, 0), 8: (1, 0), 9: (2, 0)}.get(at, (0, 3))
            ha, va = H
        h = 250.0
        for fn in ("height", "textHeight"):
            if hasattr(o, fn):
                try:
                    h = float(getattr(o, fn)()) * scale
                    break
                except Exception:
                    pass
        rot = 0.0
        try:
            rot = float(o.rotation())
        except Exception:
            pass
        if not take((pos[0] - 10, pos[1] - 10, pos[0] + 10, pos[1] + 10), ln):
            out["skipped"] += 1
            return
        out["texts"].append({"x": round(pos[0], 2), "y": round(pos[1], 2), "t": s,
                             "h": round(h, 2), "r": round(rot, 4),
                             "ha": ha, "va": va, "l": ln})
    else:
        return

    out["layers"][ln] = out["layers"].get(ln, 0) + 1


def classify(o):
    for k in CLS:
        try:
            if o.dxfName() == k:
                return k
        except Exception:
            pass
    if hasattr(o, "startPoint"):
        return "AcDbLine"
    if hasattr(o, "numVerts"):
        return "AcDbPolyline"
    if hasattr(o, "textString"):
        return "AcDbText"
    return None


with P.Db.PyDbTrans() as tr:
    for cls in CLS:
        try:
            ids = db.objectIds(cls)
        except Exception:
            continue
        out["stats"][cls] = len(ids)
        for eid in ids:
            try:
                emit(tr.openObject(eid, 0), cls, None)
            except Exception as ex:
                out["err"].append("%s:%s" % (cls, str(ex)[:50]))

    for eid in db.objectIds("AcDbBlockReference"):
        try:
            ref = tr.openObject(eid, 0)
            try:
                eb = ref.getGeomExtents()
                rb = (eb[0].x, eb[0].y, eb[1].x, eb[1].y)
            except Exception:
                continue
            if not take(rb, ""):
                continue
            try:
                br = tr.openObject(ref.blockTableRecordId(), 0)
            except Exception:
                continue
            mat = None
            try:
                mat = ref.blockTransform()
            except Exception:
                pass
            sc = 1.0
            try:
                sf = ref.scaleFactors()
                if isinstance(sf, (tuple, list)):
                    sc = abs(float(sf[0]))
                elif hasattr(sf, "x"):
                    sc = abs(float(sf.x))
            except Exception:
                pass
            if abs(sc - 1.0) < 1e-9:
                try:
                    m = mat.toNumpy()
                    sc = abs(float(m[0][0]))
                except Exception:
                    pass
            if sc <= 1e-6:
                sc = 1.0
            for cid in br.entityIds():
                try:
                    c = tr.openObject(cid, 0)
                    ccls = classify(c)
                    if ccls:
                        emit(c, ccls, mat, sc)
                except Exception as ex:
                    out["err"].append("be:%s" % str(ex)[:50])
        except Exception as ex:
            out["err"].append("br:%s" % str(ex)[:50])

out["stats"]["lines"] = len(out["lines"])
out["stats"]["plines"] = len(out["plines"])
out["stats"]["circles"] = len(out["circles"])
out["stats"]["texts"] = len(out["texts"])
open(r"C:\temp\sys.json", "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False))
result = "ok"
