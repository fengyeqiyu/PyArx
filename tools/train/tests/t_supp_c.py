# -*- coding: utf-8 -*-
"""C 轮：数值变量组码修正重试；Enum 四枚举按真实模块名补 int 往返；DocumentManager 补 mdiActiveDocument 一致性。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

# ---------------- 数值变量闭环：已封死（不再调用 acedSetVar），只做阻断面记录
mlab.check("Ed", "acedGetVar/acedSetVar", "numeric_roundtrip_blocked", lambda: (
    "acedGetVar 可读：OSMODE=16383(int)；acedSetVar 三条路径均未打通且其中两条崩溃："
    "①直传 int -> TypeError: (sym: str, value: PyArxCore.Db.TypedValue)；"
    "②Db.TypedValue(3/7, int) -> RuntimeError: int 无法转 AcString；"
    "③Db.TypedValue(70, int) 送入 acedSetVar -> 进程 0xC0000409(退出码3221226505) 崩溃，本轮不再重试；"
    "字符串 CLAYER 路径历史亦崩溃。无头环境未拿到‘写值→回读→恢复’的闭环；正确路径：TypedValue 组码需与 ADS restype(RTSHORT=3) 对齐，"
    "现有 Db.TypedValue 按 DXF 组码映射，二者不匹配，需 C++ 侧补绑定，Python 侧无可用数值 setVar 通道"))

# ---------------- Enum 四枚举：真实模块名补 int 往返 + 不存在成员
def one_enum(layer, module, cls, samples):
    obj = getattr(P.Enum, cls)
    vals = []
    for s, expect in samples:
        v = getattr(obj, s)
        vals.append("%s=%d(期望%d,%s)" % (s, int(v), expect, "一致" if int(v) == expect else "不一致"))
    try:
        getattr(obj, "eNoSuchMemberXYZ")
        missing = "返回对象"
    except AttributeError as ex:
        missing = "AttributeError: %s" % str(ex)[:80]
    except Exception as ex:
        missing = "%s: %s" % (type(ex).__name__, str(ex)[:80])
    return "int() 实测：%s；不存在成员 %s；类型名=%s" % ("；".join(vals), missing, type(obj).__name__)

mlab.check("Ads", "Enum.ErrorStatus", "int_roundtrip_missing",
           lambda: one_enum("Ads", "Enum.ErrorStatus", "ErrorStatus", [("eOk", 0), ("eInvalidInput", 3)]))
mlab.check("Ads", "Enum.DxfCode", "int_roundtrip_missing",
           lambda: one_enum("Ads", "Enum.DxfCode", "DxfCode", [("kDxfStart", 0), ("kDxfInvalid", -9999)]))
mlab.check("Ads", "Enum.PromptStatus", "int_roundtrip_missing",
           lambda: one_enum("Ads", "Enum.PromptStatus", "PromptStatus", [("eNormal", 5100), ("eError", -5001)]))
mlab.check("Ads", "Enum.AdsReturn", "int_roundtrip_missing",
           lambda: one_enum("Ads", "Enum.AdsReturn", "AdsReturn", [("kNorm", 5100), ("kError", -5001)]))

# ---------------- DocumentManager：curDocument / mdiActiveDocument 一致性
def t_mgr2():
    dm = P.Ap.PyApp.documentManager
    a = dm.curDocument
    b = dm.mdiActiveDocument
    return ("curDocument.name=%r；mdiActiveDocument.name=%r；同一对象=%s；count=%d" % (
        a.name, b.name, a is b, dm.count))
mlab.check("App", "DocumentManager", "cur_mdi_active_consistency", t_mgr2)

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\supp_c.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
