# -*- coding: utf-8 -*-
"""
PyArxCore 与 PyRx 接口对齐审计（机器化逐项比对）

数据源：
  A) PyRx 权威接口面：G:\\PyRx-main\\pyrx\\Py*.pyi（boost-python 生成的桩文件）
     以及纯 Python 层：pyrx\\*.py、pyrx\\db|ed|ge|ap|utils\\*.py
  B) PyArxCore 实际导出：G:\\PyArx\\PyArxCore\\*.cpp 里的 py::class_ / py::enum_ /
     .def / .def_prop_ro / .value / m.def 绑定，按 bind_PyXxx 归入子模块

输出到 docs_notes\\api_parity\\：
  classes.csv          每个 PyRx 类 → PyArxCore 类 的匹配与成员差
  missing_classes.csv  PyRx 有、PyArxCore 没有的类
  missing_members.csv  类已存在但成员缺失（含枚举成员）
  extra_members.csv    PyArxCore 多出来的成员
  name_mismatch.csv    类已存在但注册名与 PyRx 不一致（影响 drop-in）
  module_funcs.csv     模块级函数对齐状态
  py_layer.csv         PyRx 纯 Python 层接口 vs PyArx
"""
from __future__ import annotations

import csv
import os
import re
from collections import defaultdict

PYRX_DIR = r"G:\PyRx-main\pyrx"
ARX_DIR = r"G:\PyArx\PyArxCore"
OUT_DIR = r"G:\PyArx\docs_notes\api_parity"

# PyRx 顶层扩展模块 -> PyArxCore 子模块
MODULE_MAP = {
    "PyDb": "Db",
    "PyGe": "Ge",
    "PyEd": "Ed",
    "PyGi": "Gi",
    "PyRx": "Rx",
    "PyAp": "Ap",
}
# 无对应子模块的 PyRx 扩展模块（只统计，不计入对齐分母）
EXTRA_MODULES = ["PyGs", "PySm", "PyPl", "PyAx", "PyBr", "PyBrx", "PyBrxBim", "PyBrxCv"]

# bind_PyXxx 函数名前缀 -> PyArxCore 子模块
BIND_PREFIX = [
    ("bind_PyDb", "Db"), ("pybind11_init_PyDb", "Db"),
    ("bind_PyGe", "Ge"), ("bind_PyGi", "Gi"),
    ("bind_PyEd", "Ed"), ("bind_PyRx", "Rx"),
    ("bind_PyAp", "Ap"), ("bind_PyApp", "Ap"), ("bind_PyPaletteSet", "Ap"),
    ("bind_PyStatusBar", "Ap"), ("bind_PyApDocument", "Ap"), ("bind_PyApDocManager", "Ap"),
    ("bind_PyEnum", "Enum"), ("bind_PyCore", "Core"), ("bind_PyUtility", "Utility"),
    ("bind_PyDebug", "Debug"),
]

# PyRx 名无法用「Py/Ac + 模块标签 + 名」推导时的显式映射
ALIAS = {
    ("PyDb", "Vertex2d"): "PyDb2dVertex",
    ("PyDb", "Polyline2d"): "PyDb2dPolyline",
    ("PyDb", "Polyline3d"): "PyDb3dPolyline",
    ("PyDb", "Polyline3dVertex"): "PyDb3dPolylineVertex",
    ("PyDb", "LineAngularDimension2"): "PyDb2LineAngularDimension",
    ("PyDb", "Point3AngularDimension"): "PyDb3PointAngularDimension",
    ("PyDb", "Transaction"): "PyDbTrans",
    ("PyDb", "AttributeReference"): "PyDbAttribute",
    ("PyDb", "DatabaseSummaryInfo"): "PyDbSummaryInfo",
    ("PyDb", "DxfCode"): "PyDxfCode",
    ("PyDb", "Color"): "PyColor",
    ("PyDb", "SubentId"): "PyDbSubentId",
    ("PyDb", "Placeholder"): "PyDbPlaceHolder",
    ("PyAp", "Application"): "PyApp",
    ("PyAp", "DocManager"): "PyApDocManager",
    ("PyAp", "Document"): "PyApDocument",
    ("PyAp", "PaletteSet"): "PyPaletteSet",
    ("PyEd", "Jig"): "AcEdJig",
    ("PyEd", "DrawJig"): "AcEdJig",
    ("PyEd", "Core"): "PyEdCore",
    ("PyRx", "RxObject"): "PyRxObject",
    ("PyRx", "RxClass"): "PyRxClass",
}

