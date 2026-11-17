# -*- coding: utf-8 -*-
"""诊断：新构造 PyDbDatabase 上各只读调用分别是否抛异常。"""
import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

ACCUM = mlab.out_path(r"C:\temp\db_diag.json")
res = {}
d = P.Db.PyDbDatabase()
for meth, fn in [
    ("isNull", lambda: d.isNull()),
    ("approxNumObjects", lambda: d.approxNumObjects()),
    ("className", lambda: d.className()),
    ("objectIds(AcDbEntity)", lambda: len(list(d.objectIds("AcDbEntity")))),
    ("getFilename", lambda: d.getFilename()),
    ("currentSpaceId", lambda: d.currentSpaceId()),
]:
    try:
        v = fn()
        res[meth] = {"ok": True, "ret": repr(v)[:120], "type": type(v).__name__}
    except Exception as e:
        res[meth] = {"ok": False, "err": "%s: %s" % (type(e).__name__, str(e)[:180])}

with open(ACCUM, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print(json.dumps(res, ensure_ascii=False))
