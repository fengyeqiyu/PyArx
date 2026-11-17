# -*- coding: utf-8 -*-
"""PyArx 建图元示例函数集（全部在 AutoCAD 2024 里实测通过）

用法（在 CAD 内、由 bridge/MCP 注入 P=PyArxCore 时执行）：

    import runpy, json
    m = runpy.run_path(r"G:/PyArx/tools/cadmcp/examples/create_entities.py")
    result = m["run_demo"]()

设计约束（踩过坑才这么写）：
  * 入参用 (x, y) / (x, y, z) 元组，函数内部统一转 AcGePoint3d / AcGePoint2d，
    避免每处都写 P.Ge.xxx 出错。
  * 所有加实体都走 `with P.Db.PyDbTrans() as tr: ... tr.addEntity(e)`。
  * 引线必须 appendVertex（空引线上 setVertexAt 会越界写堆 → 0xc0000374 崩溃）。
  * 样条用 setFitData(5 参起)；填充要 evaluateHatch 才会出现图案。
  * 图层用命令通道（PyDbLayerTableRecord 没有默认构造）；块定义走命令通道。
"""

try:
    P                # bridge / PYRUNFILE 已注入
except NameError:
    P = None

LAST_ERROR = {}


def _v(p):
    if len(p) == 2:
        return P.Ge.AcGeVector3d(p[0], p[1], 0.0)
    return P.Ge.AcGeVector3d(p[0], p[1], p[2])


def _p(p):
    if len(p) == 2:
        return P.Ge.AcGePoint2d(p[0], p[1])
    return P.Ge.AcGePoint3d(p[0], p[1], p[2])


def _p3(p):
    if len(p) == 2:
        return P.Ge.AcGePoint3d(p[0], p[1], 0.0)
    return P.Ge.AcGePoint3d(p[0], p[1], p[2])


def _add(ent):
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(ent)
    return ent


def current_space_ids():
    return [str(i) for i in P.Db.curDb().objectIds("AcDbEntity")]


# ---------------------------------------------------------------- 直线/多段线

def create_line(p1, p2):
    """直线：PyDbLine(startPoint, endPoint)。"""
    return _add(P.Db.PyDbLine(_p3(p1), _p3(p2)))


def create_polyline(pts, closed=False):
    """二维多段线：PyDbPolyline() + addVertexAt(i, AcGePoint2d) + setClosed。"""
    pl = P.Db.PyDbPolyline()
    for i, p in enumerate(pts):
        pl.addVertexAt(i, _p(p))
    pl.setClosed(bool(closed))
    return _add(pl)


def create_polyline3d(pts, closed=False):
    """三维多段线：PyDbPolyline3d() + appendVertex(PyDb3dPolylineVertex(pt))（不收索引）。"""
    pl = P.Db.PyDbPolyline3d()
    for p in pts:
        pl.appendVertex(P.Db.PyDb3dPolylineVertex(_p3(p)))
    pl.setClosed(bool(closed))
    return _add(pl)


# ---------------------------------------------------------------- 圆/弧/椭圆/样条/点

def create_circle(center, radius, normal=(0, 0, 1)):
    """圆：PyDbCircle(center, normal, radius)。"""
    return _add(P.Db.PyDbCircle(_p3(center), _v(normal), float(radius)))


def create_arc(center, radius, start_deg, end_deg, normal=(0, 0, 1)):
    """圆弧：PyDbArc(center, radius, startRad, endRad) —— 角度是弧度。"""
    import math
    return _add(P.Db.PyDbArc(_p3(center), float(radius),
                             math.radians(start_deg), math.radians(end_deg)))


def create_ellipse(center, major_axis_vec, ratio):
    """椭圆：PyDbEllipse(center, normal, majorAxisVec, ratioOfMinorToMajor)。"""
    return _add(P.Db.PyDbEllipse(_p3(center), _v((0, 0, 1)), _v(major_axis_vec), float(ratio)))


def create_spline(pts, start_tan=(1, 0, 0), end_tan=(-1, 0, 0)):
    """样条：setFitData(fitPoints, startTangent, endTangent, knotParam, degree, tol)。"""
    sp = P.Db.PyDbSpline()
    ok = sp.setFitData([_p3(p) for p in pts], _v(start_tan), _v(end_tan), 0, 3, 0.0)
    if not ok:
        raise RuntimeError("setFitData 返回 False")
    return _add(sp)


def create_point(pt):
    """点：PyDbPoint(point)。"""
    return _add(P.Db.PyDbPoint(_p3(pt)))


# ---------------------------------------------------------------- 射线/构造线

def create_ray(base, through):
    """射线：默认构造 + setBasePoint + setSecondPoint。"""
    r = P.Db.PyDbRay()
    r.setBasePoint(_p3(base))
    r.setSecondPoint(_p3(through))
    return _add(r)


