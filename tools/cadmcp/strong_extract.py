import json, math, re, PyArxCore as P

db = P.Db.curDb()
out = {"file": db.getFilename(), "lines": [], "labels": [], "fittings": []}

MODEL = None
try:
    bt = db.getBlockTable()
    MODEL = bt.getAt("*Model_Space").objectId()
except Exception:
    MODEL = None


def pt(o):
    return (round(o.x, 3), round(o.y, 3), round(o.z, 3))


def strip_rtf(s):
    s = str(s)
    if "{" in s and s.endswith("}"):
        s = s[s.rindex("{") + 1:-1]
    return s.replace("\\P", " | ").strip()


def in_model(o):
    if MODEL is None:
        return True
    try:
        return int(o.ownerId()) == int(MODEL)
    except Exception:
        return True


with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbLine"):
        ln = tr.openObject(eid, 0)
        lay = str(ln.layer())
        if not lay.startswith("E-CABL-TRAY"):
            continue
        if not in_model(ln):
            continue
        a = pt(ln.startPoint())
        b = pt(ln.endPoint())
        out["lines"].append({"layer": lay, "handle": str(ln.handle()), "start": a, "end": b,
                             "length": round(math.dist(a, b), 1)})

    for eid in db.objectIds("AcDbMText"):
        mt = tr.openObject(eid, 0)
        lay = str(mt.layer())
        if lay != "E-CABL-TRAY-IDEN":
            continue
        if not in_model(mt):
            continue
        out["labels"].append({"handle": str(mt.handle()), "text": strip_rtf(mt.contents()),
                              "pos": pt(mt.location()), "rot": round(float(mt.rotation()), 4),
                              "height": round(float(mt.textHeight()), 2)})

    for eid in db.objectIds("AcDbBlockReference"):
        br = tr.openObject(eid, 0)
        lay = str(br.layer())
        if not lay.startswith("E-CABL-TRAY"):
            continue
        if not in_model(br):
            continue
        name = ""
        try:
            name = str(tr.openObject(br.blockTableRecordId(), 0).getName())
        except Exception:
            pass
        # 真实世界包围盒：嵌套块的插入点常是父块里的局部坐标，只有 extents 才是 WCS 真值
        # getGeomExtents 返回 (minPt, maxPt) 元组
        bb = None
        try:
            ex = br.getGeomExtents()
            if isinstance(ex, tuple):
                lo, hi = pt(ex[0]), pt(ex[1])
            else:
                lo, hi = pt(ex.minPoint()), pt(ex.maxPoint())
            if all(abs(v) < 1e7 for v in lo + hi) and (hi[0] - lo[0]) < 5e4 and (hi[1] - lo[1]) < 5e4:
                bb = [lo, hi]
        except Exception:
            bb = None
        out["fittings"].append({"handle": str(br.handle()), "block": name,
                                "pos": pt(br.position()), "rot": round(float(br.rotation()), 4),
                                "bbox": bb,
                                "scale": [round(float(x), 3) for x in
                                          (br.scaleFactors().sx, br.scaleFactors().sy, br.scaleFactors().sz)]
                                if hasattr(br.scaleFactors(), "sx") else None})

json.dump(out, open(r"G:\PyArx\tools\cadmcp\strong_out.json", "w", encoding="utf-8"),
          ensure_ascii=False, indent=1, default=str)
print("lines=%d labels=%d fittings=%d" % (len(out["lines"]), len(out["labels"]), len(out["fittings"])))
