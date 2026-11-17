# -*- coding: utf-8 -*-
"""
test_synth_fixture.py —— 用合成夹具自测 compare_truth 的判定与退出码。

四种情形：
  A 完全一致            → 全 pass，fail_count=0
  B 坐标超差 1e-5        → 模型层几何 fail，max_err>1e-6
  C 缺 1 个实体         → 对象层总数/类型 fail
  D 图层颜色不同        → 对象层图层 fail

直接调用 compare_truth.compare_drawing（不启子进程、不碰真实 DWG）。
退出码 0=夹具断言全部正确；1=脚本自身判定有误。
"""
from __future__ import annotations

import copy
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import compare_truth as ct  # noqa: E402


def base_truth() -> dict:
    return {
        "dxf_version": "AC1027",
        "layers": [
            {"name": "0", "color": 7, "ltype": "Continuous"},
            {"name": "E-CABL", "color": 211, "ltype": "Continuous"},
        ],
        "blocks": [
            {"name": "B1", "base_point": [0.0, 0.0, 0.0], "anonymous": False},
        ],
        "entities": [
            {"handle": "1A", "type": "LINE", "layer": "0", "color": 7,
             "ltype": "Continuous", "space": "model_space",
             "geometry": {"start": [0.0, 0.0, 0.0], "end": [10.0, 0.0, 0.0]}},
            {"handle": "1B", "type": "CIRCLE", "layer": "0", "color": 7,
             "ltype": "Continuous", "space": "model_space",
             "geometry": {"center": [5.0, 5.0, 0.0], "radius": 2.0}},
            {"handle": "1C", "type": "TEXT", "layer": "E-CABL", "color": 211,
             "ltype": "Continuous", "space": "model_space",
             "geometry": {"position": [1.0, 2.0, 0.0], "text": "abc", "height": 3.0}},
            {"handle": "2A", "type": "LINE", "layer": "0", "color": 7,
             "ltype": "Continuous", "space": "block", "block": "B1",
             "geometry": {"start": [0.0, 0.0, 0.0], "end": [1.0, 1.0, 0.0]}},
        ],
    }


def base_parser() -> dict:
    return {
        "version": "AC1021",
        "layers": [
            {"name": "0", "color": 7, "ltype": "Continuous"},
            {"name": "E-CABL", "color": 211, "ltype": "Continuous"},
        ],
        "blocks": [
            {"name": "B1", "base": [0.0, 0.0, 0.0], "anonymous": False,
             "entities": [
                 {"handle": "2A", "type": "LINE", "layer": "0", "color": 7,
                  "ltype": "Continuous", "space": "block:B1",
                  "geometry": {"start": [0.0, 0.0, 0.0], "end": [1.0, 1.0, 0.0]}},
             ], "entity_count": 1},
        ],
        "entities": [
            {"handle": "1A", "type": "LINE", "layer": "0", "color": 7,
             "ltype": "Continuous", "space": "model",
             "geometry": {"start": [0.0, 0.0, 0.0], "end": [10.0, 0.0, 0.0]}},
            {"handle": "1B", "type": "CIRCLE", "layer": "0", "color": 7,
             "ltype": "Continuous", "space": "model",
             "geometry": {"center": [5.0, 5.0, 0.0], "radius": 2.0}},
            {"handle": "1C", "type": "TEXT", "layer": "E-CABL", "color": 211,
             "ltype": "Continuous", "space": "model",
             "geometry": {"insert": [1.0, 2.0, 0.0], "text": "abc", "height": 3.0}},
        ],
        "stats": {"entity_total": 4, "unknown": 0, "unknown_ratio": 0.0},
    }


def check(results: list, cond: bool, label: str) -> None:
    results.append((label, bool(cond)))
    print(f"  [{'OK' if cond else 'FAIL'}] {label}")


def main() -> int:
    failures = []

    # A 完全一致
    print("情形 A：完全一致")
    ra = ct.compare_drawing(base_parser(), base_truth())
    check(failures, ra["fail_count"] == 0, f"fail_count=0（实际 {ra['fail_count']}）")
    check(failures, all(c["status"] == "pass" for c in ra["checks"]), "全部检查 pass")

    # B 坐标超差 1e-5
    print("情形 B：LINE 终点 y 偏差 1e-5")
    pb = base_parser()
    pb["entities"][0]["geometry"]["end"] = [10.0, 1e-5, 0.0]
    rb = ct.compare_drawing(pb, base_truth())
    geo = next(c for c in rb["checks"] if c["id"] == "geometry")
    check(failures, geo["status"] == "fail", "几何检查 fail")
    check(failures, geo["details"]["max_abs_err"] > 1e-6,
          f"max_err={geo['details']['max_abs_err']:.2e} > 1e-6")
    check(failures, geo["details"]["over_tolerance_total"] >= 1, "存在超差条目")
    check(failures, rb["fail_count"] >= 1, "整体判定 fail")

    # C 缺 1 实体（TEXT 1C）
    print("情形 C：解析器缺 TEXT(1C)")
    pc = base_parser()
    pc["entities"] = [e for e in pc["entities"] if e["handle"] != "1C"]
    rc = ct.compare_drawing(pc, base_truth())
    cnt = next(c for c in rc["checks"] if c["id"] == "count.model")
    typ = next(c for c in rc["checks"] if c["id"] == "type.model")
    check(failures, cnt["status"] == "fail", "模型空间总数 fail（期望 3 / 实际 2）")
    check(failures, typ["status"] == "fail", "模型空间类型计数 fail")
    geo_c = next(c for c in rc["checks"] if c["id"] == "geometry")
    check(failures, geo_c["details"]["only_truth"] == 1, "真值独有 handle=1C 被列出")

    # D 图层颜色不同
    print("情形 D：E-CABL 颜色 211 → 1")
    pd = base_parser()
    pd["layers"][1]["color"] = 1
    rd = ct.compare_drawing(pd, base_truth())
    lay = next(c for c in rd["checks"] if c["id"] == "layers")
    row = next(r for r in lay["details"]["rows"] if r["name"] == "E-CABL")
    check(failures, lay["status"] == "fail", "图层检查 fail")
    check(failures, row["status"] == "fail" and not row["color_ok"], "颜色不一致被标出")

    # 退出码语义复核：全通过=0，有差异=1，前置缺失=2（compare_one 已封装）
    print("\n汇总：", "全部夹具断言正确" if all(ok for _, ok in failures) else "存在断言失败")
    return 0 if all(ok for _, ok in failures) else 1


if __name__ == "__main__":
    sys.exit(main())
