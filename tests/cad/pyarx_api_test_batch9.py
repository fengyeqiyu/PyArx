# -*- coding: utf-8 -*-
"""PyArxCore 学习测试 batch9 —— 全程不使用事务（PyDbTrans）。

写入路径：PyDbBlockTableRecord(db.currentSpaceId(), 1).appendAcDbEntity()
读取路径：直接用 Python 侧保留的实体包装；符号表用 getAt(name, mode)
所有结果同时写入 pyarx_learn.log 与 pyarx_test_batch9.json
"""
import datetime
import json
import math
import os
import traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch9.json"
IDS = {}
R = {"items": [], "done": False}


def _ts():
    return datetime.datetime.now().strftime("%H:%M:%S")


def log(tag, msg):
    try:
        with open(LOG, "a", encoding="utf-8") as f:
            f.write("[%s] %-6s %s\n" % (_ts(), tag, msg))
    except Exception:
        pass


def flush():
    try:
        with open(RES, "w", encoding="utf-8") as f:
            json.dump(R, f, ensure_ascii=False, indent=1)
    except Exception:
        pass


def item(name, desc, fn):
    rec = {"item": name, "desc": desc}
    log("START", "%s | %s" % (name, desc))
    try:
        rec["data"] = fn()
        rec["ok"] = True
        log("OK", name)
    except Exception as e:
        rec["ok"] = False
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
        log("FAIL", "%s | %s" % (name, rec["err"]))
        log("TRACE", traceback.format_exc().splitlines()[-1][:200])
    R["items"].append(rec)
    flush()
    return rec


def pitfall(text):
    log("坑点", text)


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def V3(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


def P2(x, y):
    return P.Ge.AcGePoint2d(float(x), float(y))


log("====", "batch9 脚本已进入，import 成功")
db = P.Db.curDb()
try:
    doc = P.Ap.PyApp.documentManager.curDocument
    _docname = str(doc.name())
except Exception as _e:
    _docname = "doc? " + type(_e).__name__ + ": " + str(_e)[:200]

log("====", "batch9 开始 doc=%s" % _docname)


# ---------------------------------------------------------------- T1 空间
def t1_space():
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    out = {"spaceId": db.currentSpaceId(), "numEntities": sp.numEntities(),
           "wrapper": type(sp).__name__}
    sp.close()
    return out


item("T1_space", "无事务打开当前空间(模型空间)，读实体数", t1_space)


# ---------------------------------------------------------------- T2 绘图
def _makers():
    return [
        ("line", lambda: P.Db.PyDbLine(P3(0, 0), P3(100, 0))),
        ("circle", lambda: P.Db.PyDbCircle(P3(50, 50), V3(0, 0, 1), 20)),
        ("arc", lambda: P.Db.PyDbArc(P3(0, 60), 30, 0.0, math.pi)),
        ("text", lambda: P.Db.PyDbText(P3(0, -20), "PyArx T9", 4.0, 0.0)),
        ("ellipse", lambda: P.Db.PyDbEllipse(P3(120, 50), V3(0, 0, 1), V3(25, 0, 0), 0.4)),
        ("point", lambda: P.Db.PyDbPoint(P3(150, 0))),
        ("solid", lambda: P.Db.PyDbSolid(P3(0, 100), P3(20, 100), P3(20, 120), P3(0, 120))),
    ]


def t2_draw():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    for k, mk in _makers():
        try:
            e = mk()
            ret = sp.appendAcDbEntity(e)
            IDS[k] = e.objectId()
            out[k] = {"append": ret, "id": IDS[k], "cls": str(e.className())}
        except Exception as ex:
            out[k] = "ERR " + type(ex).__name__ + ": " + str(ex)[:140]
            pitfall("%s 追加失败: %s" % (k, out[k]))
    out["numEntities"] = sp.numEntities()
    sp.close()
    return out


item("T2_draw", "无事务创建 7 类图元并 appendAcDbEntity 到模型空间", t2_draw)


# ---------------------------------------------------------------- T3 读回
def t3_read():
    """用 getEntityById 读回，验证无事务读取路径。"""
    out = {}
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
    for k in IDS:
        try:
            e = sp.getEntityById(IDS[k], 0)
            if e is None:
                out[k] = "None"
                continue
            ext = e.getGeomExtents() if hasattr(e, "getGeomExtents") else None
            out[k] = {"cls": str(e.className()), "wrapper": type(e).__name__,
                      "ext": None if ext is None else [str(ext[0]), str(ext[1])]}
            e.close()
        except Exception as ex:
            out[k] = "ERR " + type(ex).__name__ + ": " + str(ex)[:140]
    out["ids_len"] = len(sp.entityIds())
    out["num"] = sp.numEntities()
    sp.close()
    return out


item("T3_read", "无事务用 getEntityById 读回全部图元(类/包围盒)", t3_read)


# ---------------------------------------------------------------- T4 修改
def t4_modify():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)

    e = sp.getEntityById(IDS["line"], 1)
    out["line_wrapper"] = type(e).__name__
    out["line_color"] = e.setColorIndex(3)
    out["line_layer"] = e.setLayer("0")
    e.close()

    ent = sp.getEntityById(IDS["circle"], 1)
    c = P.Db.PyDbCircle.cast(ent)
    out["circle_radius"] = c.setRadius(35.0)
    out["radius_now"] = c.radius()
    c.close()
    pitfall("cast 后的包装与原包装指向同一对象，两个包装只能 close() 一次，否则二次关闭失败")

    ent2 = sp.getEntityById(IDS["text"], 1)
    t = P.Db.PyDbText.cast(ent2)
    out["text_set"] = t.setTextString("PyArx T9 已修改")
    out["text_now"] = str(t.textString())
    t.close()
    sp.close()
    return out


