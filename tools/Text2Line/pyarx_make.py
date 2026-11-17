"""探针：PyDbPolyline() + addVertexAt + addToModelspace 的可行写法，并核对实体数变化。"""

import json
import os
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\pyarx_make.json"


def try_call(name, fn):
    try:
        return name + " -> OK " + str(fn())[:100]
    except Exception as e:
        return name + " -> ERR " + str(e).replace("\n", " | ")[:260]


def main():
    import PyArxCore as P
    Db = P.Db
    rep = {}
    db = Db.PyAcadDatabase.current()
    space = db.getBlockTable().getAt("*Model_Space")
    rep["before"] = len(space.entityIds())

    pl = Db.PyDbPolyline()
    rep["pt2d_ctor"] = try_call("PyGePoint2d(0,0)", lambda: P.Ge.PyGePoint2d(0.0, 0.0))
    rep["addVertexAt(0,pt)"] = try_call("addVertexAt(0, pt2d)", lambda: pl.addVertexAt(0, P.Ge.PyGePoint2d(0.0, 0.0)))
    rep["addVertexAt(1,xy)"] = try_call("addVertexAt(1, 100, 0)", lambda: pl.addVertexAt(1, 100.0, 0.0))
    rep["addVertexAt(2,xy)"] = try_call("addVertexAt(2, 100, 100)", lambda: pl.addVertexAt(2, 100.0, 100.0))
    rep["addVertexAt(3,xy)"] = try_call("addVertexAt(3, 0, 100)", lambda: pl.addVertexAt(3, 0.0, 100.0))
    rep["setClosed"] = try_call("setClosed(True)", lambda: pl.setClosed(True))
    rep["add"] = try_call("db.addToModelspace(pl)", lambda: db.addToModelspace(pl))
    rep["after"] = len(space.entityIds())
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[MAKE]", rep)
