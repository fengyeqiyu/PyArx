"""阶段 1 探针：读取回调的 int 数组与实际顶点，验证 a2 语义并落盘。"""

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

OUT = os.path.join(HERE, "probe_data.json")
FONT_DIR = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts"
TXT = os.path.join(FONT_DIR, "txt.shx")
BIG = os.path.join(FONT_DIR, "gbcbig.shx")

SAMPLES = [
    ("A", TXT, "", "A"),
    ("AOB0g", TXT, "", "AOB0g"),
    ("mid", TXT, BIG, "\u4e2d"),
    ("8", TXT, "", "8"),
]


def main():
    import ShxProbe
    rep = {"python": sys.version, "exe": sys.executable,
           "version": ShxProbe.version(), "create": bool(ShxProbe.create())}
    if not rep["create"]:
        return rep
    for name, font, big, text in SAMPLES:
        try:
            r = ShxProbe.tessellate(font, big, text, 1.0, 0.05, False, 1)
            r["polys"] = r.pop("ints_head", [])
            rep[name] = r
        except Exception:
            rep[name + "_error"] = traceback.format_exc()
    return rep


if __name__ == "__main__":
    rep = main()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[ShxProbe] create =", rep.get("create"))
    for k, v in rep.items():
        if not isinstance(v, dict) or "call_count" not in v:
            continue
        print("[ShxProbe] %-8s calls=%s a1=%s polys=%s total=%s pts=%s"
              % (k, v.get("call_count"), v.get("int_arg"), v.get("polys"),
                 v.get("point_total"), v.get("point_count")))
        off = 0
        for i, n in enumerate(v.get("polys") or []):
            seg = [(round(x, 3), round(y, 3)) for x, y in
                   zip(v["x"][off:off + n], v["y"][off:off + n])]
            print("      poly%d n=%d %s" % (i, n, seg))
            off += n
    for k in rep:
        if k.endswith("_error"):
            print("[ShxProbe] !!", k, rep[k].strip().splitlines()[-1])
    print("[ShxProbe] JSON ->", OUT)
