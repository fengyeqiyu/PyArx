"""通用 getter 缺口生成器：按 ARX 头文件类体提取零参 const getter，
与目标 cpp 中已绑定的 .def("name") 做差，输出待补绑定代码块。

用法：
  python gen_class_getters.py --header dbtable.h --class AcDbTable --cpp PyDbTable.cpp --wrapper PyDbTable --out plan.txt

规则（与项目铁律一致）：
  * ObjectId -> uint64（null 归 0，asOldId）
  * 枚举/int 类 -> int；double/bool/AcString 原样返回
  * 只补 getter，不触碰已有签名与行为
"""
import argparse
import re

TYPES = ["bool", "int", "double", "Adesk::Int16", "Adesk::UInt16", "Adesk::Int32",
         "Adesk::UInt32", "long", "AcString", "AcDbObjectId"]


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


def emit(name, ctype, wrapper):
    arg = "const %s& o" % wrapper
    if ctype == "AcDbObjectId":
        return ('.def("{0}", []({1}) -> uint64_t {{ auto id = o.impObj()->{0}(); '
                'return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }})').format(name, arg)
    if ctype in ("AcString", "double", "bool"):
        return '.def("{0}", []({1}) -> {2} {{ return o.impObj()->{0}(); }})'.format(name, arg, ctype)
    return '.def("{0}", []({1}) -> int {{ return static_cast<int>(o.impObj()->{0}()); }})'.format(name, arg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header", required=True)
    ap.add_argument("--class", dest="cls", required=True)
    ap.add_argument("--cpp", required=True)
    ap.add_argument("--wrapper", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    header = open(args.header, encoding="utf-8", errors="replace").read()
    pat = r"class\s+(?:ACDB_PORT\s*)?" + args.cls + r"\s*:"
    m = re.search(pat, header)
    if not m:
        raise SystemExit("class not found: " + args.cls)
    end = class_extent(header, m.start())
    body = header[m.start():end]

    types = "|".join(re.escape(t) for t in TYPES)
    rx = re.compile(r"\b(" + types + r")\s+([A-Za-z_][A-Za-z_0-9]*)\s*\(\s*(?:void)?\s*\)\s*const")
    getters = {h.group(2): h.group(1) for h in rx.finditer(body)}

    cpp = open(args.cpp, encoding="utf-8", errors="replace").read()
    bound = set(re.findall(r'\.def\("([A-Za-z_0-9]+)"', cpp)) | set(re.findall(r'\.def\("([A-Za-z_0-9]+)"', cpp))

    missing = sorted(k for k in getters if k not in bound)
    lines = [emit(k, getters[k], args.wrapper) for k in missing]
    head = "class=%s getters=%d bound=%d missing=%d" % (args.cls, len(getters), len(bound), len(missing))
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write(head + "\n\n")
        f.write("\n".join("%s=%s" % (k, getters[k]) for k in missing) + "\n\n@@@CODE@@@\n")
        f.write("\n".join(lines) + "\n")
    print(head)


if __name__ == "__main__":
    main()
