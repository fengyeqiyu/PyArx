"""创建文字实体 -> 用 AcGiTextEngine 转成折线（文字"炸开"）。

处理完保留原文字，便于并排对比；结果写入 make_text.json。
"""

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

OUT = os.path.join(HERE, "make_text.json")

SAMPLES = [
    ("PyArx Text2Line 0123", 6.0, 0.0, 0.0, 0.0, 1.0),
    ("文字转线条 中工", 6.0, 0.0, -12.0, 0.0, 1.0),
    ("Rotated AOB0g", 6.0, 0.0, -24.0, 25.0, 0.8),
]


def main():
    import ShxProbe
    rep = {"version": ShxProbe.version(), "created": [], "errors": []}
    for text, h, x, y, rot, wf in SAMPLES:
        try:
            ok = ShxProbe.addText(text, h, x, y, rot, wf)
            rep["created"].append({"text": text, "ok": bool(ok)})
        except Exception:
            rep["errors"].append(traceback.format_exc())
    try:
        rep["explode"] = ShxProbe.explodeTexts(0.1, False, False)
    except Exception:
        rep["errors"].append(traceback.format_exc())
    return rep


if __name__ == "__main__":
    rep = main()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[T2L] created:", [c["ok"] for c in rep["created"]])
    print("[T2L] explode:", rep.get("explode"))
    for e in rep["errors"]:
        print("[T2L] !!", e.strip().splitlines()[-1])
    print("[T2L] JSON ->", OUT)
