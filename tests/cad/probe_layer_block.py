# -*- coding: utf-8 -*-
"""图层表/块表 逐调用隔离探针（不使用事务），结果写 pyarx_learn.log。"""
import datetime
import json

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\probe_layer_block.json"


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def flush(d):
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(d, f, ensure_ascii=False, indent=1)


def try_(tag, fn, out):
    try:
        v = fn()
        out[tag] = v if isinstance(v, (int, float, bool, str, type(None))) else str(v)[:70]
        log("OK", "%s -> %s" % (tag, out[tag]))
    except Exception as e:
        out[tag] = "ERR " + type(e).__name__ + ": " + str(e)[:120]
        log("FAIL", "%s -> %s" % (tag, out[tag]))


DB = P.Db.curDb()
log("====", "probe_layer_block 开始")
OUT = {"L": {}, "B": {}}
flush(OUT)

# ---------- 图层表 ----------
lt = None


def l_open():
    global lt
    lt = DB.getLayerTable(0)
    return type(lt).__name__


try_("L.open", l_open, OUT["L"])
try_("L.keys", lambda: len(lt.keys()), OUT["L"])
try_("L.has", lambda: lt.has("PYX_T11_LAYER"), OUT["L"])
try_("L.numRecords", lambda: lt.numRecords(), OUT["L"])
try_("L.getIdAt", lambda: lt.getIdAt("PYX_T11_LAYER"), OUT["L"])

rec = {}


def l_getAt():
    rec["r"] = lt.getAt("PYX_T11_LAYER")
    return type(rec["r"]).__name__


try_("L.getAt", l_getAt, OUT["L"])
try_("L.rec.isNull", lambda: rec["r"].isNull(), OUT["L"])
try_("L.rec.getName", lambda: str(rec["r"].getName()), OUT["L"])
try_("L.rec.color", lambda: str(rec["r"].color()), OUT["L"])
try_("L.rec.upgrade", lambda: rec["r"].upgradeOpen(), OUT["L"])
try_("L.rec.isOff", lambda: rec["r"].isOff(), OUT["L"])
try_("L.rec.close", lambda: rec["r"].close(), OUT["L"])
try_("L.close", lambda: lt.close(), OUT["L"])
flush(OUT)

# ---------- 块表 ----------
bt = None
brec = {}


def b_open():
    global bt
    bt = DB.getBlockTable(0)
    return type(bt).__name__


try_("B.open", b_open, OUT["B"])
try_("B.keys", lambda: list(bt.keys())[:6], OUT["B"])
try_("B.numRecords", lambda: bt.numRecords(), OUT["B"])


def b_getAt():
    brec["r"] = bt.getAt("PYX_T11_BLOCK", 0)
    return type(brec["r"]).__name__


try_("B.getAt", b_getAt, OUT["B"])
try_("B.rec.isNull", lambda: brec["r"].isNull(), OUT["B"])
try_("B.rec.getName", lambda: str(brec["r"].getName()), OUT["B"])
try_("B.rec.numEntities", lambda: brec["r"].numEntities(), OUT["B"])
try_("B.rec.entityIds", lambda: len(brec["r"].entityIds()), OUT["B"])
try_("B.rec.close", lambda: brec["r"].close(), OUT["B"])
try_("B.close", lambda: bt.close(), OUT["B"])

OUT["done"] = True
flush(OUT)
log("====", "probe_layer_block 结束")
print("[PyArx] probe_layer_block done")
