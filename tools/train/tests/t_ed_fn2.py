# -*- coding: utf-8 -*-
"""Ed 轮 B2：ssadd/entmake/command/失败面；不调 acedSetVar（已确认无头崩溃）。"""
import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

L = "Ed"
SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"
LOG = r"C:\temp\ed_bisect2.log"

def mark(s):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(s + "\n")

line = P.Db.PyDbLine(P.Ge.AcGePoint3d(1.0, 2.0, 0.0), P.Ge.AcGePoint3d(9.0, 2.0, 0.0))
with P.Db.PyDbTrans() as tr:
    tr.addEntity(line)

def t_ssadd():
    mark("ssadd: entlast")
    e = P.Ed.entlast()
    mark("ssadd: entlast=%r" % e)
    out = {}
    try:
        empty = P.Ed.ssadd()
        out["empty"] = "%s length=%d" % (type(empty).__name__, P.Ed.sslength(empty))
        mark("ssadd empty ok")
    except Exception as ex:
        out["empty"] = "%s: %s" % (type(ex).__name__, str(ex)[:160])
        mark("ssadd empty exc %s" % ex)
    try:
        ss2 = P.Ed.ssadd(e)
        mark("ssadd(e) ret")
        out["add_one"] = "%s length=%d member=%s" % (
            type(ss2).__name__, P.Ed.sslength(ss2), P.Ed.ssmemb(P.Ed.ssname(ss2, 0), ss2))
        mark("ssadd add_one ok")
    except Exception as ex:
        out["add_one"] = "%s: %s" % (type(ex).__name__, str(ex)[:160])
        mark("ssadd add_one exc %s" % ex)
    return "ssadd 行为: %s" % out
mlab.check(L, "ssadd", "empty_and_add_one", t_ssadd)

def t_entmake():
    dxf = {0: "LINE", 8: "0", 10: [10.0, 20.0, 0.0], 11: [19.0, 20.0, 0.0]}
    mark("entmake call")
    e = P.Ed.entmake(dxf)
    mark("entmake ret %r" % e)
    if e is None or (hasattr(e, "isEmpty") and e.isEmpty()):
        return "entmake 返回空: %r" % (e,)
    mark("entmake entget call")
    d = P.Ed.entget(e)
    mark("entmake entget ok")
    return "entmake -> %s.%s data=%r isValid=%s；entget 0=%r 10=%r 11=%r" % (
        type(e).__module__, type(e).__name__, e.data(), e.isValid(),
        (d or {}).get(0), (d or {}).get(10), (d or {}).get(11))
mlab.check(L, "entmake", "make_line+readback", t_entmake)

def t_getvar():
    mark("getvar call")
    v = P.Ed.acedGetVar("CLAYER")
    mark("getvar ret %r" % v)
    return "acedGetVar('CLAYER')=%r type=%s" % (v, type(v).__name__)
mlab.check(L, "acedGetVar", "clayer_read_only", t_getvar)

# acedSetVar 已在二分轮确认：传字符串 -> 进程 0xC0000409 崩溃（MessageBox 自动 Yes 后退出），按口径不再调用第三次
mlab.check(L, "acedSetVar", "headless_crash_not_retried", lambda: (
    "二分轮 t_ed_fn1 日志：getvar 读回 '0' 后在 acedSetVar('CLAYER','0') 内进程崩溃退出码 3221226505(0xC0000409)；返回值未取得；本轮按规则不再重试。签名: acedSetVar(sym: str, value: PyArxCore.Db.TypedValue) -> bool"))

def PP(mod, name, fn, allow):
    mark("probe %s/%s start" % (mod, name))
    mlab.probe(L, mod, name, fn, allow=allow)
    mark("probe %s/%s done" % (mod, name))

mlab.check(L, "command", "exists+signature", lambda: (
    "command(*args) -> bool；doc: %s" % " ".join((P.Ed.command.__doc__ or "").split())[:200]))
PP("entget", "无效句柄字符串", lambda: P.Ed.entget("ZZZ9999"), allow=("Exception", "None", "return"))
PP("entget", "入参 None", lambda: P.Ed.entget(None), allow=("Exception", "None", "return"))
PP("entmake", "空 dict", lambda: P.Ed.entmake({}), allow=("Exception", "None", "return"))
PP("ssget", "不存在类型过滤", lambda: P.Ed.ssget("X", filter=[(0, "NOPE_TYPE_XYZ")]), allow=("Exception", "None", "return"))
PP("acedGetVar", "未定义变量", lambda: P.Ed.acedGetVar("NONEXISTENT_VAR_XYZ"), allow=("Exception", "None", "return"))
PP("acdbEntGetX", "入参 None", lambda: P.Ed.acdbEntGetX(None), allow=("Exception", "None", "return"))
PP("command", "无参", lambda: P.Ed.command(), allow=("Exception",))
mlab.check(L, "command", "int_arg_headless_crash_not_retried", lambda: (
    "二分日志：command(123) 探针 start 后无 done，进程崩溃退出码 3221226505(0xC0000409)；int 入参路径未取得返回值，按规则不再重试。command() 无参行为见同模块 [probe]"))

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\ed_fn2.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
