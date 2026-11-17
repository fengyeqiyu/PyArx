import glob
import json
import os
import tempfile
from collections import Counter, defaultdict

cache = os.path.join(tempfile.gettempdir(), "pyarx_scan")
p = sorted(glob.glob(os.path.join(cache, "*101*.json")), key=os.path.getmtime, reverse=True)[0]
s = json.load(open(p, encoding="utf-8"))
blocks = s["blocks"]
print("blocks:", len(blocks))
b0 = blocks[0]
print("首条字段:", {k: (v if not isinstance(v, list) or len(v) <= 8 else "list[%d]" % len(v)) for k, v in b0.items()})

dd = s["block_defs"]
k0 = list(dd)[0]
print("\nblock_defs 首条:", k0, {k: (v if not isinstance(v, (list, dict)) else "%s[%d]" % (type(v).__name__, len(v))) for k, v in dd[k0].items()})

# 按尺寸筛"柜体量级"的块参照
big = []
for b in blocks:
    bb = b.get("b") or []
    if len(bb) == 4 and (bb[2] - bb[0]) > 1000:
        w, h = bb[2] - bb[0], bb[3] - bb[1]
        if 12000 <= w <= 40000 and 25000 <= h <= 60000:
            big.append((b.get("n"), w, h, bb))
print("\n柜体量级块参照:", len(big))
cnt = Counter(r[0] for r in big)
for name, c in cnt.most_common(10):
    ws = [r[1] for r in big if r[0] == name]
    hs = [r[2] for r in big if r[0] == name]
    print("  %-24s %3d 次  宽 %.0f~%.0f  高 %.0f~%.0f" % (name[:24], c, min(ws), max(ws), min(hs), max(hs)))

if big:
    print("\n前 12 个：")
    for r in big[:12]:
        print("  %-18s %.0fx%.0f  %s" % (str(r[0])[:18], r[1], r[2], [round(v) for v in r[3]]))
