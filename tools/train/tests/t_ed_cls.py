# -*- coding: utf-8 -*-
"""Ed 轮 A：名字陷阱 + 类构造/成员（不碰实体/选择集/系统变量）。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

L = "Ed"
SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

TRAPS = {
    "PyEdEnt": ("实体结果/实体名真实对应物",
                ["EntityValue", "SubEntityValue", "AdsName", "entget", "entmake", "handent", "entlast"]),
    "PyEdFunc": ("Ed 函数族真实挂在 P.Ed 顶层",
                 ["ssget", "entget", "entmake", "command", "acedGetVar", "acutDistance", "acdbEntGetX"]),
    "PyEdSel": ("选集真实对应物", ["SelectionSet", "ssget", "ssadd", "sslength", "ssname", "ssmemb", "acedSSGet"]),
    "PyEdSelectionSets": ("选集真实类名", ["SelectionSet"]),
    "PyEdJig": ("jig 真实类名", ["AcEdJig"]),
    "PyEdUtil": ("工具函数真实挂在 P.Ed 顶层", ["acdbAngToS", "acutDistance", "acutPolar", "acutWcMatch", "acdbRToS"]),
    "PyEdUcsVp": ("UCS/视口函数真实挂在 P.Ed 顶层", ["acedGetCurrentUCS", "acedActiveViewportId", "acedVports"]),
}
for trap, (note, real) in TRAPS.items():
    def t_trap(trap=trap, note=note, real=real):
        present = hasattr(P.Ed, trap)
        real_ok = {r: hasattr(P.Ed, r) for r in real}
        return "hasattr(P.Ed.%s)=%s；%s；真实存在对应物: %s" % (trap, present, note, real_ok)
    mlab.check(L, trap, "name_trap_missing", t_trap)

def members(obj):
    return [x for x in dir(obj) if not x.startswith("_")]

mlab.check(L, "AcEdJig", "exists+default_ctor+type", lambda: (
    "type=%s.%s members=%s" % (type(P.Ed.AcEdJig()).__module__, type(P.Ed.AcEdJig()).__name__,
                               members(P.Ed.AcEdJig))))
mlab.probe(L, "AcEdJig", "ctor 带参", lambda: P.Ed.AcEdJig(1), allow=("Exception",))
mlab.check(L, "AcEdJig", "headless_interactive_unavailable",
           lambda: "acquirePoint/acquireDist/acquireAngle/acquireString/drag 为交互拖动环，无头阻塞等待用户输入，按口径不调用；成员=%s" % members(P.Ed.AcEdJig))

mlab.check(L, "SelectionSet", "exists+default_ctor+type+hint", lambda: (
    "() -> %s.%s members=%s；typeerror(x)=%s" % (
        type(P.Ed.SelectionSet()).__module__, type(P.Ed.SelectionSet()).__name__,
        members(P.Ed.SelectionSet()), mlab.typeerror_hint(P.Ed.SelectionSet, "x"))))

mlab.check(L, "PyEdResult", "exists+members+ctor_hint", lambda: (
    "members=%s；typeerror(x)=%s" % (members(P.Ed.PyEdResult), mlab.typeerror_hint(P.Ed.PyEdResult, "x"))))
mlab.check(L, "PyEdResult", "fresh_instance_state", lambda: (
    lambda r: "fresh: status=%r value=%r isOk=%s isCanceled=%s isKeyWord=%s isError=%s isNone=%s" % (
        r.status, r.value, r.isOk, r.isCanceled, r.isKeyWord, r.isError, r.isNone))(P.Ed.PyEdResult()))

mlab.check(L, "PromptOptions", "exists+members+ctor_hint", lambda: (
    "members=%s；typeerror(x)=%s" % (members(P.Ed.PromptOptions), mlab.typeerror_hint(P.Ed.PromptOptions, "x"))))

mlab.check(L, "AdsName", "exists+default_ctor+members+hint", lambda: (
    "members=%s；ctor()=%s.%s；typeerror(x)=%s" % (
        members(P.Ed.AdsName), type(P.Ed.AdsName()).__module__, type(P.Ed.AdsName()).__name__,
        mlab.typeerror_hint(P.Ed.AdsName, "x"))))

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\ed_cls.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
