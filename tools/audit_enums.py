"""枚举对齐审计：PyRx 桩文件 (_BoostPythonEnum) vs PyArx 运行时 dump。"""
import io
import json
import os
import re

PYRX = r"G:\PyRx-main\pyrx"
DUMP = r"G:\PyArx\log\enum_dump.json"
OUTDIR = r"G:\PyArx\docs_notes\api_parity"

CLS = re.compile(r"^class (\w+)\(_BoostPythonEnum\):")
MEM = re.compile(r"^\s+(\w+):\s*[^#]*?(?:#\s*(-?\d+))?\s*$")


def parse_pyi(path):
    """-> {enum_name: {member: value_or_None}}"""
    enums = {}
    cur = None
    for line in io.open(path, encoding="utf-8", errors="ignore"):
        line = line.rstrip("\r\n")
        m = CLS.match(line)
        if m:
            cur = m.group(1)
            enums[cur] = {}
            continue
        if cur is None:
            continue
        if not line.strip():
            continue
        if not line.startswith((" ", "\t")):
            cur = None
            continue
        mm = MEM.match(line)
        if mm:
            val = int(mm.group(2)) if mm.group(2) else None
            enums[cur][mm.group(1)] = val
    return enums


def main():
    dump = json.load(open(DUMP))
    real = {m: v for m, v in dump.items() if m != "Enum"}
    # 模块内枚举 + 跨模块可见别名（PyRx 在别的模块里也能看到同名枚举的情况按模块对齐）
    pyrx = {}
    for fn in sorted(os.listdir(PYRX)):
        if not fn.startswith("Py") or not fn.endswith(".pyi"):
            continue
        mod = fn[2:-4]  # PyDb.pyi -> Db
        pyrx[mod] = parse_pyi(os.path.join(PYRX, fn))

    brx = {"Br", "Brx", "BrxBim", "BrxCv", "Ax"}
    todo = [m for m in pyrx if m not in brx]

    lines = []
    tot_enum = tot_miss_enum = tot_mem = tot_miss_mem = tot_mis_val = 0
    rows = []
    for mod in sorted(todo):
        want = pyrx[mod]
        have = real.get(mod, {})
        miss_enums = [e for e in want if e not in have]
        rows.append((mod, len(want), len(have), len(miss_enums)))
        tot_enum += len(want)
        tot_miss_enum += len(miss_enums)
        for e, members in want.items():
            tot_mem += len(members)
            if e not in have:
                tot_miss_mem += len(members)
                continue
            hv = have[e]
            for k, v in members.items():
                if k not in hv:
                    tot_miss_mem += 1
                elif v is not None and hv[k] is not None and v != hv[k]:
                    tot_mis_val += 1

    lines.append("| 模块 | PyRx 枚举 | PyArx 枚举 | 缺失枚举 |")
    lines.append("|---|---|---|---|")
    for mod, w, h, mm in rows:
        lines.append("| %s | %d | %d | %d |" % (mod, w, h, mm))
    lines.append("")
    lines.append("合计：PyRx 枚举 %d / PyArx %d，缺枚举类型 %d；成员 %d，缺成员 %d，值不一致 %d"
                 % (tot_enum, sum(len(real.get(m, {})) for m in todo), tot_miss_enum,
                    tot_mem, tot_miss_mem, tot_mis_val))
    print("\n".join(lines))

    # 明细
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    with io.open(os.path.join(OUTDIR, "missing_enums.csv"), "w", encoding="utf-8", newline="") as f:
        f.write("module,enum,status,py_rx_members\n")
        for mod in sorted(todo):
            have = real.get(mod, {})
            for e, members in sorted(pyrx[mod].items()):
                st = "missing" if e not in have else "ok"
                f.write("%s,%s,%s,%d\n" % (mod, e, st, len(members)))
    with io.open(os.path.join(OUTDIR, "missing_enum_members.csv"), "w", encoding="utf-8", newline="") as f:
        f.write("module,enum,member,py_rx_value,py_arx_value\n")
        for mod in sorted(todo):
            have = real.get(mod, {})
            for e, members in sorted(pyrx[mod].items()):
                hv = have.get(e)
                for k, v in sorted(members.items()):
                    if hv is None:
                        f.write("%s,%s,%s,%s,ENUM_MISSING\n" % (mod, e, k, v))
                    elif k not in hv:
                        f.write("%s,%s,%s,%s,MEMBER_MISSING\n" % (mod, e, k, v))
                    elif v is not None and hv[k] is not None and v != hv[k]:
                        f.write("%s,%s,%s,%s,%s\n" % (mod, e, k, v, hv[k]))
    print("[out] missing_enums.csv / missing_enum_members.csv 已写入 %s" % OUTDIR)

    # 缺枚举最多的模块 top 列表
    print("\nDb 缺失枚举（前 30）：")
    have = real.get("Db", {})
    miss = [e for e in pyrx["Db"] if e not in have]
    print(", ".join(miss[:30]))
    print("Db 缺失枚举总数 = %d" % len(miss))


if __name__ == "__main__":
    main()
