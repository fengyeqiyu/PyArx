"""细化枚举缺口：区分「只缺别名（类型已存在，只是不在 PyRx 的模块/名下）」与「真的没绑」。"""
import io
import json
import os
import re

PYRX = r"G:\PyRx-main\pyrx"
DUMP = r"G:\PyArx\log\enum_dump.json"

CLS = re.compile(r"^class (\w+)\(_BoostPythonEnum\):")
MEM = re.compile(r"^\s+(\w+):\s*[^#]*?(?:#\s*(-?\d+))?\s*$")


def parse_pyi(path):
    enums = {}
    cur = None
    for line in io.open(path, encoding="utf-8", errors="ignore"):
        line = line.rstrip("\r\n")
        m = CLS.match(line)
        if m:
            cur = m.group(1)
            enums[cur] = {}
            continue
        if cur is None or not line.strip():
            continue
        if not line.startswith((" ", "\t")):
            cur = None
            continue
        mm = MEM.match(line)
        if mm:
            enums[cur][mm.group(1)] = int(mm.group(2)) if mm.group(2) else None
    return enums


dump = json.load(open(DUMP))
real = {m: v for m, v in dump.items() if m != "Enum"}
alltypes = {}
alias_src = {}
for mod, enums in dump.items():
    for n, mem in enums.items():
        alltypes.setdefault(n, []).append(mod)

# 成员签名索引：值集合 -> 类型名（用于识别同一枚举挂在别的模块/别的名字下）
sig = {}
for mod, enums in real.items():
    for n, mem in enums.items():
        key = frozenset((k, v) for k, v in mem.items() if v is not None)
        if key:
            sig.setdefault(key, []).append("%s.%s" % (mod, n))

brx = {"Br", "Brx", "BrxBim", "BrxCv", "Ax"}
rows = []
alias_only = []
real_missing = []
mem_gap_existing = 0
for fn in sorted(os.listdir(PYRX)):
    if not fn.startswith("Py") or not fn.endswith(".pyi"):
        continue
    mod = fn[2:-4]
    if mod in brx:
        continue
    want = parse_pyi(os.path.join(PYRX, fn))
    have = real.get(mod, {})
    for e, members in sorted(want.items()):
        key = frozenset((k, v) for k, v in members.items() if v is not None)
        if e in have:
            miss = [k for k in members if k not in have[e]]
            mem_gap_existing += len(miss)
            if miss:
                rows.append((mod, e, "member_missing", len(miss)))
            continue
        cand = [c for c in sig.get(key, []) if not c.startswith(mod + ".")]
        if cand:
            alias_only.append((mod, e, cand[0], len(members)))
        else:
            real_missing.append((mod, e, len(members)))

print("缺枚举类型总数 = %d  (别名可补 %d / 需新建 %d)"
      % (len(alias_only) + len(real_missing), len(alias_only), len(real_missing)))
print("已存在枚举里的缺成员 = %d（涉及 %d 个枚举）"
      % (mem_gap_existing, len([r for r in rows if r[2] == "member_missing"])) )

print("\n-- 别名可补（前 20）--")
for mod, e, src, n in alias_only[:20]:
    print("  %-4s %-32s <- %s (%d members)" % (mod, e, src, n))

print("\n-- 需新建（按模块统计）--")
per = {}
for mod, e, n in real_missing:
    per.setdefault(mod, []).append((e, n))
for mod in sorted(per):
    print("  %s: %d 个，成员合计 %d" % (mod, len(per[mod]), sum(n for _, n in per[mod])))

print("\n-- 已存在但缺成员（前 12）--")
for r in [x for x in rows if x[2] == "member_missing"][:12]:
    print("  %-4s %-32s 缺 %d" % (r[0], r[1], r[3]))
