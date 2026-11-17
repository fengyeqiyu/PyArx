# -*- coding: utf-8 -*-
"""PyArx 无头冒烟审计（分步打印，崩溃可定位到最后一行 [S] step）。"""
import os
import PyArxCore as C

Db, Ge, Ed, Ap = C.Db, C.Ge, C.Ed, C.Ap
db = Db.workingDb()
tmp = os.path.join(os.environ.get("TEMP", "C:/temp"), "pyarx_smoke")


def step(name):
    print("[S] step %s" % name)
    import sys
    sys.stdout.flush()


def t(name, fn):
    step(name)
    try:
        fn()
        print("[S] PASS %s" % name)
    except AttributeError as e:
        print("[S] 缺 %s <- %s" % (name, str(e)[:70]))
    except Exception as e:
        print("[S] FAIL %s <- %s: %s" % (name, type(e).__name__, str(e)[:70]))
    import sys
    sys.stdout.flush()


def mkline(y=0.0):
    li = Db.Line()
    li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0, y, 0))
    li.setEndPoint(Ge.Point3d(100, y, 0))
    return li


t("Db.workingDb/databaseTitle", lambda: db.databaseTitle())
t("Db.addToModelspace", lambda: db.addToModelspace(mkline(10)) and None)


def t_space():
    sp = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead)
    assert len(sp.entityIds()) == sp.numEntities()
t("BTR.numEntities/entityIds", t_space)


def t_open():
    sp = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead)
    i = sp.entityIds()[0]
    e = sp.getEntityById(i, 0)
    assert e.objectId() == i
t("BTR.getEntityById", t_open)


def t_edit():
    sp = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
    i = sp.entityIds()[-1]
    e = sp.getEntityById(i, 1)  # 改属性必须写打开
    e.setColorIndex(3)
    assert e.colorIndex() == 3
t("Entity.setColorIndex", t_edit)

t("Ge.Point3d.distanceTo", lambda: Ge.Point3d(1, 2, 3).distanceTo(Ge.Point3d(0, 0, 0)))
t("Ge.Vector3d.crossProduct", lambda: Ge.Vector3d(1, 0, 0).crossProduct(Ge.Vector3d(0, 1, 0)))
t("Ge.LineSeg3d.length", lambda: Ge.LineSeg3d(Ge.Point3d(0, 0, 0), Ge.Point3d(1, 1, 1)).length())
t("Ge.CircArc3d", lambda: Ge.CircArc3d(Ge.Point3d(0, 0, 0), Ge.Vector3d(0, 0, 1), 5.0))


def t_mx():
    m = Ge.Matrix3d.translation(Ge.Vector3d(1, 0, 0))
    p = Ge.Point3d(0, 0, 0).transformBy(m)
    assert p is not None
t("Ge.Matrix3d.translation", t_mx)

t("Ap.curDoc.fileName", lambda: isinstance(Ap.curDoc().fileName(), str))


def t_saveas():
    os.makedirs(tmp, exist_ok=True)
    p = os.path.join(tmp, "smoke_out.dwg").replace("\\", "/")
    r = db.saveAs(p)
    assert r == 0, "saveAs -> %r" % (r,)
    assert os.path.exists(p), "文件未生成"
t("Db.saveAs", t_saveas)


def t_dxfout():
    os.makedirs(tmp, exist_ok=True)
    p = os.path.join(tmp, "smoke_out.dxf").replace("\\", "/")
    r = db.dxfOut(p, 16)
    assert r and os.path.exists(p), "dxfOut -> %r 存在=%r" % (r, os.path.exists(p))
t("Db.dxfOut", t_dxfout)


def t_clone():
    sp = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead)
    src = sp.getEntityById(sp.entityIds()[0], 0)
    cp = src.clone()
    assert cp is not None
t("Entity.clone", t_clone)

step("done")
print("[S] all done")