IGNORE_CLASS = re.compile(r"^(PyApp|PyStatusBar|PyPalette|_)$")

# boost-python 协议层成员：PyArxCore 由 pybind11 原生实现或有等价行为，不计入缺口
PROTOCOL = {"__reduce__", "__repr__", "__str__", "__hash__", "__eq__", "__ne__", "__len__",
            "__iter__", "__next__", "__getitem__", "__setitem__", "__contains__"}


def member_set(members):
    return {n for n in members if not (n.startswith("__") and n.endswith("__") and n in PROTOCOL)}


FN_DEF_PAT = r"(bind_\w+|register\w+|pybind11_init_PyDb)"
FN_CALL_PAT = r"\b(bind_\w+|register\w+|pybind11_init_PyDb)\s*\("


def parse_pyi_aced(path):
    """从 PyRx 桩的 docstring 里抽 acedXxx 原始名 -> (class, method)。
    PyRx 把 acedXxx 改短名挂在 Core 静态类上，docstring 仍保留 aced 原名。"""
    with open(path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    mapping = {}
    cur_class, cur_meth = None, None
    in_doc = False
    for raw in lines:
        line = raw.strip()
        if line.startswith("class ") and (len(raw) - len(raw.lstrip())) == 0:
            cur_class = re.match(r"class\s+(\w+)", line).group(1)
            cur_meth = None
            continue
        if line.startswith("def "):
            cur_meth = re.match(r"def\s+(\w+)", line).group(1)
            continue
        quotes = line.count('"""')
        if in_doc:
            for nm in re.findall(r"aced([A-Za-z0-9_]+)\(", line):
                mapping.setdefault("aced" + nm, (cur_class, cur_meth))
            if quotes:
                in_doc = False
            continue
        if quotes == 1:
            in_doc = True
        elif quotes >= 2:
            for nm in re.findall(r"aced([A-Za-z0-9_]+)\(", line):
                mapping.setdefault("aced" + nm, (cur_class, cur_meth))
    return mapping


def strip_docstrings(text: str) -> str:
    return re.sub(r'"""(?:.|\n)*?"""', '""', text)


# ---------------------------------------------------------------- PyRx 桩解析
def parse_pyi(path):
    with open(path, encoding="utf-8") as f:
        text = strip_docstrings(f.read())
    classes, funcs, attrs = {}, set(), set()
    cur, decorators = None, []
    for raw in text.splitlines():
        if not raw.strip():
            continue
        indent = len(raw) - len(raw.lstrip())
        line = raw.strip()
        if indent == 0:
            decorators = []
            m = re.match(r"class\s+(\w+)\s*(?:\(([^)]*)\))?", line)
            if m:
                cur = m.group(1)
                classes[cur] = {"base": (m.group(2) or "").strip(), "members": {}}
                continue
            cur = None
            m = re.match(r"def\s+(\w+)\s*\(", line)
            if m:
                funcs.add(m.group(1))
                continue
            m = re.match(r"(\w+)\s*(?::|=)", line)
            if m:
                attrs.add(m.group(1))
            continue
        if cur is None:
            continue
        if line.startswith("@"):
            decorators.append(line)
            continue
        m = re.match(r"def\s+(\w+)\s*\(", line)
        if m:
            kind = "property" if any("property" in d for d in decorators) else "method"
            cur_members = classes[cur]["members"]
            # overload 去重：同名保留一次
            cur_members.setdefault(m.group(1), kind)
            decorators = []
            continue
        m = re.match(r"(\w+)\s*(?::|=)", line)
        if m:
            classes[cur]["members"].setdefault(m.group(1), "attr")
        decorators = []
    return classes, funcs, attrs


# --------------------------------------------------------------- C++ 绑定解析
CLASS_RE = re.compile(r"py::class_<\s*(?:py::\w+\s*,\s*)?([\w:]+)\s*(?:,\s*([\w:]+)\s*)?>")
ENUM_RE = re.compile(r"py::enum_<\s*([\w:]+)\s*(?:,\s*([\w:]+)\s*)?>")
DEF_RE = re.compile(
    r"\.\s*def(?:_static|_readonly|_readwrite|_prop_ro|_prop_rw|_static_prop_ro|_static_prop_rw)?"
    r"\s*\(\s*\"([^\"]+)\""
)
VALUE_RE = re.compile(r"\.\s*value\s*\(\s*\"([^\"]+)\"")
BIND_RE = re.compile(
    r"void\s+(bind_\w+|pybind11_init_PyDb)\s*\(\s*(?:pym|py::module_?|pybind11::module_?)\s*&\s*\w+\s*\)"
)
NAME_LITERAL_RE = re.compile(r"\"([A-Za-z_]\w*)\"")


def module_of(bind_name, fallback_file):
    for pre, mod in BIND_PREFIX:
        if bind_name.startswith(pre):
            return mod
    return "?" + fallback_file.replace(".cpp", "")


def parse_cpp_dir(root):
    classes, enums = {}, {}
    module_funcs = defaultdict(set)
    issues = []

    sources = {}
    for fn in sorted(os.listdir(root)):
        if fn.endswith(".cpp"):
            with open(os.path.join(root, fn), encoding="utf-8", errors="replace") as f:
                sources[fn] = strip_docstrings(f.read())

    # ---- 绑定函数 -> 子模块：先按命名前缀定性，再沿 bind_X(...) 调用链向下传播 ----
    # 部分 per-class binder 不带模块前缀（bind_AcEdJig / bind_PyColor / bind_TvList ...），
    # 由 PyRx 对应模块反推，避免这些类被误判成「整类缺失」。
    BIND_GUESS = [
        (r".*ErrorStatus|.*DxfCode|.*Enums$|.*CommandFlags|.*ResbufType|.*OptionFlags|.*LoftNormalOption",
         "Enum"),
        (r".*Jig|.*DragStyle|.*CursorType", "Ed"),
        (r".*Resbuf|.*TvList|.*TypedValue|.*DxfValue|.*Color\b", "Db"),
    ]
    func_module = {}
    for fn, text in sources.items():
        for name in re.findall(r"void\s+" + FN_DEF_PAT + r"\s*\(", text):
            for pre, mod in BIND_PREFIX:
                if name.startswith(pre):
                    func_module.setdefault(name, mod)
            if name not in func_module:
                for pat, mod in BIND_GUESS:
                    if re.match(pat, name) or re.match(pat, fn):
                        func_module[name] = mod
                        break
    calls = {}
    for fn, text in sources.items():
        for name in re.findall(r"void\s+" + FN_DEF_PAT + r"\s*\(", text):
            calls.setdefault(name, set())
    for fn, text in sources.items():
        # 函数体范围
        defs = [(m.start(), m.group(1)) for m in
                re.finditer(r"void\s+" + FN_DEF_PAT + r"\s*\(", text)]
        defs.append((len(text), None))
        for i in range(len(defs) - 1):
            body = text[defs[i][0]:defs[i + 1][0]]
            for callee in re.findall(FN_CALL_PAT, body):
                if callee != defs[i][1]:
                    calls.setdefault(defs[i][1], set()).add(callee)
    changed = True
    while changed:
        changed = False
        for caller, callees in calls.items():
            parent = func_module.get(caller)
            if parent and parent in ("Db", "Ge", "Ed", "Gi", "Rx", "Ap", "Enum", "Core",
                                     "Utility", "Debug"):
                for c in callees:
                    if c not in func_module:
                        func_module[c] = parent
                        changed = True

    def module_of(bind_name, fallback_file):
        m = func_module.get(bind_name)
        if m in ("Db", "Ge", "Ed", "Gi", "Rx", "Ap", "Enum", "Core", "Utility", "Debug"):
            return m
        return "?" + fallback_file.replace(".cpp", "")

    for fn, text in sources.items():
        binds = [(m.start(), m.group(1)) for m in
                 re.finditer(r"void\s+" + FN_DEF_PAT + r"\s*\(\s*(?:pym|py::module_?|pybind11::module_?)\s*&",
                             text)]
        marks = [m.start() for m in re.finditer(r"py::(class_|enum_)<", text)] + [len(text)]

        def mod_at(pos):
            cur = None
            for bpos, bname in binds:
                if bpos <= pos:
                    cur = bname
                else:
                    break
            return module_of(cur or "", fn)

        for i in range(len(marks) - 1):
            start, end = marks[i], marks[i + 1]
            chunk = text[start:end]
            head_end = chunk.find(";")
            head = chunk[:head_end if head_end > 0 else 400]
            is_enum = chunk.startswith("py::enum_")
            typed = ENUM_RE.match(chunk) if is_enum else CLASS_RE.match(chunk)
            if not typed:
                issues.append((fn, "注册头无法解析: " + chunk[:100].replace("\n", " ")))
                continue
            # 注册名字符串必须在类型模板之后
            nm = NAME_LITERAL_RE.search(chunk, typed.end())
            if not nm:
                issues.append((fn, "注册名缺失: " + head[:100].replace("\n", " ")
                               + " | type=" + typed.group(1)))
                continue
            name = nm.group(1)
            cpp = typed.group(1).split("::")[-1]
            base = (typed.group(2) or "").strip()
            members = {}
            if is_enum:
                for vm in VALUE_RE.finditer(chunk):
                    members.setdefault(vm.group(1), "enum")
            else:
                for dm in DEF_RE.finditer(chunk):
                    members.setdefault(dm.group(1), "member")
                # 模块级 m.def("x") 不算类成员（否则 curDb/lockDocument 会被记进 Database）
                for nm2 in re.findall(r"\bm\.\s*def\(\s*\"([^\"]+)\"", chunk):
                    members.pop(nm2, None)
                # PyWrapper_BIND 宏一次性挂 desc/cast/className/appName/dxfName
                if "PyWrapper_BIND(" in chunk:
                    for nm2 in ("desc", "cast", "className", "appName", "dxfName"):
                        members.setdefault(nm2, "macro")
                if "py::init<" in chunk or "py::init(" in chunk:
                    members.setdefault("__init__", "ctor")
            rec = {"cpp": cpp, "base": base, "members": members,
                   "module": mod_at(start), "file": fn}
            (enums if is_enum else classes)[name] = rec

        for dm in re.finditer(r"\bm\.\s*def\(\s*\"([^\"]+)\"", text):
            module_funcs[mod_at(dm.start())].add(dm.group(1))
    return classes, enums, module_funcs, issues


def short(t: str) -> str:
    t = t.split("::")[-1]
    t = re.sub(r"^Py(?:Db|Ge|Ed|Gi|Rx|Ap|App)", "", t)
    t = re.sub(r"^Ac(?:Db|Ge|Ed|Gi|Rx|Ap|Cm)", "", t)
    t = re.sub(r"^Py", "", t) if t.startswith("PyDb") else t
    return t


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    pyrx = {}
    for mod in list(MODULE_MAP) + EXTRA_MODULES:
        p = os.path.join(PYRX_DIR, mod + ".pyi")
        if os.path.exists(p):
            pyrx[mod] = parse_pyi(p)

    arx, arx_enums, arx_funcs, issues = parse_cpp_dir(ARX_DIR)
    all_arx = {**arx, **arx_enums}     # 枚举也参与匹配

    def candidates(mod, cls):
        out = []
        if (mod, cls) in ALIAS:
            out.append(ALIAS[(mod, cls)])
        tag = mod[2:]
        for pre in ("Py", "Ac"):
            out.append(pre + tag + cls)
        out.append("Py" + cls)      # PyRxObject / PyEdJig 这类无模块标签的包装名
        out.append(cls)
        return [c for c in out if c]

    # 反向索引：短名 / Ac 类型名 -> 注册名，用于跨命名风格匹配
    by_short, by_cpp = defaultdict(list), defaultdict(list)
    for name, rec in all_arx.items():
        by_short[short(name)].append(name)
        by_cpp[short(rec["cpp"])].append(name)

    def pick(names, sub):
        """只接受挂在目标子模块下的同名类；跨子模块只记提示，不算对齐"""
        for n in names:
            if all_arx[n]["module"] == sub:
                return n
        return None

    def match_class(mod, cls):
        sub = MODULE_MAP[mod]
        for c in candidates(mod, cls):
            if c in all_arx:
                return c, "exact" if c == cls else "renamed"
        for table in (by_short, by_cpp):
            hit = pick(table.get(cls, []), sub)
            if hit:
                return hit, "renamed"
        return None, None

    def elsewhere(cls):
        names = by_short.get(cls, []) + by_cpp.get(cls, [])
        return all_arx[names[0]]["module"] + "." + names[0] if names else ""

    rows, missing_classes, name_mismatch, moved = [], [], [], []
    matched, used = {}, set()

    def ac_expected(pyrx_base):
        """PyRx 基类 -> 期望的 ARX C++ 类型名，如 PyDb.DbObject -> AcDbObject"""
        if not pyrx_base or pyrx_base.startswith("_"):
            return ""
        parts = pyrx_base.split(".")
        if len(parts) == 2:
            return "Ac" + parts[0][2:] + parts[1]
        return "Ac" + pyrx_base

    ALIAS_BASE = {"PyRxObject": "AcRxObject", "PyRxClass": "AcRxClass"}

    for mod in MODULE_MAP:
        if mod not in pyrx:
            continue
        sub = MODULE_MAP[mod]
        for cls, info in sorted(pyrx[mod][0].items()):
            hit, how = match_class(mod, cls)
            if not hit:
                missing_classes.append([mod, cls,
                                        "enum" if info["base"].endswith("_BoostPythonEnum") else "class",
                                        info["base"], len(info["members"]), elsewhere(cls)])
                continue
            matched[(mod, cls)] = hit
            used.add(hit)
            if all_arx[hit]["module"] != sub:
                moved.append([mod, cls, hit, all_arx[hit]["module"]])
            if hit != cls:
                name_mismatch.append([mod, cls, hit, len(info["members"]), how])
            am, pm = all_arx[hit]["members"], info["members"]
            am_f, pm_f = member_set(am), member_set(pm)
            arx_base = all_arx[hit]["base"].split("::")[-1]
            # 基类同名判定：PyRx 基类名 -> PyArxCore 包装类名
            if not info["base"] or info["base"].startswith("_"):
                base_match = "n/a"
            else:
                parts = info["base"].split(".")
                bmod = parts[0] if len(parts) == 2 else "PyDb"
                bcls = parts[-1]
                base_match = "yes" if arx_base and arx_base in candidates(bmod, bcls) else "no"
            rows.append({
                "module": mod, "pyrx_class": cls, "pyarx_class": hit,
                "name_aligned": "yes" if hit == cls else "no",
                "ac_name": all_arx[hit]["cpp"],
                "pyrx_base": info["base"],
                "pyarx_base": arx_base,
                "base_match": base_match,
                "pyrx_members": len(pm_f), "pyarx_members": len(am_f),
                "missing": len(pm_f - am_f), "extra": len(am_f - pm_f),
                "pyarx_file": all_arx[hit]["file"],
            })

    def w(name, header, data):
        with open(os.path.join(OUT_DIR, name), "w", encoding="utf-8-sig", newline="") as f:
            cw = csv.writer(f)
            cw.writerow(header)
            cw.writerows(data)

    w("classes.csv", ["module", "pyrx_class", "pyarx_class", "name_aligned", "ac_name", "pyrx_base",
                      "pyarx_base", "base_match", "pyrx_members", "pyarx_members", "missing", "extra",
                      "pyarx_file"],
      [[r[k] for k in ("module", "pyrx_class", "pyarx_class", "name_aligned", "ac_name", "pyrx_base",
                       "pyarx_base", "base_match", "pyrx_members", "pyarx_members", "missing", "extra",
                       "pyarx_file")] for r in rows])
    w("missing_classes.csv", ["module", "class", "kind", "base", "pyrx_members", "note"], missing_classes)
    w("name_mismatch.csv", ["module", "pyrx_name", "pyarx_registered_name", "pyrx_members", "how"],
      name_mismatch)
    w("wrong_module.csv", ["module", "pyrx_class", "pyarx_class", "actual_module"], moved)

    missing_members, extra_members = [], []
    for (mod, cls), hit in matched.items():
        pm = member_set(pyrx[mod][0][cls]["members"])
        am = member_set(all_arx[hit]["members"])
        for nm in sorted(pm):
            if nm not in am:
                missing_members.append([mod, cls, hit, nm,
                                        pyrx[mod][0][cls]["members"].get(nm, "member")])
        for nm in sorted(am - pm):
            extra_members.append([mod, cls, hit, nm])
    w("missing_members.csv", ["module", "pyrx_class", "pyarx_class", "member", "kind"], missing_members)
    w("extra_members.csv", ["module", "pyrx_class", "pyarx_class", "member"], extra_members)

    arx_only = [[n, i["base"], len(i["members"]), i["module"], i["file"]]
                for n, i in sorted(arx.items()) if n not in used]
    w("pyarx_only_classes.csv", ["pyarx_class", "base", "members", "module", "file"], arx_only)

    module_rows = []
    for mod in MODULE_MAP:
        if mod not in pyrx:
            continue
        sub = MODULE_MAP[mod]
        have = arx_funcs.get(sub, set())
        for f in sorted(pyrx[mod][1]):
            module_rows.append([mod, f, "present" if f in have else "missing"])
    w("module_funcs.csv", ["module", "func", "status"], module_rows)

    # 纯 Python 层
    py_rows = []
    py_mods = {
        "pyrx/__init__.py": os.path.join(PYRX_DIR, "__init__.py"),
        "pyrx/config.py": os.path.join(PYRX_DIR, "config.py"),
        "pyrx/console.py": os.path.join(PYRX_DIR, "console.py"),
        "pyrx/commands.py": os.path.join(PYRX_DIR, "commands.py"),
        "pyrx/db/block.py": os.path.join(PYRX_DIR, "db", "block.py"),
        "pyrx/db/database.py": os.path.join(PYRX_DIR, "db", "database.py"),
        "pyrx/db/layout.py": os.path.join(PYRX_DIR, "db", "layout.py"),
        "pyrx/db/block_reference.py": os.path.join(PYRX_DIR, "db", "block_reference.py"),
        "pyrx/ed/prompt.py": os.path.join(PYRX_DIR, "ed", "prompt.py"),
        "pyrx/ge/point.py": os.path.join(PYRX_DIR, "ge", "point.py"),
        "pyrx/ap/utils.py": os.path.join(PYRX_DIR, "ap", "utils.py"),
        "pyrx/utils/decorators.py": os.path.join(PYRX_DIR, "utils", "decorators.py"),
        "pyrx/utils/reload.py": os.path.join(PYRX_DIR, "utils", "reload.py"),
        "pyrx/utils/test_runner.py": os.path.join(PYRX_DIR, "utils", "test_runner.py"),
        "pyrx/utils/wx.py": os.path.join(PYRX_DIR, "utils", "wx.py"),
    }
    for label, p in py_mods.items():
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8") as f:
            src = f.read()
        funcs = sorted(set(re.findall(r"^\s*def\s+(\w+)", src, re.M)))
        classes = sorted(set(re.findall(r"^\s*class\s+(\w+)", src, re.M)))
        py_rows.append([label, "wx(排除)" if "wx" in label else "需对齐",
                        ";".join(funcs) if funcs else "", ";".join(classes) if classes else ""])
    w("py_layer.csv", ["pyrx_module", "scope", "functions", "classes"], py_rows)

    # aced* 函数：PyArxCore 模块级 vs PyRx Core 静态类改短名
    aced_map = {}
    for f in ("PyDb.pyi", "PyEd.pyi", "PyGs.pyi", "PyPl.pyi"):
        p = os.path.join(PYRX_DIR, f)
        if os.path.exists(p):
            for k, v in parse_pyi_aced(p).items():
                aced_map.setdefault(k, v)
    aced_rows = []
    for mod in ("Ed", "Core", "Db"):
        for fn in sorted(v for v in arx_funcs.get(mod, set()) if v.startswith("aced")):
            v = aced_map.get(fn)
            aced_rows.append([fn, mod, v[0] if v else "", v[1] if v else "",
                              "PyRx 改短名" if v else "PyRx 亦无"])
    w("aced_mapping.csv", ["pyarx_func", "pyarx_module", "pyrx_class", "pyrx_method", "status"],
      aced_rows)

    # ---- 全量模块清单：6 个已对齐模块 + 8 个未覆盖模块，统一口径出数 ----
    def match_any(mod, cls):
        """未覆盖模块用宽松匹配：先在任意子模块里找同名/同类型类，命中即为「有对应物」"""
        for c in candidates(mod, cls):
            if c in all_arx:
                return c
        for table in (by_short, by_cpp):
            for n in table.get(cls, []):
                return n
        return None

    EXTRA_NOTE = {
        "PyGs": "AutoCAD 正式 API（AcGs 视图/显示）",
        "PyPl": "AutoCAD 正式 API（打印/发布）",
        "PySm": "AutoCAD 正式 API（图纸集）",
        "PyAx": "COM 自动化（AcAx）",
        "PyBr": "BricsCAD 专有",
        "PyBrx": "BricsCAD 专有",
        "PyBrxBim": "BricsCAD 专有",
        "PyBrxCv": "BricsCAD 专有（土木）",
    }
    inv_rows, extra_hits = [], []
    REAL_HIT = {("PyAx", "AcadDatabase")}
    for mod in list(MODULE_MAP) + EXTRA_MODULES:
        if mod not in pyrx:
            continue
        pcls = pyrx[mod][0]
        tot, hit, mt, ma = len(pcls), 0, 0, 0
        where = defaultdict(int)
        for cls, info in pcls.items():
            pmf = member_set(info["members"])
            mt += len(pmf)
            if mod in MODULE_MAP:
                h = matched.get((mod, cls))
            else:
                h = match_any(mod, cls)
            if h:
                hit += 1
                ma += len(pmf & member_set(all_arx[h]["members"]))
                where[all_arx[h]["module"]] += 1
                if mod not in MODULE_MAP:
                    extra_hits.append([
                        mod, cls, h, all_arx[h]["module"],
                        len(pmf & member_set(all_arx[h]["members"])),
                        "真实雏形" if (mod, cls) in REAL_HIT else "同名字面冲突（非同一接口）",
                    ])
            if mod in MODULE_MAP:
                continue
        sub_hint = "/".join("%s:%d" % (k, v) for k, v in sorted(where.items())) or "-"
        inv_rows.append([
            mod, "已对齐模块" if mod in MODULE_MAP else "未覆盖模块",
            tot, hit, round(hit * 100.0 / tot, 1) if tot else 0.0,
            mt, ma, round(ma * 100.0 / mt, 1) if mt else 0.0,
            MODULE_MAP.get(mod, "-"), sub_hint,
            "" if mod in MODULE_MAP else EXTRA_NOTE.get(mod, ""),
        ])
    w("module_inventory.csv",
      ["module", "scope", "pyrx_classes", "matched_classes", "class_pct",
       "pyrx_members", "aligned_members", "member_pct",
       "pyarx_submodule", "found_in", "note"], inv_rows)
    w("extra_module_matches.csv",
      ["module", "pyrx_class", "pyarx_class", "found_module", "shared_members", "verdict"],
      extra_hits)

    # 控制台摘要
    print("PyArxCore 注册类 %d / 枚举 %d" % (len(arx), len(arx_enums)))
    for mod in MODULE_MAP:
        if mod not in pyrx:
            continue
        total = len(pyrx[mod][0])
        have = sum(1 for c in pyrx[mod][0] if (mod, c) in matched)
        m_total = sum(r["pyrx_members"] for r in rows if r["module"] == mod)
        m_miss = sum(r["missing"] for r in rows if r["module"] == mod)
        f_total = len(pyrx[mod][1])
        f_have = sum(1 for r in module_rows if r[0] == mod and r[2] == "present")
        pct = (m_total - m_miss) * 100.0 / m_total if m_total else 0
        print("%-5s 类 %3d/%-3d (%3.0f%%)  成员 %4d/%-4d (%3.0f%%)  模块函数 %d/%d"
              % (mod, have, total, have * 100.0 / total, m_total - m_miss, m_total, pct, f_have, f_total))
    print("命名不一致类 %d，缺失类 %d，缺失成员 %d，PyArx 独有成员 %d"
          % (len(name_mismatch), len(missing_classes), len(missing_members), len(extra_members)))
    for r in inv_rows:
        if r[1] == "未覆盖模块":
            print("%-9s 类 %3d  命中 %2d (%4.1f%%)  成员 %4d  对齐 %4d (%4.1f%%)  %s"
                  % (r[0], r[2], r[3], r[4], r[5], r[6], r[7], r[10]))
    if issues:
        print("解析告警 %d 处：" % len(issues))
        for i in issues[:8]:
            print("  ", i)


if __name__ == "__main__":
    main()
