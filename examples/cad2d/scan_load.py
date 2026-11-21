# -*- coding: utf-8 -*-
"""scan_load —— PyArx 扫描结果（scan.json）→ cad2d 内部文档适配器。

分工（与 dwg_load 同级、同构）：
  · PyArx（acoreconsole + PyArxEngine.crx + pyarx_vision/scan.py）只负责"读图出数"，
    产出一份中立 JSON：lines / plines / circles / arcs / texts / blocks / block_defs / layers；
  · 本模块只做字段、单位、坐标语义到 cad2d 内部 ``db / ent`` 的映射；
  · 渲染仍走 cad2d 现有画布（render.to_svg），本模块不碰绘制。

为什么需要它：
  PyArxDwg 直接解码在新版 DWG 上会撞 ``AcDb:FileDepList`` 加密 section（实测 4/4 失败），
  而 accoreconsole 是 AutoCAD 本体引擎，不存在版本与加密覆盖问题。

单位约定（scan.json 的实测语义）：
  · 所有角度（arc 的 a0/a1、text 的 r、block 的 r）都是 **弧度**，内部 ent 用 **度**；
  · 块定义 geom 的坐标是 **块局部坐标**，基点按 (0,0)，由 Insert 的 s/sy/r 摆位；
  · 颜色 256 = ByLayer，内部映射为 0；
  · layer 表是 ``{名: ACI}``，style 表是 ``{句柄: 样式名}``。

入口：
  · ``load_scan(path) -> doc.Document``            读现成 JSON
  · ``build_document(data) -> doc.Document``       对已解析的 dict 映射
  · ``scan_dwg(dwg, out_json=None, ...)``          端到端：跑 console 取数并映射（带缓存）
"""
import hashlib
import json
import math
import os
import shutil
import subprocess
import tempfile
import time

import doc
import ent
from dwg_load import _clean_mtext, _color, _font


CRX = r"G:\PyArx\X64\Release\PyArxEngine.crx"
SCAN_PY = r"G:\PyArx\pyarx_vision\scan.py"
_CACHE_DIR = os.path.join(tempfile.gettempdir(), "pyarx_scan")


def console_path():
    """无头控制台：优先项目自带副本（G:\\PyArx\\ACAD），回退本机安装。"""
    p = os.environ.get("PYARX_CONSOLE") or r"G:\PyArx\ACAD\accoreconsole.exe"
    if os.path.isfile(p):
        return p
    return r"C:\Program Files\Autodesk\AutoCAD 2024\accoreconsole.exe"


def _deg(rad):
    try:
        return math.degrees(float(rad or 0.0))
    except (TypeError, ValueError):
        return 0.0


def _xy(p, default=0.0):
    if not p:
        return (default, default)
    return (float(p[0]), float(p[1] if len(p) > 1 else default))


def _align(ha, va):
    """AutoCAD 的 ha/va → 内部 Text.align（1-9：上排 1/2/3、中排 4/5/6、下排 7/8/9）。"""
    col = {0: 0, 1: 1, 2: 2, 3: 0, 4: 1, 5: 0}.get(int(ha or 0), 0)
    row = 0 if int(va or 0) == 3 else (1 if int(va or 0) == 2 else 2)
    return row * 3 + col + 1


# ------------------------------------------------------------------ 主空间图元
def _line(rec):
    """[x1, y1, x2, y2, layer, color] —— 主空间只报二维端点，没有 z。"""
    if len(rec) < 5:
        return None
    layer = rec[4] if len(rec) > 4 else "0"
    color = _color(rec[5] if len(rec) > 5 else 0)
    return ent.Line(float(rec[0]), float(rec[1]), float(rec[2]), float(rec[3]),
                    layer=str(layer), color=color)


def _pline(rec):
    pts = [_xy(p) for p in (rec.get("p") or [])]
    if len(pts) < 2:
        return None
    return ent.Polyline(pts, closed=bool(rec.get("c")),
                        layer=str(rec.get("l") or rec.get("ly") or "0"),
                        color=_color(rec.get("ci")))


def _circle(rec):
    """[cx, cy, r, layer, color]"""
    if len(rec) < 3:
        return None
    return ent.Circle(float(rec[0]), float(rec[1]), float(rec[2]),
                      layer=str(rec[3] if len(rec) > 3 else "0"),
                      color=_color(rec[4] if len(rec) > 4 else 0))


def _arc(rec):
    """{c:[cx,cy], r, a0, a1, l, ci}  角度为弧度"""
    cx, cy = _xy(rec.get("c"))
    return ent.Arc(cx, cy, float(rec.get("r") or 0.0),
                   _deg(rec.get("a0")), _deg(rec.get("a1")),
                   layer=str(rec.get("l") or "0"), color=_color(rec.get("ci")))


