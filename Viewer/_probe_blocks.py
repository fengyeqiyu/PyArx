"""从 scan 缓存的 block_defs 看柜体块的 bbox，判断能否用块参照直接得到每台柜的真实范围。"""
import glob
import json
import os
import tempfile
from collections import Counter, defaultdict

cache = os.path.join(tempfile.gettempdir(), "pyarx_scan")
files = sorted(glob.glob(os.path.join(cache, "*101*.json")), key=os.path.getmtime, reverse=True)
print("缓存候选:", [os.path.basename(f) for f in files[:3]])
if not files:
    raise SystemExit("没找到 scan 缓存")

p = files[0]
print("读取:", p, round(os.path.getsize(p) / 1024 / 1024, 1), "MB")
s = json.load(open(p, encoding="utf-8"))
defs = s.get("block_defs") or {}
print("块定义:", len(defs))

# 块参照使用次数
use = Counter(b.get("n") for b in s.get("blocks") or [])
print("块参照总数:", len(s.get("blocks") or []))

rows = []
for name, bd in defs.items():
    b = bd.get("b") or []
    if len(b) != 4:
        continue
    w, h = b[2] - b[0], b[3] - b[1]
    rows.append((name, w, h, use.get(name, 0), len(bd.get("texts") or []), (bd.get("types") or {})))

rows.sort(key=lambda r: -r[3])
print("\n块名 / 宽 / 高 / 用量 / 定义内文字数 / 类型")
for r in rows[:18]:
    print("  %-22s %8.0f %8.0f %5d %5d %s" % (r[0][:22], r[1], r[2], r[3], r[4], r[5]))

print("\n尺寸分布在 1.8万~2.6万宽、3.5万~5万高 的块：")
for r in rows:
    if 18000 <= r[1] <= 26000 and 35000 <= r[2] <= 50000:
        print("  %-24s %8.0f x %8.0f  用量 %d  文字 %d" % (r[0][:24], r[1], r[2], r[3], r[4]))
