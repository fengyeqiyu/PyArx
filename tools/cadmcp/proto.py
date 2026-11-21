# -*- coding: utf-8 -*-
"""proto.py —— PyArxCore 工具样板库（图元 / 几何运算 / 选择集）。

三层职责：
  1. 映射：kind -> 真实 PyArxCore 类（cad_line -> Db.PyDbLine）
  2. 代码：由模型推算的 args 生成可直接执行的 Python 片段
  3. 验证：每类的断言项（入库成功 / 类型名 / 包围盒 / 属性回读）

约定：
  - 只写"已由实跑证实"的构造；未证实的放 candidates，由 proto_probe 逐条试。
  - 参数一律用图纸单位；角度一律用度，由 _rad() 转弧度（模型不算弧度）。
  - 任何需要猜的几何都由 PyArxCore 算（等分、偏移、包围盒），样板里不写坐标运算。
"""

# ---------------------------------------------------------------- 参数类型
PT = "pt"          # [x, y] 或 [x, y, z]
PTS = "pts"        # [[x,y], ...]
NUM = "number"
STR = "string"
INT = "int"
LAYER = "layer"    # 图层名，缺省随当前层
COLOR = "color"    # ACI 0-256

# ---------------------------------------------------------------- 图元样板
# build: 生成实体的 Python 语句，变量统一叫 e；helpers 有 _p2/_p3/_rad/_a
# checks: 入库后的断言；bbox=True 表示要求 getBoundingPoints 可读
ENTITY = {
    "line": {
        "cls": "PyDbLine",
        "args": {"p1": PT, "p2": PT},
        "build": "e = Db.PyDbLine(_p3(a['p1']), _p3(a['p2']))",
        "checks": ["handle", "typename", "bbox"],
        "sample": {"p1": [0, 0], "p2": [120, 0]},
    },
    "polyline": {
        "cls": "PyDbPolyline",
        "args": {"pts": PTS, "closed": "bool"},
        "build": ("e = Db.PyDbPolyline()\n"
                  "for _i, _q in enumerate(a['pts']):\n"
                  "    e.addVertexAt(_i, _p2(_q))\n"
                  "e.setClosed(bool(a.get('closed', False)))"),
        "checks": ["handle", "typename", "bbox"],
        "sample": {"pts": [[0, 0], [100, 0], [100, 60], [0, 60]], "closed": True},
    },
    "circle": {
        "cls": "PyDbCircle",
        "args": {"center": PT, "radius": NUM},
        "build": ("e = Db.PyDbCircle(_p3(a['center']),"
                  " Ge.AcGeVector3d(0.0, 0.0, 1.0), float(a['radius']))"),
        "checks": ["handle", "typename", "bbox"],
        "sample": {"center": [50, 50], "radius": 30},
    },
    "arc": {
        "cls": "PyDbArc",
        "args": {"center": PT, "radius": NUM, "start_deg": NUM, "end_deg": NUM},
        "build": ("e = Db.PyDbArc(_p3(a['center']), float(a['radius']),"
                  " _rad(a.get('start_deg', 0.0)), _rad(a.get('end_deg', 180.0)))"),
        "checks": ["handle", "typename", "bbox"],
        "sample": {"center": [100, 100], "radius": 50, "start_deg": 0, "end_deg": 90},
    },
    "text": {
        "cls": "PyDbText",
        "args": {"pos": PT, "text": STR, "height": NUM, "rotation_deg": NUM},
        "build": ("e = Db.PyDbText(_p3(a['pos']), str(a['text']),"
                  " float(a.get('height', 5.0)), _rad(a.get('rotation_deg', 0.0)))"),
        "checks": ["handle", "typename"],
        "sample": {"pos": [10, 10], "text": "柜A", "height": 5},
    },
    "mtext": {
        "cls": "PyDbMText",
        "args": {"pos": PT, "text": STR, "height": NUM, "width": NUM},
        "build": ("e = Db.PyDbMText()\n"
                  "e.setLocation(_p3(a['pos']))\n"
                  "e.setContents(str(a['text']))\n"
                  "e.setTextHeight(float(a.get('height', 5.0)))\n"
                  "e.setWidth(float(a.get('width', 100.0)))"),
        "checks": ["handle", "typename"],
        "sample": {"pos": [10, 10], "text": "主回路\\n备用", "height": 5},
    },
    "point": {
        "cls": "PyDbPoint",
        "args": {"pos": PT},
        # 实测：PyDbPoint 无带点构造，只有 setPosition
        "candidates": ["e = Db.PyDbPoint()\ne.setPosition(_p3(a['pos']))"],
        "checks": ["handle", "typename"],
        "sample": {"pos": [40, 40]},
    },
    "solid": {
        "cls": "PyDbSolid",
        "args": {"p1": PT, "p2": PT, "p3": PT, "p4": PT},
        # 实测：只有 setPointAt(i, pt)，无四参构造
        "candidates": ["e = Db.PyDbSolid()\n"
                       "for _i, _k in enumerate(('p1', 'p2', 'p3', 'p4')):\n"
                       "    e.setPointAt(_i, _p3(a[_k]))"],
        "checks": ["handle", "typename"],
        "sample": {"p1": [0, 0], "p2": [50, 0], "p3": [50, 40], "p4": [0, 40]},
    },
    "xline": {
        "cls": "PyDbXline",
        "args": {"base": PT, "dir": PT},
        # 实测：setBasePoint + setUnitDir
        "candidates": ["e = Db.PyDbXline()\n"
                       "e.setBasePoint(_p3(a['base']))\n"
                       "e.setUnitDir(Ge.AcGeVector3d(float(a['dir'][0]), float(a['dir'][1]), 0.0))"],
        "checks": ["handle", "typename"],
        "sample": {"base": [0, 0], "dir": [1, 1]},
    },
    "ray": {
        "cls": "PyDbRay",
        "args": {"base": PT, "dir": PT},
        "candidates": ["e = Db.PyDbRay()\n"
                       "e.setBasePoint(_p3(a['base']))\n"
                       "e.setUnitDir(Ge.AcGeVector3d(float(a['dir'][0]), float(a['dir'][1]), 0.0))"],
        "checks": ["handle", "typename"],
        "sample": {"base": [0, 0], "dir": [1, 0]},
    },
    "ellipse": {
        "cls": "PyDbEllipse",
        "args": {"center": PT, "major_r": NUM, "ratio": NUM},
        # 实测方法表：setCenter / setMajorRadius / setRadiusRatio（无 setMajorAxis）。
        # 候选 1：先入库再写属性（cad_ops 的既定约定：入库前对象未 setDatabaseDefaults）
        "candidates": [
            "e = Db.PyDbEllipse()\n"
            "if tr.addEntity(e):\n"
            "    ns['added'] = True\n"
            "e.setCenter(_p3(a['center']))\n"
            "e.setMajorRadius(float(a['major_r']))\n"
            "e.setRadiusRatio(float(a['ratio']))",
            "e = Db.PyDbEllipse()\n"
            "e.setDatabaseDefaults()\n"
            "e.setCenter(_p3(a['center']))\n"
            "e.setMajorRadius(float(a['major_r']))\n"
            "e.setRadiusRatio(float(a['ratio']))",
        ],
        "checks": ["handle", "typename", "bbox"],
        "sample": {"center": [50, 50], "major_r": 40, "ratio": 0.5},
    },
    "spline": {
        "cls": "PyDbSpline",
        "args": {"pts": PTS},
        # 实测方法表：setFitPointAt / setNumFitPoints 可能存在，逐条试
        "candidates": [
            "e = Db.PyDbSpline()\n"
            "e.setFitPointAt(0, _p3(a['pts'][0]))\n"
            "for _i, _q in enumerate(a['pts'][1:], 1):\n"
            "    e.insertFitPointAt(_i, _p3(_q))",
            "e = Db.PyDbSpline()\n"
            "_ns = Ge.AcGePoint3dArray() if hasattr(Ge, 'AcGePoint3dArray') else list(a['pts'])\n"
            "e.setFitData([_p3(q) for q in a['pts']])",
        ],
        "checks": ["handle", "typename"],
        "sample": {"pts": [[0, 0], [60, 50], [120, 0], [180, 60]]},
    },
    "mline": {
        "cls": "PyDbMline",
        "args": {"pts": PTS},
        # 实测方法表：appendSeg（不是 appendSegments），切点由 appendSeg 追加
        "candidates": ["e = Db.PyDbMline()\n"
                       "for _q in a['pts']:\n"
                       "    e.appendSeg(_p3(_q))",
                       "e = Db.PyDbMline()\n"
                       "e.setStyle(0)"],
        "checks": ["handle", "typename"],
        "sample": {"pts": [[0, 0], [200, 0], [200, 100]]},
    },
    "leader": {
        "cls": "PyDbLeader",
        "args": {"pts": PTS},
        "candidates": ["e = Db.PyDbLeader()\n"
                       "for _q in a['pts']:\n"
                       "    e.appendVertex(_p3(_q))\n"
                       "e.setToStraightLeader()"],
        "checks": ["handle", "typename"],
        "sample": {"pts": [[0, 0], [100, 50], [200, 80]]},
    },
    "wipeout": {
        "cls": "PyDbWipeout",
        "args": {"pts": PTS},
        # 实测方法表：11 个方法里只有 append（无 appendLoopPoints）
        "candidates": ["e = Db.PyDbWipeout()\n"
                       "e.setDatabaseDefaults()\n"
                       "e.append([(float(q[0]), float(q[1])) for q in a['pts']])",
                       "e = Db.PyDbWipeout()"],
        "checks": ["handle", "typename"],
        "sample": {"pts": [[0, 0], [80, 0], [80, 40], [0, 40]]},
    },
    "table": {
        "cls": "PyDbTable",
        "args": {"pos": PT, "rows": INT, "cols": INT, "texts": "texts"},
        "build": ("e = Db.PyDbTable()\n"
                  "e.setSize(int(a.get('rows', 3)), int(a.get('cols', 3)))\n"
                  "e.setPosition(_p3(a['pos']))\n"
                  "for _i in range(int(a.get('cols', 3))):\n"
                  "    e.set_column_width(_i, float(a.get('col_w', 30.0)))\n"
                  "for _i in range(int(a.get('rows', 3))):\n"
                  "    e.setRowHeight(_i, float(a.get('row_h', 8.0)))\n"
                  "for _r, _row in enumerate(a.get('texts') or []):\n"
                  "    for _c, _v in enumerate(_row or []):\n"
                  "        e.setTextString(_r, _c, str(_v))"),
        "checks": ["handle", "typename"],
        "sample": {"pos": [0, 100], "rows": 2, "cols": 2, "texts": [["A", "B"], ["C", "D"]]},
    },
    "hatch": {
        "cls": "PyDbHatch",
        "args": {"pts": PTS, "pattern": STR, "scale": NUM},
        "build": ("e = Db.PyDbHatch()\n"
                  "e.setDatabaseDefaults()\n"
                  "e.setAssociative(False)\n"
                  "e.setPattern(1, str(a.get('pattern', 'SOLID')))\n"
                  "e.setPatternScale(float(a.get('scale', 1.0)))\n"
                  "_ok = e.appendLoopPoints(1, [_p2(q) for q in a['pts']], [])\n"
                  "if not _ok:\n"
                  "    raise RuntimeError('appendLoopPoints 返回 False')"),
        "checks": ["handle", "typename"],
        "sample": {"pts": [[0, 0], [100, 0], [100, 60], [0, 60]], "pattern": "SOLID"},
    },
    "rotated_dim": {
        "cls": "PyDbRotatedDimension",
        "args": {"p1": PT, "p2": PT, "dim_line": PT},
        "candidates": ["e = Db.PyDbRotatedDimension(_rad(0.0), _p3(a['p1']), _p3(a['p2']), _p3(a['dim_line']), '', 0)",
                       "e = Db.PyDbRotatedDimension()"],
        "checks": ["handle", "typename"],
        "sample": {"p1": [0, 0], "p2": [120, 0], "dim_line": [0, 20]},
    },
}

