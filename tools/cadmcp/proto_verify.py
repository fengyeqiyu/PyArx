# -*- coding: utf-8 -*-
"""proto_verify.py —— 把 proto.py 的样板逐条送进 accoreconsole 无头验证。

流程（与 harness 的 headless 通道同一套机制）：
  副本图 -> 内层脚本 -> .scr(arxload + PYRUNFILE) -> accoreconsole -> 结果 JSON

输出：
  proto_result.json  每条样板的结论（ok/handle/class/bbox/err）
  控制台摘要          图元 N/M 通过、几何运算 N/M 通过

用法：
  python proto_verify.py            # 全部样板
  python proto_verify.py line circle   # 只验指定图元
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TMP = r"C:\temp"
CONSOLE = os.environ.get("PYARX_CONSOLE", r"G:\PyArx\ACAD\accoreconsole.exe")
ENGINE = r"G:\PyArx\X64\Release\PyArxEngine.crx"
TEMPLATE = r"C:\temp\pyarx_db_save_test.dwg"
WORK = os.path.join(TMP, "proto_probe.dwg")
OUT_JSON = os.path.join(HERE, "proto_result.json")

sys.path.insert(0, HERE)
import proto  # noqa: E402
from proto import ENTITY, GEO, HELPERS  # noqa: E402


INNER_HEAD = '''# -*- coding: utf-8 -*-
import json, math, traceback
import PyArxCore as P
from PyArxCore import Db, Ge

''' + HELPERS + """
DB = P.Db.curDb()

