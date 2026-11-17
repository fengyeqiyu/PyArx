"""探针：确认 PyArx 里“取当前数据库 / 遍历模型空间 / 建块表记录”的调用方式。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\ShxProbe\pyarx_howto.json"


def main():
    import PyArxCore as P
    rep = {}
    rep["PyApp"] = [m for m in dir(P.Ap.PyApp) if not m.startswith("_")]
    rep["Document"] = [m for m in dir(P.Ap.Document) if not m.startswith("_")]
    rep["DocumentManager"] = [m for m in dir(P.Ap.DocumentManager) if not m.startswith("_")]
    rep["PyAcadDatabase"] = [m for m in dir(P.Db.PyAcadDatabase) if not m.startswith("_")]
    # try obvious entry points
    tries = {}
    try:
        tries["PyApp()"] = repr(P.Ap.PyApp())
    except Exception as e:
        tries["PyApp()"] = "ERR " + repr(e)
    try:
        app = P.Ap.PyApp()
        tries["app.document()"] = repr(app.document())
    except Exception as e:
        tries["app.document()"] = "ERR " + repr(e)
    try:
        app = P.Ap.PyApp()
        d = app.document()
        tries["doc.database()"] = repr(d.database())
    except Exception as e:
        tries["doc.database()"] = "ERR " + repr(e)
    rep["tries"] = tries
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception as e:
        rep = {"fatal": repr(e)}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[API] howto dumped")
