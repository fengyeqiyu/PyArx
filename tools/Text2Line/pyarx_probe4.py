"""PyArx 正解：PyDbTrans（锁文档+事务）内建实体 / openObject 读实体 / 选择集取 id。"""

import json
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\pyarx_probe4.json"


def main():
    import PyArxCore as P
    Db, Ed = P.Db, P.Ed
    rep = {}

    def t(k, fn):
        try:
            rep[k] = str(fn())[:300]
        except Exception as e:
            rep[k] = "ERR " + repr(e)[:260]

    db = Db.curDb()
    rep["filename"] = db.getFilename()
    rep["entities before"] = t("objectIds", lambda: len(db.objectIds("AcDbEntity")))

    def build():
        with Db.PyDbTrans() as tr:
            pl = Db.PyDbPolyline()
            for i, (x, y) in enumerate([(0.0, 0.0), (500.0, 0.0), (500.0, 500.0), (0.0, 500.0)]):
                pl.addVertexAt(i, P.Ge.AcGePoint2d(x, y))
            pl.setClosed(True)
            tr.addEntity(pl)
            return "added"

    rep["trans.addEntity"] = t("build", build)
    rep["entities after"] = t("objectIds", lambda: len(db.objectIds("AcDbEntity")))

    # 选择集 + 事务读取（用户提示的路线）
    def read():
        ids = db.objectIds("AcDbPolyline")
        out = "polylines=%d " % len(ids)
        if ids:
            with Db.PyDbTrans() as tr:
                obj = tr.openObject(ids[-1], 0)
                out += "last=%r " % (obj,)
                out += "verts=%d " % (obj.numVerts() if hasattr(obj, "numVerts") else -1)
        return out

    rep["openObject by id"] = t("read", read)
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[PROBE4]", list(rep.items()))
