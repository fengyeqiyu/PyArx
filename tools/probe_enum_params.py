"""运行时扫描：所有绑定入口的 __doc__ 签名里，哪些形参是枚举类型（而非 int）。

pybind11 生成的签名形如 `method(self, arg0: Db.OpenMode, arg1: int) -> None`，
据此可精确统计「枚举型形参」的绑定点，不受源码写法影响。
"""
import io
import re

import PyArxCore as C

OUT = r"G:\PyArx\log\enum_params_runtime.txt"
MODS = [("Db", C.Db), ("Ge", C.Ge), ("Gi", C.Gi), ("Ed", C.Ed), ("Ap", C.Ap), ("Rx", C.Rx)]

# 1) 收集已注册枚举的 Python 类型名（含模块前缀形式）
enum_names = []
for mn, mod in MODS:
    for n in dir(mod):
        if n.startswith("_"):
            continue
        obj = getattr(mod, n)
        if isinstance(getattr(obj, "__members__", None), dict) and obj.__members__:
            enum_names.append("%s.%s" % (mod.__name__.split(".")[-1] if hasattr(mod, "__name__") else mn, n))
            enum_names.append("%s.%s" % (mn, n))
            enum_names.append(n)

enum_names = sorted(set(enum_names), key=len, reverse=True)
pat = re.compile("|".join(re.escape(e) for e in enum_names))

sites = []
for mn, mod in MODS:
    for n in dir(mod):
        if n.startswith("_"):
            continue
        obj = getattr(mod, n)
        docs = []
        if callable(obj) and not isinstance(obj, type):
            docs.append(("%s.%s" % (mn, n), getattr(obj, "__doc__", "") or ""))
        if isinstance(obj, type):
            for attr in dir(obj):
                if attr.startswith("_"):
                    continue
                try:
                    sub = getattr(obj, attr)
                except Exception:                  # noqa: BLE001
                    continue
                d = getattr(sub, "__doc__", "") or ""
                if d:
                    docs.append(("%s.%s.%s" % (mn, n, attr), d))
        for where, doc in docs:
            for line in doc.splitlines():
                if ":" in line and pat.search(line):
                    sites.append((where, line.strip()[:150], pat.search(line).group(0)))

lines = ["枚举类型数(名去重近似)=%d" % len(set(enum_names))]
lines.append("枚举型形参绑定点=%d" % len(sites))
for where, sig, hit in sites[:200]:
    lines.append("%-52s | %s" % (where, sig))
io.open(OUT, "w", encoding="utf-8").write("\n".join(lines))
print("[ENUMPARAM] sites=%d" % len(sites))

# 2) 实测：裸 int 能否进枚举形参 / 枚举成员能否进 int 形参
try:
    Db.Field(Db.ObjectId(1), 1, False)
    print("[ENUMPARAM] int->enum accepted")
except TypeError as ex:
    print("[ENUMPARAM] int->enum rejected: " + str(ex).splitlines()[0][:90])
except Exception as ex:                        # noqa: BLE001
    print("[ENUMPARAM] int->enum other: " + type(ex).__name__)

try:
    Db.Field(Db.ObjectId(1), Db.OpenMode.kForRead, False)
    print("[ENUMPARAM] enum->enum accepted")
except Exception as ex:                        # noqa: BLE001
    print("[ENUMPARAM] enum->enum other: " + type(ex).__name__ + ":" + str(ex)[:60])
