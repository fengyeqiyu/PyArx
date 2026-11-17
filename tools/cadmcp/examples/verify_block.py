# -*- coding: utf-8 -*-
"""headless：用热模块 + cad_ops 引用**已存在**的块定义，验证 cad_block_ref 真正入库。"""

import importlib
import json
import sys
import traceback

OUT = r"C:\temp\cadops_block_verify.json"
OUTDIR = r"G:\PyArx\X64\Release"
MODTXT = r"G:\PyArx\tools\hotbuild\module.txt"
CADMCP = r"G:\PyArx\tools\cadmcp"

rep = {}


def main():
    name = open(MODTXT, encoding="utf-8").read().strip()
    rep["module"] = name
    if OUTDIR not in sys.path:
        sys.path.insert(0, OUTDIR)
    cache = sys.modules.setdefault("__pyarx_old__", [])
    for k in list(sys.modules.keys()):
        if k == "PyArxCore" or k.startswith("PyArxCore.") or k == name:
            old = sys.modules.pop(k, None)
            if old is not None:
                cache.append(old)
    m = importlib.import_module(name)
    sys.modules["PyArxCore"] = m

    if CADMCP not in sys.path:
        sys.path.insert(0, CADMCP)
    import cad_ops

    DB = m.Db.curDb()
    bt = DB.getBlockTable()
    rep["bt_methods"] = [x for x in dir(bt) if x in ("names", "has", "getAt", "getIdAt", "records", "keys")]
    try:
        names = [str(x) for x in list(bt.names())]
    except Exception:
        names = []
    try:
        bt.close()          # 块表一个进程只能打开一次，探测完必须关掉
    except Exception:
        pass
    rep["block_count"] = len(names)
    skip = ("*Model_Space", "*Paper_Space", "*Paper_Space0")
    # xref 派生块（名字含 $0$ / xref）不可被引用，必须排除
    target = next((n for n in names
                   if n not in skip and "$" not in n and "xref" not in n.lower()), None)
    rep["target"] = target

    if target:
        code = cad_ops.script_for("cad_block_ref",
                                  {"name": target, "pos": [0, 0], "scale": 1.0})
        g = {"P": m}
        exec(compile(code, "<cadops-block>", "exec"), g)
        rep["block_ref"] = g.get("result")


try:
    main()
    rep["ok"] = bool((rep.get("block_ref") or {}).get("ok"))
except Exception:
    rep["ok"] = False
    rep["exception"] = traceback.format_exc()[-700:]

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(rep, f, ensure_ascii=False, indent=1)
print("written:", OUT)
