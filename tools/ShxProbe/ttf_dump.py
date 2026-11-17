"""导出 TTF 轮廓（不过滤任何东西），用于离线核对笔画是否完整。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\ShxProbe\ttf_probe.json"
SAMPLES = ["口", "工", "艺", "配", "电", "桥", "架", "工艺配电(桥架)"]


def main():
    import Ttf2Line
    out = {"version": Ttf2Line.version(), "samples": {}}
    for s in SAMPLES:
        r = Ttf2Line.probe("SimSun.ttf", s, 0.02)
        out["samples"][s] = {"counts": r["counts"], "xs": r["xs"], "ys": r["ys"]}
    return out


if __name__ == "__main__":
    rep = main()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False)
    print("[T2L] probe dumped:", {k: len(v["counts"]) for k, v in rep["samples"].items()})
