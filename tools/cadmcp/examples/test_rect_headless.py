# -*- coding: utf-8 -*-
"""headless 验证 cad_rect 生成的几何是否正确落库。"""

import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\cadmcp")

import PyArxCore as P
import cad_ops

code = cad_ops.script_for("cad_rect", {"p1": [120, 300], "p2": [0, 0], "layer": "RECT_TEST"})
g = {"P": P}
exec(compile(code, "<rect>", "exec"), g)
rect_out = g.get("result")

ents = []
with P.Db.PyDbTrans() as tr:
    for eid in P.Db.curDb().objectIds("AcDbEntity"):
        o = tr.openObject(eid, 0)
        if o is None:
            continue
        row = {"cls": o.className() if hasattr(o, "className") else "?", "layer": None}
        try:
            row["layer"] = str(o.layer())
        except Exception:
            pass
        if hasattr(o, "numberOfVertices"):
            try:
                n = int(o.numberOfVertices())
                pts = []
                for i in range(n):
                    p = o.getPoint2dAt(i)
                    pts.append([round(p.x, 3), round(p.y, 3)])
                row["closed"] = bool(o.isClosed())
                row["pts"] = pts
            except Exception as ex:
                row["vert_err"] = repr(ex)[:80]
        ents.append(row)

out = {"rect": rect_out, "entities": ents}
with open(r"C:\temp\rect_test.json", "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=2)
print(json.dumps(out, ensure_ascii=False)[:900])
