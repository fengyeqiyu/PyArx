"""把当前图形里的文字换成折线（真正的"炸开"：转换后删除原文字）。"""

import json
import os
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)
HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

OUT = os.path.join(HERE, "explode_this.json")
DEVIATION = 0.01


def main():
    import ShxProbe
    r = ShxProbe.explodeTexts(DEVIATION, True, False)
    r["deviation"] = DEVIATION
    return r


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"error": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[T2L] explode:", rep)
