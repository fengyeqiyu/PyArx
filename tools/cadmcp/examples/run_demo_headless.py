import json

import PyArxCore as P

SRC = r"G:/PyArx/tools/cadmcp/examples/create_entities.py"
OUT = r"G:\PyArx\tools\cadmcp\headless_demo_out.json"

ns = {"__name__": "create_entities", "__file__": SRC, "P": P}
with open(SRC, encoding="utf-8") as f:
    exec(compile(f.read(), SRC, "exec"), ns)

out = ns["run_demo"](clear=True)
out["ok"] = not out["failed"]
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1)