# ---------------------------------------------------------------- 几何运算
# 每条：args 说明 + 代码 + 期望值（纯 Python 算，用于断言 PyArxCore 结果一致）
GEO = {
    "vec3_len": {
        "cls": "AcGeVector3d",
        "task": "向量的模",
        "build": "r = Ge.AcGeVector3d(3.0, 4.0, 0.0).length()",
        "expect": 5.0, "tol": 1e-6,
    },
    "vec3_dot": {
        "cls": "AcGeVector3d",
        "task": "点积",
        "build": "r = Ge.AcGeVector3d(1.0, 2.0, 3.0).dotProduct(Ge.AcGeVector3d(4.0, -5.0, 6.0))",
        "expect": 4 - 10 + 18, "tol": 1e-6,
    },
    "vec3_cross": {
        "cls": "AcGeVector3d",
        "task": "叉积（z 分量）",
        "build": "r = Ge.AcGeVector3d(1.0, 0.0, 0.0).crossProduct(Ge.AcGeVector3d(0.0, 1.0, 0.0)).z",
        "expect": 1.0, "tol": 1e-6,
    },
    "vec3_normalize": {
        "cls": "AcGeVector3d",
        "task": "归一化后长度=1",
        "build": ("v = Ge.AcGeVector3d(3.0, 4.0, 0.0)\n"
                  "v.normalize()\n"
                  "r = v.length()"),
        "expect": 1.0, "tol": 1e-6,
    },
    "vec3_angle": {
        "cls": "AcGeVector3d",
        "task": "两向量夹角（度）",
        "build": "r = Ge.AcGeVector3d(1.0, 0.0, 0.0).angleTo(Ge.AcGeVector3d(0.0, 1.0, 0.0))",
        "expect": 1.5707963, "tol": 1e-4,
    },
    "mat3_translate": {
        "cls": "AcGeMatrix3d",
        "task": "平移矩阵作用于点",
        "build": ("m = Ge.AcGeMatrix3d()\n"
                  "m.setToTranslation(Ge.AcGeVector3d(10.0, 20.0, 0.0))\n"
                  "p = Ge.AcGePoint3d(1.0, 2.0, 0.0)\n"
                  "p.transformBy(m)\n"
                  "r = [p.x, p.y]"),
        "expect": [11.0, 22.0], "tol": 1e-6,
    },
    "mat3_rotate": {
        "cls": "AcGeMatrix3d",
        "task": "绕 Z 轴 90 度后 (1,0) -> (0,1)",
        "build": ("m = Ge.AcGeMatrix3d()\n"
                  "m.setToRotation(1.5707963, Ge.AcGeVector3d(0.0, 0.0, 1.0))\n"
                  "p = Ge.AcGePoint3d(1.0, 0.0, 0.0)\n"
                  "p.transformBy(m)\n"
                  "r = [p.x, p.y]"),
        "expect": [0.0, 1.0], "tol": 1e-4,
    },
    "vec2_perp": {
        "cls": "AcGeVector2d",
        "task": "二维向量垂直（旋转 90 度）",
        "build": ("v = Ge.AcGeVector2d(1.0, 0.0)\n"
                  "r = [v.x, v.y]"),
        "expect": [1.0, 0.0], "tol": 1e-6,
    },
    "seg3_length": {
        "cls": "AcGeLineSeg3d",
        "task": "线段长度",
        "build": ("s = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0.0, 0.0, 0.0), Ge.AcGePoint3d(3.0, 4.0, 0.0))\n"
                  "r = s.length()"),
        "expect": 5.0, "tol": 1e-6,
    },
}