RES_ENT = {}
RES_GEO = {}
"""

INNER_RUN_KIND = '''
def run_kind(kind, spec):
    a = dict(spec.get("sample") or {})
    codes = [spec["build"]] if spec.get("build") else list(spec.get("candidates") or [])
    last = ""
    for idx, code in enumerate(codes):
        try:
            ns = {"P": P, "Db": Db, "Ge": Ge, "math": math, "a": a,
                  "_p2": _p2, "_p3": _p3, "_rad": _rad}
            with P.Db.PyDbTrans() as tr:
                ns["tr"] = tr
                exec(code, ns)
                e = ns.get("e")
                if e is None:
                    raise RuntimeError("未生成实体变量 e")
                # 样板可以自己先入库再写属性（cad_ops 的约定）：
                # 把 ns['added'] 置 True 表示已入库，这里不重复 addEntity
                if not ns.get("added"):
                    if not bool(tr.addEntity(e)):
                        raise RuntimeError("addEntity 返回 False")
                h = str(e.handle())
                try:
                    cls = str(e.isA())
                except Exception:
                    cls = type(e).__name__
                bbox = None
                try:
                    _lo, _hi = e.bounds()
                    bbox = [[round(float(_lo.x), 3), round(float(_lo.y), 3)],
                            [round(float(_hi.x), 3), round(float(_hi.y), 3)]]
                except Exception as ex:
                    bbox = "err:" + repr(ex)[:80]
            RES_ENT[kind] = {"ok": True, "cand": idx, "handle": h,
                             "class": cls, "bbox": bbox}
            return
        except Exception as ex:
            last = repr(ex)[:260]
            RES_ENT[kind] = {"ok": False, "cand": idx, "err": last}
'''

INNER_GEO = '''
def _eq(got, want, tol):
    if isinstance(want, (list, tuple)):
        if not isinstance(got, (list, tuple)) or len(got) != len(want):
            return False
        return all(abs(float(g) - float(w)) <= tol for g, w in zip(got, want))
    return abs(float(got) - float(want)) <= tol


def run_geo(name, spec):
    try:
        ns = {"P": P, "Db": Db, "Ge": Ge, "math": math}
        exec(spec["build"], ns)
        r = ns["r"]
        ok = _eq(r, spec["expect"], spec.get("tol", 1e-6))
        RES_GEO[name] = {"ok": ok, "got": r, "expect": spec["expect"],
                         "task": spec["task"]}
    except Exception as ex:
        RES_GEO[name] = {"ok": False, "err": repr(ex)[:260], "task": spec["task"]}
'''

INNER_TAIL = '''
KINDS = __KINDS__
GEOS = __GEOS__
for _k in KINDS:
    run_kind(_k, ENTITY[_k])
for _g in GEOS:
    run_geo(_g, GEO[_g])

# ---- 选择集 / 读取链验证：遍历图元 + 按句柄打开 ----
RES_SEL = {}
try:
    _ids = DB.objectIds("AcDbEntity")
    RES_SEL["objectIds"] = {"ok": len(_ids) > 0, "count": len(_ids)}
except Exception as _ex:
    RES_SEL["objectIds"] = {"ok": False, "err": repr(_ex)[:200]}
_any_h = ""
for _v in RES_ENT.values():
    if _v.get("ok") and _v.get("handle"):
        _any_h = _v["handle"]
        break
if _any_h:
    # 实测：没有 openObjectByIdString / openObject，唯一通道是事务内 tr.openObject(id, 0)
    # （0=kForRead；写模式必须显式传 1，绝不省略）
    try:
        _want = _any_h.upper().lstrip("0")
        _o = None
        with P.Db.PyDbTrans() as _tr:
            for _eid in DB.objectIds("AcDbEntity"):
                _c = _tr.openObject(_eid, 0)
                if _c is not None and str(_c.handle()).upper().lstrip("0") == _want:
                    _o = _c
                    break
            if _o is None:
                raise RuntimeError("按句柄未找到 " + _any_h)
            RES_SEL["by_handle"] = {"ok": True, "handle": str(_o.handle()),
                                    "class": str(_o.className()), "layer": str(_o.layer())}
    except Exception as _ex:
        RES_SEL["by_handle"] = {"ok": False, "handle": _any_h, "err": repr(_ex)[:200]}
# 按类名取选择集（cad_query 的语义）
try:
    with P.Db.PyDbTrans() as _tr2:
        _n = 0
        for _eid in DB.objectIds("AcDbLine"):
            if _tr2.openObject(_eid, 0) is not None:
                _n += 1
        RES_SEL["by_class"] = {"ok": _n > 0, "class": "AcDbLine", "count": _n}
except Exception as _ex:
    RES_SEL["by_class"] = {"ok": False, "err": repr(_ex)[:200]}

ok_e = sum(1 for v in RES_ENT.values() if v.get("ok"))
ok_g = sum(1 for v in RES_GEO.values() if v.get("ok"))
ok_s = sum(1 for v in RES_SEL.values() if v.get("ok"))
OUT = {"entities": RES_ENT, "geometry": RES_GEO, "select": RES_SEL,
       "summary": {"ent_ok": ok_e, "ent_total": len(RES_ENT),
                   "geo_ok": ok_g, "geo_total": len(RES_GEO),
                   "sel_ok": ok_s, "sel_total": len(RES_SEL)}}
with open(r"__OUT__", "w", encoding="utf-8") as f:
    json.dump(OUT, f, ensure_ascii=False, indent=1)
'''


def make_inner(kinds, geos):
    body = INNER_HEAD + INNER_RUN_KIND + INNER_GEO + INNER_TAIL
    # proto 定义需要一并带进脚本（子进程没有我们的 sys.path）
    ent_src = "ENTITY = " + repr({k: ENTITY[k] for k in kinds}) + "\n"
    geo_src = "GEO = " + repr({g: GEO[g] for g in geos}) + "\n"
    body = body.replace("__KINDS__", repr(kinds)).replace("__GEOS__", repr(geos))
    body = body.replace('RES_ENT = {}\nRES_GEO = {}',
                        ent_src + geo_src + "\nRES_ENT = {}\nRES_GEO = {}")
    body = body.replace("__OUT__", OUT_JSON.replace("\\", "/"))
    return body


def run(kinds=None, geos=None):
    kinds = kinds or proto.kinds()
    geos = geos or sorted(GEO)
    if not os.path.isfile(TEMPLATE):
        raise SystemExit("模板图不存在：" + TEMPLATE)
    shutil.copyfile(TEMPLATE, WORK)
    py = os.path.join(TMP, "proto_inner.py")
    scr = os.path.join(TMP, "proto_inner.scr")
    with open(py, "w", encoding="utf-8") as f:
        f.write(make_inner(kinds, geos))
    with open(scr, "w", encoding="utf-8") as f:
        f.write('(setvar "SECURELOAD" 0)\n')
        f.write('(arxload "%s")\n' % ENGINE.replace("\\", "/"))
        f.write('(command "PYRUNFILE" "%s")\n' % py.replace("\\", "/"))
    if os.path.exists(OUT_JSON):
        os.remove(OUT_JSON)
    proc = subprocess.run([CONSOLE, "/i", WORK, "/s", scr],
                          capture_output=True, timeout=300)
    if not os.path.exists(OUT_JSON):
        print("内层脚本没有写出结果（exit=%s）" % proc.returncode)
        print(proc.stdout.decode("utf-8", "ignore")[-800:])
        return None
    with open(OUT_JSON, encoding="utf-8") as f:
        return json.load(f)


def report(res):
    if not res:
        return 1
    print("==== 图元样板 ====")
    for k, v in sorted(res["entities"].items()):
        if v.get("ok"):
            print("  OK   %-12s %-22s handle=%s bbox=%s"
                  % (k, v.get("class"), v.get("handle"), v.get("bbox")))
        else:
            print("  FAIL %-12s %s" % (k, v.get("err")))
    print("==== 几何运算 ====")
    for k, v in sorted(res["geometry"].items()):
        if v.get("ok"):
            print("  OK   %-16s %s -> %s" % (k, v.get("task"), v.get("got")))
        else:
            print("  FAIL %-16s %s | got=%s expect=%s"
                  % (k, v.get("err") or v.get("task"), v.get("got"), v.get("expect")))
    print("==== 选择集 / 读取链 ====")
    for k, v in sorted((res.get("select") or {}).items()):
        if v.get("ok"):
            print("  OK   %-14s %s" % (k, v.get("count") if k == "objectIds"
                                     else (v.get("handle"), v.get("class"), v.get("layer"))))
        else:
            print("  FAIL %-14s %s" % (k, v.get("err")))
    s = res["summary"]
    print("---- 图元 %d/%d  几何 %d/%d  选择 %d/%d ----"
          % (s["ent_ok"], s["ent_total"], s["geo_ok"], s["geo_total"],
             s.get("sel_ok", 0), s.get("sel_total", 0)))
    return 0


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    kinds = args if args else None
    r = run(kinds)
    if r is not None:
        with open(OUT_JSON, "w", encoding="utf-8") as f:
            json.dump(r, f, ensure_ascii=False, indent=1)
    sys.exit(report(r))
