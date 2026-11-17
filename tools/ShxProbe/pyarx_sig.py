"""探针：用错误调用逼出 pybind 的函数签名，一次拿到全部需要的接口。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\ShxProbe\pyarx_sig.json"


def sig(fn, *a):
    try:
        fn(*a)
        return "OK-call"
    except TypeError as e:
        return "SIG: " + str(e).replace("\n", " | ")
    except Exception as e:
        return "ERR " + repr(e)


def main():
    import PyArxCore as P
    Db = P.Db
    db = Db.PyAcadDatabase.current()
    dbc = type(db)
    out = {}
    out["db_type"] = str(dbc)
    out["db.dir_hint"] = [m for m in dir(db) if "Block" in m or "Space" in m or "Style" in m][:40]
    out["db.getBlockTable"] = sig(db.getBlockTable)
    out["db.currentSpaceId"] = sig(db.currentSpaceId)
    out["db.addToModelspace"] = sig(db.addToModelspace)
    out["db.getTextStyleTable"] = sig(db.getTextStyleTable)

    class Fake:
        pass

    bt = None
    try:
        bt = db.getBlockTable(Db.PyDbBlockTable(), Db.OpenMode.kForRead)
    except Exception as e:
        out["getBlockTable_try"] = repr(e)
    out["bt"] = repr(bt)

    # enum probes
    out["OpenMode"] = [m for m in dir(Db.OpenMode) if not m.startswith("_")] if hasattr(Db, "OpenMode") else "MISSING"
    return out


if __name__ == "__main__":
    try:
        rep = main()
    except Exception as e:
        rep = {"fatal": repr(e)}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[SIG] dumped")
