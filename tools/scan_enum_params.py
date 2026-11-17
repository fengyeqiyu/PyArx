"""静态扫描：PyArxCore 里哪些绑定入口的 C++ 形参是「枚举类型」而不是 int。

只做两件事，不猜：
1) 收集全部 py::enum_<T> 注册的 T（这些是 Python 侧已存在的枚举类型）；
2) 在绑定代码里找形参/返回出现这些 T 的位置，统计「枚举型形参」的绑定点。
"""
import io
import os
import re

ROOT = r"G:\PyArx\PyArxCore"
OUT = r"G:\PyArx\log\enum_params_scan.txt"

cpp_files = []
for dirpath, _dirnames, filenames in os.walk(ROOT):
    for fn in filenames:
        if fn.endswith(".cpp") or fn.endswith(".h"):
            cpp_files.append(os.path.join(dirpath, fn))

enum_type_re = re.compile(r"py::enum_<\s*([A-Za-z_][A-Za-z0-9_:<>]*)\s*>")
enums = set()
for path in cpp_files:
    with io.open(path, encoding="utf-8", errors="ignore") as fp:
        for line in fp:
            m = enum_type_re.search(line)
            if m:
                enums.add(m.group(1).strip())

# 只保留「作用域枚举」形式（AcDb::OpenMode 这类），裸名容易误报
scoped = sorted(e for e in enums if "::" in e)
bare = sorted(e for e in enums if "::" not in e)

hits = {}
for path in cpp_files:
    rel = os.path.relpath(path, ROOT)
    if not rel.endswith(".cpp"):
        continue
    with io.open(path, encoding="utf-8", errors="ignore") as fp:
        for lineno, line in enumerate(fp, 1):
            s = line.strip()
            if not (s.startswith(".def") or ".def(" in s or "m.def(" in s or "class_.def" in s):
                continue
            for e in scoped:
                if e in s:
                    hits.setdefault(e, []).append((rel, lineno, s[:140]))

lines = ["== py::enum_ 注册类型 =="]
lines.append("scoped=%d bare=%d" % (len(scoped), len(bare)))
lines.append(", ".join(scoped))
lines.append("")
lines.append("== 绑定点里形参出现枚举类型 ==")
total = 0
for e in sorted(hits, key=lambda k: -len(hits[k])):
    total += len(hits[e])
    lines.append("%-40s %d 处" % (e, len(hits[e])))
lines.append("TOTAL binding sites with enum-typed param = %d" % total)
lines.append("")
for e in sorted(hits, key=lambda k: -len(hits[k])):
    lines.append("--- %s" % e)
    for rel, lineno, s in hits[e][:60]:
        lines.append("  %s:%d  %s" % (rel, lineno, s))

io.open(OUT, "w", encoding="utf-8").write("\n".join(lines))
print("enum types: scoped=%d bare=%d" % (len(scoped), len(bare)))
print("binding sites with enum-typed param: %d" % total)
for e in sorted(hits, key=lambda k: -len(hits[k]))[:10]:
    print("  %-36s %d" % (e, len(hits[e])))
