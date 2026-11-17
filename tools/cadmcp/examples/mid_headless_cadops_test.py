# -*- coding: utf-8 -*-
"""headless 验收：用 cad_ops 生成的脚本在 accoreconsole 里真写图元，再只读复查。

PYRUNFILE 不注入 P，所以这里自己 import；exec 的环境字典里放 P，
cad_ops 生成的代码就是按全局 P 写的。
"""

import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\cadmcp")

OUT = r"C:\temp\cadops_headless_out.json"

import PyArxCore as P
import cad_ops

OPS = [
    {"tool": "layer", "args": {"name": "CADOPS_TEST", "rgb": [0, 255, 0]}},
    {"tool": "line", "args": {"p1": [0, 0], "p2": [100, 0], "layer": "CADOPS_TEST", "color": 1}},
    {"tool": "polyline", "args": {"pts": [[0, 50], [80, 50], [80, 120], [0, 120]],
                                  "closed": True, "layer": "CADOPS_TEST"}},
    {"tool": "text", "args": {"pos": [0, 150], "text": "CADOPS OK", "height": 8.0,
                              "layer": "CADOPS_TEST"}},
    {"tool": "circle", "args": {"center": [200, 0], "radius": 25.0}},
    {"tool": "hatch", "args": {"pts": [[300, 0], [380, 0], [380, 60], [300, 60]]}},
    {"tool": "arc", "args": {"center": [200, 100], "radius": 30.0, "start_deg": 0, "end_deg": 90}},
]

report = {"steps": []}


def run(label, code):
    g = {"P": P}
    try:
        exec(compile(code, "<cadops:%s>" % label, "exec"), g)
    except Exception:
        return {"ok": False, "label": label,
                "error": traceback.format_exc().splitlines()[-1]}, None
    return g.get("result"), g


try:
    code = cad_ops.script_for("cad_batch", {"ops": OPS})
    res, _ = run("batch", code)
    report["batch"] = res
except Exception:
    report["batch"] = {"ok": False, "error": traceback.format_exc().splitlines()[-1]}

try:
    sres, _ = run("stats", cad_ops.stats_script(400))
    report["stats_after"] = sres
except Exception:
    report["stats_after"] = {"ok": False, "error": traceback.format_exc().splitlines()[-1]}

try:
    qres, _ = run("query", cad_ops.query_script("", "CADOPS_TEST", 40))
    report["query_layer"] = qres
except Exception:
    report["query_layer"] = {"ok": False, "error": traceback.format_exc().splitlines()[-1]}

# erase 端到端：删第一条 batch 实体（line），再统计应 6 -> 5
hs = (report.get("batch") or {}).get("handles") or []
if hs:
    try:
        eres, _ = run("erase", cad_ops.script_for("cad_erase", {"handles": [hs[0]]}))
        report["erase_one"] = eres
        s2, _ = run("stats2", cad_ops.stats_script(400))
        report["after_erase_total"] = (s2 or {}).get("total")
    except Exception:
        report["erase_one"] = {"ok": False, "error": traceback.format_exc().splitlines()[-1]}

# block_ref 报错路径：引用不存在的块定义必须明确失败，不得静默建空块
try:
    bres, _ = run("block", cad_ops.script_for("cad_block_ref",
                                             {"name": "NO_SUCH_BLOCK_XYZ", "pos": [0, 0]}))
    report["block_missing"] = bres
except Exception:
    report["block_missing"] = {"exec": traceback.format_exc().splitlines()[-1]}

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=1)
print("written:", OUT)
