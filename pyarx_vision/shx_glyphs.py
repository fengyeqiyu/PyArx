# -*- coding: utf-8 -*-
"""Dump SHX glyph polylines for every top-level TEXT in the current drawing.

Runs inside CAD (PYRUNFILE). Output:
    G:/PyArx/docs/drb018_glyphs.json
    {
      "glyphs": {"<text>\x01<font>\x01<big>": {"s": [[[x,y],...],...], "e": [w,h]}},
      "texts":  [{"x":..,"y":..,"h":..,"wf":..,"r":..,"ha":..,"va":..,"g": key, "l": layer}],
      "stats":  {...}
    }
Coordinates of glyph strokes are unit space: cap height = 1, baseline y = 0, +X advance.
"""
import io
import json
import math
import traceback

OUT = r"G:/PyArx/docs/drb018_glyphs.json"
FONT_DIRS = [
    r"C:/Program Files/Autodesk/AutoCAD 2024/Fonts/",
    r"G:/ObjectArx/2024/",
]


def _first(mod, names, default=None):
    for n in names:
        try:
            v = getattr(mod, n)
            if callable(v):
                return v()
            return v
        except Exception:
            continue
    return default


def _str(o, names, default=""):
    for n in names:
        try:
            v = getattr(o, n)
            v = v() if callable(v) else v
            if v is None:
                continue
            s = str(v)
            if s:
                return s
        except Exception:
            continue
    return default


def _num(o, names, default=0.0):
    for n in names:
        try:
            v = getattr(o, n)
            v = v() if callable(v) else v
            if v is None:
                continue
            return float(v)
        except Exception:
            continue
    return default


def _pt(o, names):
    for n in names:
        try:
            v = getattr(o, n)
            v = v() if callable(v) else v
            if v is None:
                continue
            return (float(v.x), float(v.y))
        except Exception:
            continue
    return None


def load_module():
    import sys
    sys.path.insert(0, r"G:/PyArx/X64/Release")
    cands = ["PyArxCore", "PyArxCoreHgi3", "PyArxCoreHgi2", "PyArxCoreHgi1"]
    for name in cands:
        try:
            mod = __import__(name)
        except Exception:
            continue
        if hasattr(mod, "Gi") and hasattr(mod.Gi, "PyGiTextEngine"):
            return mod
    raise RuntimeError("no PyArxCore build with Gi.PyGiTextEngine found")


def font_path(name, big):
    name = (name or "").strip()
    if not name:
        return None
    low = name.lower()
    if low.startswith("@"):
        low = low[1:]
    for d in FONT_DIRS:
        p = d + low
        try:
            if __import__("os").path.exists(p):
                return p
        except Exception:
            pass
    return None


def main():
    P = load_module()
    db = P.Db.curDb()
    eng = P.Gi.PyGiTextEngine()
    styles = {}
    glyphs = {}
    texts = []
    errs = []
    stats = {"entities": 0, "text": 0, "tess": 0, "fail": 0, "no_font": 0}

    with P.Db.PyDbTrans() as tr:
        sp = tr.openObject(db.currentSpaceId(), 0)
        ids = sp.entityIds()
        stats["entities"] = len(ids)
        for eid in ids:
            try:
                o = tr.openObject(eid, 0)
                short = o.dxfName()
            except Exception:
                continue
            if short != "AcDbText":
                continue
            stats["text"] += 1
            try:
                s = _str(o, ("textString",))
                if not s.strip():
                    continue
                try:
                    ha = int(_num(o, ("horizontalMode",), 0))
                except Exception:
                    ha = 0
                try:
                    va = int(_num(o, ("verticalMode",), 0))
                except Exception:
                    va = 0
                pos = _pt(o, ("position", "getPosition", "location"))
                ap = _pt(o, ("alignmentPoint", "getAlignmentPoint"))
                if (ha or va) and ap and (abs(ap[0]) + abs(ap[1]) > 1e-6):
                    pos = ap
                if not pos:
                    continue
                h = _num(o, ("height", "textHeight"), 250.0) or 250.0
                wf = _num(o, ("widthFactor",), 1.0) or 1.0
                rot = _num(o, ("rotation",), 0.0)
                obl = _num(o, ("oblique", "obliquingAngle"), 0.0)

                font = ""
                big = ""
                try:
                    sid = o.textStyle()
                    key = str(sid)
                    if key not in styles:
                        rec = tr.openObject(sid, 0)
                        fn = _str(rec, ("fileName",))
                        bf = _str(rec, ("bigFontFileName",))
                        styles[key] = (fn.split("/")[-1], bf.split("/")[-1])
                    font, big = styles[key]
                except Exception:
                    pass

                gkey = s + "\x01" + font + "\x01" + big
                if gkey not in glyphs:
                    fp = font_path(font, big)
                    bp = font_path(big, big) if big else None
                    if not fp:
                        stats["no_font"] += 1
                        continue
                    try:
                        st = P.Gi.PyGiTextStyle(
                            font_file=fp,
                            big_font=(bp or ""),
                            text_size=1.0, x_scale=1.0, oblique_angle=0.0,
                            tracking_percent=1.0)
                        r = eng.tessellate(st, s, False, 0.02)
                        segs = []
                        for seg in r["segments"]:
                            if len(seg) < 2:
                                continue
                            segs.append([[round(p[0], 5), round(p[1], 5)] for p in seg])
                        ext = list(r["extents"]) if r["extents"] else [0.0, 1.0]
                        glyphs[gkey] = {"s": segs, "e": [round(ext[0], 5), round(ext[1], 5)]}
                        stats["tess"] += 1
                    except Exception as ex:
                        stats["fail"] += 1
                        if len(errs) < 8:
                            errs.append("tess:%s" % str(ex)[:60])
                        continue

                texts.append({"x": round(pos[0], 2), "y": round(pos[1], 2),
                              "h": round(h, 2), "wf": round(wf, 3),
                              "r": round(rot, 4), "ha": ha, "va": va,
                              "ob": round(obl, 4),
                              "g": gkey, "l": _str(o, ("layer",), "0")})
            except Exception as ex:
                if len(errs) < 8:
                    errs.append("ent:%s" % str(ex)[:60])

    out = {"glyphs": glyphs, "texts": texts, "stats": stats, "err": errs}
    io.open(OUT, "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False, separators=(",", ":")))
    print("GLYPHS ok texts=%d glyphs=%d tess=%d fail=%d nofont=%d"
          % (len(texts), len(glyphs), stats["tess"], stats["fail"], stats["no_font"]))


try:
    main()
except Exception:
    io.open(r"C:/temp/glyph_err.txt", "w", encoding="utf-8").write(traceback.format_exc())
    print("GLYPHS FAILED see C:/temp/glyph_err.txt")
