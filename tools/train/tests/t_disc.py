# -*- coding: utf-8 -*-
"""发现轮：只读 dir() + __doc__，不调交互 API，为后续断言脚本定签名。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

TARGET_ED = ["ssget", "ssadd", "ssdel", "sslength", "ssname", "ssmemb", "sssetfirst",
             "entget", "entmake", "entmod", "entdel", "entlast", "entnext", "handent",
             "command", "acedGetVar", "acedSetVar", "acedSSGet", "acdbEntGetX",
             "acedArxLoad", "acedArxLoaded", "acedGetAppName", "acedRegCmdsPtr",
             "pyedGetPoint", "pyedSelect", "pyedEntSel", "PromptOptions", "AdsName",
             "AcEdJig", "SelectionSet", "PyEdResult"]

ed = {}
for n in TARGET_ED:
    obj = getattr(P.Ed, n, None)
    if obj is None:
        ed[n] = {"exists": False}
        continue
    doc = (obj.__doc__ or "")
    ed[n] = {"exists": True, "type": type(obj).__name__,
             "doc": " ".join(doc.split())[:300],
             "members": [x for x in dir(obj) if not x.startswith("_")][:30] if not callable(obj) else None}

# 全 Ed 清单（函数名分类）
ed_all = sorted(x for x in dir(P.Ed) if not x.startswith("_"))

# 跨模块 Ads / Engine / Cmd / Binder / Redir / Dispatch 扫描
scan = {}
for mod in ("Core", "Rx", "Utility", "Enum", "Cmd", "Redir", "Debug", "Ap", "Gi", "Db", "Ge"):
    m = getattr(P, mod, None)
    if m is None:
        scan[mod] = None
        continue
    names = dir(m)
    scan[mod] = {
        "count": len(names),
        "ads": [x for x in names if "Ads" in x],
        "engine": [x for x in names if "Engine" in x],
        "cmd": [x for x in names if "Cmd" in x],
        "binder": [x for x in names if "Binder" in x],
        "redir": [x for x in names if "Redir" in x],
        "dispatch": [x for x in names if "Dispatch" in x],
    }

# Ap 目标成员签名
ap = {}
for n in ("PyApp", "Document", "DocumentManager", "PaletteSet", "Palette", "StatusBar"):
    obj = getattr(P.Ap, n, None)
    ap[n] = {"exists": obj is not None,
             "doc": " ".join((obj.__doc__ or "").split())[:200] if obj is not None else None,
             "members": [x for x in dir(obj) if not x.startswith("_")] if obj is not None else None}

# P 顶层子模块
top = sorted(x for x in dir(P) if not x.startswith("_"))

out = mlab.out_path(r"C:\temp\disc.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"ed": ed, "ed_all": ed_all, "scan": scan, "ap": ap, "top": top,
               "pyd": str(getattr(P, "__file__", "?"))}, f, ensure_ascii=False, indent=1)
print(json.dumps({"written": out, "ed_n": len(ed_all), "ap": {k: v["exists"] for k, v in ap.items()}}))
