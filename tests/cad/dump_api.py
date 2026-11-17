# -*- coding: utf-8 -*-
"""把 PyArxCore 的公开 API 导出为 JSON（模块 / 类 / 方法 / 签名首行）。"""
import json
import os
import types

OUT = r"G:\PyArx\log\pyarx_api_dump.json"
R = {"top": [], "modules": {}, "classes": {}, "done": False}


def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


flush()
import PyArxCore as P

R["version"] = getattr(P, "__version__", None)


def docline(obj):
    d = getattr(obj, "__doc__", None) or ""
    for line in d.splitlines():
        s = line.strip()
        if s:
            return s[:160]
    return ""


for name in dir(P):
    if name.startswith("_"):
        continue
    obj = getattr(P, name)
    kind = "module" if isinstance(obj, types.ModuleType) else (
        "class" if isinstance(obj, type) else type(obj).__name__)
    R["top"].append({"name": name, "kind": kind, "doc": docline(obj)})
    if kind == "module":
        sub = {"classes": {}, "funcs": [], "attrs": []}
        for n2 in dir(obj):
            if n2.startswith("_"):
                continue
            o2 = getattr(obj, n2)
            if isinstance(o2, type):
                methods = []
                for m in dir(o2):
                    if m.startswith("_"):
                        continue
                    try:
                        a = getattr(o2, m)
                    except Exception:
                        continue
                    if callable(a):
                        methods.append({"n": m, "d": docline(a)})
                sub["classes"][n2] = {"doc": docline(o2), "methods": methods}
            elif isinstance(o2, types.ModuleType):
                sub["classes"]["<mod> " + n2] = {"doc": ""}
            elif callable(o2):
                sub["funcs"].append({"n": n2, "d": docline(o2)})
            else:
                sub["attrs"].append(n2)
        R["modules"][name] = sub
    elif kind == "class":
        methods = []
        for m in dir(obj):
            if m.startswith("_"):
                continue
            try:
                a = getattr(obj, m)
            except Exception:
                continue
            if callable(a):
                methods.append({"n": m, "d": docline(a)})
        R["classes"][name] = {"doc": docline(obj), "methods": methods}

R["done"] = True
flush()
print("[PyArx] dump_api done", len(R["modules"]), len(R["classes"]))
