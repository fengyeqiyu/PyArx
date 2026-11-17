"""按 PyRx 面统计「整类缺失」的缺口规模，按成员数排序（只看读结果，不改审计数据）。"""
import csv
import io
import os

BASE = r"G:\PyArx\docs_notes\api_parity"


def read(name):
    with io.open(os.path.join(BASE, name), encoding="utf-8-sig", newline="") as fp:
        return list(csv.DictReader(fp))


missing = read("missing_members.csv")
print("missing_members 行数:", len(missing), "列:", list(missing[0].keys()) if missing else None)

# 按类的缺失成员数聚合（只用 PyRx 模块 + 类名）
agg = {}
for r in missing:
    key = (r.get("module") or "", r.get("class") or "", r.get("pyrx_class") or "")
    agg[key] = agg.get(key, 0) + 1

for k, v in sorted(agg.items(), key=lambda kv: -kv[1])[:30]:
    print("%-28s %-34s %3d" % (k[0], k[1] or k[2], v))
