# -*- utf-8 -*-
"""pyarx_vision 命令行：图元 JSON → 识别 → 设计模型

  python -m pyarx_vision.cli --in C:/temp/sys.json --kind sys \
         --recog C:/temp/recog.json --design G:/PyArx/docs/trunk_design.json

退出码：0 识别通过；1 识别发现问题（问题会打印，不静默）
"""
import argparse
import json
import sys

import pyarx_vision as V


def main(argv=None):
    ap = argparse.ArgumentParser(prog="pyarx_vision.cli")
    ap.add_argument("--in", dest="src", required=True, help="图元导出 JSON")
    ap.add_argument("--kind", default=None, help="sys / strong / tray，省略则按文件名猜")
    ap.add_argument("--recog", default=None, help="识别结果落盘路径")
    ap.add_argument("--design", default=None, help="设计模型落盘路径")
    ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args(argv)

    es = V.load(a.src, a.kind)
    res = V.recognize(es)

    if not a.quiet:
        print("== 识别日志 ==")
        for line in res.log:
            print("  " + line)
        print("== 结果 ==", res.summary())
        for p in res.problems:
            print("  ! " + p)

    if a.recog:
        json.dump({"kind": res.kind, "buses": res.buses, "loops": res.loops,
                   "problems": res.problems, "log": res.log, "stats": res.stats,
                   "frame": res.frame, "source": res.source},
                  open(a.recog, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    if a.design:
        V.dump_design(res.to_design(), a.design)
    return 0 if res.ok else 1


if __name__ == "__main__":
    sys.exit(main())
