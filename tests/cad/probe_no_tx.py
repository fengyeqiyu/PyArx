# -*- coding: utf-8 -*-
"""无事务读取路径定位探针：逐调用隔离，结果写入 probe_no_tx.log。"""
import traceback

import PyArxCore as P

_LOG = open(r"G:\PyArx\log\probe_no_tx.log", "a", encoding="utf-8")


def say(*args):
    s = " ".join(str(a) for a in args)
    _LOG.write(s + "\n")
    _LOG.flush()
    print(s)

SPACE = P.Db.curDb().currentSpaceId()
DB = P.Db.curDb()


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def try_(tag, fn):
    try:
        v = fn()
        say("[OK  ] %-34s -> %s" % (tag, str(v)[:90]))
    except Exception as e:
        say("[FAIL] %-34s -> %s: %s" % (tag, type(e).__name__, str(e)[:120]))


say("==== space=%s entities=%s" % (SPACE, len(DB.objectIds("AcDbEntity"))))

# 1) 先写一个实体，确认 id 可复现
sp_w = P.Db.PyDbBlockTableRecord(SPACE, 1)
e = P.Db.PyDbLine(P3(600, 0), P3(700, 0))
try_("append", lambda: sp_w.appendAcDbEntity(e))
ID = e.objectId()
try_("objectId", lambda: ID)
try_("entityIds", lambda: len(sp_w.entityIds()))
try_("numEntities", lambda: sp_w.numEntities())
try_("close_w", lambda: sp_w.close())

# 2) 读模式重开空间 + 各种取对象方式
sp_r = P.Db.PyDbBlockTableRecord(SPACE, 0)
try_("r.numEntities", lambda: sp_r.numEntities())
try_("r.entityIds", lambda: len(sp_r.entityIds()))
try_("r.getEntityById(id,0)", lambda: sp_r.getEntityById(ID, 0))
try_("r.getEntityById(id,1)", lambda: sp_r.getEntityById(ID, 1))
try_("r[id].isNull", lambda: sp_r[ID].isNull())
try_("r.iterEntities(0,True)", lambda: len(list(sp_r.iterEntities(0, True))))
try_("r.iterEntities(1,True)", lambda: len(list(sp_r.iterEntities(1, True))))
try_("r.__len__", lambda: len(sp_r))
try_("r.close", lambda: sp_r.close())

# 3) 写模式重开空间
sp_w2 = P.Db.PyDbBlockTableRecord(SPACE, 1)
try_("w.getEntityById(id,1)", lambda: sp_w2.getEntityById(ID, 1))
try_("w.getEntityById(id,0)", lambda: sp_w2.getEntityById(ID, 0))
try_("w.iterEntities(1,True)", lambda: len(list(sp_w2.iterEntities(1, True))))
try_("w.iterEntities(0,True)", lambda: len(list(sp_w2.iterEntities(0, True))))
try_("w.close", lambda: sp_w2.close())

# 4) 符号表（参数个数不同）
try_("db.getLayerTable(0)", lambda: type(DB.getLayerTable(0)).__name__)
lt = DB.getLayerTable(0)
try_("lt.keys", lambda: len(lt.keys()))
try_("lt.getAt('0')", lambda: type(lt.getAt("0")).__name__)
try_("lt.getAt('0',0)", lambda: lt.getAt("0", 0))
try_("lt.records(0)", lambda: len(lt.records(0)))
try_("lt.close", lambda: lt.close())

bt = DB.getBlockTable(0)
try_("bt.getAt('*Model_Space',0)", lambda: type(bt.getAt("*Model_Space", 0)).__name__)
try_("bt.records(0)", lambda: len(bt.records(0)))
try_("bt.keys", lambda: list(bt.keys())[:3])
try_("bt.close", lambda: bt.close())

say("==== end")
import json
with open(r"G:\PyArx\log\probe_no_tx.json", "w", encoding="utf-8") as f:
    json.dump({"done": True}, f)
