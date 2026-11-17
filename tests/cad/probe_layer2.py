# -*- coding: utf-8 -*-
"""probe_layer2：建图层后立刻 rec.close()，再按名读取；对比旧调整记录。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_layer2.json"
LOG = r"G:\PyArx\log\pyarx_learn.log"
OUT = {"steps": [], "done": False}


def step(tag, fn):
    rec = {"tag": tag}
    try:
        v = fn()
        rec["ok"] = True
        rec["v"] = v if isinstance(v, (int, float, bool, str, type(None), list, dict)) else str(v)[:90]
    except Exception as e:
        rec["ok"] = False
        rec["v"] = type(e).__name__ + ": " + str(e)[:120]
    OUT["steps"].append(rec)
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(OUT, f, ensure_ascii=False, indent=1, default=str)


db = P.Db.curDb()
h = {}


def create():
    tr = P.Db.PyDbTrans()
    lt = None
    try:
        lt = db.getLayerTable(1)
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName("PYX_T15_LAYER")
        rec.setColor(80, 160, 220)
        ok = lt.add(rec)
        closed = rec.close()
        tr.commit()
        return {"add": ok, "rec_close": closed}
    except Exception:
        try:
            tr.abort()
        except Exception:
            pass
        raise
    finally:
        if lt is not None:
            try:
                lt.close()
            except Exception:
                pass


step("create_t15", create)
step("lt_open", lambda: h.setdefault("lt", db.getLayerTable(0)).__class__.__name__)
step("getAt_t15_null", lambda: h["lt"].getAt("PYX_T15_LAYER").isNull())
step("getAt_t14_null", lambda: h["lt"].getAt("PYX_T14_LAYER").isNull())
step("getAt_0_null", lambda: h["lt"].getAt("0").isNull())
step("keys", lambda: list(h["lt"].keys()))
step("numRecords", lambda: h["lt"].numRecords())
step("close", lambda: h["lt"].close())

OUT["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(OUT, f, ensure_ascii=False, indent=1, default=str)
with open(LOG, "a", encoding="utf-8") as f:
    f.write("[PROBE_LAYER2] %s\n" % json.dumps(OUT["steps"], ensure_ascii=False, default=str)[:400])
print("[PyArx] probe_layer2 done")
