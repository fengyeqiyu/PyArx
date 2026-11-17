# -*- coding: utf-8 -*-
"""探针3：openAttribute 的正确签名（在带属性块的 attr_test.dwg 上试多种参数形态）。"""
import json
import traceback

import PyArxCore as P

OUT = r"C:\temp\_att_probe3.json"
res = {"calls": [], "err": ""}
try:
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        ms = tr.openObject(db.currentSpaceId(), 0)
        for eid in list(ms.entityIds()):
            o = tr.openObject(eid, 0)
            if o.dxfName() not in ("INSERT", "MINSERT"):
                continue
            try:
                btr = tr.openObject(o.blockTableRecordId(), 0)
                nm = str(btr.getName())
            except Exception:
                nm = "?"
            if nm != "PY_ATTR_BOX":
                continue
            ids = o.attributeIds()
            res["ids"] = [str(x) for x in ids]
            for aid in ids:
                for label, args in (
                    ("int0", (aid, 0, False)),
                    ("int0_2", (aid, 0)),
                    ("enumRead", (aid, P.Enum.OpenMode.kForRead, False)),
                    ("int1", (aid, 1, False)),
                ):
                    try:
                        a = o.openAttribute(*args)
                        res["calls"].append({
                            "how": label, "ok": True,
                            "dxf": a.dxfName(),
                            "tag": str(a.tag()) if hasattr(a, "tag") else None,
                            "txt": str(a.textString()) if hasattr(a, "textString") else None,
                        })
                    except Exception as ex:
                        res["calls"].append({"how": label, "ok": False, "ex": str(ex)[:220]})
except Exception:
    res["err"] = traceback.format_exc()[-400:]
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print("probe3 written")
