"""探针 2：定位真正被绘制/可写的数据库，并用 AcGePoint2d 建一条闭合多段线。"""

import json
import os
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\pyarx_make2.json"


def t(name, fn):
    try:
        return name + " -> OK " + str(fn())[:100]
    except Exception as e:
        return name + " -> ERR " + str(e).replace("\n", " | ")[:240]


def main():
    import PyArxCore as P
    Db = P.Db
    rep = {}

    rep["DocumentManager()"] = t("DocumentManager()", lambda: P.Ap.DocumentManager())
    dm = None
    try:
        dm = P.Ap.DocumentManager()
    except Exception:
        pass
    if dm is not None:
        rep["dm.mdiActiveDocument()"] = t("mdiActiveDocument()", lambda: dm.mdiActiveDocument())
        rep["dm.curDocument()"] = t("curDocument()", lambda: dm.curDocument())

    db_a = Db.PyAcadDatabase.current()
    rep["A.current entityIds"] = len(db_a.getBlockTable().getAt("*Model_Space").entityIds())

    db_b = None
    try:
        db_b = dm.mdiActiveDocument().database()
        rep["B.mdi db entityIds"] = len(db_b.getBlockTable().getAt("*Model_Space").entityIds())
    except Exception as e:
        rep["B"] = "ERR " + repr(e)[:200]

    target = db_b if db_b is not None else db_a
    space = target.getBlockTable().getAt("*Model_Space")

    pl = Db.PyDbPolyline()
    pt = P.Ge.AcGePoint2d
    for i, (x, y) in enumerate([(0.0, 0.0), (500.0, 0.0), (500.0, 500.0), (0.0, 500.0)]):
        t(f"addVertexAt({i})", lambda i=i, x=x, y=y: pl.addVertexAt(i, pt(x, y)))
    rep["closed"] = t("setClosed", lambda: pl.setClosed(True))
    rep["add"] = t("addToModelspace", lambda: target.addToModelspace(pl))
    rep["after"] = len(space.entityIds())
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[MAKE2]", {k: v for k, v in rep.items() if "after" in k or "entityIds" in k})
