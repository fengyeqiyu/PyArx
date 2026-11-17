# -*- coding: utf-8 -*-
"""Ed 层实测：名字陷阱 + 类构造/成员 + 函数族闭环 + 失败面。不调任何交互 API。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

L = "Ed"
SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

# ------------------------------------------------------------ 1. 名字陷阱（运行时 hasattr 断言）
TRAPS = {
    "PyEdEnt": ("实体结果/实体名真实对应物",
                ["EntityValue", "SubEntityValue", "AdsName", "entget", "entmake", "handent", "entlast"]),
    "PyEdFunc": ("Ed 函数族真实挂在 P.Ed 顶层（aced*/ss*/ent*/acut*）",
                 ["ssget", "entget", "entmake", "command", "acedGetVar", "acutDistance", "acdbEntGetX"]),
    "PyEdSel": ("选集真实对应物", ["SelectionSet", "ssget", "ssadd", "sslength", "ssname", "ssmemb", "acedSSGet"]),
    "PyEdSelectionSets": ("选集真实类名", ["SelectionSet"]),
    "PyEdJig": ("jig 真实类名", ["AcEdJig"]),
    "PyEdUtil": ("工具函数真实挂在 P.Ed 顶层", ["acdbAngToS", "acutDistance", "acutPolar", "acutWcMatch", "acdbRToS"]),
    "PyEdUcsVp": ("UCS/视口函数真实挂在 P.Ed 顶层", ["acedGetCurrentUCS", "acedActiveViewportId", "acedVports", "acedCurVportPixelToDisplay"]),
}
for trap, (note, real) in TRAPS.items():
    def t_trap(trap=trap, note=note, real=real):
        present = hasattr(P.Ed, trap)
        real_ok = {r: hasattr(P.Ed, r) for r in real}
        return "hasattr(P.Ed.%s)=%s；%s；真实存在对应物: %s" % (trap, present, note, real_ok)
    mlab.check(L, trap, "name_trap_missing", t_trap)

# ------------------------------------------------------------ 2. 类：存在性 / 构造 / 类型 / 成员
def members(obj):
    return [x for x in dir(obj) if not x.startswith("_")]

mlab.check(L, "AcEdJig", "exists+default_ctor+type", lambda: (
    "type=%s.%s members=%s" % (type(P.Ed.AcEdJig()).__module__, type(P.Ed.AcEdJig()).__name__,
                               members(P.Ed.AcEdJig))))
mlab.probe(L, "AcEdJig", "ctor 带参", lambda: P.Ed.AcEdJig(1), allow=("Exception",))
mlab.check(L, "AcEdJig", "headless_interactive_unavailable",
           lambda: "acquirePoint/acquireDist/acquireAngle/acquireString/drag 为交互拖动环，无头环境阻塞等待用户输入，按口径不调用；成员清单=%s" % members(P.Ed.AcEdJig))

mlab.check(L, "SelectionSet", "exists+ctor_forms+type", lambda: (
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

# AdsName 也在 Ed 层记一条（真实路径 PyArxCore.Ed.AdsName）
mlab.check(L, "AdsName", "exists+ctor_forms+members", lambda: (
    "members=%s；ctor()=%s.%s；typeerror(x)=%s" % (
        members(P.Ed.AdsName), type(P.Ed.AdsName()).__module__, type(P.Ed.AdsName()).__name__,
        mlab.typeerror_hint(P.Ed.AdsName, "x"))))

# ------------------------------------------------------------ 3. 函数族闭环
LINE = {0: "LINE", 8: "0", 10: [1.0, 2.0, 0.0], 11: [9.0, 2.0, 0.0]}

def t_entmake():
    e = P.Ed.entmake(LINE)
    if e is None or (hasattr(e, "isEmpty") and e.isEmpty()):
        return "entmake 返回空: %r" % (e,)
    d = P.Ed.entget(e)
    return "entmake -> %s.%s (%s)；entget 读回组码: %s" % (
        type(e).__module__, type(e).__name__,
        "data=%s isValid=%s" % (e.data(), e.isValid()),
        {k: d.get(k) for k in (0, 8, 10, 11)} if isinstance(d, dict) else d)
mlab.check(L, "entmake", "make_line+readback", t_entmake)

mlab.check(L, "entlast", "after_entmake", lambda: (
    lambda e: "entlast()=%r type=%s" % (e, type(e).__name__) if e is not None else "entlast()=None")(P.Ed.entlast()))

mlab.check(L, "entget", "readback_line", lambda: (
    lambda e: "entget(%r) 顶层组码 0=%r 8=%r 10=%r 11=%r" % (
        e, (P.Ed.entget(e) or {}).get(0), (P.Ed.entget(e) or {}).get(8),
        (P.Ed.entget(e) or {}).get(10), (P.Ed.entget(e) or {}).get(11)))(P.Ed.entlast()))

def t_ssget_loop():
    ss = P.Ed.ssget("X")
    if ss is None:
        return "ssget('X')=None（空集）"
    n = P.Ed.sslength(ss)
    first = P.Ed.ssname(ss, 0) if n > 0 else None
    memb = P.Ed.ssmemb(first, ss) if first is not None else None
    d = P.Ed.entget(first) if first is not None else None
    return "ssget('X') -> %s.%s；sslength=%d；ssname(0)=%s；ssmemb=%s；entget 0=%r" % (
        type(ss).__module__, type(ss).__name__, n, first, memb,
        (d or {}).get(0) if isinstance(d, dict) else d)
mlab.check(L, "ssget", "x_all_count_name_member_readback", t_ssget_loop)

mlab.check(L, "ssadd", "empty_set_then_add", lambda: (
    lambda e: (lambda ss2: "ssadd()=%s.%s count=%d；ssadd(e)后=%s count=%d" % (
        type(P.Ed.ssadd()).__name__, type(P.Ed.ssadd()).__name__, P.Ed.sslength(P.Ed.ssadd()),
        type(ss2).__name__, P.Ed.sslength(ss2)))(P.Ed.ssadd(e)))(P.Ed.entlast()))

mlab.check(L, "acedSSGet", "mode_X", lambda: (
    lambda ss: "acedSSGet('X')=%s.%s length=%s" % (
        type(ss).__module__, type(ss).__name__, P.Ed.sslength(ss)) if ss is not None else "acedSSGet('X')=None")(
    P.Ed.acedSSGet("X")))

mlab.check(L, "acdbEntGetX", "readback_with_xdata_args", lambda: (
    lambda e: "acdbEntGetX(%r) -> type=%s head=%r" % (
        e, type(P.Ed.acdbEntGetX(e)).__name__, str(P.Ed.acdbEntGetX(e))[:160]))(P.Ed.entlast()))

def t_var_roundtrip():
    before = P.Ed.acedGetVar("CLAYER")
    ok = P.Ed.acedSetVar("CLAYER", before)
    after = P.Ed.acedGetVar("CLAYER")
    return "CLAYER 原值=%r(set 返回=%s) 写回后=%r；原值已恢复" % (before, ok, after)
mlab.check(L, "acedGetVar/acedSetVar", "clayer_roundtrip_restore", t_var_roundtrip)

# ------------------------------------------------------------ 4. 失败面
mlab.probe(L, "entget", "无效句柄字符串", lambda: P.Ed.entget("ZZZ9999"), allow=("Exception", "None", "return"))
mlab.probe(L, "entget", "入参 None", lambda: P.Ed.entget(None), allow=("Exception", "None", "return"))
mlab.probe(L, "entmake", "空 dict", lambda: P.Ed.entmake({}), allow=("Exception", "None", "return"))
mlab.probe(L, "ssget", "不存在的类型过滤", lambda: P.Ed.ssget("X", filter=[(0, "NOPE_TYPE_XYZ")]),
           allow=("Exception", "None", "return"))
mlab.probe(L, "acedGetVar", "未定义变量", lambda: P.Ed.acedGetVar("NONEXISTENT_VAR_XYZ"),
           allow=("Exception", "None", "return"))
mlab.probe(L, "command", "无参", lambda: P.Ed.command(), allow=("Exception",))
mlab.probe(L, "command", "参数类型 int", lambda: P.Ed.command(123), allow=("Exception",))
mlab.probe(L, "acdbEntGetX", "入参 None", lambda: P.Ed.acdbEntGetX(None), allow=("Exception", "None", "return"))

# 原始清单备查
raw = {
    "enum_ads_like": [x for x in dir(P.Enum) if any(k in x for k in ("Ads", "ErrorStatus", "DxfCode", "PromptStatus", "Prompt", "Return"))][:30],
    "core": members(P.Core), "rx": members(P.Rx), "utility": members(P.Utility),
    "has_Engine": hasattr(P, "Engine"), "has_Cmd": hasattr(P, "Cmd"), "has_Redir": hasattr(P, "Redir"),
}
rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\ed_probe.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"raw": raw, "report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"written": out, "dump": dump, "modules": len(rep)}, ensure_ascii=False))
