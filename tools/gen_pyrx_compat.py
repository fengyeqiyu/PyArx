"""从审计 CSV 生成 PyRx 兼容别名层（P0）。

输出：
  PyArxCore/PyArxCompat.h
  PyArxCore/PyArxCompat.cpp

只追加别名，不改动任何现有绑定；PyRx 名已存在或 PyArx 源名不存在时静默跳过。
数据源：docs_notes/api_parity/{name_mismatch,wrong_module,aced_mapping}.csv
"""
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import api_parity_audit as audit

ARX_DIR = r"G:\PyArx\PyArxCore"
OUT_CPP = os.path.join(ARX_DIR, "PyArxCompat.cpp")
OUT_H = os.path.join(ARX_DIR, "PyArxCompat.h")
CSV_DIR = audit.OUT_DIR

SUBMOD = {"PyDb": "Db", "PyGe": "Ge", "PyEd": "Ed",
          "PyGi": "Gi", "PyRx": "Rx", "PyAp": "Ap"}
ORDER = ["Db", "Ge", "Ed", "Gi", "Rx", "Ap"]


def rows(name):
    with open(os.path.join(CSV_DIR, name), encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def core_methods(pyi_name):
    p = os.path.join(audit.PYRX_DIR, pyi_name)
    if not os.path.exists(p):
        return []
    classes, _, _ = audit.parse_pyi(p)
    return list(classes.get("Core", {}).get("members", {}).keys())


ed_core_names = core_methods("PyEd.pyi")
db_core_names = core_methods("PyDb.pyi")
ed_core = {n.lower() for n in ed_core_names}
db_core = {n.lower() for n in db_core_names}

# ---- 1 类名别名 ----
name_rows = rows("name_mismatch.csv")
by_mod = {}
for r in name_rows:
    sub = SUBMOD.get(r["module"])
    if not sub:
        continue
    by_mod.setdefault(sub, []).append((r["pyrx_name"], r["pyarx_registered_name"]))

# ---- 2 跨子模块（同名但挂错地方） ----
wrong_rows = []
for r in rows("wrong_module.csv"):
    sub = SUBMOD.get(r["module"])
    if sub:
        wrong_rows.append((sub, r["pyrx_class"], r["actual_module"], r["pyarx_class"]))

# ---- 3 Core 静态类：PyRx 的 Ed.Core / Db.Core 成员 -> PyArx 侧同名/aced* 转发 ----
# 依据 PyRx pyi 的 Core 成员表直接生成；源候选 = {短名, aced+短名} × {本模块, Core}，
# 运行时 hasattr 兜底，缺则跳过（不猜 PyArx 命名，也不依赖 aced_mapping 的 status）。
core_rows = []
for name in ed_core_names + db_core_names:
    if name.startswith("__"):
        continue
    target = "Ed" if name in ed_core_names else "Db"
    short = name
    core_rows.append((target, short, "ed" if target == "Ed" else "db"))

# ---- 生成 ----
head = '''#pragma once
// PyRx 兼容层（P0）入口声明。实现见 PyArxCompat.cpp（自动生成）。
#include "stdafx.h"

void bind_PyArxCompat(pym& m);
'''

lines = []
lines.append('''// ===========================================================================
// PyArxCompat.cpp —— PyRx 接口兼容层（P0：命名与承载别名）
//
// 由 tools/gen_pyrx_compat.py 从审计结果自动生成，请勿手改。
// 原则：只追加别名，不修改、不删除任何现有绑定；源名不存在或目标名已存在时静默跳过。
// 数据源：docs_notes/api_parity/{name_mismatch,wrong_module,aced_mapping}.csv
// ===========================================================================
#include "stdafx.h"
#include "PyDbDatabase.h"
#include "PyApDocument.h"
#include "PyErrorStatusException.h"
#include "PyArxCompat.h"

namespace
{
    int g_alias = 0;

    // 在持有 GIL 的作用域内调用 Python 函数（PyRx 的 Ap.using_scope 语义）
    py::object CallWithGil(const py::function& f, py::args a, py::kwargs k)
    {
        py::gil_scoped_acquire gil;
        PyObject* r = PyObject_Call(f.ptr(), a.ptr(), k.ptr());
        if (!r)
            throw py::error_already_set();
        return py::reinterpret_steal<py::object>(r);
    }

    // 同一子模块内：把 PyArx 注册名 from 挂成 PyRx 短名 to
    void A(py::module_& m, const char* to, const char* from)
    {
        if (!py::hasattr(m, from) || py::hasattr(m, to))
            return;
        py::object src = py::reinterpret_steal<py::object>(
            PyObject_GetAttrString(m.ptr(), from));
        if (PyObject_SetAttrString(m.ptr(), to, src.ptr()) == 0)
            ++g_alias;
    }

    // 跨子模块：把 src 子模块的 from 挂成 m 的 to
    void A2(py::module_& m, const char* to, py::module_ src, const char* from)
    {
        if (!py::hasattr(src, from) || py::hasattr(m, to))
            return;
        py::object o = py::reinterpret_steal<py::object>(
            PyObject_GetAttrString(src.ptr(), from));
        if (PyObject_SetAttrString(m.ptr(), to, o.ptr()) == 0)
            ++g_alias;
    }
}

void bind_PyArxCompat(pym& m)
{
''' + "".join(
    '    py::module_ %s = m.attr("%s").cast<py::module_>();\n' % (s.lower(), s)
    for s in ORDER) + '    py::module_ coremod = m.attr("Core").cast<py::module_>();\n')

for sub in ORDER:
    if sub not in by_mod:
        continue
    lines.append('\n    // ---- %s：类名 -> PyRx 短名（%d 条）----\n' % (sub, len(by_mod[sub])))
    for to, frm in by_mod[sub]:
        lines.append('    A(%s, "%s", "%s");\n' % (sub.lower(), to, frm))

if wrong_rows:
    lines.append('\n    // ---- 跨子模块枚举（同名但 PyArx 挂错位置，%d 条）----\n' % len(wrong_rows))
    for sub, to, src, frm in wrong_rows:
        var = src.lower() if src in ORDER else None
        if var:
            lines.append('    A2(%s, "%s", %s, "%s");\n' % (sub.lower(), to, var, frm))
        else:
            # 目标子模块不是常规六模块（如 Enum），用动态取模块
            lines.append('    A2(%s, "%s", m.attr("%s").cast<py::module_>(), "%s");\n'
                         % (sub.lower(), to, src, frm))

if core_rows:
    lines.append('\n    // ---- Core 静态类：aced* -> PyRx 短名（%d 条）----\n' % len(core_rows))
    lines.append('    py::module_ edcore = ed.def_submodule("Core", "PyRx 兼容：aced* 短名转发（Ed）");\n')
    lines.append('    py::module_ dbcore = db.def_submodule("Core", "PyRx 兼容：aced* 短名转发（Db）");\n')
    CORE_OVERRIDES = {"cmds": "acedCmd"}   # PyRx 名 -> PyArx 真实名（探查得出）
    for target, short, srcv in core_rows:
        tgt = "edcore" if target == "Ed" else "dbcore"
        cap = short[:1].upper() + short[1:]
        cands = [short, "aced" + cap, "acdb" + cap]
        ov = CORE_OVERRIDES.get(short.lower())
        if ov:
            cands.insert(0, ov)
        for cand in cands:
            for s in (srcv, "coremod"):
                lines.append('    A2(%s, "%s", %s, "%s");\n' % (tgt, short, s, cand))

lines.append('''
    // ---- 模块级函数（PyRx 有、PyArx 缺）----
    if (!py::hasattr(db, "workingDb"))
        db.def("workingDb", []() {
            return PyDbDatabase(acdbHostApplicationServices()->workingDatabase(), false);
        }, "PyRx 兼容：当前工作数据库（不拥有）");

    if (!py::hasattr(db, "setWorkingDb"))
        db.def("setWorkingDb", [](PyDbDatabase& d) {
            acdbHostApplicationServices()->setWorkingDatabase(d.impObj());
        }, "PyRx 兼容：设置工作数据库");

    // PyRx 短名：Db.ObjectId / Db.Handle（原生 AcDbObjectId / AcDbHandle 直接绑定）
    A(db, "ObjectId", "PyDbObjectId");
    A(db, "Handle", "PyDbHandle");

    // ---- Ap：SDK 级封装（对应 PyAcAp.cpp 的 curPyDoc / PyUsingDecorator）----
    if (!py::hasattr(ap, "curDoc"))
        ap.def("curDoc", []() {
            AcApDocument* d = ::curDoc();
            if (d == nullptr)
                throw PyErrorStatusException(Acad::eNoDocument, AcString(L"Ap.curDoc"));
            return PyApDocument(d);
        }, "PyRx 兼容：当前上下文文档（acDocManager->curDocument()），无文档抛 eNoDocument");

    if (!py::hasattr(ap, "using_scope"))
        ap.def("using_scope", []() {
            return py::cpp_function([](py::function f) {
                return py::cpp_function([f](py::args a, py::kwargs k) {
                    return CallWithGil(f, a, k);
                });
            });
        }, "PyRx 兼容：装饰器，保证被装饰函数在持有 GIL 的作用域内执行");

    acutPrintf(L"[PyArx] PyRxCompat: %d aliases\\n", g_alias);
}
''')

with open(OUT_H, "w", encoding="utf-8-sig") as f:
    f.write(head)
with open(OUT_CPP, "w", encoding="utf-8-sig") as f:
    f.write("".join(lines))

print("生成 %s" % OUT_CPP)
print("  类名别名 %d / 跨模块 %d / Core %d"
      % (len(name_rows), len(wrong_rows), len(core_rows)))
