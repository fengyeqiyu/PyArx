# -*- coding: utf-8 -*-
"""扫描：可构造实体的真实成员清单 + 基类方法，供后续断言脚本选属性用。只读图，不入库。"""
import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

OUT = mlab.out_path(r"C:\temp\dbscan.json")

NOARG = ["PyDbDatabase", "PyDbTrans", "PyDbLine", "PyDbPolyline", "PyDbText",
         "PyDbMText", "PyDbArc", "PyDbBlockReference", "PyDbHatch", "PyDbTable",
         "PyDbViewport", "PyDbBlockTableRecord"]
REFONLY = ["PyDbEntity", "PyDbObject", "PyDbLayerTableRecord", "PyDbCurve"]

scan = {}


def members_of(obj):
    try:
        return [x for x in dir(obj) if not x.startswith("_")]
    except Exception as e:
        return ["<dir err %s>" % type(e).__name__]


for name in NOARG:
    cls = getattr(P.Db, name, None)
    if cls is None:
        scan[name] = {"exists": False}
        continue
    rec = {"exists": True, "cls_members": members_of(cls)}
    try:
        inst = cls()
        rec["ctor"] = "no-arg ok"
        rec["inst_type"] = "%s.%s" % (type(inst).__module__, type(inst).__name__)
        rec["inst_members"] = members_of(inst)
        # 无参实例上抓几个候选 getter 的返回形态（只读）
        peek = {}
        for m in rec["inst_members"]:
            if m.startswith("get") or m in ("handle", "layer", "color", "position", "center",
                                           "radius", "text", "height", "rotation", "startPoint",
                                           "endPoint", "numberOfVertices", "normal", "insertionPoint"):
                fn = getattr(inst, m, None)
                if callable(fn):
                    try:
                        v = fn()
                        peek[m] = {"ret": repr(v)[:120], "type": type(v).__name__}
                    except Exception as e:
                        peek[m] = {"err": "%s: %s" % (type(e).__name__, str(e)[:120])}
        rec["peek"] = peek
    except Exception as e:
        rec["ctor"] = "%s: %s" % (type(e).__name__, str(e)[:200])
    scan[name] = rec

for name in REFONLY:
    cls = getattr(P.Db, name, None)
    scan[name] = {"exists": cls is not None,
                  "cls_members": members_of(cls) if cls is not None else None,
                  "hint": mlab.typeerror_hint(cls, "x") if cls is not None else "missing"}

# 数据库 / 事务侧可用 API
try:
    db = P.Db.curDb()
    scan["<curDb>"] = {"type": "%s.%s" % (type(db).__module__, type(db).__name__),
                        "members": members_of(db)}
except Exception as e:
    scan["<curDb>"] = {"err": "%s: %s" % (type(e).__name__, str(e)[:200])}

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(scan, f, ensure_ascii=False, indent=1)
print(json.dumps({"written": OUT, "n": len(scan)}, ensure_ascii=False))
