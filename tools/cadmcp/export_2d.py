"""按 CAD 原样导出 2D 图元：E-CABL-TRAY 三个图层的线、文字、块参照（含块内图形展开）。"""
import json
import math
import PyArxCore as P

db = P.Db.curDb()
out = {"lines": [], "texts": [], "fits": [], "layer_colors": {}, "types": {}}
LAYERS = ("E-CABL-TRAY", "E-CABL-TRAY-CNTR", "E-CABL-TRAY-IDEN")

# ---- 层色（CAD 默认：TRAY=4 青, CNTR=1 红, IDEN=2 黄）----
lt = db.getLayerTable()
for nm in LAYERS:
    idx = None
    for attempt in (lambda: lt.getAt(nm), lambda: db.getLayerTableRecord(nm)):
        try:
            rec = attempt()
            if rec is None:
                continue
            c = rec.color()
            idx = int(c.colorIndex())
            break
        except Exception:
            continue
    out["layer_colors"][nm] = idx if idx is not None else 0

# ---- 块参照内的实体展开 ----
def xform(pt, pos, rot, sx, sy):
    ca, sa = math.cos(rot), math.sin(rot)
    lx, ly = pt[0] * sx, pt[1] * sy
    return (pos[0] + lx * ca - ly * sa, pos[1] + lx * sa + ly * ca)


def walk(tr, rec_id, pos, rot, sx, sy, depth, acc):
    if depth > 3:
        return
    try:
        ids = tr.openObject(rec_id, 0).entityIds()
    except Exception:
        ids = []
    for eid in ids:
        o = tr.openObject(eid, 0)
        t = type(o).__name__
        acc_types = out["types"]
        acc_types[t] = acc_types.get(t, 0) + 1
        if t.endswith("Line") and "Poly" not in t:
            a = o.startPoint()
            b = o.endPoint()
            acc.append([xform((a.x, a.y), pos, rot, sx, sy),
                        xform((b.x, b.y), pos, rot, sx, sy)])
        elif t.endswith("Circle"):
            c = o.center()
            r = float(o.radius())
            pts = [xform((c.x + r * math.cos(k * math.pi / 16.0),
                          c.y + r * math.sin(k * math.pi / 16.0)), pos, rot, sx, sy)
                   for k in range(33)]
            for i in range(32):
                acc.append([pts[i], pts[i + 1]])
        elif t.endswith("Arc"):
            c = o.center()
            r = float(o.radius())
            a0 = float(o.startAngle())
            a1 = float(o.endAngle())
            if a1 <= a0:
                a1 += 2 * math.pi
            n = max(6, int((a1 - a0) / (math.pi / 18.0)))
            pts = [xform((c.x + r * math.cos(a0 + (a1 - a0) * k / n),
                          c.y + r * math.sin(a0 + (a1 - a0) * k / n)), pos, rot, sx, sy)
                   for k in range(n + 1)]
            for i in range(n):
                acc.append([pts[i], pts[i + 1]])
        elif t.endswith("Polyline"):
            try:
                n = int(o.numVerts())
                pts = []
                for k in range(n):
                    p = o.getPointAt(k)
                    pts.append(xform((p.x, p.y), pos, rot, sx, sy))
                for i in range(len(pts) - 1):
                    acc.append([pts[i], pts[i + 1]])
            except Exception:
                pass
        elif t.endswith("BlockReference"):
            try:
                p2 = o.position()
                walk(tr, o.blockTableRecordId(),
                     xform((p2.x, p2.y), pos, rot, sx, sy),
                     rot + float(o.rotation()), sx, sy, depth + 1, acc)
            except Exception:
                pass


with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbLine"):
        ln = tr.openObject(eid, 0)
        lay = str(ln.layer())
        if lay not in LAYERS:
            continue
        a, b = ln.startPoint(), ln.endPoint()
        col = -1
        try:
            col = int(ln.color().colorIndex())
        except Exception:
            pass
        out["lines"].append({"lay": lay, "a": [round(a.x, 2), round(a.y, 2)],
                             "b": [round(b.x, 2), round(b.y, 2)], "rgb": col})

    for eid in db.objectIds("AcDbMText"):
        mt = tr.openObject(eid, 0)
        lay = str(mt.layer())
        if lay not in LAYERS:
            continue
        s = str(mt.contents())
        if "{" in s and s.endswith("}"):
            s = s[s.rindex("{") + 1:-1]
        s = s.replace("\\P", " | ").strip()
        p = mt.location()
        out["texts"].append({"lay": lay, "text": s,
                             "pos": [round(p.x, 2), round(p.y, 2)],
                             "rot": round(float(mt.rotation()), 4),
                             "h": round(float(mt.textHeight()), 2)})

    for eid in db.objectIds("AcDbBlockReference"):
        br = tr.openObject(eid, 0)
        lay = str(br.layer())
        if lay not in LAYERS:
            continue
        p = br.position()
        rot = float(br.rotation())
        try:
            sf = br.scaleFactors()
            sx, sy = float(sf.sx), float(sf.sy)
        except Exception:
            sx = sy = 1.0
        name = ""
        try:
            name = str(tr.openObject(br.blockTableRecordId(), 0).getName())
        except Exception:
            pass
        acc = []
        walk(tr, br.blockTableRecordId(), (p.x, p.y), rot, sx, sy, 0, acc)
        out["fits"].append({"block": name, "pos": [round(p.x, 2), round(p.y, 2)],
                            "rot": round(rot, 4), "segs": [[[round(u[0], 2), round(u[1], 2)],
                                                            [round(v[0], 2), round(v[1], 2)]]
                                                           for u, v in acc]})

json.dump(out, open(r"G:\PyArx\tools\cadmcp\strong_2d.json", "w", encoding="utf-8"),
          ensure_ascii=False, separators=(",", ":"), default=str)
print("lines=%d texts=%d fits=%d fitsegs=%d colors=%s types=%s"
      % (len(out["lines"]), len(out["texts"]), len(out["fits"]),
         sum(len(f["segs"]) for f in out["fits"]), out["layer_colors"], out["types"]))
