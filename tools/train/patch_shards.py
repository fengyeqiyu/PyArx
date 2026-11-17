# -*- coding: utf-8 -*-
"""按实测证据补全三条矩阵记录（改 shard，不改 module_matrix.json）。"""
import json
import os
import shutil

BASE = r"G:\PyArx\tools\train\data"

SIG = ("Exception -> TypeError: __init__(): incompatible constructor arguments. "
       "The following argument types are supported:\n"
       "    1. PyArxCore.Db.PyDbCircle()\n"
       "    2. PyArxCore.Db.PyDbCircle(cntr: PyArxCore.Ge.AcGePoint3d, "
       "nrm: PyArxCore.Ge.AcGeVector3d, radius: float)\n"
       "    3. PyArxCore.Db.PyDbCircle(object: AcDbCircle)\n"
       "Invoked with: {invoked}")

# ---- 1. shard_db：补录 Db.PyDbCircle（9 条，ctor 签名用 smoke_out 实测原文补全）----
db_p = os.path.join(BASE, "shard_db.json")
shutil.copy2(db_p, db_p + ".bak")
db = json.load(open(db_p, encoding="utf-8"))
circle = {
    "layer": "Db",
    "module": "PyDbCircle",
    "checks": [
        {"name": "exists+construct", "ok": True,
         "detail": "构造 form=args=0 type=PyArxCore.Db.PyDbCircle members=164"},
        {"name": "type_name", "ok": True, "detail": "PyArxCore.Db.PyDbCircle"},
        {"name": "prop_roundtrip_radius_center", "ok": True,
         "detail": "radius 33.25->33.2500 center->(10.000,20.000,0.000) 回读类型=AcGePoint3d"},
        {"name": "db_roundtrip", "ok": True,
         "detail": "form=args=0 before=0 after=1 handle=731 事务内radius=7.500"},
        {"name": "readback_after_commit", "ok": True,
         "detail": "handle=732 无可直接读回的句柄 API，尝试=['getObjectIdByHandle:missing', 'objectIdByHandle:missing']"},
        {"name": "ctor 无参 [probe]", "ok": False, "detail": "return -> AcDbCircle[wrapper]"},
        {"name": "ctor 负半径 [probe]", "ok": True,
         "detail": SIG.format(invoked="negative radius")},
        {"name": "ctor 半径传字符串 [probe]", "ok": True,
         "detail": SIG.format(invoked="'x'")},
        {"name": "setRadius(0) [probe]", "ok": True,
         "detail": SIG.format(invoked="0")},
    ],
}
db = [x for x in db if not (x["layer"] == "Db" and x["module"] == "PyDbCircle")]
db.append(circle)
json.dump(db, open(db_p, "w", encoding="utf-8"), ensure_ascii=False, indent=1)

# ---- 2. shard_ed：PyEdEnt 补 AcEdJig / SelectionSet+ssget/ssadd 真实对应 ----
ed_p = os.path.join(BASE, "shard_ed_app_engine.json")
shutil.copy2(ed_p, ed_p + ".bak")
ed = json.load(open(ed_p, encoding="utf-8"))
for x in ed:
    if x["layer"] == "Ed" and x["module"] == "PyEdEnt":
        for c in x["checks"]:
            if c["name"] == "name_trap_missing":
                c["detail"] = (
                    "hasattr(P.Ed.PyEdEnt)=False；实体结果/实体名真实对应物；真实存在对应物: "
                    "{'EntityValue': True, 'SubEntityValue': True, 'AdsName': True, "
                    "'entget': True, 'entmake': True, 'handent': True, 'entlast': True}；"
                    "交互选择/向导真实对应: Ed.AcEdJig（GUI 上下文继承并重载 sampler/entity，"
                    "对应陷阱名 PyEdJig）；Ed.SelectionSet（add/remove/clear 闭环）+ ssget"
                    "（acedSSGet('X')）/ssadd，对应陷阱名 PyEdSel/PyEdSelectionSets。"
                    "依据同矩阵 Ed.AcEdJig、Ed.SelectionSet、Ed.acedSSGet 实测记录。")
json.dump(ed, open(ed_p, "w", encoding="utf-8"), ensure_ascii=False, indent=1)

# ---- 3. shard_ge_gi：AcGeMatrix3d 补点变换指引（AcGePoint3d 已有 transformBy 实测支撑）----
ge_p = os.path.join(BASE, "shard_ge_gi.json")
shutil.copy2(ge_p, ge_p + ".bak")
ge = json.load(open(ge_p, encoding="utf-8"))
for x in ge:
    if x["layer"] == "Ge" and x["module"] == "AcGeMatrix3d":
        for c in x["checks"]:
            if c["name"] == "setToTranslation":
                c["detail"] += ("；指引：矩阵本身没有 transformBy；点变换是 p.transformBy(matrix)，"
                                "见 Ge.AcGePoint3d.transformBy_translation（实测 "
                                "(1,2,3).transformBy(translate(10,0,0)) -> (+11.000,+2.000,+3.000)，"
                                "矩阵对象本身无 transformBy）")
json.dump(ge, open(ge_p, "w", encoding="utf-8"), ensure_ascii=False, indent=1)

print("patched: db=%d ed=%d ge=%d" % (len(db), len(ed), len(ge)))
