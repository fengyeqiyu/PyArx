# -*- coding: utf-8 -*-
"""A 轮：Ed 选集/图元真闭环 + 数值系统变量往返 + 命令注册佐证。不碰字符串变量、不重入命令。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

# 建两条线（模板图无实体；均落默认层 0，EDSEL 建层路径未在 Db 暴露）
pt = P.Ge.AcGePoint3d
l1 = P.Db.PyDbLine(pt(1.0, 2.0, 0.0), pt(9.0, 2.0, 0.0))
l2 = P.Db.PyDbLine(pt(3.0, 5.0, 0.0), pt(8.0, 5.0, 0.0))
with P.Db.PyDbTrans() as tr:
    tr.addEntity(l1)
    tr.addEntity(l2)
h1, h2 = str(l1.handle()), str(l2.handle())
oids = list(P.Db.curDb().objectIds("AcDbLine"))

# ---------------- ssget 闭环
def t_ssget():
    ss = P.Ed.ssget("X", filter=[(0, "LINE")])
    if ss is None:
        return "ssget('X',filter=[(0,'LINE')])=None（空集）"
    n = P.Ed.sslength(ss)
    e0 = P.Ed.ssname(ss, 0) if n > 0 else None
    e1 = P.Ed.ssname(ss, 1) if n > 1 else None
    m0 = P.Ed.ssmemb(e0, ss) if e0 is not None else None
    m1 = P.Ed.ssmemb(e1, ss) if e1 is not None else None
    d0 = P.Ed.entget(e0) if e0 is not None else None
    return ("count=%d handle0=%r handle1=%r ssmemb0=%s ssmemb1=%s；"
            "entget(handle0) 0=%r 10=%r 11=%r；建实体句柄 h1=%s h2=%s" % (
                n, e0, e1, m0, m1, (d0 or {}).get(0), (d0 or {}).get(10), (d0 or {}).get(11), h1, h2))
mlab.check("Ed", "ssget", "closed_loop_two_lines", t_ssget)
mlab.probe("Ed", "ssget", "不存在类型过滤", lambda: P.Ed.ssget("X", filter=[(0, "NOPE_TYPE_XYZ")]),
           allow=("Exception", "None", "return"))

# ---------------- ssadd / ssdel 闭环
def t_ssadd():
    e = P.Ed.entlast()
    out = {}
    ss = P.Ed.ssadd(e)
    out["add_one"] = "length=%d ssmemb=%s" % (P.Ed.sslength(ss), P.Ed.ssmemb(e, ss))
    ok = P.Ed.ssdel(e, ss)
    out["ssdel"] = "返回=%s length=%d" % (ok, P.Ed.sslength(ss))
    try:
        empty = P.Ed.ssadd()
        out["empty"] = "length=%d（无参未抛错）" % P.Ed.sslength(empty)
    except Exception as ex:
        out["empty"] = "%s: %s" % (type(ex).__name__, str(ex)[:120])
    return out
mlab.check("Ed", "ssadd", "add_one_ssdel_zero", t_ssadd)

# ---------------- acedSSGet 与 ssget 计数一致
def t_acedssget():
    a = P.Ed.acedSSGet("X")
    b = P.Ed.ssget("X")
    na, nb = P.Ed.sslength(a), (None if b is None else P.Ed.sslength(b))
    return "acedSSGet('X') length=%d；ssget('X') length=%s；一致=%s" % (na, nb, na == nb)
mlab.check("Ed", "acedSSGet", "count_matches_ssget", t_acedssget)

# ---------------- entget DXF 组码
def t_entget():
    e = P.Ed.ssname(P.Ed.ssget("X", filter=[(0, "LINE")]), 0)
    d = P.Ed.entget(e)
    return "entget(%r) 0=%r 8=%r 10=%r 11=%r 5=%r；组码类型=%s" % (
        e, d.get(0), d.get(8), d.get(10), d.get(11), d.get(5), type(d).__name__)
mlab.check("Ed", "entget", "dxf_fields_readback", t_entget)

# ---------------- entlast / handent
def t_entlast():
    e = P.Ed.entlast()
    d = P.Ed.entget(e)
    return "entlast()=%r 11=%r（最后 addEntity 端点）；实体句柄 h2=%s" % (e, (d or {}).get(11), h2)
mlab.check("Ed", "entlast", "last_entity_readback", t_entlast)

def t_handent():
    n = P.Ed.handent(h1)
    back = P.Ed.entget(n) if n is not None else None
    return "handent(%r)=%r type=%s；可 entget=%s（acdbHandEnt 状态 -5001 时归一 None）" % (
        h1, n, type(n).__name__, back is not None)
mlab.check("Ed", "handent", "handle_to_name", t_handent)

# ---------------- 数值系统变量往返（OSMODE int），不碰 CLAYER 字符串
def t_var():
    before = P.Ed.acedGetVar("OSMODE")
    written = None
    second = None
    restored = None
    after_restore = None
    try:
        written = P.Ed.acedSetVar("OSMODE", before)
        second = P.Ed.acedGetVar("OSMODE")
        # 改成另一个 int 再读回
        alt = 0 if int(before) != 0 else 1
        P.Ed.acedSetVar("OSMODE", alt)
        written2 = P.Ed.acedGetVar("OSMODE")
        P.Ed.acedSetVar("OSMODE", before)
        after_restore = P.Ed.acedGetVar("OSMODE")
        return ("OSMODE 原值=%r(type=%s)；写回同值返回=%s 回读=%r；改 alt=%s 回读=%r；恢复原值=%s 回读=%r" % (
            before, type(before).__name__, written, second, alt, written2,
            "已恢复" if str(after_restore) == str(before) else "未恢复", after_restore))
    except Exception as ex:
        return "原值=%r；acedSetVar 数值路径异常: %s: %s" % (before, type(ex).__name__, str(ex)[:160])
mlab.check("Ed", "acedGetVar/acedSetVar", "osmode_roundtrip_restore", t_var)

# ---------------- command 注册佐证（源码清单 + 指针非空，不重入）
def t_cmd_reg():
    ptr = P.Ed.acedRegCmdsPtr()
    src = ["PYLOAD", "PYRUN", "PYEXEC", "PIPM", "PYDEBUG", "PYARXDEPLOY", "PYRUNFILE"]
    return ("源码 acrxEntryPoint.cpp 组 PyArxEngine 命令清单=%s；acedRegCmdsPtr()=%r type=%s 非空=%s；"
            "Python 侧无按名 lookup，未调 command('PYRUNFILE') 避免重入；本轮脚本本身由 PYRUNFILE 链执行" % (
                src, ptr, type(ptr).__name__, bool(ptr)))
mlab.check("Ed", "command", "registry_evidence_no_reentry", t_cmd_reg)
# 已记在案：command() 无参静默 True；command(123) 崩溃不重试（保留原记录，不重跑）

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\supp_a.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
