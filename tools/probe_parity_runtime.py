"""在 CAD 内做「运行时真面对比」：PyRx pyi 的类成员 vs PyArx 实际导出。

输出 ASCII 摘要 + 明细写到 G:\PyArx\log\parity_runtime.txt，避免依赖审计脚本的源码解析口径。
"""
import io
import re

import PyArxCore as C

PYI = r"G:\PyRx-main\pyrx"
MODS = [("PyDb", "Db"), ("PyGe", "Ge"), ("PyGi", "Gi"), ("PyEd", "Ed"), ("PyAp", "Ap"), ("PyRx", "Rx")]
OUT = r"G:\PyArx\log\parity_runtime.txt"


def members_of(pyi_path, cls_name):
    """从 pyi 里取某个类的 def 成员名（含 @overload 重复的只取一次）。"""
    names = set()
    in_cls = False
    with io.open(pyi_path, encoding="utf-8") as fp:
        for line in fp:
            if line.startswith("class "):
                in_cls = line.startswith("class %s(" % cls_name) or line.startswith("class %s:" % cls_name)
                continue
            if not in_cls:
                continue
            m = re.match(r"\s+def ([A-Za-z_][A-Za-z0-9_]*)\(", line)
            if m:
                names.add(m.group(1))
    names.discard("__init__")
    return names


lines = []
total_missing = 0
total_checked = 0
for pyi_mod, arx_mod_name in MODS:
    mod = getattr(C, arx_mod_name, None)
    if mod is None:
        lines.append("MODULE MISSING %s" % arx_mod_name)
        continue
    path = r"%s\%s.pyi" % (PYI, pyi_mod)
    try:
        text_cls = []
        with io.open(path, encoding="utf-8") as fp:
            for line in fp:
                m = re.match(r"class ([A-Za-z_][A-Za-z0-9_]*)\(", line)
                if m:
                    text_cls.append(m.group(1))
    except IOError:
        lines.append("PYI MISSING %s" % pyi_mod)
        continue
    for cls_name in text_cls:
        obj = getattr(mod, cls_name, None)
        if obj is None:
            continue
        want = members_of(path, cls_name)
        have = set(dir(obj))
        miss = sorted(want - have)
        total_checked += len(want)
        total_missing += len(miss)
        if miss:
            lines.append("%-6s %-32s miss %3d/%3d: %s"
                         % (arx_mod_name, cls_name, len(miss), len(want), ",".join(miss[:40])))

lines.append("TOTAL want=%d miss=%d" % (total_checked, total_missing))
io.open(OUT, "w", encoding="utf-8").write("\n".join(lines))
print("[PARITY] want=%d miss=%d classes_with_gap=%d"
      % (total_checked, total_missing, sum(1 for x in lines if " miss " in x)))
