import json, math, PyArxCore as P

db = P.Db.curDb()
out = {"file": db.getFilename(), "lines": [], "labels": [], "fittings": []}


def pt(o):
    return (round(o.x, 3), round(o.y, 3))


def strip_rtf(s):
    """去掉 MTEXT 的 \\pxqc...; 格式前缀，取 {..} 里的正文"""
    s = str(s)
    if "{" in s and s.endswith("}"):
        s = s[s.rindex("{") + 1:-1]
    return s.replace("\\P", " | ").strip()


with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbLine"):
        ln = tr.openObject(eid, 0)
        a = pt(ln.startPoint())
        b = pt(ln.endPoint())
        L = math.hypot(b[0] - a[0], b[1] - a[1])
        out["lines"].append({
            "layer": str(ln.layer()),
            "handle": str(ln.handle()),
            "start": a, "end": b,
            "length": round(L, 1),
            "dir": "V" if abs(b[0] - a[0]) < 1e-6 else ("H" if abs(b[1] - a[1]) < 1e-6 else "D"),
        })

    for eid in db.objectIds("AcDbMText"):
        mt = tr.openObject(eid, 0)
        out["labels"].append({
            "layer": str(mt.layer()),
            "handle": str(mt.handle()),
            "text": strip_rtf(mt.contents()),
            "pos": pt(mt.location()),
            "rotation": round(float(mt.rotation()), 4),
        })

    for eid in db.objectIds("AcDbBlockReference"):
        br = tr.openObject(eid, 0)
        name = ""
        try:
            name = str(tr.openObject(br.blockTableRecordId(), 0).getName())
        except Exception:
            pass
        out["fittings"].append({
            "layer": str(br.layer()),
            "handle": str(br.handle()),
            "block": name,
            "pos": pt(br.position()),
            "rotation": round(float(br.rotation()), 4),
        })

# 只保留桥架相关图层
tray_layers = [l for l in set([x["layer"] for x in out["lines"]] + [x["layer"] for x in out["fittings"]])]
out["layers"] = sorted(tray_layers)

json.dump(out, open(r"G:\PyArx\tools\cadmcp\tray_out.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("lines=%d labels=%d fittings=%d" % (len(out["lines"]), len(out["labels"]), len(out["fittings"])))
