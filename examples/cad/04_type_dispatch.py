# -*- coding: utf-8 -*-
"""sample 04：判类型 -> cast -> 调类型特有方法（无事务，直接 database）

要点：
1. 类型判断用 db.classNameOf(oid)（C++ 内部 isKindOf），不要拿 cast 试错。
2. 包装类名 = PyDb + 类名去掉 AcDb 前缀：AcDbCircle -> P.Db.PyDbCircle。
3. cast 自带类型检查，类型不符给空包装，必须先判 isNull()。
4. 包装在函数内创建并 close，不要挂模块全局（否则锁库）。
"""
import json

import PyArxCore as P

db = P.Db.curDb()
OUT = r"G:\PyArx\log\sample04_steps.json"
ROWS = []


def out(msg, **kv):
    ROWS.append(dict(step=msg, **kv))
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(ROWS, f, ensure_ascii=False, indent=1, default=str)


def find_class(cls):
    """AcDbCircle -> (名字, PyDbCircle)；找不到返回 (None, None)。"""
    short = cls[4:] if cls.startswith("AcDb") else cls
    for name in ("PyDb" + short, "PyDb" + short + "Ex"):
        c = getattr(P.Db, name, None)
        if c is not None:
            return name, c
    return None, None


def open_typed(oid):
    """按 id 打开并按实际类型 cast；返回 (类名, 包装) 或 (类名, None)。"""
    cls = db.classNameOf(oid)
    cname, pycls = find_class(cls)
    if pycls is None:
        return cls, None
    obj = db.openObject(oid, 0)
    if obj.isNull():
        return cls, None
    typed = pycls.cast(obj)
    obj.close()  # cast 成功后包装独立，原包装可关
    if typed.isNull():
        return cls, None
    return cls, typed


def main():
    # 读一个具体值：每个类型一个"特有 getter"
    GETTER = {
        "AcDbCircle": lambda o: o.radius(),
        "AcDbLine": lambda o: o.endPoint(),
        "AcDbArc": lambda o: o.radius(),
        "AcDbText": lambda o: o.textString(),
        "AcDbPolyline": lambda o: o.numVerts(),
        "AcDbPoint": lambda o: o.position(),
        "AcDbBlockReference": lambda o: o.position(),
    }
    ids = db.objectIds("AcDbEntity")
    print("[s04] 图元总数=%d" % len(ids))
    out("total", n=len(ids))
    for oid in ids:
        cls, typed = open_typed(oid)
        if typed is None:
            print("[s04] %-22s 无法转换" % cls)
            out("bad", id=int(oid), type=cls)
            continue
        fn = GETTER.get(cls)
        val = "(无 getter)" if fn is None else str(fn(typed))
        print("[s04] %-22s %s" % (cls, val))
        out("typed", id=int(oid), type=cls, value=val)
        typed.close()
    out("done", ok=True)


main()
