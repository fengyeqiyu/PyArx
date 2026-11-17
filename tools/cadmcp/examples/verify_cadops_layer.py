# -*- coding: utf-8 -*-
"""headless 全链路：热模块（含 setLayerId）+ cad_ops 生成的 batch，验证图层真正落层。

与 verify_setlayerid.py 的区别：这里测的是**工具层**——模型填 layer 名，工具层
自己选 setLayerId 还是字符串重载，最后用 stats 的 by_layer 复查。
"""

import importlib
import json
import sys
import traceback

OUT = r"C:\temp\cadops_layer_verify.json"
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
    rep["has_setLayerId"] = hasattr(m.Db.PyDbEntity, "setLayerId")

    if CADMCP not in sys.path:
        sys.path.insert(0, CADMCP)
    import cad_ops

    ops = [
        {"tool": "layer", "args": {"name": "BATCH_LAYER_B", "rgb": [0, 200, 120]}},
        {"tool": "line", "args": {"p1": [0, 0], "p2": [100, 0], "layer": "BATCH_LAYER_B"}},
        {"tool": "polyline", "args": {"pts": [[0, 50], [50, 50], [50, 100]],
                                      "closed": True, "layer": "BATCH_LAYER_B"}},
        {"tool": "text", "args": {"pos": [0, 120], "text": "LAYER OK", "height": 8.0,
                                  "layer": "BATCH_LAYER_B"}},
        {"tool": "circle", "args": {"center": [200, 0], "radius": 20.0}},
    ]
    code = cad_ops.script_for("cad_batch", {"ops": ops})
    g = {"P": m}
    exec(compile(code, "<cadops-layer>", "exec"), g)
    rep["batch"] = g.get("result")

    g2 = {"P": m}
    exec(compile(cad_ops.stats_script(400), "<stats>", "exec"), g2)
    rep["stats"] = g2.get("result")


try:
    main()
    b = rep.get("batch") or {}
    rep["ok"] = bool(b.get("ok")) and not b.get("warnings")
except Exception:
    rep["ok"] = False
    rep["exception"] = traceback.format_exc()[-700:]

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(rep, f, ensure_ascii=False, indent=1)
print("written:", OUT)
