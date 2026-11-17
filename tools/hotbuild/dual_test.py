"""dual_test.py - import a hot build and report basic state."""

import importlib
import json
import os
import sys
import traceback

OUT = r"G:\PyArx\tools\hotbuild\dual_out.json"
sys.path.insert(0, r"G:\PyArx\X64\Release")
TARGETS = ["PyArxCoreHhB"]

if __name__ == "__main__":
    rep = {"ok": True}
    try:
        for n in TARGETS:
            m = importlib.import_module(n)
            sys.modules["PyArxCore"] = m
            db = m.Db.curDb()
            rep[n] = {"file": os.path.basename(m.__file__), "db": db.getFilename(),
                      "ents": len(db.objectIds("AcDbEntity"))}
    except Exception:
        rep = {"ok": False, "exception": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[dual] " + json.dumps(rep, ensure_ascii=False)[:400])