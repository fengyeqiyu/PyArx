# -*- coding: utf-8 -*-
"""sample 03：读、改、删

要点：
- 类型判定不能用 PyDbXxx.cast 试投：cast 不做类型检查，任意实体都会包装成你指定的类，
  调用错方法立即 unknown exception。先用 db.objectIds("AcDbCircle") 按类过滤。
- erase 之后不能用 getEntityById 重新打开（返回空包装），恢复要在同一个包装上 erase(False)。
- 自己持有写打开的空间/表时，db.objectIds() 会静默返回 0 条，先 close 再统计。
本脚本不保存图形。
"""
import PyArxCore as P

db = P.Db.curDb()

# --- 读：按类过滤 + openObject + cast ---
print("[sample03] 各类数量：")
for cls in ("AcDbLine", "AcDbCircle", "AcDbArc", "AcDbText",
            "AcDbEllipse", "AcDbPolyline", "AcDbPoint", "AcDbSolid",
            "AcDbBlockReference"):
    print("   %-20s %d" % (cls, len(db.objectIds(cls))))

for oid in db.objectIds("AcDbCircle"):
    obj = db.openObject(oid, 0)          # 0=读，1=写
    if obj.isNull():
        continue
    c = P.Db.PyDbCircle.cast(obj)        # 已知是圆，cast 才安全
    print("[sample03] circle id=%s r=%s center=(%s,%s) layer=%s"
          % (oid, c.radius(), c.center().x, c.center().y, c.layer()))
    obj.close()                          # cast 出的包装与 obj 指向同一对象，只 close 一次

# --- 改：写模式打开后 setXxx ---
ids = db.objectIds("AcDbCircle")
if ids:
    obj = db.openObject(ids[0], 1)
    c = P.Db.PyDbCircle.cast(obj)
    c.setColorIndex(3)
    c.setRadius(35.0)
    print("[sample03] 改后 r=%s color=%s" % (c.radius(), c.colorIndex()))
    obj.close()

# --- 删：同一包装内 erase，再恢复 ---
ids = db.objectIds("AcDbPoint")
if ids:
    obj = db.openObject(ids[0], 1)
    print("[sample03] erase ->", obj.erase(), "isErased:", obj.isErased())
    print("[sample03] 恢复 ->", obj.erase(False), "isErased:", obj.isErased())
    obj.close()

# 注意：统计数量前确保没有自己留下的写打开包装，否则 objectIds 静默返回 0。
print("[sample03] 结束时实体数=%d" % len(db.objectIds("AcDbEntity")))
