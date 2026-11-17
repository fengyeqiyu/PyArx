"""检查各模块导出枚举的取值是否超出 C++ int（32 位有符号）范围。

超过时，绑定到 int 形参会溢出 / 抛 OverflowError，需要改用 int64/uint 或强类型枚举形参。
"""
import io

import PyArxCore as C

OUT = r"G:\PyArx\log\enum_range.txt"
INT_MIN, INT_MAX = -2147483648, 2147483647

mods = [("Db", C.Db), ("Ge", C.Ge), ("Gi", C.Gi), ("Ed", C.Ed), ("Ap", C.Ap), ("Rx", C.Rx)]
lines, bad, checked, skipped = [], [], 0, 0

for mod_name, mod in mods:
    for name in dir(mod):
        if name.startswith("_"):
            continue
        obj = getattr(mod, name)
        members = getattr(obj, "__members__", None)
        if not isinstance(members, dict) or not members:
            continue
        vals = {}
        for mname in members:
            try:
                vals[mname] = int(getattr(obj, mname))
                checked += 1
            except Exception:                      # noqa: BLE001
                skipped += 1
        if not vals:
            continue
        lo, hi = min(vals.values()), max(vals.values())
        if lo < INT_MIN or hi > INT_MAX:
            offenders = sorted(vals.items(), key=lambda kv: -abs(kv[1]))[:4]
            bad.append("%s.%s range=[%d, %d] worst=%s" % (
                mod_name, name, lo, hi,
                ", ".join("%s=%d" % (k, v) for k, v in offenders)))
        else:
            lines.append("%s.%s range=[%d, %d] (ok)" % (mod_name, name, lo, hi))

report = bad + ["---- ok 枚举 ----"] + sorted(lines)
report.append("TOTAL members=%d skipped=%d out_of_int_range=%d" % (checked, skipped, len(bad)))
io.open(OUT, "w", encoding="utf-8").write("\n".join(report))
print("[ENUMRANGE] members=%d out_of_int_range=%d" % (checked, len(bad)))
for b in bad:
    print("[ENUMRANGE] " + b)
