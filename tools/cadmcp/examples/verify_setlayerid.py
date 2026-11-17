# -*- coding: utf-8 -*-
"""headless 验证 setLayerId：热模块替换 + 对已存在图层 / 新建图层分别设层并读回。

模板图用 G:\PyArx\tests\楼层平面-强电.dwg（98 个图层），同时覆盖两种情形。
accoreconsole 会先 arxload 引擎（已导入基础 PyArxCore），这里按 reload.py 的
顺序清空 sys.modules 后再导入热模块，旧模块保活。
"""

import importlib
import json
import sys
import traceback

OUT = r"C:\temp\setlayerid_verify.json"
OUTDIR = r"G:\PyArx\X64\Release"
MODTXT = r"G:\PyArx\tools\hotbuild\module.txt"

rep = {}


def main():
    name = open(MODTXT, encoding="utf-8").read().strip()
    rep["module"] = name
    if OUTDIR not in sys.path:
        sys.path.insert(0, OUTDIR)

    cache = sys.modules.setdefault("__pyarx_old__", [])
    cleared = []
    for k in list(sys.modules.keys()):
        if k == "PyArxCore" or k.startswith("PyArxCore.") or k == name:
            old = sys.modules.pop(k, None)
            if old is not None:
                cache.append(old)
                cleared.append(k)
    rep["cleared"] = cleared

    m = importlib.import_module(name)
    sys.modules["PyArxCore"] = m
    rep["alias_file"] = getattr(m, "__file__", "")
    rep["has_setLayerId"] = hasattr(m.Db.PyDbEntity, "setLayerId")

    DB = m.Db.curDb()
    lt = DB.getLayerTable()
    names = [str(x) for x in list(lt.names())]
    rep["layer_count"] = len(names)
    existing = next((n for n in names if n not in ("0", "Defpoints")), None)
    rep["existing_target"] = existing

    # 情形 1：已存在的图层
    if existing:
        with m.Db.PyDbTrans() as tr:
            ln = m.Db.PyDbLine(m.Ge.AcGePoint3d(0, 0, 0), m.Ge.AcGePoint3d(10, 0, 0))
            tr.addEntity(ln)
            r = ln.setLayerId(lt.getAt(existing).objectId())
            rep["existing"] = {"ret": repr(r), "layer": str(ln.layer())}

    # 情形 2：本会话新建的图层
    newname = "VERIFY_LAYER_NEW"
    if not lt.has(newname):
        rec = m.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(newname)
        rec.setColor(255, 40, 40)
        rep["add_new"] = bool(lt.add(rec))
    with m.Db.PyDbTrans() as tr:
        ln2 = m.Db.PyDbLine(m.Ge.AcGePoint3d(0, 20, 0), m.Ge.AcGePoint3d(10, 20, 0))
        tr.addEntity(ln2)
        r2 = ln2.setLayerId(lt.getAt(newname).objectId())
        rep["new"] = {"ret": repr(r2), "layer": str(ln2.layer())}

    # 对照：字符串重载仍然失败（用于确认结论不变）
    with m.Db.PyDbTrans() as tr:
        ln3 = m.Db.PyDbLine(m.Ge.AcGePoint3d(0, 40, 0), m.Ge.AcGePoint3d(10, 40, 0))
        tr.addEntity(ln3)
        r3 = ln3.setLayer(newname)
        rep["by_name"] = {"ret": repr(r3), "layer": str(ln3.layer())}


try:
    main()
    rep["ok"] = (
        rep.get("existing", {}).get("layer") == rep.get("existing_target")
        and rep.get("new", {}).get("layer") == "VERIFY_LAYER_NEW"
    )
except Exception:
    rep["ok"] = False
    rep["exception"] = traceback.format_exc()[-700:]

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(rep, f, ensure_ascii=False, indent=1)
print("written:", OUT)
