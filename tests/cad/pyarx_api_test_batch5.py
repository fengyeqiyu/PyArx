# -*- coding: utf-8 -*-
"""batch5：补测三项 —— 块参照链路 / entget+entmod 回写 / entdel 异常路径。

每步独立落盘 JSON，卡住时最后一个 key 即卡点。
块定义遵循 cad_db_basics.py 的已验证链路：外层同一 PyDbTrans 内先
appendAcDbEntity（裸追加）再 bt.add(btr)，不在记录内自起嵌套事务。
"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch5.json"
R = {"steps": []}
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(R, f, ensure_ascii=False, indent=1)

db = P.Db.curDb()
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d


def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def step(k, fn):
    rec = {"step": k, "created": False, "readback": None,
           "blocked": False, "ok": False, "err": None}
    try:
        out = fn()
        if isinstance(out, dict):
            rec.update(out)
        else:
            rec["readback"] = out
        rec["ok"] = rec["created"] and rec["readback"] is not None
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:220]
    R["steps"].append(rec)
    flush()


BTR_ID = {"id": 0, "name": ""}


def br_setup():
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable(1)  # 写表必须 kForWrite(1)；默认 0 为读，add 时 eNotOpenForWrite
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T5")
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 4.0)
        btr.appendAcDbEntity(c)
        bt.add(btr)
        BTR_ID["id"] = btr.objectId()
        BTR_ID["name"] = str(btr.getName())
    return {"created": True,
            "readback": {"btr_name": BTR_ID["name"], "btr_id": BTR_ID["id"]}}


def br_insert():
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(BTR_ID["id"])
    br.setPosition(P3(30, 0, 0))
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(br)
    pos = br.position()
    return {"created": True, "handle": str(br.handle()),
            "readback": {"pos": (pos.x, pos.y),
                          "btr_id": br.blockTableRecordId(),
                          "match": br.blockTableRecordId() == BTR_ID["id"]}}


ED = {"ename": None, "handle": None}


def ed_create():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        ED["handle"] = str(ln.handle())
    return {"created": True, "handle": ED["handle"]}


def ed_handent():
    ED["ename"] = P.Ed.handent(ED["handle"])
    return {"created": True, "readback": {"is_none": ED["ename"] is None}}


def ed_entget_before():
    dxf = P.Ed.entget(ED["ename"])
    return {"created": True,
            "readback": {"keys": sorted(dxf.keys()) if dxf else None,
                         "color": dxf.get(62) if dxf else None}}


def ed_entmod():
    dxf = P.Ed.entget(ED["ename"])
    full = dict(dxf)
    full[62] = 1
    ok = P.Ed.entmod(full)
    return {"created": True, "readback": {"entmod_ok": ok}}


def ed_entget_after():
    dxf = P.Ed.entget(ED["ename"])
    return {"created": True,
            "readback": {"color": dxf.get(62) if dxf else None,
                         "layer": dxf.get(8) if dxf else None}}


def ed_entdel():
    ok = P.Ed.entdel(ED["ename"])
    return {"created": True, "readback": {"entdel_ok": ok}}


def ed_entget_del():
    dxf = P.Ed.entget(ED["ename"])
    return {"created": False, "blocked": True,
            "readback": {"entget_after_erase": None if dxf is None else "non-null"}}


def ed_entmod_del():
    dxf = P.Ed.entget(ED["ename"])
    full = dict(dxf) if dxf else {0: "LINE", 62: 1}
    ok = P.Ed.entmod(full)
    return {"created": False, "blocked": True,
            "readback": {"entmod_on_erased": ok}}


def ed_restore():
    ok = P.Ed.entdel(ED["ename"])
    return {"created": True, "readback": {"restore_ok": ok}}


step("br_setup", br_setup)
step("br_insert", br_insert)

step("ed_create", ed_create)
step("ed_handent", ed_handent)
step("ed_entget_before", ed_entget_before)
step("ed_entmod", ed_entmod)
step("ed_entget_after", ed_entget_after)

step("ed_entdel", ed_entdel)
step("ed_entget_del", ed_entget_del)
step("ed_entmod_del", ed_entmod_del)
step("ed_restore", ed_restore)

R["done"] = True
flush()
print("[PyArx] batch5 done -> " + OUT)