# ---------------------------------------------------------------- 选择集 / 交互
SELECT = {
    "query_by_layer": {
        "task": "按图层取实体句柄",
        "code": ("ids = DB.objectIds('AcDbEntity')\n"
                 "rows = []\n"
                 "for _id in ids:\n"
                 "    _o = P.Db.PyDbEntity.openObject(_id)\n"
                 "    if str(_o.layer()) == LAYER_WANT:\n"
                 "        rows.append(str(_o.handle()))"),
    },
    "query_by_class": {
        "task": "按 DXF 类名取句柄",
        "code": "rows = [str(P.Db.PyDbEntity.openObject(i).handle()) for i in DB.objectIds(CLS_WANT)]",
    },
    "pick_handle": {
        "task": "按句柄打开并读类型/图层",
        "code": ("_o = P.Db.PyDbEntity.openObjectByIdString(HANDLE_WANT)\n"
                 "rows = {'class': str(_o.isA()), 'layer': str(_o.layer())}"),
    },
}

HELPERS = (
    "def _p2(p):\n"
    "    return Ge.AcGePoint2d(float(p[0]), float(p[1]))\n"
    "\n"
    "def _p3(p):\n"
    "    if len(p) > 2:\n"
    "        return Ge.AcGePoint3d(float(p[0]), float(p[1]), float(p[2]))\n"
    "    return Ge.AcGePoint3d(float(p[0]), float(p[1]), 0.0)\n"
    "\n"
    "def _rad(d):\n"
    "    return math.radians(float(d))\n"
)


def build_code(kind, args_expr=None):
    """返回该图元的构造代码（可直接嵌进脚本）。args_expr 为变量名，默认 a。"""
    spec = ENTITY.get(kind)
    if not spec:
        raise KeyError("未知图元：%s" % kind)
    code = spec.get("build")
    if code is None:
        cands = spec.get("candidates") or []
        code = cands[0] if cands else None
    if code is None:
        raise RuntimeError("图元 %s 还没有可用构造" % kind)
    return code.replace("a[", (args_expr or "a") + "[")


def kinds():
    return sorted(ENTITY)


if __name__ == "__main__":
    print("图元样板 %d 个：%s" % (len(ENTITY), ", ".join(kinds())))
    print("几何运算样板 %d 个：%s" % (len(GEO), ", ".join(sorted(GEO))))
    print("选择集样板 %d 个：%s" % (len(SELECT), ", ".join(sorted(SELECT))))
    print()
    print(build_code("line"))
