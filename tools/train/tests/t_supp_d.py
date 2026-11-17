# -*- coding: utf-8 -*-
"""D 轮：合并被 dump 覆盖的旧断言——Enum 存在性+int 往返同记录；DocumentManager 两条一致性同记录。"""
import json
import os
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

# 预填现有 checks（dump 按 layer+module 整记录覆盖，必须把旧断言带上）
existing = {}
if os.path.isfile(SHARD):
    with open(SHARD, encoding="utf-8") as f:
        for r in json.load(f):
            existing[(r["layer"], r["module"])] = r["checks"]
for k, v in existing.items():
    mlab.ROWS[k] = list(v)

# ---------------- Enum 四枚举：存在性/真实路径 + int 往返 + 不存在成员
def enum_record(cls, samples):
    obj = getattr(P.Enum, cls)
    vals = []
    for s, expect in samples:
        v = getattr(obj, s)
        vals.append("%s=%d(期望%d,%s)" % (s, int(v), expect, "一致" if int(v) == expect else "不一致"))
    try:
        getattr(obj, "eNoSuchMemberXYZ")
        missing = "返回对象"
    except Exception as ex:
        missing = "%s: %s" % (type(ex).__name__, str(ex)[:80])
    return ("存在性：P.Ed 无 PyAds%s（Ads 枚举不在 Ed）；真实路径 P.Enum.%s；type=%s；"
            "int() 实测：%s；不存在成员：%s" % (cls, cls, type(obj).__name__, "；".join(vals), missing))

mlab.check("Ads", "Enum.ErrorStatus", "existence_path_int_roundtrip",
           lambda: enum_record("ErrorStatus", [("eOk", 0), ("eInvalidInput", 3)]))
mlab.check("Ads", "Enum.DxfCode", "existence_path_int_roundtrip",
           lambda: enum_record("DxfCode", [("kDxfStart", 0), ("kDxfInvalid", -9999)]))
mlab.check("Ads", "Enum.PromptStatus", "existence_path_int_roundtrip",
           lambda: enum_record("PromptStatus", [("eNormal", 5100), ("eError", -5001)]))
mlab.check("Ads", "Enum.AdsReturn", "existence_path_int_roundtrip",
           lambda: enum_record("AdsReturn", [("kNorm", 5100), ("kError", -5001)]))

# ---------------- DocumentManager：count/len/documentAt + cur/mdi 同记录
def t_mgr_all():
    dm = P.Ap.PyApp.documentManager
    docs = dm.documents
    d0 = dm.documentAt(0) if dm.count > 0 else None
    a, b = dm.curDocument, dm.mdiActiveDocument
    return ("count=%d；len(documents)=%d 一致=%s；documentAt(0).name=%r；curDocument.name=%r 一致=%s；"
            "mdiActiveDocument.name=%r 与 cur 同名=%s（同一对象=%s）；isApplicationContext=%s" % (
                dm.count, len(docs), dm.count == len(docs),
                d0.name if d0 is not None else None, dm.curDocument.name,
                d0 is not None and d0.name == dm.curDocument.name,
                b.name, b.name == a.name, a is b, dm.isApplicationContext))
mlab.check("App", "DocumentManager", "count_getdocument_cur_mdi_all", t_mgr_all)

dump = mlab.dump(SHARD)
rep = mlab.report()
out = mlab.out_path(r"C:\temp\supp_d.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
