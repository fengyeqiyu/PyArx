# -*- coding: utf-8 -*-
"""probe_symtab：符号表读取 API 逐项（keys/values/records/numRecords/getAt/has）。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_symtab.json"
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


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(OUT, f, ensure_ascii=False, indent=1, default=str)


db = P.Db.curDb()
h = {}

step("lt_open", lambda: h.setdefault("lt", db.getLayerTable(0)).__class__.__name__)
step("lt_numRecords", lambda: h["lt"].numRecords())
step("lt_keys", lambda: list(h["lt"].keys()))
step("lt_len", lambda: len(h["lt"]))
step("lt_has14", lambda: h["lt"].has("PYX_T14_LAYER"))
step("lt_getAt_isNull", lambda: h["lt"].getAt("PYX_T14_LAYER").isNull())
step("lt_getName", lambda: str(h["lt"].getAt("PYX_T14_LAYER").getName()))
step("lt_close", lambda: h["lt"].close())
step("bt_open", lambda: h.setdefault("bt", db.getBlockTable(0)).__class__.__name__)
step("bt_numRecords", lambda: h["bt"].numRecords())
step("bt_keys", lambda: list(h["bt"].keys()))
step("bt_len", lambda: len(h["bt"]))
step("bt_getAt_null", lambda: h["bt"].getAt("PYX_T14_BLOCK", 0).isNull())
step("bt_name", lambda: str(h["bt"].getAt("PYX_T14_BLOCK", 0).getName()))
step("bt_ents", lambda: len(h["bt"].getAt("PYX_T14_BLOCK", 0).entityIds()))
step("bt_close", lambda: h["bt"].close())

OUT["done"] = True
flush()
print("[PyArx] probe_symtab done")
