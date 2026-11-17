# -*- coding: utf-8 -*-
"""Ed 轮 B1：带逐步骤日志的二分脚本，崩溃后从日志定位最后完成点。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

L = "Ed"
SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"
LOG = r"C:\temp\ed_bisect.log"

def mark(s):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(s + "\n")

mark("== start ==")

line = P.Db.PyDbLine(P.Ge.AcGePoint3d(1.0, 2.0, 0.0), P.Ge.AcGePoint3d(9.0, 2.0, 0.0))
with P.Db.PyDbTrans() as tr:
    tr.addEntity(line)
    handle = str(line.handle())
mark("db_add handle=%s" % handle)

def t_entlast():
    e = P.Ed.entlast()
    mark("entlast ok %r" % e)
    return "entlast()=%r type=%s；Db 句柄=%s" % (e, type(e).__name__, handle)
mlab.check(L, "entlast", "returns_last_entity_after_db_add", t_entlast)

def t_entget():
    e = P.Ed.entlast()
    mark("entget call e=%r" % e)
    d = P.Ed.entget(e)
    mark("entget ok %r" % (str(d)[:120]))
    return "entget -> type=%s 0=%r 8=%r 10=%r 11=%r" % (
        type(d).__name__, (d or {}).get(0), (d or {}).get(8),
        (d or {}).get(10), (d or {}).get(11))
mlab.check(L, "entget", "readback_db_line_dxf", t_entget)

def t_handent():
    n = P.Ed.handent(handle)
    mark("handent ok %r" % n)
    return "handent(%r)=%r type=%s" % (handle, n, type(n).__name__)
mlab.check(L, "handent", "handle_to_adsname", t_handent)

def t_ssget():
    mark("ssget call")
    ss = P.Ed.ssget("X")
    mark("ssget ret %r" % ss)
    if ss is None:
        return "ssget('X')=None"
    n = P.Ed.sslength(ss)
    mark("sslength=%d" % n)
    first = P.Ed.ssname(ss, 0) if n > 0 else None
    memb = P.Ed.ssmemb(first, ss) if first is not None else None
    d = P.Ed.entget(first) if first is not None else None
    return "ssget('X')=%s length=%d name=%s memb=%s entget0=%r" % (
        type(ss).__name__, n, first, memb, (d or {}).get(0) if isinstance(d, dict) else d)
mlab.check(L, "ssget", "x_all_count_name_member", t_ssget)

def t_ssadd():
    e = P.Ed.entlast()
    ss2 = P.Ed.ssadd(e)
    empty = P.Ed.ssadd()
    mark("ssadd ok")
    return "ssadd()=%s length=%d；ssadd(e)=%s length=%d" % (
        type(empty).__name__, P.Ed.sslength(empty),
        type(ss2).__name__, P.Ed.sslength(ss2))
mlab.check(L, "ssadd", "empty_and_add_one", t_ssadd)

def t_acedSSGet():
    mark("acedSSGet call")
    ss = P.Ed.acedSSGet("X")
    mark("acedSSGet ret %r" % ss)
    return "acedSSGet('X')=%s length=%s" % (type(ss).__name__, P.Ed.sslength(ss)) if ss is not None else "None"
mlab.check(L, "acedSSGet", "mode_X", t_acedSSGet)

def t_entgetx():
    e = P.Ed.entlast()
    mark("acdbEntGetX call")
    x = P.Ed.acdbEntGetX(e)
    mark("acdbEntGetX ret %r" % str(x)[:120])
    return "acdbEntGetX(%r) type=%s head=%r" % (e, type(x).__name__, str(x)[:160])
mlab.check(L, "acdbEntGetX", "readback_with_default_args", t_entgetx)

def t_var():
    mark("getvar call")
    before = P.Ed.acedGetVar("CLAYER")
    mark("getvar ret %r" % before)
    ok = P.Ed.acedSetVar("CLAYER", before)
    after = P.Ed.acedGetVar("CLAYER")
    mark("var roundtrip ok %r" % after)
    return "CLAYER 原值=%r set=%s 回读=%r（已恢复）" % (before, ok, after)
mlab.check(L, "acedGetVar/acedSetVar", "clayer_roundtrip_restore", t_var)

mlab.probe(L, "entget", "无效句柄字符串", lambda: P.Ed.entget("ZZZ9999"), allow=("Exception", "None", "return"))
mlab.probe(L, "entget", "入参 None", lambda: P.Ed.entget(None), allow=("Exception", "None", "return"))
mlab.probe(L, "ssget", "不存在类型过滤", lambda: P.Ed.ssget("X", filter=[(0, "NOPE_TYPE_XYZ")]), allow=("Exception", "None", "return"))
mlab.probe(L, "acedGetVar", "未定义变量", lambda: P.Ed.acedGetVar("NONEXISTENT_VAR_XYZ"), allow=("Exception", "None", "return"))
mlab.probe(L, "command", "无参", lambda: P.Ed.command(), allow=("Exception",))
mlab.probe(L, "command", "参数类型 int", lambda: P.Ed.command(123), allow=("Exception",))
mlab.probe(L, "acdbEntGetX", "入参 None", lambda: P.Ed.acdbEntGetX(None), allow=("Exception", "None", "return"))

rep = mlab.report()
dump = mlab.dump(SHARD)
mark("== dump %s ==" % dump)
out = mlab.out_path(r"C:\temp\ed_fn1.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
