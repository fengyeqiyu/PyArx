# -*- coding: utf-8 -*-
"""sample 02：图层 + 块定义 + 块参照（每步落 JSON，崩溃也能定位到步）

要点：
- 符号表记录必须放在 PyDbTrans 里 add，裸 add 不落库；
- 事务内自己开表要用 try/finally + abort，表只开一次、用完 close；
- 块定义 id 必须在 commit 之后取，取不到就别拿它插块参照（否则 ARX 报 eWrongDatabase 致命错）。
本脚本不保存图形。
"""
import json

import PyArxCore as P

LAYER = "PY_SAMPLE_LAYER"
BLOCK = "PY_SAMPLE_BLOCK"
RES = r"G:\PyArx\log\sample02_steps.json"
STEPS = []


def pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def vec(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


def step(tag, value):
    STEPS.append({"step": tag, "value": value})
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(STEPS, f, ensure_ascii=False, indent=1, default=str)
    print("[sample02] %s -> %s" % (tag, value))


def add_layer(db):
    # 不用事务：直接 kForWrite 打开图层表 -> add -> 记录 close -> 表 close。
    # 记录 add 后必须 close，否则按名读不到、names() 还会静默返回空。
    lt = db.getLayerTable(1)
    try:
        if lt.has(LAYER):
            return "exists"
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(LAYER)
        rec.setColor(200, 80, 40)
        ok = lt.add(rec)
        rec.close()
        return "created" if ok else "add-failed"
    finally:
        lt.close()


def add_block(db):
    bt = db.getBlockTable(1)
    try:
        if bt.has(BLOCK):
            return "exists"
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName(BLOCK)
        ok = bt.add(btr)
        btr.close()
        return "created" if ok else "add-failed"
    finally:
        bt.close()


db = P.Db.curDb()


step("layer.add", add_layer(db))
step("block.add", add_block(db))
step_add_children = True

bt = db.getBlockTable(0)
blk_id = bt.getIdAt(BLOCK)
rec = bt.getAt(BLOCK, 0)
step("block.id", blk_id)
step("block.isNull", str(rec.isNull()))
if not rec.isNull():
    step("block.ents.before", len(rec.entityIds()))
    rec.close()
bt.close()

# 子实体追加：必须用 db.addToBlock（内部走 append + 注册 + 关闭三步）。
# 直接在事务里对块记录 appendAcDbEntity 会撞 ARX 致命断言（eWrongDatabase / eNotOpenForWrite）。
if blk_id and step_add_children:
    step("child.circle", db.addToBlock(blk_id, P.Db.PyDbCircle(pt(0, 0), vec(0, 0, 1), 10)))
    step("child.line", db.addToBlock(blk_id, P.Db.PyDbLine(pt(-10, 0), pt(10, 0))))
    bt = db.getBlockTable(0)
    rec = bt.getAt(BLOCK, 0)
    if not rec.isNull():
        step("block.ents.after", len(rec.entityIds()))
        rec.close()
    bt.close()

if blk_id:
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(blk_id)
    br.setPosition(pt(500, 0))
    br.setScaleFactors(P.Ge.AcGeScale3d(2.0, 2.0, 2.0))
    step("blockref.id", db.addToCurrentspace(br))
else:
    step("blockref.id", "skip: block id = 0")

lt = db.getLayerTable(0)
lr = lt.getAt(LAYER)
step("layer.isNull", str(lr.isNull()))
if not lr.isNull():
    step("layer.name", str(lr.getName()))
    lr.close()
lt.close()

step("entities", len(db.objectIds("AcDbEntity")))
step("done", True)
