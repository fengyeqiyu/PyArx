# -*- coding: utf-8 -*-
"""ARX 无头导出：全类型实体权威字段，按 handle 落 TSV，供离线 Python 解释器逐类比对。

运行环境：accoreconsole 进程内（PyArxEngine.crx 已加载），PYRUNFILE 执行本脚本。
固定列：handle, class, layer, ltype, colorIndex, r, g, b, lineWeight, ltypeScale,
        visibility, plotStyle；geom 列为该类几何的 JSON 串（变长顶点/控制点等）。
"""
import json
import os
import sys
import traceback

import PyArxCore as P

SRC = os.environ.get(
    "PYARX_DUMP_SRC",
    r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg")
OUT = os.environ.get(
    "PYARX_DUMP_OUT",
    r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv")
ERRLOG = os.path.join(os.path.dirname(OUT), "arx_dump_all_err.log")

HEADER = ["handle", "class", "layer", "ltype", "colorIndex", "r", "g", "b",
          "lineWeight", "ltypeScale", "visibility", "plotStyle", "geom"]

stat = {"total_ents": 0, "by_class": {}, "errors": []}


def _safe(fn, default=None):
    try:
        return fn()
    except Exception as ex:
        stat["errors"].append("%s: %r" % (getattr(fn, "__name__", "?"), ex))
        return default


def _pt(p):
    if p is None:
        return None
    x = round(float(p.x), 6)
    y = round(float(p.y), 6)
    z = round(float(p.z), 6) if hasattr(p, "z") else 0.0
    return [x, y, z]


def _vec(v):
    if v is None:
        return None
    x = round(float(v.x), 6)
    y = round(float(v.y), 6)
    z = round(float(v.z), 6) if hasattr(v, "z") else 0.0
    return [x, y, z]


def _sc3(s):
    if s is None:
        return None
    return [round(float(s.sx), 6), round(float(s.sy), 6), round(float(s.sz), 6)]


def geom_of(cls, o):
    """按类提取几何，返回可 JSON 序列化的 dict。"""
    g = {}
    if cls == "AcDbLine":
        g["sp"] = _pt(o.startPoint())
        g["ep"] = _pt(o.endPoint())
    elif cls == "AcDbCircle":
        g["center"] = _pt(o.center())
        g["radius"] = _safe(o.radius)
        g["normal"] = _vec(o.normal())
    elif cls == "AcDbArc":
        g["center"] = _pt(o.center())
        g["radius"] = _safe(o.radius)
        g["startAngle"] = _safe(o.startAngle)
        g["endAngle"] = _safe(o.endAngle)
        g["normal"] = _vec(o.normal())
    elif cls == "AcDbPoint":
        g["pos"] = _pt(o.position())
        g["normal"] = _vec(o.normal())
        g["ecsRotation"] = _safe(o.ecsRotation)
    elif cls == "AcDbEllipse":
        g["center"] = _pt(o.center())
        g["normal"] = _vec(o.normal())
        g["majorAxis"] = _vec(o.majorAxis())
        g["radiusRatio"] = _safe(o.radiusRatio)
        g["startAngle"] = _safe(o.startAngle)
        g["endAngle"] = _safe(o.endAngle)
    elif cls == "AcDbPolyline":
        n = _safe(o.numVerts, 0) or 0
        verts = []
        for i in range(n):
            pt = _safe(lambda i=i: o.getPointAt(i))
            bul = _safe(lambda i=i: o.getBulgeAt(i), 0.0)
            w = _safe(lambda i=i: o.getWidthsAt(i), None)
            verts.append({"pt": _pt(pt), "bulge": bul,
                          "widths": [w[0], w[1]] if w else None})
        g["verts"] = verts
        g["elevation"] = _safe(o.elevation)
        g["thickness"] = _safe(o.thickness)
        g["normal"] = _vec(o.normal())
        g["constWidth"] = _safe(o.constantWidth)
    elif cls == "AcDbBlockReference":
        g["pos"] = _pt(o.position())
        g["rotation"] = _safe(o.rotation)
        g["scale"] = _sc3(_safe(o.scaleFactors))
        g["normal"] = _vec(o.normal())
        g["blockRecordId"] = _safe(o.blockTableRecordId, 0)
    elif cls == "AcDbText":
        g["pos"] = _pt(o.position())
        g["alignPt"] = _pt(o.alignmentPoint())
        g["height"] = _safe(o.height)
        g["rotation"] = _safe(o.rotation)
        g["widthFactor"] = _safe(o.widthFactor)
        g["oblique"] = _safe(o.oblique)
        g["text"] = _safe(o.textString, "")
        g["horizMode"] = _safe(o.horizontalMode)
        g["vertMode"] = _safe(o.verticalMode)
        g["justification"] = _safe(o.justification)
        g["mirrorX"] = _safe(o.isMirroredInX)
        g["mirrorY"] = _safe(o.isMirroredInY)
    elif cls == "AcDbMText":
        g["loc"] = _pt(o.location())
        g["direction"] = _vec(o.direction())
        g["rotation"] = _safe(o.rotation)
        g["width"] = _safe(o.width)
        g["height"] = _safe(o.textHeight)
        g["attachment"] = _safe(o.attachment)
        g["flowDir"] = _safe(o.flowDirection)
        g["contents"] = _safe(o.contents, "")
    elif cls == "AcDbSpline":
        nc = _safe(o.numControlPoints, 0) or 0
        ctrl = [_pt(_safe(lambda i=i: o.getControlPointAt(i))) for i in range(nc)]
        wts = [_safe(lambda i=i: o.weightAt(i)) for i in range(nc)]
        nf = _safe(o.numFitPoints, 0) or 0
        fit = [_pt(_safe(lambda i=i: o.getFitPointAt(i))) for i in range(nf)]
        g["degree"] = _safe(o.degree)
        g["rational"] = _safe(o.isRational)
        g["ctrl"] = ctrl
        g["weights"] = wts
        g["fit"] = fit
        g["fitTol"] = _safe(o.fitTolerance)
    elif cls == "AcDbHatch":
        nl = _safe(o.numLoops, 0) or 0
        loops = []
        for i in range(nl):
            lt, pts, bul = _safe(lambda i=i: o.getLoopAt(i), (0, [], []))
            loops.append({"type": lt,
                          "pts": [_pt(p) for p in (pts or [])],
                          "bulges": list(bul or [])})
        g["loops"] = loops
        g["patternName"] = _safe(o.patternName, "")
        g["patternType"] = _safe(o.patternType)
        g["origin"] = _pt(o.originPoint())
        g["patternAngle"] = _safe(o.patternAngle)
        g["patternSpace"] = _safe(o.patternSpace)
        g["hatchStyle"] = _safe(o.hatchStyle)
        g["area"] = _safe(o.area)
    elif cls == "AcDbDimension":
        g["text"] = _safe(o.dimensionText, "")
        g["textPos"] = _pt(o.textPosition())
        g["measurement"] = _safe(o.measurement)
        g["normal"] = _vec(o.normal())
        g["elevation"] = _safe(o.elevation)
        g["textRotation"] = _safe(o.textRotation)
        g["horizRotation"] = _safe(o.horizontalRotation)
        g["dimStyle"] = _safe(o.dimensionStyle, 0)
        g["prefix"] = _safe(o.prefix, "")
        g["suffix"] = _safe(o.suffix, "")
    elif cls == "AcDbTable":
        g["note"] = "table geometry not dumped yet"
    else:
        g["note"] = "unsupported class"
    return g


def main():
    db = None
    try:
        db = P.Db.curDb()
    except Exception as ex:
        stat["errors"].append("curDb: %r" % ex)
    if db is None:
        db = P.Db.PyDbDatabase(False, False)
        st = db.readDwgFile(SRC, 1, True, "")
        stat["read_status"] = str(st)
    stat["filename"] = _safe(lambda: str(db.getFilename()), "")

    bt = db.getBlockTable(0)
    names = _safe(lambda: bt.names(), [])
    stat["block_records"] = len(names)

    rows = []
    tr = P.Db.PyDbTrans(db)
    for name in names:
        try:
            rec = bt.getAt(name, 0)
            if rec is None:
                continue
            ids = rec.entityIds()
        except Exception as ex:
            stat["errors"].append("entityIds %s: %r" % (name, ex))
            continue
        for eid in ids:
            stat["total_ents"] += 1
            o = None
            try:
                o = tr.openObject(eid, 0)
                if o is None:
                    continue
                cls = str(o.className())
                stat["by_class"][cls] = stat["by_class"].get(cls, 0) + 1

                def rgb():
                    t = o.colorRGB()
                    return (int(t[0]), int(t[1]), int(t[2]))

                r, g, b = _safe(rgb, (-1, -1, -1))
                geom = _safe(lambda: geom_of(cls, o), {"note": "geom_error"})
                rows.append([
                    str(int(o.handle())),
                    cls,
                    str(_safe(o.layer, "?")),
                    str(_safe(o.linetype, "?")),
                    str(int(o.colorIndex())),
                    str(r), str(g), str(b),
                    str(int(_safe(o.lineWeight, -1))),
                    repr(float(_safe(o.linetypeScale, -1.0))),
                    str(int(_safe(o.visibility, -1))),
                    str(_safe(o.plotStyleName, "?")),
                    json.dumps(geom, ensure_ascii=False),
                ])
            except Exception as ex:
                stat["errors"].append("handle=%s: %r" % (
                    _safe(lambda: str(int(o.handle())) if o is not None else "?"), ex))
            finally:
                try:
                    if o is not None:
                        o.close()
                except Exception:
                    pass
    try:
        tr.abort()
    except Exception:
        pass

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\t".join(HEADER) + "\n")
        for row in rows:
            f.write("\t".join(row) + "\n")

    with open(ERRLOG, "w", encoding="utf-8") as f:
        f.write(json.dumps(stat, ensure_ascii=False, indent=1, default=str))

    print("[DUMP] ents=%d blocks=%d out=%s"
          % (stat["total_ents"], stat["block_records"], OUT), flush=True)


if __name__ == "__main__":
    try:
        main()
    except Exception:
        stat["fatal"] = traceback.format_exc()[-1500:]
        with open(ERRLOG, "w", encoding="utf-8") as f:
            f.write(json.dumps(stat, ensure_ascii=False, indent=1, default=str))
        print("[DUMP] FATAL\n" + stat["fatal"], flush=True)
