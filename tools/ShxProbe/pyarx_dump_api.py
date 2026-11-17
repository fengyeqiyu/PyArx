"""导出 PyArxCore 全量 API 清单，并实测“取块表/模型空间/实体列表”的可行调用链。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

API_OUT = r"G:\PyArx\docs\pyarx_api_raw.json"
HOWTO_OUT = r"G:\PyArx\tools\ShxProbe\pyarx_chain.json"

NS = ["Ap", "Core", "Db", "Debug", "Ed", "Enum", "Ge", "Gi", "Rx", "Utility"]


def dump_api(P):
    out = {}
    for ns in NS:
        mod = getattr(P, ns, None)
        if mod is None:
            continue
        names = [n for n in dir(mod) if not n.startswith("_")]
        out[ns] = {n: [m for m in dir(getattr(mod, n)) if not m.startswith("_")]
                   for n in names}
    return out


def chain(P):
    Db = P.Db
    rep = {}
    db = Db.PyAcadDatabase.current()
    rep["db_type"] = type(db).__name__
    for expr, fn in [
        ("db.getBlockTable()", lambda: db.getBlockTable()),
        ("db.currentSpaceId()", lambda: db.currentSpaceId()),
        ("db.textStyleTableId()", lambda: db.textStyleTableId()),
        ("Utility", lambda: [m for m in dir(P.Utility) if not m.startswith("_")][:60]),
    ]:
        try:
            v = fn()
            rep[expr] = f"{type(v).__name__} = {str(v)[:120]}"
        except Exception as e:
            rep[expr] = "ERR " + repr(e)[:200]

    bt = None
    try:
        bt = db.getBlockTable()
    except Exception:
        pass
    if bt is not None:
        for expr, fn in [
            ("bt.getAt(space)", lambda: bt.getAt(db.currentSpaceId())),
            ("bt.getAt0(space)", lambda: bt.getAt(db.currentSpaceId(), 0)),
            ("bt.names", lambda: bt.names()),
        ]:
            try:
                v = fn()
                rep[expr] = f"{type(v).__name__} = {str(v)[:160]}"
            except Exception as e:
                rep[expr] = "ERR " + repr(e)[:300]
    return rep


if __name__ == "__main__":
    import PyArxCore as P
    os.makedirs(os.path.dirname(API_OUT), exist_ok=True)
    api = dump_api(P)
    with open(API_OUT, "w", encoding="utf-8") as f:
        json.dump(api, f, ensure_ascii=False)
    rep = chain(P)
    with open(HOWTO_OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[API] namespaces:", list(api))
