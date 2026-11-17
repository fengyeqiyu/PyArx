"""PyArx 正确建实体路径：getBlockTable().getAt("*Model_Space", 1) + appendAcDbEntity。"""

import json
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\pyarx_probe2.json"


def main():
    import PyArxCore as P
    Db = P.Db
    rep = {}

    def t(k, fn):
        try:
            rep[k] = str(fn())[:200]
        except Exception as e:
            rep[k] = "ERR " + repr(e)[:240]

    db = Db.curDb()
    rep["filename"] = db.getFilename()
    rep["objects before"] = t("objectIds", lambda: len(db.objectIds("AcDbEntity")))
    rep["locked"] = Db.isDocumentLocked()

    def make():
        bt = db.getBlockTable()
        space = bt.getAt("*Model_Space", 1)          # mode 1 = kForWrite
        pl = Db.PyDbPolyline()
        pt = P.Ge.AcGePoint2d
        for i, (x, y) in enumerate([(0.0, 0.0), (300.0, 0.0), (300.0, 300.0), (0.0, 300.0)]):
            pl.addVertexAt(i, pt(x, y))
        pl.setClosed(True)
        ok = space.appendEntity(pl)
        n = len(space.entityIds())
        space.close()
        return "appendEntity=%r idsInSpace=%d" % (ok, n)

    rep["append(space opened write)"] = t("append", make)
    rep["objects after"] = t("objectIds2", lambda: len(db.objectIds("AcDbEntity")))
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[PROBE2]", list(rep.items()))
