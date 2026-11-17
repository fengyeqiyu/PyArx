"""把当前图形里的文字炸成折线：TTF 走 GDI 轮廓，SHX 走 AcGiTextEngine。"""

import json
import os
import sys

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)
HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

OUT = os.path.join(HERE, "ttf2line.json")


def main():
    import Ttf2Line
    rep = {"version": Ttf2Line.version()}
    try:
        rep["probe"] = Ttf2Line.probe("SimSun.ttf", "工艺配电(桥架)", 0.02)
        p = rep["probe"]
        p["counts"] = p.get("counts", [])[:12]
        p.pop("xs", None)
        p.pop("ys", None)
    except Exception as e:
        rep["probe_error"] = repr(e)
    rep["explode"] = Ttf2Line.explodeTexts(0.02, True)
    return rep


if __name__ == "__main__":
    rep = main()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[T2L]", rep)
