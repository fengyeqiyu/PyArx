# -*- coding: utf-8 -*-
"""sample 01：绘图（无事务，直接 database）

用法：
    PYRUNFILE G:/PyArx/examples/cad/01_draw_basic.py
前置：新开的 CAD 进程里先跑一次 G:/PyArx/tools/hotbuild/reload.py
本脚本不保存图形。
"""
import PyArxCore as P


def pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def vec(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


def pt2(x, y):
    return P.Ge.AcGePoint2d(float(x), float(y))


db = P.Db.curDb()
done = []

# 每个图元：构造 -> db.addToCurrentspace(ent) 入库，返回 ObjectId
# addToCurrentspace 内部已做 append + addNewlyCreatedDBRObject + detach，
# 包装析构不会连带删库；旧构建缺这两步会留下打不开的僵尸 id。
done.append(db.addToCurrentspace(P.Db.PyDbLine(pt(0, 0), pt(100, 0))))
done.append(db.addToCurrentspace(P.Db.PyDbCircle(pt(50, 50), vec(0, 0, 1), 20)))
done.append(db.addToCurrentspace(P.Db.PyDbArc(pt(0, 60), 30, 0.0, 3.14159)))
done.append(db.addToCurrentspace(P.Db.PyDbText(pt(0, -20), "PyArx", 4.0, 0.0)))
done.append(db.addToCurrentspace(P.Db.PyDbEllipse(pt(120, 50), vec(0, 0, 1), vec(25, 0, 0), 0.4)))
done.append(db.addToCurrentspace(P.Db.PyDbPoint(pt(150, 0))))
done.append(db.addToCurrentspace(P.Db.PyDbSolid(pt(0, 100), pt(20, 100), pt(20, 120), pt(0, 120))))

# 多段线：先加顶点，再入库
pl = P.Db.PyDbPolyline()
pl.addVertexAt(0, pt2(200, 0), 0.0, -1.0, -1.0)
pl.addVertexAt(1, pt2(240, 40), 0.5, -1.0, -1.0)
pl.addVertexAt(2, pt2(280, 0), 0.0, -1.0, -1.0)
done.append(db.addToCurrentspace(pl))

print("[sample01] 写入 %d 个图元，模型空间总数=%d" % (len(done), len(db.objectIds("AcDbEntity"))))
print("[sample01] ids=%s" % done)

# 注意：入库成功后不要再用原包装 ent 操作（内部已 detach，指针为空），
# 需要再操作就按 id 重新打开，见 03_read_modify_delete.py。