def _text(rec, layer_key="l"):
    t = _clean_mtext(rec.get("t"))
    if not t:
        return None
    return ent.Text(float(rec.get("x") or 0.0), float(rec.get("y") or 0.0), t,
                    size=float(rec.get("h") or 0.0) or 200.0,
                    style=_font(rec.get("st")),
                    rot=_deg(rec.get("r")),
                    align=_align(rec.get("ha"), rec.get("va")),
                    layer=str(rec.get(layer_key) or "0"),
                    color=_color(rec.get("ci")))


def _insert(rec, layer_key="l", name_key="n"):
    p = rec.get("p")
    if not p:
        return None
    name = str(rec.get(name_key) or "")
    if not name:
        return None
    sx = float(rec.get("s") or 1.0)
    sy = float(rec.get("sy") or sx)
    return ent.Insert(name, float(p[0]), float(p[1]), sx=sx, sy=sy,
                      rot=_deg(rec.get("r")),
                      layer=str(rec.get(layer_key) or "0"),
                      color=_color(rec.get("ci")))


# ------------------------------------------------------------------ 块定义（局部坐标）
def _block_entities(geom):
    """块定义 geom → 内部图元列表。块内各几何的字段数与主空间不同，分开解。"""
    out = []

    for rec in geom.get("lines") or []:
        # [x1, y1, x2, y2, color, layer]
        if len(rec) < 4:
            continue
        out.append(ent.Line(float(rec[0]), float(rec[1]), float(rec[2]), float(rec[3]),
                            color=_color(rec[4] if len(rec) > 4 else 0),
                            layer=str(rec[5] if len(rec) > 5 else "0")))

    for rec in geom.get("plines") or []:
        e = _pline(rec)
        if e is not None:
            out.append(e)

    for rec in geom.get("circles") or []:
        # [cx, cy, r, color, layer]
        if len(rec) < 3:
            continue
        out.append(ent.Circle(float(rec[0]), float(rec[1]), float(rec[2]),
                              color=_color(rec[3] if len(rec) > 3 else 0),
                              layer=str(rec[4] if len(rec) > 4 else "0")))

    for rec in geom.get("arcs") or []:
        # [cx, cy, r, a0, a1, color, layer]  角度弧度
        if len(rec) < 5:
            continue
        out.append(ent.Arc(float(rec[0]), float(rec[1]), float(rec[2]),
                           _deg(rec[3]), _deg(rec[4]),
                           color=_color(rec[5] if len(rec) > 5 else 0),
                           layer=str(rec[6] if len(rec) > 6 else "0")))

    for rec in geom.get("texts") or []:
        e = _text(rec, layer_key="ly")
        if e is not None:
            out.append(e)

    for rec in geom.get("ins") or []:
        e = _insert(rec, layer_key="ly")
        if e is not None:
            out.append(e)

    return out


def _bbox_of(entities):
    xs, ys = [], []
    for e in entities:
        try:
            x0, y0, x1, y1 = e.bbox()
            xs += [x0, x1]
            ys += [y0, y1]
        except Exception:
            continue
    if not xs:
        return (0.0, 0.0, 12000.0, 8000.0)
    return (min(xs), min(ys), max(xs), max(ys))


# ------------------------------------------------------------------ 文档组装
def build_document(data):
    """scan.json 的 dict → 内部 Document。"""
    docx = doc.Document(12000.0, 8000.0)

    for name, aci in (data.get("layers") or {}).items():
        if isinstance(aci, dict):
            color = _color(aci.get("color"))
            width = float(aci.get("width") or 0.0)
            ltype = aci.get("linetype") or ""
        else:
            color, width, ltype = _color(aci), 0.0, ""
        docx.layers.add(doc.Layer(str(name), color=color, width=width, linetype=ltype))

    block_defs = data.get("block_defs") or {}
    for name, bd in block_defs.items():
        if str(name).startswith("*"):      # *Model_Space / *Paper_Space
            continue
        inner = _block_entities(bd.get("geom") or {})
        docx.blocks.define(str(name), inner, base=(0.0, 0.0))

    mapped, skipped = [], {}

    def take(recs, maker, tag):
        for rec in recs or []:
            e = maker(rec)
            if e is None:
                skipped[tag] = skipped.get(tag, 0) + 1
                continue
            mapped.append(e)

    take(data.get("lines"), _line, "line")
    take(data.get("plines"), _pline, "pline")
    take(data.get("circles"), _circle, "circle")
    take(data.get("arcs"), _arc, "arc")
    take(data.get("texts"), _text, "text")
    take(data.get("blocks"), _insert, "block")

    docx.add_many(mapped)

    # 幅面取图元范围（+10% 留白）：DWG 的 frame 是图纸幅面，城市坐标的大图
    # 一张 sheet 就是上百万单位，直接拿来当幅面会把整图压成一个点。
    bx0, by0, bx1, by1 = _bbox_of(mapped)
    docx.width = max(bx1 - bx0, 1.0)
    docx.height = max(by1 - by0, 1.0)
    # 再用抗离群后的范围定幅面：游离图元不该决定单位换算和默认视图。
    ex0, ey0, ex1, ey1 = docx.extents()
    pad = max(ex1 - ex0, ey1 - ey0, 1.0) * 0.1
    docx.width = max(ex1 - ex0 + pad * 2, 1.0)
    docx.height = max(ey1 - ey0 + pad * 2, 1.0)

    docx._scan_meta = {
        "file": data.get("file"),
        "entity_total": len(mapped),
        "skipped": skipped,
        "block_defs": len(block_defs),
        "tables": len(data.get("tables") or []),
    }
    return docx


