# -*- coding: utf-8 -*-
"""headless 验证 cad_offset：源线偏移 10 后应多出一条平行线。"""

import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\cadmcp")

import PyArxCore as P
import cad_ops

g = {"P": P}
exec(compile(cad_ops.script_for("cad_line",
                                {"p1": [0, 0], "p2": [100, 0], "layer": "OFF_TEST"}),
             "<line>", "exec"), g)
src_handle = g["result"]["handles"][0]

g2 = {"P": P}
exec(compile(cad_ops.script_for("cad_offset", {"handle": src_handle, "dist": 10}),
             "<offset>", "exec"), g2)
off_out = g2["result"]

lines = []
with P.Db.PyDbTrans() as tr:
    for eid in P.Db.curDb().objectIds("AcDbEntity"):
        o = tr.openObject(eid, 0)
        if o is None:
            continue
        if str(o.className()) != "AcDbLine":
            continue
        try:
            sp, ep = o.startPoint(), o.endPoint()
            lines.append({"handle": str(o.handle()),
                          "s": [round(sp.x, 2), round(sp.y, 2)],
                          "e": [round(ep.x, 2), round(ep.y, 2)],
                          "layer": str(o.layer())})
        except Exception as ex:
            lines.append({"handle": str(o.handle()), "err": repr(ex)[:80]})

out = {"src": src_handle, "offset": off_out, "lines": lines}
with open(r"C:\temp\offset_test.json", "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=2)
print(json.dumps(out, ensure_ascii=False)[:800])
