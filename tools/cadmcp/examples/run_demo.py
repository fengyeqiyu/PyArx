import json
import os

try:
    P
except NameError:          # PYRUNFILE 直接执行时不会注入 P
    import PyArxCore as P

SRC = r"G:/PyArx/tools/cadmcp/examples/create_entities.py"

db = P.Db.curDb()
cur = db.getFilename()
# 安全闸：只允许在空白图纸（Drawing1 之类）上跑清空+建模，绝不改用户的工作图纸
base = os.path.basename(cur) if cur else ""
safe = (base == "") or base.lower().startswith("drawing")

if not safe:
    result = {"ok": False, "reason": "活动图纸不是空白图，拒绝清空", "file": cur,
              "entities": len(db.objectIds("AcDbEntity"))}
else:
    ns = {"__name__": "create_entities", "__file__": SRC, "P": P}
    with open(SRC, encoding="utf-8") as f:
        exec(compile(f.read(), SRC, "exec"), ns)
    out = ns["run_demo"](clear=True)
    out["ok"] = not out["failed"]
    result = out
