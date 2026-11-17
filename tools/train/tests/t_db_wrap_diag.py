# -*- coding: utf-8 -*-
"""诊断：子类 wrapper 向上包装为基类视角的可行形式。只做只读尝试。"""
import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P

out = {}

def rec(tag, fn):
    try:
        v = fn()
        out[tag] = {"ok": True, "type": type(v).__name__, "repr": repr(v)[:160]}
    except Exception as e:
        out[tag] = {"ok": False, "err": "%s: %s" % (type(e).__name__, str(e)[:220])}

e = P.Db.PyDbLine(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGePoint3d(10, 0, 0))

rec("intPtr type", lambda: type(e.intPtr()).__name__)
rec("PyDbEntity(wrapper)", lambda: P.Db.PyDbEntity(e))
rec("PyDbEntity(intPtr)", lambda: P.Db.PyDbEntity(e.intPtr()))
rec("PyDbObject(wrapper)", lambda: P.Db.PyDbObject(e))
rec("PyDbObject(intPtr)", lambda: P.Db.PyDbObject(e.intPtr()))
rec("PyDbCurve(wrapper)", lambda: P.Db.PyDbCurve(e))
rec("static cast via instance", lambda: e.cast())

# 入库后再看 openObject 返回类型与可包装性
db = P.Db.curDb()
with P.Db.PyDbTrans() as tr:
    tr.addEntity(e)
    oid = e.objectId()
    h = str(e.handle())
    o = tr.openObject(oid)
    out["openObject_type"] = type(o).__name__
    rec("openObject wrap PyDbEntity", lambda: P.Db.PyDbEntity(o))
    rec("openObject wrap PyDbObject", lambda: P.Db.PyDbObject(o))
    rec("openObject wrap PyDbCurve", lambda: P.Db.PyDbCurve(o))

# 图层表 getAt 返回类型
lt = db.getLayerTable()
out["layerTable_type"] = type(lt).__name__
try:
    rec0 = lt.getAt("0")
    out["getAt_type"] = type(rec0).__name__
    out["getAt_name"] = rec0.getName()
    out["getAt_isFrozen"] = rec0.isFrozen()
except Exception as ex:
    out["getAt_err"] = "%s: %s" % (type(ex).__name__, str(ex)[:220])

path = __import__("os").environ.get("PYARX_OUT", r"C:\temp\wrap_diag.json")
with open(path, "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1)
print(json.dumps(out, ensure_ascii=False))
