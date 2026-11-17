# -*- coding: utf-8 -*-
"""判类型 -> cast 成对应类 -> 调该类型特有方法。"""
import json
import traceback

import PyArxCore as P

RES = r"G:\PyArx\log\typed_calls.json"
LOG = []

PROBE = {
    "AcDbCircle": "radius",
    "AcDbLine": "startPoint",
    "AcDbArc": "radius",
    "AcDbText": "textString",
    "AcDbPolyline": "numVerts",
    "AcDbBlockReference": "position",
}


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(LOG, f, ensure_ascii=False, indent=1, default=str)


def log(tag, v):
    LOG.append({"step": tag, "value": v})
    flush()


def dir_of(obj):
    try:
        return set(dir(obj))
    except Exception:
        return set()


def find_class(dbmod, cls):
    """AcDbCircle -> PyDbCircle（前缀 PyDb + 去掉 AcDb 的类名）。"""
    short = cls[4:] if cls.startswith("AcDb") else cls
    for name in ("PyDb" + short, "PyAcDb" + short, "Py" + short):
        c = getattr(dbmod, name, None)
        if c is not None:
            return name, c
    return None, None


def run():
    db = P.Db.curDb()
    base = dir_of(P.Db.PyDbObject) | dir_of(P.Db.PyDbEntity)
    buf = getattr(P, "Db", None)
    if buf is not None:
        base |= dir_of(getattr(buf, "PyRxObject", None))
    log("base.n", len(base))
    ids = db.objectIds("AcDbEntity")
    log("total", len(ids))
    rows = []
    for oid in ids:
        cls = db.classNameOf(oid)
        cname, pycls = find_class(P.Db, cls)
        row = {"id": int(oid), "type": cls, "pycls": cname}
        if pycls is None:
            rows.append(row)
            continue
        obj = db.openObject(oid, 0)
        if obj.isNull():
            row["open"] = "null"
            rows.append(row)
            continue
        try:
            typed = pycls.cast(obj)
            row["castNull"] = bool(typed.isNull())
            if not typed.isNull():
                own = [m for m in dir(typed) if not m.startswith("_") and m not in base]
                row["own.n"] = len(own)
                row["own"] = own[:12]
                pn = PROBE.get(cls)
                if pn:
                    fn = getattr(typed, pn, None)
                    if fn is None:
                        row["probe"] = "(无 " + pn + ")"
                    else:
                        row["probe"] = pn + "=" + str(fn())
                typed.close()
        except Exception as e:
            row["err"] = type(e).__name__ + ": " + str(e)[:100]
        finally:
            try:
                if not obj.isNull():
                    obj.close()
            except Exception:
                pass
        rows.append(row)
        flush()
    log("rows", rows)
    for r in rows:
        msg = "[typed] %-20s %s" % (r["type"], str(r.get("probe", r.get("err", "")))[:70])
        print(msg)
    log("done", True)


try:
    run()
except Exception:
    log("fatal", traceback.format_exc()[-1200:])
    print("[typed] FATAL " + traceback.format_exc().splitlines()[-1])
