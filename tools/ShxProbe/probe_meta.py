"""阶段 0 探针：只记录 AcGiTextEngine 回调的元数据，不 deref 任何指针（绝对安全）。"""

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
OUT = os.path.join(HERE, "probe_meta.json")
FONT_DIR = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts"

# 独立 python.exe 里 import 时，acdb24.dll 必须能从 AutoCAD 目录解析到
for _d in (r"C:\Program Files\Autodesk\AutoCAD 2024", r"G:\PyArx\X64\Release"):
    if os.path.isdir(_d):
        try:
            os.add_dll_directory(_d)
        except Exception:
            pass


def main():
    rep = {"python": sys.version, "exe": sys.executable}
    try:
        import ShxProbe
    except Exception:
        rep["import_error"] = traceback.format_exc()
        return rep

    rep["version"] = ShxProbe.version()
    try:
        rep["create"] = bool(ShxProbe.create())
    except Exception:
        rep["create"] = None
        rep["create_error"] = traceback.format_exc()
    if not rep["create"]:
        return rep

    cases = [
        ("A", os.path.join(FONT_DIR, "txt.shx"), "", "A"),
        ("AOB0g", os.path.join(FONT_DIR, "txt.shx"), "", "AOB0g"),
        ("AOB0g_raw", os.path.join(FONT_DIR, "txt.shx"), "", "AOB0g"),
        ("cjk", os.path.join(FONT_DIR, "txt.shx"), os.path.join(FONT_DIR, "gbcbig.shx"), "中工"),
    ]
    for name, font, big, text in cases:
        try:
            raw = name.endswith("_raw")
            r = ShxProbe.tessellate(font, big, text, 1.0, 0.05, raw, 0)
            r.pop("x", None); r.pop("y", None); r.pop("z", None)
            rep[name] = r
        except Exception:
            rep[name + "_error"] = traceback.format_exc()
    return rep


if __name__ == "__main__":
    rep = main()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[ShxProbe] create =", rep.get("create"), " -> ", OUT)
    for k, v in rep.items():
        if isinstance(v, dict) and "call_count" in v:
            print("[ShxProbe] %-10s calls=%s a1_first=%s a1_max=%s args=%s a2=%s pts=%s"
                  % (k, v.get("call_count"), v.get("int_arg"), v.get("int_arg_max"),
                     v.get("int_args"), v.get("a2_addr"), v.get("pts_addr")))
    for k in list(rep):
        if k.endswith("_error"):
            print("[ShxProbe] !!", k, rep[k].strip().splitlines()[-1])
