# -*- coding: utf-8 -*-
"""前置现状查询的 headless 降级脚本：在 accoreconsole 模板图上跑只读统计，结果写 JSON。

PYRUNFILE 不注入 P，这里自己 import；复用 cad_ops.stats_script 的图元遍历口径。
"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\cadmcp")

OUT = r"C:\temp\ctx_stats_out.json"

import PyArxCore as P  # noqa: E402
import cad_ops  # noqa: E402

g = {"P": P}
try:
    exec(compile(cad_ops.stats_script(5000), "<ctx-stats>", "exec"), g)
    result = g.get("result")
except Exception:
    import traceback
    result = {"ok": False, "error": traceback.format_exc().splitlines()[-1]}

try:
    db = P.Db.curDb()
    handles = []
    handles_truncated = False
    with P.Db.PyDbTrans() as tr:
        for eid in db.objectIds("AcDbEntity"):
            o = tr.openObject(eid, 0)
            if o is None:
                continue
            try:
                h = str(o.handle())
            except Exception:
                continue
            if len(handles) < 50:
                handles.append(h)
            else:
                handles_truncated = True
                break
    result["handles"] = handles
    result["handles_truncated"] = handles_truncated
except Exception:
    result["handles"] = []
    result["handles_truncated"] = True

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, ensure_ascii=False)
