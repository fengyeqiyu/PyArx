# -*- coding: utf-8 -*-
"""跨命令核对：图层/块是否留存（开新命令读，不依赖上一条脚本的内存状态）。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_notx_verify.json"
NAMES = ["PY_LAYER_NOTX", "PY_SAMPLE_LAYER", "PYX_BLK_PROBE2", "PY_SAMPLE_BLOCK"]
LOG = []


def log(tag, v):
    LOG.append({"step": tag, "value": v})
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(LOG, f, ensure_ascii=False, indent=1, default=str)


def run():
    db = P.Db.curDb()
    lt = db.getLayerTable(0)
    log("layers", lt.names())
    lt.close()
    bt = db.getBlockTable(0)
    log("blocks.has", str({n: bt.has(n) for n in NAMES}))
    for n in NAMES[2:]:
        r = bt.getAt(n, 0)
        if not r.isNull():
            log("ents." + n, len(r.entityIds()))
            r.close()
    bt.close()
    log("entities", len(db.objectIds("AcDbEntity")))
    log("done", True)


run()
