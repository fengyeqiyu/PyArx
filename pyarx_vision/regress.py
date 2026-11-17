# -*- utf-8 -*-
"""pyarx_vision 三图回归：同一套算法跑不同图种，输出必须稳定可复现。

用法：
  python regress.py                 # 与基线比对（首次运行自动写基线）
  python regress.py --update        # 强制刷新基线
"""
import json
import os
import sys

sys.path.insert(0, r"G:\PyArx")
import pyarx_vision as V

BASE = r"G:\PyArx\pyarx_vision\baseline.json"
CASES = [
    ("sys", r"C:\temp\sys.json", "sys"),
    ("strong", r"G:\PyArx\tools\cadmcp\strong_2d.json", "strong"),
    ("tray", r"G:\PyArx\tools\cadmcp\tray_out.json", "tray"),
]


def snapshot(path, kind):
    es = V.load(path, kind)
    res = V.recognize(es)
    snap = {
        "kind": res.kind,
        "ok": res.ok,
        "stats": res.stats,
        "problems": res.problems,
        "bus_count": len(res.buses),
        "bus_loops": [b.get("count", 0) for b in res.buses],
        "buses": [{"spec": b.get("spec", "")[:40], "cap": b.get("cap", ""),
                   "station": b.get("station", ""), "count": b.get("count", 0)}
                  for b in res.buses],
        "loop_caps": [str(l.get("cap")) for l in res.loops][:80],
        "texts": sum(len(v) for v in res.pools.values()),
    }
    return snap, res


def diff(a, b, prefix=""):
    out = []
    if type(a) != type(b):
        return ["%s 类型 %s≠%s" % (prefix, type(a).__name__, type(b).__name__)]
    if isinstance(a, dict):
        for k in sorted(set(a) | set(b)):
            if k not in a or k not in b:
                out.append("%s.%s 键缺失" % (prefix, k))
            else:
                out += diff(a[k], b[k], "%s.%s" % (prefix, k))
    elif isinstance(a, list):
        if len(a) != len(b):
            out.append("%s 长度 %d≠%d" % (prefix, len(a), len(b)))
        for i, (x, y) in enumerate(zip(a, b)):
            out += diff(x, y, "%s[%d]" % (prefix, i))
    elif a != b:
        out.append("%s %r≠%r" % (prefix, a, b))
    return out


def main():
    update = "--update" in sys.argv
    snaps = {}
    print("%-8s %-6s %-5s %-40s %s" % ("CASE", "KIND", "OK", "关键数字", "问题"))
    for name, path, kind in CASES:
        if not os.path.exists(path):
            print("%-8s 文件缺失 %s" % (name, path))
            return 2
        snap, res = snapshot(path, kind)
        snaps[name] = snap
        if res.kind == "trunk":
            key = "母线 %d 段 %s / 回路 %d / 备用 %d / 无编号 %d / 无电缆 %d" % (
                len(res.buses), "+".join(str(b["count"]) for b in res.buses),
                len(res.loops), res.stats.get("spare", 0),
                res.stats.get("no_tag", 0), res.stats.get("no_cable", 0))
        else:
            key = "线 %d / 文字 %d / 总长 %.0f" % (
                res.stats.get("lines", 0), res.stats.get("texts", 0),
                res.stats.get("total_len", 0))
        print("%-8s %-6s %-5s %-40s %s" % (name, res.kind, res.ok, key,
                                           res.problems or "-"))

    if update or not os.path.exists(BASE):
        json.dump(snaps, open(BASE, "w", encoding="utf-8"), ensure_ascii=False,
                  indent=1, sort_keys=True)
        print("\n基线已写入 %s" % BASE)
        return 0

    old = json.load(open(BASE, encoding="utf-8"))
    bad = 0
    for name in snaps:
        issues = diff(old.get(name), snaps[name], name)
        if issues:
            bad += 1
            print("\n[%s] 与基线不一致：" % name)
            for s in issues[:10]:
                print("   " + s)
    print("\n回归结果：%s" % ("通过（3/3 与基线一致）" if not bad else "失败 %d 个" % bad))
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
