# -*- coding: utf-8 -*-
"""Ed 轮 B3：补落 fn1 成功但未落盘的闭环模块（不含 acedSetVar / command int）。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

L = "Ed"
SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

line = P.Db.PyDbLine(P.Ge.AcGePoint3d(1.0, 2.0, 0.0), P.Ge.AcGePoint3d(9.0, 2.0, 0.0))
with P.Db.PyDbTrans() as tr:
    tr.addEntity(line)
handle = str(line.handle())

mlab.check(L, "entlast", "returns_last_entity_after_db_add", lambda: (
    "entlast()=%r type=%s.%s；Db 实体句柄=%s" % (P.Ed.entlast(), type(P.Ed.entlast()).__module__,
    type(P.Ed.entlast()).__name__, handle)))

def t_entget():
    e = P.Ed.entlast()
    d = P.Ed.entget(e)
    return "entget(entlast) type=%s；0=%r 8=%r 10=%r 11=%r 5=%r" % (
        type(d).__name__, (d or {}).get(0), (d or {}).get(8),
        (d or {}).get(10), (d or {}).get(11), (d or {}).get(5))
mlab.check(L, "entget", "readback_db_line_dxf", t_entget)

mlab.check(L, "handent", "handle_to_adsname", lambda: (
    "handent(%r)=%r type=%s（acdbHandEnt 状态 -5001 时归一 None）" % (handle, P.Ed.handent(handle),
    type(P.Ed.handent(handle)).__name__)))

def t_ssget():
    ss = P.Ed.ssget("X")
    if ss is None:
        return "ssget('X')=None"
    n = P.Ed.sslength(ss)
    first = P.Ed.ssname(ss, 0) if n > 0 else None
    memb = P.Ed.ssmemb(first, ss) if first is not None else None
    d = P.Ed.entget(first) if first is not None else None
    return "ssget('X')=%s.%s；sslength=%d；ssname(0)=%s；ssmemb=%s；entget 0=%r" % (
        type(ss).__module__, type(ss).__name__, n, first, memb,
        (d or {}).get(0) if isinstance(d, dict) else d)
mlab.check(L, "ssget", "x_all_count_name_member", t_ssget)

def t_acedSSGet():
    ss = P.Ed.acedSSGet("X")
    return "acedSSGet('X')=%s.%s；length=%s" % (
        type(ss).__module__, type(ss).__name__, P.Ed.sslength(ss)) if ss is not None else "acedSSGet('X')=None"
mlab.check(L, "acedSSGet", "mode_X", t_acedSSGet)

def t_entgetx():
    e = P.Ed.entlast()
    x = P.Ed.acdbEntGetX(e)
    return "acdbEntGetX(%r) type=%s head=%r" % (e, type(x).__name__, str(x)[:160])
mlab.check(L, "acdbEntGetX", "readback_with_default_args", t_entgetx)

mlab.probe(L, "entget", "无效句柄字符串", lambda: P.Ed.entget("ZZZ9999"), allow=("Exception", "None", "return"))
mlab.probe(L, "entget", "入参 None", lambda: P.Ed.entget(None), allow=("Exception", "None", "return"))
mlab.probe(L, "acdbEntGetX", "入参 None", lambda: P.Ed.acdbEntGetX(None), allow=("Exception", "None", "return"))

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\ed_fn3.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
