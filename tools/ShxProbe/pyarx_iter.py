"""探针：模型空间实体怎么拿——entityIds 返回什么、能否迭代。"""

import json
import os
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\ShxProbe\pyarx_iter.json"


def main():
    import PyArxCore as P
    Db = P.Db
    rep = {}
    db = Db.PyAcadDatabase.current()
    bt = db.getBlockTable()
    space = bt.getAt("*Model_Space")
    rep["space_type"] = type(space).__name__
    rep["has_iter"] = hasattr(space, "__iter__")

    try:
        ids = space.entityIds()
        rep["entityIds"] = f"{type(ids).__name__} len={len(ids)} first_type={type(ids[0]).__name__ if ids else 'empty'}"
        rep["first_val"] = str(ids[0])[:80] if ids else ""
    except Exception:
        rep["entityIds"] = "ERR " + traceback.format_exc()[-300:]

    try:
        dbids = db.objectIds()
        rep["db.objectIds"] = f"{type(dbids).__name__} len={len(dbids)}"
    except Exception:
        rep["db.objectIds"] = "ERR " + traceback.format_exc()[-200:]

    # how to turn an id into an entity?
    for cand in ["PyDbEntity", "PyDbText"]:
        cls = getattr(Db, cand, None)
        rep[cand + ".members_open"] = [m for m in dir(cls) if "cast" in m or "open" in m or "create" in m]

    try:
        cls = Db.PyDbObject
        rep["PyDbObject.cast_sig"] = str(cls.cast.__doc__)
    except Exception as e:
        rep["PyDbObject.cast_sig"] = repr(e)

    # try iteration
    try:
        n = 0
        first = None
        for e in space:
            n += 1
            if first is None:
                first = type(e).__name__
            if n > 3:
                break
        rep["iterate"] = f"ok n={n} first={first}"
    except Exception:
        rep["iterate"] = "ERR " + traceback.format_exc()[-300:]
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[ITER] dumped")
