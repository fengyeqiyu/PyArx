"""PyArx 正确用法探针：curDb / 文档锁 / ADS 级存取。"""

import json
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\pyarx_probe.json"


def main():
    import PyArxCore as P
    Db, Ed = P.Db, P.Ed
    rep = {}

    def t(k, fn):
        try:
            rep[k] = str(fn())[:160]
        except Exception as e:
            rep[k] = "ERR " + repr(e)[:200]

    t("isAvailable", lambda: Db.PyAcadDatabase.isAvailable())
    db = Db.curDb()
    t("curDb repr", lambda: db)
    t("curDb.filename", lambda: db.getFilename())
    t("curDb.handleCount?", lambda: getattr(db, "entityCount", "n/a"))
    space = db.getBlockTable().getAt("*Model_Space")
    t("entityIds len", lambda: len(space.entityIds()))
    t("isDocumentLocked", lambda: Db.isDocumentLocked())
    t("lockDocument", lambda: Db.lockDocument())
    t("isDocumentLocked2", lambda: Db.isDocumentLocked())

    # ADS 级遍历
    t("Ed.entlast", lambda: Ed.entlast())
    t("Ed.handent B9", lambda: Ed.handent("B9"))
    t("Ed.ssget X", lambda: Ed.acdbEntGetX)  # 仅确认可访问

    # 建线测试（锁定后）
    def make():
        pl = Db.PyDbPolyline()
        pt = P.Ge.AcGePoint2d
        for i, (x, y) in enumerate([(0, 0), (100, 0), (100, 100), (0, 100)]):
            pl.addVertexAt(i, pt(float(x), float(y)))
        pl.setClosed(True)
        r = db.addToModelspace(pl)
        return "%r id=%r  after=%d" % (pl, r, len(space.entityIds()))

    t("addToModelspace(locked)", make)
    t("unlockDocument", lambda: Db.unlockDocument())
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[PROBE]", list(rep.items())[:3])
