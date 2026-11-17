# -*- coding: utf-8 -*-
"""
verify_all.py —— 一条命令跑完对拍 + 健壮性，给一句话结论。

用法：
    python verify_all.py [--cache]

退出码透传：0 全通过；1 有差异/崩溃/超时；2 前置缺失。
"""
from __future__ import annotations

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def run(script: str, extra: list) -> int:
    proc = subprocess.run(
        [sys.executable, os.path.join(HERE, script), *extra],
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)
    return proc.returncode


def main(argv=None) -> int:
    extra = [a for a in (argv or sys.argv[1:]) if a in ("--cache",)]
    rc1 = run("compare_truth.py", extra)
    rc2 = run("robustness.py", [])

    if 2 in (rc1, rc2):
        code = 2
        verdict = "前置缺失：解析器/真值未就绪，端到端比对未执行（不计 pass）"
    elif 1 in (rc1, rc2):
        code = 1
        verdict = "存在对拍差异、畸形崩溃或超时，详见 verify_report.json / robustness_report.json"
    else:
        code = 0
        verdict = "对拍与健壮性全部通过"
    print(f"\n== 结论：{verdict}（compare={rc1}, robustness={rc2}）")
    return code


if __name__ == "__main__":
    sys.exit(main())