def load_scan(path):
    with open(path, encoding="utf-8") as f:
        return build_document(json.load(f))


# ------------------------------------------------------------------ 端到端：console 取数
def _cache_path(dwg_path):
    st = os.stat(dwg_path)
    # 前戳是扫描格式版本：scan.py 新增字段（如块属性、线宽 lw/layer_lw）时要递增，强制重扫
    key = "v6|%s|%d|%d" % (os.path.abspath(dwg_path).lower(), st.st_size, int(st.st_mtime))
    tag = hashlib.md5(key.encode("utf-8")).hexdigest()[:12]
    name = os.path.splitext(os.path.basename(dwg_path))[0]
    return os.path.join(_CACHE_DIR, "%s_%s.json" % (name, tag))


def _materialize_scanner(out_json):
    """scan.py 的结果路径写死在 OUTPUT 常量上；按目标路径生成一份临时副本。"""
    with open(SCAN_PY, encoding="utf-8") as f:
        src = f.read()
    marker = 'OUTPUT = r"C:\\temp\\scan.json"'
    if marker not in src:
        raise RuntimeError("scan.py 的 OUTPUT 常量已变更，无法改写输出路径")
    src = src.replace(marker, 'OUTPUT = r"%s"' % out_json.replace("\\", "\\\\"))
    work = os.path.join(_CACHE_DIR, "scan_%d.py" % int(time.time() * 1000))
    with open(work, "w", encoding="utf-8") as f:
        f.write(src)
    return work


def scan_dwg(dwg_path, out_json=None, timeout=180, use_cache=True):
    """端到端：accoreconsole + PyArxEngine.crx + scan.py → Document。"""
    dwg_path = os.path.abspath(dwg_path)
    if not os.path.isfile(dwg_path):
        raise FileNotFoundError(dwg_path)
    out_json = out_json or _cache_path(dwg_path)
    if not os.path.isdir(_CACHE_DIR):
        os.makedirs(_CACHE_DIR, exist_ok=True)

    if use_cache and os.path.isfile(out_json) and \
            os.path.getmtime(out_json) > os.path.getmtime(dwg_path):
        return load_scan(out_json)

    work = tempfile.mkdtemp(prefix="pyarx_scan_")
    try:
        dwg = os.path.join(work, "in.dwg")
        shutil.copyfile(dwg_path, dwg)
        scanner = _materialize_scanner(out_json)
        scr = os.path.join(work, "run.scr")
        with open(scr, "w", encoding="utf-8") as f:
            f.write('(setvar "SECURELOAD" 0)\n')
            f.write('(arxload "%s")\n' % CRX.replace("\\", "/"))
            # 外部参照（建筑/天正底图）内容不在本图数据库内，块定义注册了但几何为空，
            # 门窗、轴号圈、风机盘管整类图元都会缺；先把 xref 全绑定进来再扫。
            f.write('(command "-XREF" "_B" "*")\n')
            f.write('(command "PYRUNFILE" "%s")\n' % scanner.replace("\\", "/"))
        t0 = time.time()
        proc = subprocess.run([console_path(), "/i", dwg, "/s", scr],
                              capture_output=True, timeout=timeout, cwd=work)
        elapsed = time.time() - t0
    finally:
        shutil.rmtree(work, ignore_errors=True)

    if not os.path.isfile(out_json):
        tail = (proc.stdout or b"").decode("gbk", "replace")[-600:]
        raise RuntimeError("扫描未产出 %s（exit=%s）\n%s" % (out_json, proc.returncode, tail))

    docx = load_scan(out_json)
    docx._scan_meta["elapsed_sec"] = round(elapsed, 2)
    docx._scan_meta["exit"] = proc.returncode
    return docx
