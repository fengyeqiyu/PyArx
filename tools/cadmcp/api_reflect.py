# -*- coding: utf-8 -*-
"""api_reflect.py —— 用 Python 3.15t 反射 PyArxCore，导出每个类的真实构造签名。

输出 api_ctors.json： {"Db.PyDbLine": {"doc":..., "init":..., "methods":[[name,doc]...]}}
与 api_index.jsonl（C++ 源码提取）互补：.__doc__ 是 pybind11 运行时生成的真签名，
含参数名与重载，是生成工具样板与测试代码的权威来源。
"""
import json
import os
import sys

RELEASE = r"G:\PyArx\X64\Release"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "api_ctors.json")

os.chdir(RELEASE)
sys.path.insert(0, RELEASE)

import PyArxCore as P   # noqa: E402

LAYERS = ("Db", "Ge", "Gi", "Ed", "Ap", "Pl", "Ut")


def dump():
    result = {}
    for lname in LAYERS:
        mod = getattr(P, lname, None)
        if mod is None:
            continue
        for cname in dir(mod):
            if cname.startswith("_"):
                continue
            cls = getattr(mod, cname, None)
            if not isinstance(cls, type):
                continue
            info = {"doc": (getattr(cls, "__doc__", "") or "").strip()[:300]}
            try:
                info["init"] = (getattr(cls.__init__, "__doc__", "") or "").strip()[:1500]
            except Exception as ex:
                info["init"] = ""
                info["init_err"] = repr(ex)[:150]
            methods = []
            for m in dir(cls):
                if m.startswith("_"):
                    continue
                try:
                    d = (getattr(cls, m).__doc__ or "").strip().replace("\n", " ")
                except Exception:
                    d = ""
                methods.append([m, d[:300]])
            info["methods"] = methods
            result[lname + "." + cname] = info
    return result


KEY = ("Db.PyDbLine", "Db.PyDbCircle", "Db.PyDbArc", "Db.PyDbEllipse",
       "Db.PyDbSpline", "Db.PyDbPoint", "Db.PyDbSolid", "Db.PyDbXline",
       "Db.PyDbRay", "Db.PyDbMline", "Db.PyDbLeader", "Db.PyDbWipeout",
       "Db.PyDbRotatedDimension", "Db.PyDbAlignedDimension", "Db.PyDbFace",
       "Db.PyDbTrace", "Db.PyDbShape", "Db.PyDbBlockReference", "Db.PyDbMText",
       "Db.PyDbHatch", "Db.PyDbTable", "Db.PyDbPolyline")

if __name__ == "__main__":
    data = dump()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=1)
    print("classes=%d -> %s" % (len(data), OUT))
    for k in KEY:
        it = data.get(k) or {}
        print("### %s %s" % (k, (it.get("init") or "(无构造文档)").replace("\n", " ")[:200]))
