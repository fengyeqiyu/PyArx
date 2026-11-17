# -*- coding: utf-8 -*-
"""probe_sym2：符号表按名解析失败时，getIdAt + openObject 是否可用。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_sym2.json"
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

step("lt_read_open", lambda: h.setdefault("lt", db.getLayerTable(0)).__class__.__name__)
step("lt_getIdAt_0", lambda: h["lt"].getIdAt("0"))
step("lt_getIdAt_t14", lambda: h["lt"].getIdAt("PYX_T14_LAYER"))
step("lt_getAt_0_null", lambda: h["lt"].getAt("0").isNull())
step("lt_toDict_len", lambda: len(h["lt"].toDict()))
step("lt_names", lambda: list(h["lt"].names()))
step("lt_records_len", lambda: len(h["lt"].records()))
step("lt_close", lambda: h["lt"].close())

step("open_t14_by_id", lambda: db.openObject(h["lt"].getIdAt("PYX_T14_LAYER") if hasattr(h, "__getitem__") else 0, 0).__class__.__name__)

OUT["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(OUT, f, ensure_ascii=False, indent=1, default=str)
print("[PyArx] probe_sym2 done")
