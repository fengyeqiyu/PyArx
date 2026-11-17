"""探针：列出 PyDb 关键类的成员，供后续写转换脚本用。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\ShxProbe\pyarx_members.json"
WANT = ["PyDbText", "PyDbMText", "PyDbPolyline", "PyDbBlockTableRecord",
        "PyDbBlockTable", "PyDbTextStyleTableRecord", "PyDbObjectId",
        "PyDbEntity", "PyDbDatabase", "PyDb2dPolyline", "PyDbLine"]


def main():
    import PyArxCore as P
    Db = P.Db
    out = {}
    for name in WANT:
        cls = getattr(Db, name, None)
        if cls is None:
            out[name] = "MISSING"
            continue
        out[name] = [m for m in dir(cls) if not m.startswith("_")]
    out["_Ap"] = [m for m in dir(P.Ap) if not m.startswith("_")]
    out["_Ed"] = [m for m in dir(P.Ed) if not m.startswith("_")]
    return out


if __name__ == "__main__":
    try:
        rep = main()
    except Exception as e:
        rep = {"fatal": repr(e)}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[API] dumped")