def create_xline(p1, p2):
    """构造线：无限长，默认构造 + 两点。"""
    x = P.Db.PyDbXline()
    x.setBasePoint(_p3(p1))
    x.setSecondPoint(_p3(p2))
    return _add(x)


# ---------------------------------------------------------------- 实体填充

def create_solid(p1, p2, p3, p4=None):
    """二维实体填充：PyDbSolid(p1..p4)；p4 省略时退化成三角形。"""
    if p4 is None:
        p4 = p3
    return _add(P.Db.PyDbSolid(_p3(p1), _p3(p2), _p3(p3), _p3(p4)))


def create_trace(p1, p2, p3, p4):
    """宽线（trace）。"""
    return _add(P.Db.PyDbTrace(_p3(p1), _p3(p2), _p3(p3), _p3(p4)))


def create_hatch(pts, pattern="SOLID", pattern_type=1, scale=1.0):
    """图案填充：setPattern → appendLoopPoints(1, pts, []) → evaluateHatch（三步缺一不可）。"""
    h = P.Db.PyDbHatch()
    h.setDatabaseDefaults()
    h.setAssociative(False)
    h.setPattern(pattern_type, pattern)
    h.setPatternScale(float(scale))
    if not h.appendLoopPoints(1, [_p(p) for p in pts], []):
        raise RuntimeError("appendLoopPoints 返回 False")
    h.evaluateHatch()
    return _add(h)


# ---------------------------------------------------------------- 文字/多行文字

def create_text(pos, text, height=5.0, rotation_deg=0.0):
    """单行文字：注意签名是 (position, text, height, rotation)。"""
    import math
    return _add(P.Db.PyDbText(_p3(pos), str(text), float(height), math.radians(rotation_deg)))


def create_mtext(pos, text, height=5.0, width=100.0):
    """多行文字：默认构造 + setter（没有位置/文本构造）。"""
    m = P.Db.PyDbMText()
    m.setLocation(_p3(pos))
    m.setContents(str(text))
    m.setTextHeight(float(height))
    m.setWidth(float(width))
    return _add(m)


# ---------------------------------------------------------------- 标注

def create_dim_rotated(p1, p2, dim_line_pt, text_pos=None, rotation_deg=0.0):
    """旋转标注：三点点位 + 转角。"""
    import math
    d = P.Db.PyDbRotatedDimension()
    d.setXLine1Point(_p3(p1))
    d.setXLine2Point(_p3(p2))
    d.setDimLinePoint(_p3(dim_line_pt))
    d.setRotation(math.radians(rotation_deg))
    if text_pos:
        d.setTextPosition(_p3(text_pos))
    return _add(d)


def create_dim_aligned(p1, p2, dim_line_pt):
    """对齐标注。"""
    d = P.Db.PyDbAlignedDimension()
    d.setXLine1Point(_p3(p1))
    d.setXLine2Point(_p3(p2))
    d.setDimLinePoint(_p3(dim_line_pt))
    return _add(d)


# ---------------------------------------------------------------- 引线

def create_leader(pts, text=None):
    """引线：必须 appendVertex 逐点添加（空引线 setVertexAt 会崩 CAD）。"""
    ld = P.Db.PyDbLeader()
    for p in pts:
        if not ld.appendVertex(_p3(p)):
            raise RuntimeError("appendVertex 返回 False")
    ld.setToStraightLeader()
    if text:
        ld.setAnnotationText(text) if hasattr(ld, "setAnnotationText") else None
    return _add(ld)


# ---------------------------------------------------------------- 表格

def create_table(pos, rows=3, cols=3, row_h=8.0, col_w=30.0, texts=None):
    """表格：setSize → setPosition → 行列尺寸 → setTextString。"""
    t = P.Db.PyDbTable()
    t.setSize(int(rows), int(cols))
    t.setPosition(_p3(pos))
    for i in range(int(cols)):
        t.setColumnWidth(i, float(col_w))
    for i in range(int(rows)):
        t.setRowHeight(i, float(row_h))
    if texts:
        for r in range(min(int(rows), len(texts))):
            for c in range(min(int(cols), len(texts[r]))):
                t.setTextString(r, c, str(texts[r][c]))
    return _add(t)


# ---------------------------------------------------------------- 块与图层

def create_block_ref(block_name, pos, scale=1.0, rotation_deg=0.0):
    """块参照：PyDbBlockReference() + setBlockTableRecord(块定义 id) + setPosition。"""
    import math
    bt = P.Db.curDb().getBlockTable()
    if not bt.has(block_name):
        raise KeyError("块定义不存在: " + block_name)
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(bt.getAt(block_name).objectId())
    br.setPosition(_p3(pos))
    br.setScaleFactors(scale, scale, scale)
    br.setRotation(math.radians(rotation_deg))
    return _add(br)


