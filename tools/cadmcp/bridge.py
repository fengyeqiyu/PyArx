"""bridge.py —— 在 acad.exe 内执行 PyArx 代码的常驻桥接工人。

由 (command "PYRUNFILE" "G:/PyArx/tools/cadmcp/bridge.py") 触发。
读 req.json，执行 code，写 res.json。
"""

import io
import json
import os
import sys
import traceback

BASE = r"G:\PyArx\tools\cadmcp"
REQ = os.path.join(BASE, "req.json")
RES = os.path.join(BASE, "res.json")

PRELUDE = {
    "PyArxCore": "G:\\PyArx\\X64\\Release",
}

SUBMODULES = ("Ge", "Enum", "Rx", "Db", "Ed", "Gi", "Core", "Utility", "Debug", "Ap")


def load_pyarx():
    """import PyArxCore，并对热加载残留做兼容。

    现象：hotbuild/pyreload 之后 sys.modules['PyArxCore'] 可能指向 PyArxCoreHtN 顶模块，
    而该顶模块只挂了少数子模块（实测只剩 Ge），P.Db 之类直接 AttributeError。
    真子模块仍在 sys.modules['PyArxCore.Db'] 等位置，所以这里用门面对象按需回填，
    不改动 CAD 进程里的全局模块对象。
    """
    import PyArxCore as m

    class Facade:
        pass

    f = Facade()
    missing = []
    for s in SUBMODULES:
        sub = getattr(m, s, None)
        if sub is None:
            sub = sys.modules.get("PyArxCore." + s)
        if sub is None:
            missing.append(s)
        else:
            setattr(f, s, sub)
    for a in dir(m):
        if a.startswith("_") or hasattr(f, a):
            continue
        try:
            setattr(f, a, getattr(m, a))
        except Exception:
            pass
    if missing:
        print("[cadmcp] PyArxCore 缺少子模块: " + ", ".join(missing))
    return f


def jsonable(v, depth=0):
    if depth > 3:
        return repr(v)[:200]
    if isinstance(v, (str, int, float, bool)) or v is None:
        return v
    if isinstance(v, dict):
        return {str(k): jsonable(x, depth + 1) for k, x in v.items()}
    if isinstance(v, (list, tuple)):
        return [jsonable(x, depth + 1) for x in v]
    return repr(v)[:300]


def main():
    with open(REQ, encoding="utf-8") as f:
        req = json.load(f)
    code = req.get("code", "")
    rid = req.get("id", "")

    for p in (PRELUDE["PyArxCore"],):
        if p not in sys.path:
            sys.path.insert(0, p)

    ns = {"__name__": "__cad__"}
    err = ""
    buf = io.StringIO()
    old_out = sys.stdout
    try:
        ns["P"] = load_pyarx()
        sys.stdout = buf
        exec(compile(code, "<pyarx>", "exec"), ns)
    except Exception:
        err = traceback.format_exc()
    finally:
        sys.stdout = old_out

    payload = {
        "id": rid,
        "out": buf.getvalue()[-4000:],
        "error": err[-4000:],
        "result": jsonable(ns.get("result")),
    }
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(payload, f, ensure_ascii=False, indent=1)
    try:
        print("[cadmcp] done " + rid)
    except Exception:
        pass


if __name__ == "__main__":
    main()
