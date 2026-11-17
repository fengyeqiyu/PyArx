# -*- coding: utf-8 -*-
"""headless 驱动：把测试脚本丢进 accoreconsole 跑一轮，打印结果摘要。

用法：
    python run.py <测试脚本绝对路径> [结果json路径]
未给 out 时默认 <脚本同目录>\<脚本名>_out.json。
模板图固定用 C:\temp\pyarx_db_save_test.dwg（空图，句柄基线 732-737），
call_headless 会把它复制成 C:\temp\headless_<tag>.dwg，原图不动。
"""

import json
import os
import sys

sys.path.insert(0, r"G:\PyArx\tools\cadmcp")
import server  # noqa: E402

TEMPLATE = r"C:\temp\pyarx_db_save_test.dwg"


def main():
    if len(sys.argv) < 2:
        print("usage: python run.py <script.py> [out.json]")
        return 2
    script = os.path.abspath(sys.argv[1])
    out = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else \
        os.path.join(os.path.dirname(script), os.path.splitext(os.path.basename(script))[0] + "_out.json")
    # 脚本在 accoreconsole 里跑，用环境变量把落盘路径传进去（脚本里取 PYARX_OUT）
    os.environ["PYARX_OUT"] = out
    res = server.call_headless({
        "py": script,
        "out": out,
        "dwg": TEMPLATE,
        "timeout_sec": 90,
    })
    brief = {k: v for k, v in res.items() if k != "python_out"}
    print(json.dumps(brief, ensure_ascii=False, indent=1))
    po = res.get("python_out")
    if po is not None:
        print("PY_OUT:", json.dumps(po, ensure_ascii=False)[:6000])
    return 0 if res.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
