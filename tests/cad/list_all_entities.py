# -*- coding: utf-8 -*-
"""遍历当前数据库全部图元，报每个 ObjectId 的类型（用 classNameOf 直接判，不靠 cast）。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\all_entities.json"
LOG = []


def log(tag, v):
    LOG.append({"step": tag, "value": v})
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(LOG, f, ensure_ascii=False, indent=1, default=str)


def run():
    db = P.Db.curDb()
    log("db", db.getFilename())
    ids = db.objectIds("AcDbEntity")
    log("total", len(ids))
    counts = {}
    rows = []
    for oid in ids:
        cls = db.classNameOf(oid)
        if cls == "":
            cls = "(打不开)"
        counts[cls] = counts.get(cls, 0) + 1
        rows.append({"id": int(oid), "type": cls})
    log("counts", counts)
    log("rows", rows)
    order = sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))
    for name, n in order:
        msg = "[entities] %-24s %d" % (name, n)
        print(msg)
    log("done", True)


run()
