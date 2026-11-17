# -*- coding: utf-8 -*-
"""pyarx_pitfall_probe.py — 绑定层易错点基线复现探针（AutoCAD 进程内 PYRUNFILE）。

每条用例独立 try/except，结果写 G:\\PyArx\\log\\pyarx_pitfall_probe.json。
不做任何会导致 CAD 进程崩溃的解引用；空指针解引用类缺陷只记录包装 isNull() 状态，
崩溃行为在源码层标注。
"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_pitfall_probe.json"
R = {}


def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1)


def rec(k, fn):
    try:
        v = fn()
        R[k] = {"ok": True, "val": str(v)[:300]}
    except Exception as e:
        R[k] = {"ok": False,
                "err": type(e).__name__ + ": " + str(e)[:400],
                "tb": traceback.format_exc().splitlines()[-1][:220]}
    flush()


db = P.Db.curDb()
P2 = P.Ge.AcGePoint2d
P3 = P.Ge.AcGePoint3d


def line_count():
    return len(db.objectIds("AcDbLine"))


# 0. 建一条已提交的直线，拿到有效 ObjectId 供后续用例使用
VALID_ID = {"id": 0}


def setup_valid_line():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
    ids = db.objectIds("AcDbLine")
    VALID_ID["id"] = ids[-1]
    return {"ids": len(ids), "valid_id": VALID_ID["id"]}


rec("setup_valid_line", setup_valid_line)


# 1. with 内抛异常时事务是否被提交（当前实现 exit() 无条件 commit）
def trans_exception_rollback():
    before = line_count()
    try:
        with P.Db.PyDbTrans() as tr:
            ln = P.Db.PyDbLine(P3(0, -100, 0), P3(10, -100, 0))
            tr.addEntity(ln)
            raise RuntimeError("simulate script error inside transaction")
    except RuntimeError:
        pass
    after = line_count()
    return {"before": before, "after": after,
            "rolled_back": after == before}


rec("trans_exception_rollback", trans_exception_rollback)


# 2. db.openObject 空 id / 无效 id / 越界 mode
def db_openObject_null_id():
    o = db.openObject(0, 0)
    return {"isNull": o.isNull()}


def db_openObject_bad_id():
    o = db.openObject(999999999, 0)
    return {"isNull": o.isNull()}


def db_openObject_bad_mode():
    o = db.openObject(VALID_ID["id"], 9)
    return {"isNull": o.isNull()}


rec("db_openObject_null_id", db_openObject_null_id)
rec("db_openObject_bad_id", db_openObject_bad_id)
rec("db_openObject_bad_mode", db_openObject_bad_mode)


# 3. 事务内 openObject 空 id / 越界 mode
def trans_openObject_null_id():
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(0, 0)
        return {"isNull": o is None}


def trans_openObject_bad_mode():
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(VALID_ID["id"], 9)
        return {"isNull": o is None}


rec("trans_openObject_null_id", trans_openObject_null_id)
rec("trans_openObject_bad_mode", trans_openObject_bad_mode)


# 4. 多段线索引：负数 / 空表越界读写
def polyline_neg_index():
    pl = P.Db.PyDbPolyline()
    return pl.addVertexAt(-1, P2(0, 0), 0.0, -1.0, -1.0)


def polyline_read_oob():
    pl = P.Db.PyDbPolyline()
    out = {}
    out["getPointAt"] = pl.getPointAt(0)
    out["getBulgeAt"] = pl.getBulgeAt(0)
    out["getWidthsAt"] = pl.getWidthsAt(0)
    out["segType"] = pl.segType(0)
    out["getLineSegAt"] = pl.getLineSegAt(0)
    out["setPointAt"] = pl.setPointAt(0, P2(1, 1))
    out["removeVertexAt"] = pl.removeVertexAt(0)
    return out


rec("polyline_neg_index", polyline_neg_index)
rec("polyline_read_oob", polyline_read_oob)


# 5. Ed 层 None 传入强类型 AdsName / AcString
def entget_none():
    return P.Ed.entget(None)


def entdel_none():
    return P.Ed.entdel(None)


def entupd_none():
    return P.Ed.entupd(None)


def handent_none():
    return P.Ed.handent(None)


def entnext_none():
    return P.Ed.entnext(None)


rec("entget_none", entget_none)
rec("entdel_none", entdel_none)
rec("entupd_none", entupd_none)
rec("handent_none", handent_none)
rec("entnext_none", entnext_none)


# 6. 未知 DXF 类名静默返回空表
def objectIds_unknown_class():
    return db.objectIds("AcDbNoSuchClassXyz")


rec("objectIds_unknown_class", objectIds_unknown_class)


# 7. 文字构造零字高（SDK 不允许 h<=0）
def text_zero_height():
    tx = P.Db.PyDbText(P3(0, 0, 0), "probe", 0.0, 0.0)
    return {"height": tx.height(), "text": tx.textString()}


rec("text_zero_height", text_zero_height)

R["done"] = True
flush()
print("[PyArx] pitfall probe done -> " + OUT)
