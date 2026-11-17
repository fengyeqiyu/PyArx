# -*- coding: utf-8 -*-
import json, sys
sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

out = {}
for prop in ("documentManager", "mainWindow", "nonInPlaceMainWindow", "statusBar", "acadApplication", "documentWindowCollection"):
    try:
        v = getattr(P.Ap.PyApp, prop)
        out[prop] = {"ok": True, "type": type(v).__name__, "repr": repr(v)[:120]}
    except Exception as e:
        out[prop] = {"ok": False, "exc": "%s: %s" % (type(e).__name__, str(e)[:160])}

# 不抛异常的属性再取子成员类型
rep = {}
try:
    sb = P.Ap.PyApp.statusBar
    rep["statusBar_members"] = [x for x in dir(sb) if not x.startswith("_")]
except Exception as e:
    rep["statusBar_members"] = "%s: %s" % (type(e).__name__, str(e)[:120])

unsup = {}
for meth in ("preferences", "infoCenter", "menuBar", "menuGroups", "uiBindings"):
    try:
        getattr(P.Ap.PyApp, meth)(); unsup[meth] = "returned"
    except Exception as e:
        unsup[meth] = "%s: %s" % (type(e).__name__, str(e)[:100])

mlab.check("App", "PyApp", "static_property_bisect", lambda: json.dumps(out, ensure_ascii=False))
mlab.check("App", "PyApp", "unsupported_members_degrade", lambda: "降级行为: %s" % unsup)
dump = mlab.dump(r"G:\PyArx\tools\train\data\shard_ed_app_engine.json")
o = mlab.out_path(r"C:\temp\pyapp_probe.json")
with open(o, "w", encoding="utf-8") as f:
    json.dump({"props": out, "rep": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"props": out, "rep": rep}, ensure_ascii=False))
