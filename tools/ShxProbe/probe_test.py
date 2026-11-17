"""ShxProbe 探针脚本：在宿主（AutoCAD 2024 + PyArxEngine，或带 acdb 的 python.exe）里跑。

用法：
    PYRUNFILE  G:/PyArx/tools/ShxProbe/probe_test.py
结果写到 G:/PyArx/tools/ShxProbe/probe_out.json 并打印摘要。

stage 0 = 只记录回调元数据（不读 a2 / 点数组，绝对安全）
stage 1 = 按 a1 当点数读取，最多 4096 点
"""

import json
import os
import sys
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "probe_out.json")

FONT_DIR = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts"
TXT = os.path.join(FONT_DIR, "txt.shx")
BIG = os.path.join(FONT_DIR, "gbcbig.shx")


def main():
    report = {"python": sys.version, "file": os.path.abspath(__file__)}
    try:
        import ShxProbe
    except Exception:
        report["import_error"] = traceback.format_exc()
        return report

    report["version"] = ShxProbe.version()
    try:
        report["create"] = bool(ShxProbe.create())
    except Exception:
        report["create_error"] = traceback.format_exc()
        report["create"] = None

    if not report["create"]:
        return report

    # stage 0：纯元数据，先看回调是怎么被调的
    try:
        report["stage0"] = ShxProbe.tessellate(TXT, "", "AOB0g", 1.0, 0.05, False, 0)
    except Exception:
        report["stage0_error"] = traceback.format_exc()

    # stage 1：读数据（此时已知 a1 的量级，仍限 4096 点）
    try:
        report["stage1"] = ShxProbe.tessellate(TXT, "", "AOB0g", 1.0, 0.05, False, 1)
    except Exception:
        report["stage1_error"] = traceback.format_exc()

    # 中文（bigfont）
    try:
        report["gb"] = ShxProbe.tessellate(TXT, BIG, "中工", 1.0, 0.05, False, 1)
    except Exception:
        report["gb_error"] = traceback.format_exc()

    return report


if __name__ == "__main__":
    rep = main()
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)

    def summarize(key):
        d = rep.get(key)
        if not isinstance(d, dict):
            return "%s: %s" % (key, d)
        return ("%s: calls=%s a1=%s a1max=%s point_count=%s ext=%s"
                % (key, d.get("call_count"), d.get("int_arg"),
                   d.get("int_arg_max"), d.get("point_count"), d.get("extents")))

    print("ShxProbe version:", rep.get("version"))
    print("create():", rep.get("create"))
    for k in ("stage0", "stage1", "gb"):
        print(summarize(k))
    for k in ("import_error", "create_error", "stage0_error", "stage1_error", "gb_error"):
        if rep.get(k):
            print("!!", k, "->", rep[k].strip().splitlines()[-1])
    print("JSON ->", OUT)