item("T4_modify", "无事务按 ObjectId 打开图元改颜色/半径/文字", t4_modify)


# ---------------------------------------------------------------- T5 图层
def t5_layers():
    out = {}
    lt = db.getLayerTable(1)
    rec = P.Db.PyDbLayerTableRecord.newRecord()
    out["setName"] = rec.setName("PYX_T9_LAYER")
    rec.setColor(200, 80, 40)
    out["add"] = lt.add(rec)
    IDS["layer"] = rec.objectId()
    lt.close()

    lt2 = db.getLayerTable(0)
    out["has"] = lt2.getAt("PYX_T9_LAYER", 0) is not None
    out["keys_n"] = len(lt2.keys())
    out["records_n"] = len(lt2.records(0))
    r = lt2.getAt("PYX_T9_LAYER", 0)
    out["name"] = str(r.getName())
    out["color"] = str(r.color())
    r.close()
    lt2.close()

    lt3 = db.getLayerTable(1)
    r3 = lt3.getAt("PYX_T9_LAYER", 1)
    r3.setIsOff(True)
    out["off"] = r3.isOff()
    r3.setIsOff(False)
    r3.setIsLocked(True)
    out["locked"] = r3.isLocked()
    r3.setIsLocked(False)
    out["inUse"] = r3.isInUse()
    r3.close()
    lt3.close()

    l = P.Db.PyDbLine(IDS["line"], 1)
    out["setLayer"] = l.setLayer("PYX_T9_LAYER")
    out["layer_now"] = str(l.layer())
    l.close()
    return out


item("T5_layers", "无事务建层/枚举/按名取/开关状态/图元换层", t5_layers)


# ---------------------------------------------------------------- T6 块表
def t6_blocks():
    out = {}
    bt = db.getBlockTable(1)
    btr = P.Db.PyDbBlockTableRecord()
    btr.setName("PYX_T9_BLOCK")
    btr.appendAcDbEntity(P.Db.PyDbCircle(P3(0, 0), V3(0, 0, 1), 10))
    btr.appendAcDbEntity(P.Db.PyDbLine(P3(-10, 0), P3(10, 0)))
    out["bt_add"] = bt.add(btr)
    IDS["blk"] = btr.objectId()
    out["numEntities"] = btr.numEntities()
    btr.close()
    bt.close()

    # 插块参照
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(IDS["blk"])
    br.setPosition(P3(500, 0))
    out["insert"] = sp.appendAcDbEntity(br)
    IDS["ref"] = br.objectId()
    out["ref_id"] = IDS["ref"]
    out["space_num"] = sp.numEntities()
    sp.close()

    # 回查块定义
    bt2 = db.getBlockTable(0)
    rec = bt2.getAt("PYX_T9_BLOCK", 0)
    out["block_name"] = str(rec.getName())
    out["block_ents"] = len(rec.entityIds())
    rec.close()
    bt2.close()
    return out


item("T6_blocks", "无事务建块定义/插入块参照/回查块表", t6_blocks)


# ---------------------------------------------------------------- T7 迭代
def t7_iter():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
    out["num"] = sp.numEntities()
    out["ids"] = len(sp.entityIds())
    n = 0
    cls = []
    for e in sp.iterEntities(0, True):
        n += 1
        cls.append(str(e.className()))
    out["iter_n"] = n
    out["iter_cls"] = cls[:4]
    try:
        out["len"] = len(sp)
    except Exception as ex:
        out["len"] = "ERR " + str(ex)[:80]
        pitfall("len(space) 不可用: %s" % str(ex)[:80])
    try:
        first = sp.entityIds()[0]
        out["getitem"] = sp[first].isNull()
    except Exception as ex:
        out["getitem"] = "ERR " + str(ex)[:80]
        pitfall("space[id] 不可用: %s" % str(ex)[:80])
    sp.close()
    return out


item("T7_iter", "无事务遍历空间: numEntities/entityIds/iterEntities/len", t7_iter)


# ---------------------------------------------------------------- T8 Ed
def t8_ed():
    out = {}
    ss = P.Ed.ssget("X", None, None, [(0, "LINE")])
    out["ss_n"] = None if ss is None else P.Ed.sslength(ss)
    if ss is not None and P.Ed.sslength(ss) > 0:
        en = P.Ed.ssname(ss, 0)
        dxf = P.Ed.entget(en)
        out["entget"] = {"0": dxf.get(0), "8": dxf.get(8), "62": dxf.get(62), "n": len(dxf)}
    out["entlast"] = P.Ed.entlast() is not None
    out["sssetfirst"] = P.Ed.sssetfirst([IDS["line"]])
    g = P.Ed.ssgetfirst()
    out["ssgetfirst"] = [len(g[0]), len(g[1])]
    out["clear"] = P.Ed.sssetfirst([])
    return out


item("T8_ed", "Ed 全局: ssget/entget/entlast/sssetfirst", t8_ed)


# ---------------------------------------------------------------- T9 删除
def t9_delete():
    out = {}
    n0 = len(db.objectIds("AcDbEntity"))
    sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    e = sp.getEntityById(IDS["point"], 1)
    out["erase"] = e.erase()
    out["erased"] = e.isErased()
    e.close()
    n1 = len(db.objectIds("AcDbEntity"))
    e2 = sp.getEntityById(IDS["point"], 1)
    out["unerase"] = e2.erase(False)
    out["erased_after"] = e2.isErased()
    e2.close()
    sp.close()
    n2 = len(db.objectIds("AcDbEntity"))
    out["count"] = {"before": n0, "after_erase": n1, "after_unerase": n2}
    return out


item("T9_delete", "无事务删除(POINT)并恢复", t9_delete)

R["done"] = True
flush()
log("====", "batch9 结束")
print("[PyArx] batch9 done")
