# -*- coding: utf-8 -*-
"""生成一张带属性块（ATTDEF + ATTRIB）的测试图，用于验证 scan.py 的属性解析链路。"""
import json
import traceback

import PyArxCore as P

OUT_DWG = r"C:\temp\attr_test.dwg"
BLOCK = "PY_ATTR_BOX"
res = {}


def pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def vec(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


try:
    db = P.Db.curDb()
    bt = db.getBlockTable(1)
    try:
        if not bt.has(BLOCK):
            btr = P.Db.PyDbBlockTableRecord()
            btr.setName(BLOCK)
            res["addBlock"] = bool(bt.add(btr))
            btr.close()
        else:
            res["addBlock"] = "exists"
    finally:
        bt.close()

    bt = db.getBlockTable(0)
    blk_id = bt.getIdAt(BLOCK)
    bt.close()
    res["blkId"] = str(blk_id)

    # 块内几何：一个矩形，属性定义放在中心
    for ln in ((0, 0, 60, 0), (60, 0, 60, 40), (60, 40, 0, 40), (0, 40, 0, 0)):
        db.addToBlock(blk_id, P.Db.PyDbLine(pt(ln[0], ln[1]), pt(ln[2], ln[3])))
    attdef = P.Db.PyDbAttributeDefinition(pt(6, 16), "", "TAG1", "编号")
    attdef.setHeight(8.0)
    res["attdef"] = bool(db.addToBlock(blk_id, attdef))

    # 块参照 + 属性值
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(blk_id)
    br.setPosition(pt(1000, 1000))
    att = P.Db.PyDbAttribute(pt(1006, 1016), "ATTR-VAL-OK", "TAG1")
    att.setHeight(8.0)
    res["appendAttr"] = bool(br.appendAttribute(att))
    res["brId"] = str(db.addToCurrentspace(br))

    res["save"] = bool(db.saveAs(OUT_DWG))
except Exception:
    res["err"] = traceback.format_exc()[-500:]

with open(r"C:\temp\_gen_attr.json", "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print("gen attr dwg done")
