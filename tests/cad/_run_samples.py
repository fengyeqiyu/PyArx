# -*- coding: utf-8 -*-
"""执行 examples/cad 下三个 sample，逐支捕获异常并写 JSON 结果。"""
import json, traceback

import PyArxCore as P

RES = r"G:\PyArx\log\run_samples.json"
BASE = r"G:\PyArx\examples\cad"
FILES = ["01_draw_basic.py", "02_layer_block.py", "03_read_modify_delete.py"]

out = {"items": [], "done": False}
for name in FILES:
    rec = {"file": name, "ok": False}
    try:
        path = BASE + "\\" + name
        src = open(path, encoding="utf-8").read()
        g = {"__name__": "__main__", "__file__": path}
        exec(compile(src, path, "exec"), g)
        rec["ok"] = True
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:200]
        rec["trace"] = traceback.format_exc().splitlines()[-3:]
    out["items"].append(rec)
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1, default=str)

out["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1, default=str)
print("[PyArx] run_samples done")
