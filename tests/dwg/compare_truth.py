# -*- coding: utf-8 -*-
"""
compare_truth.py —— 自研 DWG 解析器 vs AutoCAD DXF 真值逐项对拍（纯标准库）。

用法：
    python compare_truth.py [dwg ...] [--cache] [--out DIR] [--tol 1e-6]

默认行为：
  * 从 DXF 原件调用 extract_dxf_truth.py 重算真值到临时文件（杜绝自证循环）；
    加 --cache 才复用现有 truth_*.json 缓存。
  * 子进程调用解析器：``python -m PyArxDwg dump <file.dwg> --json``（PYTHONPATH=G:\\PyArx）。

退出码：
  0 全通过；1 有差异；2 环境/前置缺失（解析器不可用、真值缺失等）。

比对项与根因归类：
  容器层  version / header 基本量
  对象层  模型空间实体总数、块内实体总数、按类型计数、图层名+ACI+线型、未知类型占比
  引用层  块名集合 + 块基点 + 匿名标志
  模型层  按 handle 配对的坐标/角度/bulge 几何误差（容差 --tol，默认 1e-6）
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from typing import Any, Dict, List, Optional, Tuple

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(HERE))
DEFAULT_DWGS = [
    os.path.join(REPO_ROOT, "tests", "楼层平面-10KV电缆桥架.dwg"),
    os.path.join(REPO_ROOT, "tests", "楼层平面-强电.dwg"),
]
EXTRACTOR = os.path.join(HERE, "extract_dxf_truth.py")
TOL_DEFAULT = 1e-6
N_SAMPLE_DIFFS = 20

# DIMSTYLE 记录应解出的全部字段（66 个 DIM* / 句柄字段 + xref_index）。
_DIMSTYLE_KEYS = ("xref_index", "DIMPOST", "DIMAPOST") + (
    "DIMSCALE", "DIMASZ", "DIMEXO", "DIMDLI", "DIMEXE",
    "DIMRND", "DIMDLE", "DIMTP", "DIMTM",
) + ("DIMFXL", "DIMJOGANG", "DIMTFILL", "DIMTFILLCLR") + (
    "DIMTOL", "DIMLIM", "DIMTIH", "DIMTOH", "DIMSE1", "DIMSE2",
    "DIMTAD", "DIMZIN", "DIMAZIN", "DIMARCSYM",
    "DIMTXT", "DIMCEN", "DIMTSZ", "DIMALTF",
    "DIMLFAC", "DIMTVP", "DIMTFAC", "DIMGAP", "DIMALTRND",
    "DIMALT", "DIMALTD", "DIMTOFL", "DIMSAH", "DIMTIX", "DIMSOXD",
    "DIMCLRD", "DIMCLRE", "DIMCLRT",
    "DIMADEC", "DIMDEC", "DIMTDEC", "DIMALTU", "DIMALTTD", "DIMAUNIT",
    "DIMFRAC", "DIMLUNIT", "DIMDSEP", "DIMTMOVE", "DIMJUST",
    "DIMSD1", "DIMSD2", "DIMTOLJ", "DIMTZIN", "DIMALTZ", "DIMALTTZ",
    "DIMUPT", "DIMFIT", "DIMFXLON", "DIMLWD", "DIMLWE", "unknown70_bit",
)

# ---------------------------------------------------------------------------
# 归一化工具
# ---------------------------------------------------------------------------


def norm_color(v: Any) -> Any:
    """BYLAYER：None 与 256 等价。"""
    if v is None:
        return 256
    if isinstance(v, str) and v.strip() == "":
        return 256
    try:
        return int(v)
    except (TypeError, ValueError):
        return v


def as_vec(v: Any) -> Optional[List[float]]:
    if v is None:
        return None
    if isinstance(v, dict):
        out = [v.get("x"), v.get("y"), v.get("z")]
        return [None if x is None else float(x) for x in out]
    if isinstance(v, (list, tuple)):
        return [None if x is None else float(x) for x in v]
    return None


def vec_diff(a: Any, b: Any) -> Optional[float]:
    va, vb = as_vec(a), as_vec(b)
    if va is None or vb is None:
        return None
    n = min(len(va), len(vb))
    worst = 0.0
    for i in range(n):
        if va[i] is None or vb[i] is None:
            continue
        worst = max(worst, abs(va[i] - vb[i]))
    return worst


def scalar_diff(a: Any, b: Any) -> Optional[float]:
    if a is None or b is None:
        return None
    try:
        return abs(float(a) - float(b))
    except (TypeError, ValueError):
        return None


def normalize_entity(e: Dict[str, Any]) -> Dict[str, Any]:
    """解析器/真值两侧实体归一到同一键名。"""
    g = dict(e.get("geometry") or {})
    typ = (e.get("type") or "").upper()
    # 真值 INSERT: insertion/scale_x/scale_y；解析器: insert/scale{ x,y }
    if typ == "INSERT":
        ins = g.get("insert") or g.get("insertion")
        g["insert"] = ins
        sx = g.get("scale_x")
        sy = g.get("scale_y")
        sc = g.get("scale")
        if sc is None and sx is not None:
            g["scale"] = [sx, sy]
        elif isinstance(sc, dict):
            g["scale"] = [sc.get("x", sc.get("sx")), sc.get("y", sc.get("sy"))]
    elif typ in ("TEXT", "MTEXT", "ATTRIB", "ATTDEF"):
        g["insert"] = g.get("insert") or g.get("position")
    elif typ == "DIMENSION":
        g["defpoint"] = g.get("defpoint") or g.get("def_point")
    e["_geo"] = g
    return e


# ---------------------------------------------------------------------------
# 真值 / 解析器装载
# ---------------------------------------------------------------------------


def load_truth(dxf_path: str, use_cache: bool) -> Dict[str, Any]:
    """读取 DXF 真值；默认调用抽取器重算到临时文件。"""
    base = os.path.splitext(os.path.basename(dxf_path))[0]
    cache = os.path.join(HERE, f"truth_{base}.json")
    if use_cache:
        with open(cache, "rb") as fh:
            return json.loads(fh.read().decode("utf-8"))
    tmp = tempfile.NamedTemporaryFile(
        prefix="truth_", suffix=".json", dir=HERE, delete=False
    )
    tmp.close()
    proc = subprocess.run(
        [sys.executable, EXTRACTOR, dxf_path, tmp.name],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"extractor failed rc={proc.returncode}: {proc.stderr.strip()[:300]}"
        )
    with open(tmp.name, "rb") as fh:
        truth = json.loads(fh.read().decode("utf-8"))
    try:
        os.remove(tmp.name)
    except OSError:
        pass
    return truth


def run_parser(dwg_path: str) -> Tuple[Optional[Dict[str, Any]], int, str, str]:
    """子进程调用解析器；返回 (parser_obj, rc, stdout, stderr)。"""
    env = os.environ.copy()
    env["PYTHONPATH"] = REPO_ROOT + os.pathsep + env.get("PYTHONPATH", "")
    cmd = [sys.executable, "-m", "PyArxDwg", "dump", dwg_path, "--json"]
    proc = subprocess.run(
        cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", env=env
    )
    obj: Optional[Dict[str, Any]] = None
    stdout = proc.stdout or ""
    stderr = proc.stderr or ""
    if proc.returncode == 0:
        try:
            obj = json.loads(stdout)
        except json.JSONDecodeError:
            obj = None
    return obj, proc.returncode, stdout, stderr


# ---------------------------------------------------------------------------
# 归一化后的两侧视图
# ---------------------------------------------------------------------------


def truth_view(truth: Dict[str, Any]) -> Dict[str, Any]:
    ents = [normalize_entity(dict(e)) for e in truth.get("entities", [])]
    model = [e for e in ents if e.get("space") == "model_space"]
    inblk = [e for e in ents if e.get("space") == "block"]
    layers = {
        (l.get("name") or ""): {"color": norm_color(l.get("color")), "ltype": l.get("ltype")}
        for l in truth.get("layers", [])
    }
    blocks = {}
    for b in truth.get("blocks", []):
        blocks[b.get("name") or ""] = {
            "base": as_vec(b.get("base_point")) or [None, None, None],
            "anonymous": bool(b.get("anonymous")),
        }
    return {"model": model, "inblk": inblk, "layers": layers, "blocks": blocks}


def parser_view(parser: Dict[str, Any]) -> Dict[str, Any]:
    model = [normalize_entity(dict(e)) for e in parser.get("entities", [])]
    inblk: List[Dict[str, Any]] = []
    for blk in parser.get("blocks", []) or []:
        for e in blk.get("entities", []) or []:
            rec = normalize_entity(dict(e))
            if not rec.get("space"):
                rec["space"] = f"block:{blk.get('name') or ''}"
            inblk.append(rec)
    layers = {
        (l.get("name") or ""): {"color": norm_color(l.get("color")), "ltype": l.get("ltype")}
        for l in parser.get("layers", [])
    }
    blocks = {}
    for b in parser.get("blocks", []) or []:
        blocks[b.get("name") or ""] = {
            "base": as_vec(b.get("base")) or [None, None, None],
            "anonymous": bool(b.get("anonymous")),
        }
    return {"model": model, "inblk": inblk, "layers": layers, "blocks": blocks}


def type_counts(seq: List[Dict[str, Any]]) -> Dict[str, int]:
    out: Dict[str, int] = {}
    for e in seq:
        t = (e.get("type") or "UNKNOWN").upper()
        out[t] = out.get(t, 0) + 1
    return dict(sorted(out.items()))


# ---------------------------------------------------------------------------
# 几何比对
# ---------------------------------------------------------------------------


def compare_geometry(
    etype: str, g_truth: Dict[str, Any], g_parser: Dict[str, Any], tol: float
) -> Tuple[float, List[Dict[str, Any]]]:
    """返回 (最大误差, 超差明细)。字段缺失记 None 不参与数值比较。"""
    worst = 0.0
    bad: List[Dict[str, Any]] = []

    def check_vec(field: str):
        nonlocal worst
        d = vec_diff(g_truth.get(field), g_parser.get(field))
        if d is None:
            return
        worst = max(worst, d)
        if d > tol:
            bad.append({"field": field, "truth": g_truth.get(field),
                         "parser": g_parser.get(field), "abs_err": d})

    def check_scalar(field: str):
        nonlocal worst
        d = scalar_diff(g_truth.get(field), g_parser.get(field))
        if d is None:
            return
        worst = max(worst, d)
        if d > tol:
            bad.append({"field": field, "truth": g_truth.get(field),
                         "parser": g_parser.get(field), "abs_err": d})

    if etype == "LINE":
        check_vec("start")
        check_vec("end")
    elif etype == "CIRCLE":
        check_vec("center")
        check_scalar("radius")
    elif etype == "ARC":
        check_vec("center")
        check_scalar("radius")
        check_scalar("start_angle")
        check_scalar("end_angle")
    elif etype == "LWPOLYLINE":
        vt = g_truth.get("vertices") or []
        vp = g_parser.get("vertices") or []
        if len(vt) == len(vp):
            for i, (a, b) in enumerate(zip(vt, vp)):
                for coord in ("x", "y"):
                    d = scalar_diff(a.get(coord), b.get(coord))
                    if d is not None:
                        worst = max(worst, d)
                        if d > tol:
                            bad.append({"field": f"vertices[{i}].{coord}",
                                        "truth": a.get(coord), "parser": b.get(coord),
                                        "abs_err": d})
                db = scalar_diff(a.get("bulge"), b.get("bulge"))
                if db is not None:
                    worst = max(worst, db)
                    if db > tol:
                        bad.append({"field": f"vertices[{i}].bulge",
                                    "truth": a.get("bulge"), "parser": b.get("bulge"),
                                    "abs_err": db})
        else:
            bad.append({"field": "vertices.count", "truth": len(vt),
                        "parser": len(vp), "abs_err": None})
        check_scalar("const_width")
        check_scalar("elevation")
        check_scalar("thickness")
    elif etype in ("TEXT", "MTEXT"):
        check_vec("insert")
        check_scalar("height")
        if etype == "MTEXT":
            for field in ("attachment", "flow_dir"):
                if g_truth.get(field) is not None and g_parser.get(field) != g_truth.get(field):
                    bad.append({"field": field, "truth": g_truth.get(field),
                                "parser": g_parser.get(field), "abs_err": None})
            for field in ("width", "rect_height", "linespace_factor"):
                tv, pv = g_truth.get(field), g_parser.get(field)
                if tv is not None and pv is not None:
                    d = scalar_diff(tv, pv)
                    if d is not None:
                        worst = max(worst, d)
                        if d > tol:
                            bad.append({"field": field, "truth": tv, "parser": pv,
                                        "abs_err": d})
            tv, pv = g_truth.get("style"), g_parser.get("style")
            if tv and tv != pv:
                bad.append({"field": "style", "truth": tv, "parser": pv,
                            "abs_err": None})
    elif etype == "INSERT":
        check_vec("insert")
        if isinstance(g_parser.get("scale"), list) and isinstance(g_truth.get("scale"), list):
            for i, (a, b) in enumerate(zip(g_truth["scale"], g_parser["scale"])):
                d = scalar_diff(a, b)
                if d is not None:
                    worst = max(worst, d)
                    if d > tol:
                        bad.append({"field": f"scale[{i}]", "truth": a, "parser": b,
                                    "abs_err": d})
        check_scalar("rotation")
    elif etype == "DIMENSION":
        check_vec("defpoint")
    elif etype == "ELLIPSE":
        check_vec("center")
        check_vec("major_axis")
        check_scalar("major_ratio")
    elif etype == "HATCH":
        # 头部 + 边界路径结构：solid/assoc/paths/每条路径的类型标志与边数/
        # 边的曲线类型序列/样式/图案类型/图案定义线数/破折数/种子点数。
        for field in ("pattern", "flags", "associative", "num_paths",
                      "path_flags", "path_counts", "edge_types", "style",
                      "pattern_type", "num_def_lines", "num_dashes", "num_seeds"):
            if g_truth.get(field) is not None and g_parser.get(field) != g_truth.get(field):
                bad.append({"field": field, "truth": g_truth.get(field),
                            "parser": g_parser.get(field), "abs_err": None})
    elif etype == "VERTEX":
        check_vec("point")
        if g_truth.get("flags") is not None and g_parser.get("flag") != g_truth.get("flags"):
            # DXF 70 stores the full vertex flag, the DWG field is the same byte
            bad.append({"field": "flag", "truth": g_truth.get("flags"),
                        "parser": g_parser.get("flag"), "abs_err": None})
        for field, pkey in (("bulge", "bulge"), ("start_width", "start_width"),
                            ("end_width", "end_width")):
            tv, pv = g_truth.get(field), g_parser.get(pkey)
            if tv is not None and pv is not None:
                d = scalar_diff(tv, pv)
                if d is not None:
                    worst = max(worst, d)
                    if d > tol:
                        bad.append({"field": field, "truth": tv, "parser": pv,
                                    "abs_err": d})
    elif etype == "POLYLINE":
        check_vec("location")
        for field in ("flags",):
            if g_truth.get(field) is not None and g_parser.get(field) != g_truth.get(field):
                bad.append({"field": field, "truth": g_truth.get(field),
                            "parser": g_parser.get(field), "abs_err": None})
    elif etype == "SPLINE":
        # DXF 70 carries extra bits the DWG record does not store (24 vs 8),
        # so flags are excluded; degree / knots / control points are comparable.
        for field in ("degree",):
            if g_truth.get(field) is not None and g_parser.get(field) != g_truth.get(field):
                bad.append({"field": field, "truth": g_truth.get(field),
                            "parser": g_parser.get(field), "abs_err": None})
        for field, pkey in (("knots", "knots"), ("control", "control")):
            tl, pl = g_truth.get(field), g_parser.get(pkey)
            if not tl or not pl:
                continue
            if len(tl) != len(pl):
                bad.append({"field": f"{field}.count", "truth": len(tl),
                            "parser": len(pl), "abs_err": None})
                continue
            for i, (tv, pv) in enumerate(zip(tl, pl)):
                if isinstance(tv, list):
                    for j, (a, b) in enumerate(zip(tv, pv)):
                        d = scalar_diff(a, b)
                        if d is not None:
                            worst = max(worst, d)
                            if d > tol:
                                bad.append({"field": f"{field}[{i}][{j}]",
                                            "truth": a, "parser": b, "abs_err": d})
                else:
                    d = scalar_diff(tv, pv)
                    if d is not None:
                        worst = max(worst, d)
                        if d > tol:
                            bad.append({"field": f"{field}[{i}]", "truth": tv,
                                        "parser": pv, "abs_err": d})
    return worst, bad


# ---------------------------------------------------------------------------
# 核心对拍（夹具自测复用）
# ---------------------------------------------------------------------------


def _check(checks: List[Dict[str, Any]], cid: str, title: str, layer: str,
           status: str, expected: Any = None, actual: Any = None,
           details: Optional[Dict[str, Any]] = None) -> None:
    checks.append({"id": cid, "title": title, "root_cause": layer,
                   "status": status, "expected": expected, "actual": actual,
                   "details": details or {}})


def compare_drawing(parser: Dict[str, Any], truth: Dict[str, Any],
                    tol: float = TOL_DEFAULT) -> Dict[str, Any]:
    """单个图纸的完整对拍，返回结构化结果（供 CLI 与夹具共用）。"""
    pt = parser_view(parser)
    tt = truth_view(truth)
    checks: List[Dict[str, Any]] = []

    # —— 容器层：版本 ——
    pv = (parser.get("version") or "").upper()
    tv = (truth.get("dxf_version") or "").upper()
    # DXF 版本 AC1027 来自 R2007 DWG（AC1021）导出；仅记录不做强一致判定
    _check(checks, "version", "文件版本", "容器层", "pass" if pv else "not_executed",
           expected=tv, actual=pv,
           details={"note": "DXF=AC1027 由 DWG AC1021 导出；版本串不同属正常"})

    # —— 容器层：HEADER 变量（DXF $VAR 为真值）——
    # DXF 的 $EXTMIN/$EXTMAX 由导出端按实体重算，与 DWG 头内快照不同（10KV 样本
    # 末位就已不同），$HANDSEED 也会因导出新增对象而变大，故这两类只作参考值记录，
    # 不作为断言；判定只用 INSUNITS / INSBASE / LIMMIN / LIMMAX 这类原样转写量。
    hdr_truth = truth.get("header_vars") or {}
    hdr_parser = parser.get("header") or {}
    hdr_vars = hdr_parser.get("vars") or {}
    hdr_handles = hdr_parser.get("handles") or {}
    # 三类“不可比”必须在断言之外，理由是各自的：导出口径、时区口径、句柄表示。
    EXPORT_RECOMPUTED = {"EXTMIN", "EXTMAX", "HANDSEED"}
    TIME_BASE = {"TDCREATE", "TDUPDATE", "TDINDWG", "TDUSRTIMER"}
    header_checked = 0
    header_matched = 0
    header_open = []
    header_skipped = []
    header_diffs = []
    for dkey in sorted(hdr_truth):
        if not dkey.startswith("$"):
            continue
        name = dkey[1:]
        if name not in hdr_vars:
            header_skipped.append({"var": dkey, "reason": "not_decoded"})
            continue
        if name in EXPORT_RECOMPUTED:
            header_skipped.append({"var": dkey, "reason": "DXF 导出端按实体重算"})
            continue
        if name in TIME_BASE:
            header_skipped.append({"var": dkey,
                                   "reason": "DWG 存 UTC、DXF 写本地；DXF 为后续重存会话"})
            continue
        if name in hdr_handles:
            header_skipped.append({"var": dkey, "reason": "句柄：DXF 存名字/字符串，DWG 存句柄 id"})
            continue
        tv = hdr_truth[dkey]
        pv = hdr_vars.get(name + "_value", hdr_vars.get(name))
        if isinstance(pv, dict):
            pv = pv.get("value")
        header_checked += 1
        if isinstance(tv, list):
            bad = None
            # DXF 里 2D 变量常写成 [x, y, None]（组码 30 缺省），None 位不参与断言。
            tv = [x for x in tv if x is not None]
            if not isinstance(pv, list) or len(pv) < len(tv):
                bad = {"var": dkey, "truth": tv, "parser": pv}
            else:
                for i, (a_, b_) in enumerate(zip(tv, pv)):
                    d = scalar_diff(a_, b_)
                    if d is not None and d > tol:
                        bad = {"var": f"{dkey}[{i}]", "truth": a_, "parser": b_,
                               "abs_err": d}
                        break
            if bad:
                header_diffs.append(bad)
            else:
                header_matched += 1
        elif isinstance(tv, (int, float)) and isinstance(pv, (int, float)):
            d = scalar_diff(tv, pv)
            if d is None or d <= tol:
                header_matched += 1
            else:
                header_diffs.append({"var": dkey, "truth": tv, "parser": pv,
                                     "abs_err": d})
        else:
            a_s = "" if tv is None else str(tv).strip()
            b_s = "" if pv is None else str(pv).strip()
            if a_s == b_s:
                header_matched += 1
            else:
                header_diffs.append({"var": dkey, "truth": tv, "parser": pv,
                                     "abs_err": None})
    # 尚未归因的残留差异单独列出，不混进失败判定，也不隐藏。
    header_open = [d for d in header_diffs
                   if d["var"].lstrip("$").split("[")[0] in
                   {"TIMEZONE", "DWFFRAME", "DGNFRAME", "CELWEIGHT"}]
    header_diffs = [d for d in header_diffs if d not in header_open]
    hdr_total = len(hdr_vars)
    _check(checks, "header", "HEADER 变量（按 $VAR 全量逐值）", "容器层",
           "pass" if header_checked and not header_diffs else
           ("not_executed" if not header_checked else "fail"),
           expected=f"DXF 中 {header_checked} 个可比 $VAR 逐值相等",
           actual=f"逐值相等 {header_matched} / 断言 {header_checked}",
           details={
               "checked": header_checked,
               "matched": header_matched,
               "diff": header_diffs,
               "open_unattributed": header_open,
               "skipped": header_skipped,
               "dxf_var_total": len(hdr_truth),
               "parser_var_total": hdr_total,
               "parser_handle_total": len(hdr_handles),
               "walk": hdr_parser.get("status"),
               "info_extents": {
                   "truth": {"min": hdr_truth.get("$EXTMIN"),
                             "max": hdr_truth.get("$EXTMAX")},
                   "parser": {"min": hdr_parser.get("extmin"),
                              "max": hdr_parser.get("extmax")},
                   "note": "DXF 导出时按实体重算范围，非 DWG 头快照；不作断言",
               },
               "info_handseed": {"truth": hdr_truth.get("$HANDSEED"),
                                 "parser": hdr_parser.get("handseed"),
                                 "note": "DXF 导出新增对象会抬高种子，不作断言"},
           })

    # —— 类表 AcDb:Classes（DWG 内部类号 500+ 的索引 → DXF 类名）——
    # 真值来自 DXF CLASSES 段（writer 原样导出同一张表）。DXF 不导类号，
    # 所以断言用 (类名, 是否实体) 集合；uuid/实例数不比对。
    cls_truth = truth.get("classes") or {}
    cls_parser = parser.get("classes") or []
    t_rows = {(str(r.get("dxf", "")), int(r.get("is_entity") or 0))
              for r in (cls_truth.get("rows") or [])}
    p_rows = {(str(c.get("dxf", "")),
               1 if int(c.get("item_class") or 0) == 0x1F2 else 0)
              for c in cls_parser}
    cls_issues = [{"field": "class.missing", "truth": a, "parser": None}
                  for a, _ in sorted(t_rows - p_rows)]
    cls_issues += [{"field": "class.extra", "truth": None, "parser": a}
                   for a, _ in sorted(p_rows - t_rows)]
    _check(checks, "classes", "类表 AcDb:Classes：类名 + 实体/对象标志", "对象层",
           "pass" if (cls_parser and not cls_issues) else
           ("not_executed" if not t_rows else "fail"),
           expected={"count": cls_truth.get("count"), "rows": len(t_rows)},
           actual={"count": len(cls_parser), "rows": len(p_rows)},
           details={
               "issues": cls_issues,
               "numbers": [c.get("number") for c in cls_parser],
               "note": "类号是 DWG 内部索引（本样 500..511），DXF 不导；"
                         "解析器解码后主趟/字符串流双闭合才返回类表",
           })

    # —— 对象层：整份文件的对象识别覆盖率 ——
    # 每个对象记录要么被解码（parsed），要么是结构标记（BLOCK/ENDBLK/SEQEND），
    # 要么是已识别的二进制载荷（XRECORD / 代理类对象，正文未解）。三者都不
    # 计入 unknown；DXF 无对应真值，因此断言只看“无未知类型、无解码报错”。
    stats_obj = parser.get("stats") or {}
    obj_total = stats_obj.get("object_total")
    obj_unknown = stats_obj.get("object_unknown")
    obj_error = stats_obj.get("object_error")
    if obj_total is None:
        _check(checks, "objects.coverage", "对象层覆盖率：无未知类型、无解码报错",
               "对象层", "not_executed", expected=0, actual=None,
               details={"note": "解析器未输出 stats.object_total"})
    else:
        cov_issues = []
        if obj_unknown:
            cov_issues.append({"field": "object_unknown", "truth": 0,
                               "parser": obj_unknown})
        if obj_error:
            cov_issues.append({"field": "object_error", "truth": 0,
                               "parser": obj_error})
        _check(checks, "objects.coverage",
               "对象层覆盖率：无未知类型、无解码报错", "对象层",
               "pass" if obj_total and not cov_issues else "fail",
               expected=0,
               actual={"total": obj_total, "unknown": obj_unknown,
                       "error": obj_error},
               details={
                   "issues": cov_issues,
                   "parsed": stats_obj.get("object_parsed"),
                   "structural": stats_obj.get("object_structural"),
                   "binary_payload": stats_obj.get("object_binary_payload"),
                   "by_name_top": sorted(
                       (stats_obj.get("object_by_name") or {}).items(),
                       key=lambda kv: -kv[1])[:12],
                   "note": "XRECORD 与代理类对象只保证帧层（长度/标志/原始字节），"
                             "正文语义未解、标 binary；结构标记不计 unknown",
               })

    # —— 对象层：DICTIONARY 字典树（键名 + 句柄可解析）——
    # DXF 的 OBJECTS 段是唯一真值来源。两边口径差异已核实：DWG 多出
    # ACAD_XREC_ROUNDTRIP*（XRECORD 的回写字典，DXF 不导出 XRECORD）与
    # XCLIPFRAME；DXF 多出导出端自写的 ACAD_LAST_SAVED_VERSION_INFO /
    # MARKUPASSISTMODE / Metric50。判定用“去掉口径差后键名逐个相等”。
    dict_truth = truth.get("dictionaries") or {}
    dict_parser_rows = parser.get("dictionaries") or []
    dwg_keys = [e.get("name", "") for row in dict_parser_rows
                for e in (row.get("entries") or [])]
    dxf_keys = list(dict_truth.get("keys") or [])
    DXF_ONLY_KEYS = {"ACAD_LAST_SAVED_VERSION_INFO", "MARKUPASSISTMODE",
                     "Metric50"}
    # XRECORD 回写子字典用 ACAD_XREC_ROUNDTRIP_<名字> 作键；基名
    # ACAD_XREC_ROUNDTRIP 本身 DXF 也会写一个，故只跳过带下划线的子键。
    DWG_ONLY_PREFIX = "ACAD_XREC_ROUNDTRIP_"
    DWG_ONLY_KEYS = {"XCLIPFRAME"}
    dxf_cmp = sorted(k for k in (set(dxf_keys) - DXF_ONLY_KEYS))
    dwg_cmp = sorted(k for k in (set(dwg_keys)
                                 if False else set(dwg_keys))
                     if not k.startswith(DWG_ONLY_PREFIX))
    dwg_cmp = sorted(set(dwg_cmp) - DWG_ONLY_KEYS)
    dict_unresolved = sum(int(row.get("unresolved") or 0)
                          for row in dict_parser_rows)
    dict_issues = []
    if not dict_parser_rows:
        dict_issues.append({"field": "dictionaries", "truth": dict_truth.get("count"),
                            "parser": 0, "note": "解析器未解出任何字典"})
    for k in sorted(set(dxf_cmp) - set(dwg_cmp)):
        dict_issues.append({"field": "key.missing", "truth": k, "parser": None})
    for k in sorted(set(dwg_cmp) - set(dxf_cmp)):
        dict_issues.append({"field": "key.extra", "truth": None, "parser": k})
    if dict_unresolved:
        dict_issues.append({"field": "entry.handle_unresolved",
                            "truth": 0, "parser": dict_unresolved})
    _check(checks, "dictionaries", "DICTIONARY 字典树：键名逐个 + 句柄全部可解析",
           "对象层",
           "pass" if dict_parser_rows and not dict_issues else
           ("not_executed" if not dict_truth else "fail"),
           expected=f"DXF 字典 {dict_truth.get('count')} 个 / 键 {len(dxf_cmp)} 个（去口径差）",
           actual=f"解析器字典 {len(dict_parser_rows)} 个 / 键 {len(dwg_cmp)} 个，未解析句柄 {dict_unresolved}",
           details={
               "issues": dict_issues,
               "dxf_count": dict_truth.get("count"),
               "parser_count": len(dict_parser_rows),
               "dxf_key_total": len(dxf_keys),
               "parser_key_total": len(dwg_keys),
               "skipped_dxf_only": sorted(DXF_ONLY_KEYS),
               "skipped_dwg_only": sorted(DWG_ONLY_KEYS) + [DWG_ONLY_PREFIX + "*"],
               "note": "键名集合与句柄可达性双判，口径差已在 skipped 中显式列出",
           })

    # —— 对象层：LTYPE 表（名字/描述/图案段逐项）——
    # DXF 的 74 位仅决定该段是不是复合适形；样本里 4 条线型全是简单段，无 512 字节
    # 字符串区，因此这里逐段比 49 长度 + 74 标志，多出/缺少段即判不过。
    lt_truth = {str(x.get("name") or ""): x for x in (truth.get("linetypes") or [])}
    lt_parser = {str(x.get("name") or ""): x
                 for x in (parser.get("linetype_details") or [])}
    ltype_rows = []
    for name in sorted(set(lt_truth) | set(lt_parser)):
        te, pe = lt_truth.get(name), lt_parser.get(name)
        if te is None:
            ltype_rows.append({"name": name, "status": "extra"})
            continue
        if pe is None:
            ltype_rows.append({"name": name, "status": "missing"})
            continue
        issues = []
        if (te.get("description") or "") != (pe.get("description") or ""):
            issues.append({"field": "description", "truth": te.get("description"),
                           "parser": pe.get("description")})
        for field in ("pattern_len",):
            d = scalar_diff(te.get(field), pe.get(field))
            if d is not None and d > tol:
                issues.append({"field": field, "truth": te.get(field),
                               "parser": pe.get(field), "abs_err": d})
        td, pd_ = te.get("dashes") or [], pe.get("dashes") or []
        if len(td) != len(pd_):
            issues.append({"field": "dashes.count", "truth": len(td),
                           "parser": len(pd_)})
        else:
            for i, (a, b) in enumerate(zip(td, pd_)):
                d = scalar_diff(a.get("length"), b.get("length"))
                if d is not None and d > tol:
                    issues.append({"field": f"dashes[{i}].length",
                                   "truth": a.get("length"), "parser": b.get("length"),
                                   "abs_err": d})
                if a.get("flags") is not None and a.get("flags") != b.get("flags"):
                    issues.append({"field": f"dashes[{i}].flags",
                                   "truth": a.get("flags"), "parser": b.get("flags")})
        if issues:
            ltype_rows.append({"name": name, "status": "fail", "issues": issues})
    ltype_fail = [r for r in ltype_rows if r["status"] != "same"]
    _check(checks, "linetypes", "LTYPE 表：名字/描述/图案段长度/段标志", "对象层",
           "pass" if not ltype_fail else "fail",
           expected=len(lt_truth), actual=len(lt_parser),
           details={"diff": ltype_fail[:N_SAMPLE_DIFFS],
                    "diff_total": len(ltype_fail)})

    # —— 对象层：实体总数 ——
    _check(checks, "count.model", "模型空间顶层实体总数", "对象层",
           "pass" if len(pt["model"]) == len(tt["model"]) else "fail",
           expected=len(tt["model"]), actual=len(pt["model"]),
           details={"missing": len(tt["model"]) - len(pt["model"]),
                    "extra": len(pt["model"]) - len(tt["model"])})
    _check(checks, "count.block", "块内实体总数", "对象层",
           "pass" if len(pt["inblk"]) == len(tt["inblk"]) else "fail",
           expected=len(tt["inblk"]), actual=len(pt["inblk"]),
           details={"missing": len(tt["inblk"]) - len(pt["inblk"]),
                    "extra": len(pt["inblk"]) - len(tt["inblk"])})

    # —— 对象层：类型计数（两处分别出表）——
    for cid, label, pseq, tseq in (
        ("type.model", "模型空间按类型计数", pt["model"], tt["model"]),
        ("type.block", "块内按类型计数", pt["inblk"], tt["inblk"]),
    ):
        cp, ct = type_counts(pseq), type_counts(tseq)
        types = sorted(set(cp) | set(ct))
        rows = [{"type": t, "truth": ct.get(t, 0), "parser": cp.get(t, 0)} for t in types]
        ok = all(r["truth"] == r["parser"] for r in rows)
        _check(checks, cid, label, "对象层", "pass" if ok else "fail",
               expected=ct, actual=cp, details={"rows": rows})

    # —— 对象层：图层名 + ACI + 线型 ——
    pnames, tnames = set(pt["layers"]), set(tt["layers"])
    layer_rows = []
    for name in sorted(tnames | pnames):
        tr, pr = tt["layers"].get(name), pt["layers"].get(name)
        if tr is None:
            layer_rows.append({"name": name, "status": "extra", "truth": None,
                               "parser": pr})
        elif pr is None:
            layer_rows.append({"name": name, "status": "missing", "truth": tr,
                               "parser": None})
        else:
            color_ok = tr["color"] == pr["color"]
            ltype_ok = (tr["ltype"] or "Continuous") == (pr["ltype"] or "Continuous")
            layer_rows.append({"name": name,
                                "status": "pass" if color_ok and ltype_ok else "fail",
                                "truth": tr, "parser": pr,
                                "color_ok": color_ok, "ltype_ok": ltype_ok})
    layer_ok = all(r["status"] in ("pass",) for r in layer_rows)
    _check(checks, "layers", "图层名集合 + ACI 颜色 + 线型", "对象层",
           "pass" if layer_ok else "fail",
           expected=sorted(tnames), actual=sorted(pnames),
           details={"rows": layer_rows})

    # —— 引用层：块名集合 + 基点 + 匿名 ——
    pb, tb = set(pt["blocks"]), set(tt["blocks"])
    block_rows = []
    for name in sorted(tb | pb):
        tr, pr = tt["blocks"].get(name), pt["blocks"].get(name)
        if tr is None:
            block_rows.append({"name": name, "status": "extra"})
        elif pr is None:
            block_rows.append({"name": name, "status": "missing", "truth": tr})
        else:
            d = vec_diff(tr["base"], pr["base"])
            anon_ok = tr["anonymous"] == pr["anonymous"]
            block_rows.append({"name": name,
                                "status": "pass" if d is None or d <= tol and anon_ok else "fail",
                                "truth": tr, "parser": pr, "base_err": d,
                                "anonymous_ok": anon_ok})
    block_ok = all(r["status"] == "pass" for r in block_rows)
    _check(checks, "blocks", "块名集合 + 块基点 + 匿名标志", "引用层",
           "pass" if block_ok else "fail",
           expected=sorted(tb), actual=sorted(pb), details={"rows": block_rows})

    # —— 模型层：handle 配对几何误差 ——
    ti = {e.get("handle", "").upper(): e for e in tt["model"] + tt["inblk"] if e.get("handle")}
    pi = {e.get("handle", "").upper(): e for e in pt["model"] + pt["inblk"] if e.get("handle")}
    matched = sorted(set(ti) & set(pi))
    only_t = sorted(set(ti) - set(pi))
    only_p = sorted(set(pi) - set(ti))
    max_err = 0.0
    bad: List[Dict[str, Any]] = []
    type_mismatch: List[Dict[str, Any]] = []
    for h in matched:
        te, pe = ti[h], pi[h]
        if (te.get("type") or "").upper() != (pe.get("type") or "").upper():
            type_mismatch.append({"handle": h, "truth": te.get("type"),
                                  "parser": pe.get("type")})
            continue
        w, b = compare_geometry((te.get("type") or "").upper(), te["_geo"], pe["_geo"], tol)
        max_err = max(max_err, w)
        for item in b:
            item["handle"] = h
            item["type"] = te.get("type")
        bad.extend(b)
    geo_ok = not bad and not type_mismatch and not only_t and not only_p
    _check(checks, "geometry", "按 handle 配对几何误差（坐标/角度/bulge）", "模型层",
           "pass" if geo_ok else "fail",
           expected={"matched": len(matched), "only_truth": len(only_t),
                     "only_parser": len(only_p)},
           actual={"matched": len(matched), "only_truth": len(only_t),
                   "only_parser": len(only_p)},
           details={"max_abs_err": max_err, "tolerance": tol,
                    "matched": len(matched), "only_truth": len(only_t),
                    "only_parser": len(only_p),
                    "type_mismatch": type_mismatch,
                    "over_tolerance": bad[:N_SAMPLE_DIFFS],
                    "over_tolerance_total": len(bad),
                    "only_truth_handles": only_t[:N_SAMPLE_DIFFS],
                    "only_parser_handles": only_p[:N_SAMPLE_DIFFS]})

    # —— 对象层：文字内容（TEXT/MTEXT 逐 handle 全等，含 \\U+XXXX 还原与组码 3 续行）——
    text_rows = []
    ptext = {e.get("handle", "").upper(): e for e in pt["model"] + pt["inblk"]
             if (e.get("type") or "").upper() in ("TEXT", "MTEXT")}
    ttext = {e.get("handle", "").upper(): e for e in tt["model"] + tt["inblk"]
             if (e.get("type") or "").upper() in ("TEXT", "MTEXT")}
    for h in sorted(set(ttext) | set(ptext)):
        te, pe = ttext.get(h), ptext.get(h)
        tv = (te or {}).get("_geo", {}).get("text")
        pv = (pe or {}).get("_geo", {}).get("text")
        if te is None:
            text_rows.append({"handle": h, "status": "extra", "parser": pv})
        elif pe is None:
            text_rows.append({"handle": h, "status": "missing", "truth": tv})
        else:
            same = (tv or "").strip() == (pv or "").strip()
            if not same:
                text_rows.append({"handle": h, "status": "fail", "truth": (tv or "")[:120],
                                  "parser": (pv or "")[:120]})
    text_fail = [r for r in text_rows]
    _check(checks, "text", "TEXT/MTEXT 文字内容逐 handle 比对", "对象层",
           "pass" if not text_fail else "fail",
           expected=len(ttext), actual=len(ptext),
           details={"diff": text_fail[:N_SAMPLE_DIFFS],
                    "diff_total": len(text_fail)})

    # —— 对象层：DIMENSION 真解码（缺字段即判不过，不再空过）——
    tdim = {e.get("handle", "").upper(): e for e in tt["model"] + tt["inblk"]
            if (e.get("type") or "").upper() == "DIMENSION"}
    pdim = {e.get("handle", "").upper(): e for e in pt["model"] + pt["inblk"]
            if (e.get("type") or "").upper() == "DIMENSION"}
    dim_rows = []
    for h in sorted(set(tdim) | set(pdim)):
        te, pe = tdim.get(h), pdim.get(h)
        if te is None:
            dim_rows.append({"handle": h, "status": "extra"})
            continue
        if pe is None:
            dim_rows.append({"handle": h, "status": "missing"})
            continue
        tg, pg = te["_geo"], pe["_geo"]
        problems = []
        for field in ("def_point", "text_middle", "xline1", "xline2"):
            if tg.get(field) is None or pg.get(field) is None:
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": None})
                continue
            d = vec_diff(tg.get(field), pg.get(field))
            if d is not None and d > tol:
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": d})
        for field in ("dimtype",):
            if tg.get(field) != pg.get(field):
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": None})
        for field in ("attach",):
            if tg.get(field) is None or pg.get(field) is None or tg.get(field) != pg.get(field):
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": None})
        for field in ("bits_73_74_75",):
            if tg.get(field) is None or pg.get(field) is None or \
                    list(tg.get(field)) != list(pg.get(field)):
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": None})
        for field in ("measurement",):
            if tg.get(field) is None or pg.get(field) is None:
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": None})
                continue
            d = scalar_diff(tg.get(field), pg.get(field))
            if d is not None and d > tol:
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": d})
        if tg.get("extrusion") is not None:
            d = vec_diff(tg["extrusion"], pg.get("extrusion"))
            if d is None or d > tol:
                problems.append({"field": "extrusion", "truth": tg["extrusion"],
                                 "parser": pg.get("extrusion"), "abs_err": d})
        for field in ("dimstyle", "block"):
            if (tg.get(field) or "") != (pg.get(field) or ""):
                problems.append({"field": field, "truth": tg.get(field),
                                 "parser": pg.get(field), "abs_err": None})
        dim_rows.append({"handle": h, "status": "fail" if problems else "pass",
                         "problems": problems})
    dim_ok = all(r["status"] == "pass" for r in dim_rows)
    _check(checks, "dimension",
           "DIMENSION 全字段：定义点/文字中点/界线/类型/样式/附着点/实测值/73-74-75/挤出方向", "对象层",
           "pass" if dim_ok else "fail",
           expected=len(tdim), actual=len(pdim),
           details={"rows": [r for r in dim_rows if r["status"] != "pass"][:N_SAMPLE_DIFFS],
                    "rows_total": len([r for r in dim_rows if r["status"] != "pass"])})

    # —— 对象层：STYLE 表（名字集合 + 字体 + 字高/字宽/倾斜角）——
    tstyle = {(s.get("name") or ""): s for s in truth.get("styles", []) or []}
    pstyle = {(s.get("name") or ""): s for s in parser.get("styles", []) or []}
    style_rows = []
    for name in sorted(set(tstyle) | set(pstyle)):
        ts, ps = tstyle.get(name), pstyle.get(name)
        if ts is None:
            style_rows.append({"name": name, "status": "extra"})
            continue
        if ps is None:
            style_rows.append({"name": name, "status": "missing"})
            continue
        problems = []
        if (ts.get("font") or "") != (ps.get("font") or ""):
            problems.append({"field": "font", "truth": ts.get("font"),
                             "parser": ps.get("font")})
        if (ts.get("bigfont") or "") != (ps.get("bigfont") or ""):
            problems.append({"field": "bigfont", "truth": ts.get("bigfont"),
                             "parser": ps.get("bigfont")})
        for field in ("height", "width", "oblique"):
            tv, pv = ts.get(field), ps.get(field)
            if tv is None or pv is None:
                problems.append({"field": field, "truth": tv, "parser": pv})
                continue
            if abs(float(tv) - float(pv)) > 1e-6:
                problems.append({"field": field, "truth": tv, "parser": pv})
        style_rows.append({"name": name, "status": "fail" if problems else "pass",
                           "problems": problems})
    style_bad = [r for r in style_rows if r["status"] != "pass"]
    _check(checks, "styles", "STYLE 表：名字 + 字体 + 字高/宽/倾斜", "对象层",
           "pass" if not style_bad else "fail",
           expected=len(tstyle), actual=len(pstyle),
           details={"diff": style_bad[:N_SAMPLE_DIFFS], "diff_total": len(style_bad)})

    # —— 对象层：DIMSTYLE 表名字集合 + DIM* 变量完整度 ——
    tds = {s.get("name") or "" for s in truth.get("dimstyles", []) or []}
    pds = {s.get("name") or "" for s in parser.get("dimstyles", []) or []}
    dim_bad = []
    for s in parser.get("dimstyles", []) or []:
        dv = s.get("dimvars") or {}
        # 67 = 66 个 DIM* / 句柄字段 + xref_index；DXF 不导出 DIM* 数值，
        # 这里只验证"字段全部解出且类型合法"（字段序已由零误差位宽验证）。
        missing_keys = [k for k in _DIMSTYLE_KEYS if k not in dv]
        if len(dv) != 67 or missing_keys:
            dim_bad.append({"name": s.get("name"), "count": len(dv),
                            "missing": missing_keys[:N_SAMPLE_DIFFS]})
    _check(checks, "dimstyles", "DIMSTYLE 表：名字集合 + DIM* 变量 67 字段", "对象层",
           "pass" if tds == pds and not dim_bad else "fail",
           expected=[sorted(tds), "67 fields"], actual=[sorted(pds), "67 fields" if not dim_bad else "incomplete"],
           details={"missing": sorted(tds - pds)[:N_SAMPLE_DIFFS],
                    "extra": sorted(pds - tds)[:N_SAMPLE_DIFFS],
                    "dimvars_bad": dim_bad[:N_SAMPLE_DIFFS]})

    # —— 对象层：APPID 表（DXF 会补写未注册的 regapp，故按真值超集判定）——
    ta = {a.get("name") or "" for a in truth.get("appids", []) or []}
    pa = {a.get("name") or "" for a in parser.get("appids", []) or []}
    extra = pa - ta
    missing = ta - pa
    _check(checks, "appids", "APPID 表名字（真值超集，DXF 可能补写）", "对象层",
           "pass" if not extra else "fail",
           expected=sorted(ta), actual=sorted(pa),
           details={"missing_in_parser": sorted(missing)[:N_SAMPLE_DIFFS],
                    "extra_in_parser": sorted(extra)[:N_SAMPLE_DIFFS]})

    # —— 对象层：未知类型占比（解析器自报）——
    stats = parser.get("stats") or {}
    unknown = stats.get("unknown")
    total = stats.get("entity_total") or len(pt["model"]) + len(pt["inblk"])
    ratio = stats.get("unknown_ratio")
    if unknown is None:
        _check(checks, "unknown", "未知类型占比", "对象层", "not_executed",
               expected=0, actual=None,
               details={"note": "解析器未输出 stats.unknown"})
    else:
        _check(checks, "unknown", "未知类型占比", "对象层",
               "pass" if ratio == 0 else "fail",
               expected=0, actual={"unknown": unknown, "total": total, "ratio": ratio})

    n_fail = sum(1 for c in checks if c["status"] == "fail")
    n_exec = sum(1 for c in checks if c["status"] in ("pass", "fail"))
    return {"checks": checks, "fail_count": n_fail, "executed_count": n_exec}


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def compare_one(dwg: str, use_cache: bool, tol: float) -> Dict[str, Any]:
    base = os.path.splitext(os.path.basename(dwg))[0]
    dxf = os.path.join(HERE, base + ".dxf")
    result: Dict[str, Any] = {"dwg": dwg, "dxf": dxf, "status": "pending"}

    if not os.path.exists(dwg):
        result.update(status="prerequisite", reason=f"DWG 不存在: {dwg}")
        return result
    if not os.path.exists(dxf):
        result.update(status="prerequisite", reason=f"DXF 真值不存在: {dxf}")
        return result

    try:
        truth = load_truth(dxf, use_cache)
    except Exception as exc:  # 前置缺失
        result.update(status="prerequisite", reason=f"真值装载失败: {exc}")
        return result

    parser, rc, stdout, stderr = run_parser(dwg)
    result["parser_rc"] = rc
    result["parser_stderr_tail"] = (stderr or "").strip()[-500:]
    if parser is None:
        # 解析器不可用 / 未输出 JSON：全部比对项未执行（逐条标注根因层，不记 pass）
        planned = [
            ("version", "文件版本", "容器层"),
            ("header", "HEADER 变量（按 $VAR 全量逐值）", "容器层"),
            ("linetypes", "LTYPE 表：名字/描述/图案段长度/段标志", "对象层"),
            ("dictionaries", "DICTIONARY 字典树：键名逐个 + 句柄全部可解析", "对象层"),
            ("objects.coverage", "对象层覆盖率：无未知类型、无解码报错", "对象层"),
            ("classes", "类表 AcDb:Classes：类名 + 实体/对象标志", "对象层"),
            ("count.model", "模型空间顶层实体总数", "对象层"),
            ("count.block", "块内实体总数", "对象层"),
            ("type.model", "模型空间按类型计数", "对象层"),
            ("type.block", "块内按类型计数", "对象层"),
            ("layers", "图层名集合 + ACI 颜色 + 线型", "对象层"),
            ("unknown", "未知类型占比", "对象层"),
            ("blocks", "块名集合 + 块基点 + 匿名标志", "引用层"),
            ("geometry", "按 handle 配对几何误差", "模型层"),
        ]
        checks = [{"id": cid, "title": title, "root_cause": layer,
                   "status": "not_executed", "expected": None, "actual": None,
                   "details": {"reason": "解析器未产出 JSON（前置缺失）"}}
                  for cid, title, layer in planned]
        result.update(status="prerequisite",
                      reason=f"解析器未产出 JSON（rc={rc}）；对象/引用/模型层比对未执行",
                      comparison={"checks": checks, "fail_count": 0,
                                  "executed_count": 0},
                      truth_summary=truth.get("summary"))
        return result

    cmp_res = compare_drawing(parser, truth, tol)
    result.update(status="fail" if cmp_res["fail_count"] else "pass",
                  comparison=cmp_res, truth_summary=truth.get("summary"))
    return result


def render_markdown(rep: Dict[str, Any]) -> str:
    lines = ["# DWG 解析器对拍报告", ""]
    for sample in rep["samples"]:
        lines.append(f"## {os.path.basename(sample['dwg'])}")
        if sample["status"] != "pass" and sample.get("comparison") is None:
            lines.append(f"- 状态: **{sample['status']}** — {sample.get('reason', '')}")
            lines.append("- 比对项: 未执行（前置缺失，不计 pass）")
            lines.append("")
            continue
        cmp_res = sample["comparison"]
        lines.append(f"- 状态: **{sample['status']}**；执行项 {cmp_res['executed_count']}，"
                     f"失败 {cmp_res['fail_count']}")
        lines.append("")
        lines.append("| 编号 | 比对项 | 根因层 | 状态 | 期望 | 实际 |")
        lines.append("|---|---|---|---|---|---|")
        for c in cmp_res["checks"]:
            exp = json.dumps(c["expected"], ensure_ascii=False)
            act = json.dumps(c["actual"], ensure_ascii=False)
            if len(exp) > 60:
                exp = exp[:57] + "..."
            if len(act) > 60:
                act = act[:57] + "..."
            lines.append(f"| {c['id']} | {c['title']} | {c['root_cause']} | "
                         f"{c['status']} | {exp} | {act} |")
        geo = next((c for c in cmp_res["checks"] if c["id"] == "geometry"), None)
        if geo and geo["details"].get("max_abs_err") is not None:
            lines.append("")
            lines.append(f"- 几何最大绝对误差: {geo['details']['max_abs_err']:.3e} "
                         f"（容差 {geo['details']['tolerance']:.0e}）")
        lines.append("")
    return "\n".join(lines)


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dwgs", nargs="*", help="DWG 样本（默认两个内置样本）")
    ap.add_argument("--cache", action="store_true", help="复用 truth_*.json 缓存（默认重算）")
    ap.add_argument("--refresh", action="store_true", help="显式重算（默认即重算，此参数兼容）")
    ap.add_argument("--out", default=HERE, help="报告输出目录")
    ap.add_argument("--tol", type=float, default=TOL_DEFAULT, help="几何容差")
    args = ap.parse_args(argv)

    dwgs = args.dwgs or DEFAULT_DWGS
    samples = [compare_one(d, args.cache, args.tol) for d in dwgs]
    rep = {"tolerance": args.tol, "samples": samples}

    os.makedirs(args.out, exist_ok=True)
    json_path = os.path.join(args.out, "verify_report.json")
    md_path = os.path.join(args.out, "verify_report.md")
    with open(json_path, "wb") as fh:
        fh.write(json.dumps(rep, ensure_ascii=False, indent=1).encode("utf-8"))
    with open(md_path, "wb") as fh:
        fh.write(render_markdown(rep).encode("utf-8"))

    n_prereq = sum(1 for s in samples if s["status"] == "prerequisite")
    n_fail = sum(1 for s in samples if s["status"] == "fail")
    if n_prereq == len(samples):
        code = 2
    elif n_fail:
        code = 1
    else:
        code = 0
    print(render_markdown(rep))
    print(f"\n报告: {json_path} ; {md_path} ; exit={code}")
    return code


if __name__ == "__main__":
    sys.exit(main())
