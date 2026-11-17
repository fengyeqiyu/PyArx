"""扫描 AcDbDatabase 头文件的零参 const getter，与 PyDbDatabase.cpp 已绑定的 def 名称做差，
生成可直接插入 PyDbDatabase.cpp 的 .def 代码块（仅 getter，不改既有行为）。

用法：python gen_database_getters.py --header <dbmain.h> --cpp <PyDbDatabase.cpp> --out <plan.txt>
"""
import argparse
import re

TYPES = ["bool", "int", "double", "Adesk::Int16", "Adesk::UInt16", "Adesk::Int32",
         "Adesk::UInt32", "long", "AcString", "AcDbObjectId"]

GETTER_RE = re.compile(
    r"\b(" + "|".join(re.escape(t) for t in TYPES) + r")\s+([A-Za-z_][A-Za-z_0-9]*)\s*\(\s*(?:void)?\s*\)\s*const"
)
DEF_RE = re.compile(r'\.def\("([A-Za-z_0-9]+)"')


def class_extent(text, start):
    depth = 0
    for i in range(start, len(text)):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def emit(name, ctype):
    if ctype == "AcDbObjectId":
        return ('.def("{0}", [](const PyDbDatabase& d) -> uint64_t {{ auto id = d.impObj()->{0}(); '
                'return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }})').format(name)
    if ctype == "AcString":
        return '.def("{0}", [](const PyDbDatabase& d) -> AcString {{ return d.impObj()->{0}(); }})'.format(name)
    if ctype == "double":
        return '.def("{0}", [](const PyDbDatabase& d) -> double {{ return d.impObj()->{0}(); }})'.format(name)
    if ctype == "bool":
        return '.def("{0}", [](const PyDbDatabase& d) -> bool {{ return d.impObj()->{0}(); }})'.format(name)
    return '.def("{0}", [](const PyDbDatabase& d) -> int {{ return static_cast<int>(d.impObj()->{0}()); }})'.format(name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header", required=True)
    ap.add_argument("--cpp", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    header = open(args.header, encoding="utf-8", errors="replace").read()
    m = re.search(r"class\s+ACDB_PORT\s*AcDbDatabase\s*:", header)
    if not m:
        m = re.search(r"class\s+AcDbDatabase\s*:", header)
    end = class_extent(header, m.start())
    body = header[m.start():end]

    getters = {}
    for hit in GETTER_RE.finditer(body):
        getters[hit.group(2)] = hit.group(1)

    cpp = open(args.cpp, encoding="utf-8", errors="replace").read()
    bound = set(DEF_RE.findall(cpp))

    missing = sorted(k for k in getters if k not in bound)
    lines = [emit(k, getters[k]) for k in missing]
    report = ["getters_in_class=%d bound_defs=%d missing=%d" % (len(getters), len(bound), len(missing)), ""]
    report += ["%s=%s" % (k, getters[k]) for k in missing]
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(report) + "\n\n@@@CODE@@@\n" + "\n".join(lines) + "\n")
    print(report[0])


if __name__ == "__main__":
    main()
