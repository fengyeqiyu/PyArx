"""探针：在 CAD 进程内列出 PyArxCore 的可用接口。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\ShxProbe\pyarx_api.json"


def main():
    rep = {}
    import PyArxCore as P
    names = [n for n in dir(P) if not n.startswith("_")]
    rep["top"] = names
    for n in names:
        try:
            obj = getattr(P, n)
            sub = [x for x in dir(obj) if not x.startswith("_")]
            if len(sub) > 3:
                rep[n] = sub[:200]
        except Exception as e:
            rep[n] = "ERR " + repr(e)
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception as e:
        rep = {"fatal": repr(e)}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[API]", list(rep)[:20])
