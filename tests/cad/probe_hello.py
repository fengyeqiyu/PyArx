# -*- coding: utf-8 -*-
"""最小探针：确认 PyArxCore 在当前 acad 进程内可用。"""
import json
import os

OUT = r"G:\PyArx\log\probe_hello.json"
R = {"steps": [], "done": False}
os.makedirs(os.path.dirname(OUT), exist_ok=True)


def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


flush()
import PyArxCore as P

flush()
db = P.Db.curDb()
R["steps"].append({"module": getattr(P, "__name__", "?"), "file": getattr(P, "__file__", "?")})
R["steps"].append({"filename": str(db.getFilename()), "entities": len(db.objectIds("AcDbEntity"))})
R["done"] = True
flush()
print("[PyArx] probe_hello done")