def create_layer(name, rgb=(255, 255, 0)):
    """新建图层（纯 API，不调任何 CAD 命令）。

    PyDbLayerTableRecord 原来只绑了裸指针构造，Python 侧无法 new；
    已补工厂 PyDbLayerTableRecord.newRecord() → lt.add(rec) → addNewlyCreatedDBObject。
    """
    with P.Db.PyDbTrans() as tr:
        lt = P.Db.curDb().getLayerTable()
        if lt.has(name):
            return {"layer": name, "exists": True, "created": False}
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(name)
        rec.setColor(int(rgb[0]), int(rgb[1]), int(rgb[2]))
        if not lt.add(rec):
            raise RuntimeError("LayerTable.add 返回 False")
        tr.addNewlyCreatedDBObject(rec, True)
    with P.Db.PyDbTrans() as tr:
        exists = bool(P.Db.curDb().getLayerTable().has(name))
    return {"layer": name, "exists": exists, "created": exists}


def create_block_def_command(name, base_pt=(0, 0, 0), ents=None):
    """块定义仍未打通 API（事务外 getBlockTable().add 会断言）。
    按"所有操作必须经过 PyArx"的要求，这里不提供命令通道替代，直接报未实现。"""
    raise NotImplementedError(
        "块定义 API 未打通：PyDbTrans.openObject(blockTableId) 返回 PyDbObject，缺 PyDbBlockTable 强转；"
        "需补 bind 后再开放")


def create_block_def_command(name, base_pt=(0, 0, 0), ents=None):
    """块定义：同样走命令通道（PyDbBlockTable.add 在事务外会断言/挂死）。"""
    raise NotImplementedError(
        "块定义请用命令通道：_.-BLOCK / _.INSERT，或先在模板图里做好块定义再 create_block_ref")


# ---------------------------------------------------------------- DEMO

DEMO = [
    ("line",         lambda: create_line((0, 0), (100, 50))),
    ("polyline",     lambda: create_polyline([(120, 0), (180, 0), (180, 60), (120, 60)], closed=True)),
    ("circle",       lambda: create_circle((250, 30), 30)),
    ("arc",          lambda: create_arc((320, 30), 30, 0, 180)),
    ("ellipse",      lambda: create_ellipse((400, 30), (40, 0, 0), 0.5)),
    ("spline",       lambda: create_spline([(470, 0), (500, 60), (540, 10), (580, 60)])),
    ("point",        lambda: create_point((620, 30))),
    ("ray",          lambda: create_ray((650, 0), (700, 40))),
    ("xline",        lambda: create_xline((650, 80), (700, 60))),
    ("solid",        lambda: create_solid((0, -80), (60, -80), (60, -20), (0, -20))),
    ("trace",        lambda: create_trace((80, -80), (140, -80), (140, -40), (80, -40))),
    ("hatch",        lambda: create_hatch([(160, -80), (240, -80), (240, -20), (160, -20)])),
    ("text",         lambda: create_text((0, -130), "PyArx Text", 8.0)),
    ("mtext",        lambda: create_mtext((100, -130), "多行文字\\nMText 示例", 6.0, 150.0)),
    ("dim_rot",      lambda: create_dim_rotated((0, -200), (100, -200), (50, -230))),
    ("dim_align",    lambda: create_dim_aligned((150, -200), (250, -210), (200, -240))),
    ("leader",       lambda: create_leader([(320, -190), (360, -220), (420, -220)])),
    ("polyline3d",   lambda: create_polyline3d([(0, 0, 0), (60, 30, 20), (120, 0, 40)], False)),
    ("table",        lambda: create_table((450, -220), 3, 3, 8.0, 30.0,
                                          [["PyArx", "表格", "示例"],
                                           ["line", "circle", "arc"],
                                           ["hatch", "spline", "leader"]])),
    ("layer",        lambda: create_layer("PYARX_DEMO", (255, 255, 0))),
]


def _fmt(ent):
    if isinstance(ent, dict):
        return ent
    try:
        return {"handle": str(ent.handle()), "class": ent.className()}
    except Exception:
        return {"handle": "(n/a)"}


def run_demo(clear=True, only=None, verbose=False):
    """逐项执行 DEMO，返回 {name: handle} 与失败清单。"""
    out = {"created": {}, "failed": {}, "errors": {}}
    before = len(current_space_ids())

    if clear:
        with P.Db.PyDbTrans() as tr:
            for eid in P.Db.curDb().objectIds("AcDbEntity"):
                tr.openObject(eid, 1).erase()

    for name, fn in DEMO:
        if only and name not in only:
            continue
        try:
            r = fn()
            out["created"][name] = _fmt(r)
        except Exception as e:
            LAST_ERROR[name] = repr(e)
            out["failed"][name] = repr(e)

    out["entity_count_before"] = before
    out["entity_count_after"] = len(current_space_ids())
    out["file"] = P.Db.curDb().getFilename()
    return out
