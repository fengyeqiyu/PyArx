# -*- coding: utf-8 -*-
"""cad_ops —— cadmcp 的业务工具层（受约束的原子操作）

目标：模型不再直接写 PyArx/pybind 代码，只填坐标、字高、图层名。
事务、写模式、AcGePoint 类型、图层建表、句柄回收全部由本层完成。

每个工具都返回同一套契约（模型据此自纠，不靠猜）：
  {"ok": bool, "count": n, "handles": [...], "layers": {...},
   "warnings": [...], "before": n0, "after": n1,
   "failed_index": i, "errors": [...], "rolled_back": bool}

约束依据 docs_notes/PYARX_防错手册.md 的宿主实测结论：
  * 写实体必须走 `with P.Db.PyDbTrans()`；事务内抛异常会自动 abort（已修复）
  * 图层：newRecord() -> lt.add() 即已落库（names/numRecords 可见），
    PyDbLayerTableRecord 没有可用默认构造。PyDbTrans 上没有
    addNewlyCreatedDBObject，只有 PyDbTransactionManager.addNewlyCreatedDBRObject，
    对已 add 的记录返回 False，加了也没用
  * setLayer(name) 恒返回 False（图层名编码问题，含已存在图层，宿主 98 层实测），
    已在绑定层补 setLayerId(uint64) 走 id 版；工具层优先用它，缺失时才回退
  * addEntity 返回 bool，失败**不抛异常**（块参照被 xref 块拒绝时实体数不涨、
    handle 停在 "0"），所以返回值与句柄都要校验
  * 保存：`db.saveAs()` 的 SecurityParams 传 nullptr 会在 acdb24.dll 解引用空指针
    崩溃（0xc0000005），绑定层已改成无安全参数时走不带 SecurityParams 的重载；
    另外建层后在事务外拿的图层表写锁必须 close()，否则带着符号表写锁存盘同样崩
    （headless 里只是 eWasOpenForWrite，文档库里直接崩）
  * 新建实体先 addEntity 再写属性（入库前对象未 setDatabaseDefaults）
  * 块定义 API 未打通（PyDbBlockTable.add 在事务外断言/挂死），
    所以 block_ref 只引用**已存在**的块定义，找不到就明确报错，不做命令通道兜底
  * openObject 显式传 0=kForRead / 1=kForWrite，绝不省略

批量语义：一次事务内全有或全无；失败回滚后 handles 置空，failed_index 指出第几步。
"""

import ast
import json

# ------------------------------------------------------------------ schema

_COMMON_PROPS = {
    "layer": {"type": "string",
              "description": "目标图层名；不存在会自动建。"
              "需要 pyd 含 setLayerId（hotbuild 产物）；基础 pyd 会退回字符串重载，"
              "而后者无效，此时实体会落 0 层并在 warnings 里显式记录。"},
    "layer_rgb": {"type": "array", "items": {"type": "integer"},
                  "description": "新图层颜色 [r,g,b]，缺省 [255,255,0]"},
    "color": {"type": "integer", "description": "实体 ACI 色号 0-256；缺省随层"},
    "timeout_sec": {"type": "integer", "default": 120},
}

_PT = {"type": "array", "items": {"type": "number"},
       "description": "(x, y) 或 (x, y, z)，图纸单位"}


def _schema(name, desc, props, required):
    p = dict(props)
    p.update(_COMMON_PROPS)
    return {"name": name, "description": desc,
            "inputSchema": {"type": "object", "properties": p, "required": required}}


SCHEMAS = [
    _schema(
        "cad_line",
        "画一条直线。参数就是两个点，不用写任何 PyArx 代码；事务、图层已封装。"
        "返回字段为 ok/count/handles/before/after。",
        {"p1": _PT, "p2": _PT}, ["p1", "p2"]),

    _schema(
        "cad_polyline",
        "画一条二维多段线（LWPOLYLINE）。pts 是顶点列表，closed=true 闭合。"
        "这是画柜体边框、母线槽、桥架轮廓的主力工具。",
        {"pts": {"type": "array", "items": _PT, "description": "顶点列表，至少 2 个"},
         "closed": {"type": "boolean", "default": False}},
        ["pts"]),

    _schema(
        "cad_text",
        "写单行文字（TEXT）。pos 是插入点，height 是字高（图纸单位，图纸缩小时别小于 2），"
        "rotation_deg 是角度（度，不是弧度）。文字内容原样写入，不做转义。",
        {"pos": _PT, "text": {"type": "string"},
         "height": {"type": "number", "default": 5.0},
         "rotation_deg": {"type": "number", "default": 0.0}},
        ["pos", "text"]),

    _schema(
        "cad_mtext",
        "写多行文字（MTEXT）。text 里用 \\n 换行，width 是段落宽度（超出自动折行）。",
        {"pos": _PT, "text": {"type": "string"},
         "height": {"type": "number", "default": 5.0},
         "width": {"type": "number", "default": 100.0}},
        ["pos", "text"]),

    _schema(
        "cad_circle",
        "画圆。center 圆心，radius 半径，都是图纸单位。",
        {"center": _PT, "radius": {"type": "number"}}, ["center", "radius"]),

    _schema(
        "cad_arc",
        "画圆弧。角度用**度**（0-360，逆时针），本层内部转弧度，模型不要自己转。",
        {"center": _PT, "radius": {"type": "number"},
         "start_deg": {"type": "number", "default": 0.0},
         "end_deg": {"type": "number", "default": 180.0}},
        ["center", "radius"]),

    _schema(
        "cad_hatch",
        "图案填充（HATCH）。pts 是闭合边界点，pattern 缺省 SOLID；"
        "内部已按 设置->appendLoopPoints->evaluateHatch 三步走，缺一步图案不显示。",
        {"pts": {"type": "array", "items": _PT},
         "pattern": {"type": "string", "default": "SOLID"},
         "scale": {"type": "number", "default": 1.0}},
        ["pts"]),

    _schema(
        "cad_table",
        "画表格（ACAD TABLE）。texts 是二维数组，行列不足会自动截断；"
        "row_h/col_w 是行高列宽的图纸单位。",
        {"pos": _PT,
         "rows": {"type": "integer", "default": 3},
         "cols": {"type": "integer", "default": 3},
         "texts": {"type": "array", "items": {"type": "array", "items": {"type": "string"}}},
         "row_h": {"type": "number", "default": 8.0},
         "col_w": {"type": "number", "default": 30.0}},
        ["pos"]),

    _schema(
        "cad_block_ref",
        "插入块参照（INSERT）。只能引用**当前图里已存在**的块定义；"
        "块定义 API 未打通（事务外建定义会挂死 CAD），所以不提供现场建块。"
        "找不到块名会明确返回错误，不要反复重试同一个名字。",
        {"name": {"type": "string"},
         "pos": _PT,
         "scale": {"type": "number", "default": 1.0},
         "rotation_deg": {"type": "number", "default": 0.0}},
        ["name", "pos"]),

    _schema(
        "cad_layer",
        "新建图层（幂等：已存在则返回 exists，不报错）。rgb 是 [r,g,b] 0-255。",
        {"name": {"type": "string"},
         "rgb": {"type": "array", "items": {"type": "integer"}}},
        ["name"]),

    _schema(
        "cad_batch",
        "把多个操作打包成**一次**事务执行，全有或全无。"
        "这是推荐用法：一次往返画完一台柜，而不是一个图元一次调用。"
        "ops 形如 [{\"tool\":\"polyline\",\"args\":{...}}, {\"tool\":\"text\",\"args\":{...}}]，"
        "tool 取 line/polyline/text/mtext/circle/arc/hatch/table/block_ref/layer/rect（rect 会按四角展开成闭合线）。"
        "任一 op 失败则整批回滚，failed_index 指出第几步。",
        {"ops": {"type": "array", "items": {
            "type": "object",
            "properties": {"tool": {"type": "string"}, "args": {"type": "object"}},
            "required": ["tool"]}}},
        ["ops"]),

    _schema(
        "cad_stats",
        "只读统计当前图纸：图元总数、按类名计数、按图层计数、文件名。"
        "max_scan 限制扫描量（默认 5000，万级图元时避免卡死）。不写任何东西。",
        {"max_scan": {"type": "integer", "default": 5000}}, []),

    _schema(
        "cad_query",
        "只读查询图元：按类名/图层过滤，返回 handle、class、layer 列表。"
        "拿到的 handle 可直接喂给 cad_erase。",
        {"cls": {"type": "string", "description": "DXF 类名，如 AcDbLine；缺省 AcDbEntity"},
         "layer": {"type": "string"},
         "limit": {"type": "integer", "default": 200}}, []),

    _schema(
        "cad_erase",
        "删除图元。三种方式：handles 列表精确删；layer 删整层；"
        "all=true 清空当前空间（必须同时传 confirm=\"ERASE_ALL\"，防止误清图）。",
        {"handles": {"type": "array", "items": {"type": "string"}},
         "layer": {"type": "string"},
         "all": {"type": "boolean", "default": False},
         "confirm": {"type": "string"}},
        []),
    {"name": "cad_rect",
     "description": "画矩形：给两个对角点，四个角由工具层算，模型不用自己推坐标。"
     "要等分时给 vdivide=N（竖线 N 等分）或 hdivide=N（横线 N 等分），"
     "等分线坐标由工具层按宽度算，模型不要自己算竖线 x 值。"
     "返回同 cad_polyline（ok/count/handles/before/after）。",
     "inputSchema": {"type": "object",
                     "properties": dict({"p1": _PT, "p2": _PT,
                                         "vdivide": {"type": "integer",
                                                     "description": "竖向 N 等分，工具层画 N-1 条竖线"},
                                         "hdivide": {"type": "integer",
                                                  "description": "横向 N 等分，工具层画 N-1 条横线"}},
                                        **_COMMON_PROPS),
                     "required": ["p1", "p2"]}},
    {"name": "cad_offset",
     "description": "把指定图元偏移出一条平行图元：给源句柄和距离，几何由 CAD 自己算，"
     "模型不用推坐标。支持直线/圆/弧/多段线，新图元继承源图元的图层。"
     "返回 ok/count/handles（新句柄）。",
     "inputSchema": {"type": "object",
                     "properties": {
                         "handle": {"type": "string", "description": "源图元句柄"},
                         "dist": {"type": "number",
                                  "description": "偏移距离；符号定方向（直线按法向，圆按半径增减）"},
                         "timeout_sec": {"type": "integer", "default": 120}},
                     "required": ["handle", "dist"]}},
    {"name": "cad_save",
     "description": "保存当前图形：给 path 就另存为（saveAs），不给就按当前文件名保存。"
     "返回 ok/status/path/bytes。未命名图形必须先给 path，否则返回 eInvalidInput。",
     "inputSchema": {"type": "object",
                     "properties": {
                         "path": {"type": "string",
                                  "description": "目标 dwg 绝对路径；省略则保存到当前文件名"},
                         "timeout_sec": {"type": "integer", "default": 120}},
                     "required": []}},
]

NAMES = set(s["name"] for s in SCHEMAS)

# 批量工具可嵌套的原子操作名
_OP_NAMES = ("line", "polyline", "text", "mtext", "circle", "arc",
             "hatch", "table", "block_ref", "layer")

# ------------------------------------------------------------------ 代码生成

_HEAD = '''# -*- coding: utf-8 -*-
import json, math, traceback
try:
    import PyArxCore as P
    from PyArxCore import Db, Ge
except Exception:
    # 宿主（cadmcp server / playground）会在 globals 里注入 P，这里只是兜底
    P = globals().get("P")
    Db = getattr(P, "Db", None)
    Ge = getattr(P, "Ge", None)

CALLS = __CALLS__

OUT = {"ok": True, "count": 0, "handles": [], "steps": [], "layers": {},
       "warnings": [], "before": 0, "after": 0, "errors": [],
       "failed_index": -1, "rolled_back": False}

BT = None

DB = P.Db.curDb()

def _p2(p):
    return P.Ge.AcGePoint2d(float(p[0]), float(p[1]))

def _p3(p):
    if len(p) > 2:
        return P.Ge.AcGePoint3d(float(p[0]), float(p[1]), float(p[2]))
    return P.Ge.AcGePoint3d(float(p[0]), float(p[1]), 0.0)

def _rad(d):
    return math.radians(float(d))
'''

_MAKE = '''
def _make(kind, a):
    if kind == "line":
        return P.Db.PyDbLine(_p3(a["p1"]), _p3(a["p2"]))

    if kind == "polyline":
        pl = P.Db.PyDbPolyline()
        for i, p in enumerate(a["pts"]):
            pl.addVertexAt(i, _p2(p))
        pl.setClosed(bool(a.get("closed", False)))
        return pl

    if kind == "circle":
        return P.Db.PyDbCircle(_p3(a["center"]),
                               P.Ge.AcGeVector3d(0.0, 0.0, 1.0),
                               float(a["radius"]))

    if kind == "arc":
        return P.Db.PyDbArc(_p3(a["center"]), float(a["radius"]),
                            _rad(a.get("start_deg", 0.0)),
                            _rad(a.get("end_deg", 180.0)))

    if kind == "text":
        return P.Db.PyDbText(_p3(a["pos"]), str(a["text"]),
                             float(a.get("height", 5.0)),
                             _rad(a.get("rotation_deg", 0.0)))

    if kind == "mtext":
        m = P.Db.PyDbMText()
        m.setLocation(_p3(a["pos"]))
        m.setContents(str(a["text"]))
        m.setTextHeight(float(a.get("height", 5.0)))
        m.setWidth(float(a.get("width", 100.0)))
        return m

    if kind == "hatch":
        h = P.Db.PyDbHatch()
        h.setDatabaseDefaults()
        h.setAssociative(False)
        h.setPattern(1, str(a.get("pattern", "SOLID")))
        h.setPatternScale(float(a.get("scale", 1.0)))
        # 边界与求值必须留到入库之后（_RUN 里 addEntity 成功后再做）：
        # 对象还不在数据库时 appendLoopPoints / evaluateHatch 会走到
        # dbobji.cpp 的 eNotOpenForWrite，那是 AutoCAD 致命错误，
        # Python 侧 try/except 根本接不住，整个 accoreconsole 会弹窗退出。
        return h

    if kind == "table":
        t = P.Db.PyDbTable()
        rows, cols = int(a.get("rows", 3)), int(a.get("cols", 3))
        t.setSize(rows, cols)
        t.setPosition(_p3(a["pos"]))
        for i in range(cols):
            t.setColumnWidth(i, float(a.get("col_w", 30.0)))
        for i in range(rows):
            t.setRowHeight(i, float(a.get("row_h", 8.0)))
        for r, row in enumerate(a.get("texts") or []):
            for c, v in enumerate(row or []):
                if r < rows and c < cols:
                    t.setTextString(r, c, str(v))
        return t

    if kind in ("block_ref", "cad_block_ref"):
        name = str(a["name"])
        if BT is None:
            raise RuntimeError("块表未打开（生成脚本里应先在事务外取 getBlockTable）")
        try:
            exists = bool(BT.has(name))
        except Exception as ex:
            raise RuntimeError("BlockTable.has() 失败：" + repr(ex)[:120])
        if not exists:
            raise KeyError("块定义不存在：" + name + "（本图里没有这个块，需先在模板图定义）")
        try:
            br = P.Db.PyDbBlockReference()
        except Exception as ex:
            raise RuntimeError("PyDbBlockReference() 构造失败（绑定可能无默认构造）：" + repr(ex)[:140])
        try:
            br.setDatabaseDefaults()   # 与 hatch 同样的入库前初始化
        except Exception:
            pass
        try:
            # objectId() 是 uint64 oldId（PyArrayCaster.h 约定），
            # setBlockTableRecord 的签名正是 uint64_t，不需要 PyDbObjectId。
            br.setBlockTableRecord(BT.getAt(name).objectId())
        except Exception as ex:
            raise RuntimeError("setBlockTableRecord(objectId) 失败：" + repr(ex)[:140])
        br.setPosition(_p3(a["pos"]))
        s = float(a.get("scale", 1.0))
        br.setScaleFactors(P.Ge.AcGeScale3d(s, s, s))
        br.setRotation(_rad(a.get("rotation_deg", 0.0)))
        return br

    raise ValueError("不支持的 op：" + str(kind))
'''

_RUN = '''
need = {}
_need_bt = False
for c in CALLS:
    a = c.get("args") or {}
    if a.get("layer"):
        need[str(a["layer"])] = a.get("layer_rgb") or [255, 255, 0]
    if str(c.get("tool") or "") in ("block_ref", "cad_block_ref"):
        _need_bt = True

# 块表同样必须**事务外**打开（事务内 getSymbolTable(kForWrite) 会 eWasOpenForWrite），
# 且一个进程里只能打开一次，所以先取好、结束后 close。
if _need_bt:
    BT = DB.getBlockTable()

if need:
    # 图层**不能**在事务里建：事务持有写锁时 getSymbolTable(kForWrite) 会报
    # "eWasOpenForWrite"，而且 PyDbTrans 上根本没有 addNewlyCreatedDBObject
    # （只有 PyDbTransactionManager 有）。宿主实测的正确路径是事务外取可写
    # 符号表 → newRecord → lt.add，符号表自己负责落库。
    lt = DB.getLayerTable()
    for name, rgb in need.items():
        if lt.has(name):
            OUT["layers"][name] = "exists"
            continue
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(name)
        rec.setColor(int(rgb[0]), int(rgb[1]), int(rgb[2]))
        if not lt.add(rec):
            raise RuntimeError("LayerTable.add 返回 False：" + name)
        OUT["layers"][name] = "created"

OUT["before"] = len(DB.objectIds("AcDbEntity"))
added = []
i = -1
kind = ""
try:
    with P.Db.PyDbTrans() as tr:
        for i, c in enumerate(CALLS):
            kind = str(c.get("tool") or "")
            a = c.get("args") or {}
            try:
                if kind == "layer":
                    OUT["steps"].append({"i": i, "tool": "layer", "ok": True})
                    continue
                ent = _make(kind, a)
                try:
                    baked = tr.addEntity(ent)
                except Exception as ex:
                    raise RuntimeError("addEntity(%s) 失败：%s" % (kind, repr(ex)[:140]))
                if not baked:
                    # addEntity 返回 bool 且失败时不抛异常：句柄会停在 "0"、实体数不涨。
                    raise RuntimeError("addEntity(%s) 返回 False：appendAcDbEntity 被拒"
                                       "（xref 派生块 $0$ 不能引用 / 对象未初始化）" % kind)
                if a.get("layer"):
                    # 优先走 id 版 setLayerId（uint64 oldId，绕开图层名编码问题）；
                    # 只有基础 pyd（未含 setLayerId）才退回字符串重载，而后者对
                    # 已存在图层也返回 False —— 所以退回时如实记 warning。
                    want = str(a["layer"])
                    try:
                        ok = False
                        if hasattr(ent, "setLayerId"):
                            try:
                                ok = bool(ent.setLayerId(lt.getAt(want).objectId()))
                            except Exception:
                                ok = False
                        if not ok:
                            try:
                                ok = bool(ent.setLayer(want))
                            except Exception:
                                ok = False
                        got = str(ent.layer())
                        if got != want:
                            OUT["warnings"].append(
                                {"i": i, "setLayer": "未生效：目标=%s 实际=%s" % (want, got),
                                 "hint": "当前 pyd 无 setLayerId 或图层 id 无效"})
                    except Exception as ex:
                        OUT["warnings"].append({"i": i, "setLayer": repr(ex)})
                if a.get("color") is not None:
                    try:
                        ent.setColorIndex(int(a["color"]))
                        if int(ent.colorIndex()) != int(a["color"]):
                            OUT["warnings"].append({"i": i, "setColorIndex": "未生效"})
                    except Exception as ex:
                        OUT["warnings"].append({"i": i, "setColorIndex": repr(ex)})
                added.append((i, kind, ent))
                OUT["steps"].append({"i": i, "tool": kind, "ok": True})
            except Exception:
                # 先在**事务内**把真实异常记下来：PyDbTrans.__exit__ 在 abort 时
                # 会再抛一次 "Caught an unknown exception!"，它会覆盖真正的错误，
                # 所以首个异常必须在这里保存，外层只做兜底。
                if not OUT["errors"]:
                    OUT["failed_index"] = i
                    OUT["errors"].append({"i": i, "tool": kind,
                                          "error": traceback.format_exc().splitlines()[-1]})
                raise
except Exception:
    OUT["ok"] = False
    OUT["rolled_back"] = True
    if OUT["failed_index"] < 0:
        OUT["failed_index"] = i
    OUT["handles"] = []
    added = []
    if not OUT["errors"]:
        OUT["errors"].append({"i": i, "tool": kind,
                              "error": traceback.format_exc().splitlines()[-1]})

# 句柄必须在事务提交**之后**取：事务内 ent.handle() 还是 "0"（宿主实测），
# 事务提交后才有真实句柄。handle 仍是 0 说明 addEntity 被静默拒绝、实体没入库
# （块参照实测过），这种情况不能算进 count，否则模型会以为画上了。
for i2, k2, ent in added:
    try:
        h = str(ent.handle())
    except Exception:
        h = "?"
    OUT["handles"].append(h)
    if h in ("0", "?"):
        OUT["warnings"].append(
            {"i": i2, "tool": k2, "unbaked": "实体未入库：handle=%s" % h})

OUT["count"] = sum(1 for h in OUT["handles"] if h not in ("0", "?"))
OUT["after"] = len(DB.objectIds("AcDbEntity"))
if OUT["after"] <= OUT["before"] and added:
    OUT["warnings"].append({"unbaked": "实体数未增加（before=%s after=%s）"
                            % (OUT["before"], OUT["after"])})
OUT["after"] = len(DB.objectIds("AcDbEntity"))
if BT is not None:
    try:
        BT.close()
    except Exception:
        pass
# 图层表同理：建层时在事务外拿了写锁，必须 close。否则库一直处于"有符号表打开写"
# 的状态，后续 AcDbDatabase::saveAs 会崩在 acdb24.dll（headless 里表现是
# eWasOpenForWrite，文档库里直接 0xc0000005）。存盘前务必走完这一步。
try:
    lt.close()
except Exception:
    pass
result = OUT
'''

_OFFSET = '''# -*- coding: utf-8 -*-
import traceback

DB = P.Db.curDb()
SRC = __HANDLE__
DIST = __DIST__

OUT = {"ok": False, "count": 0, "handles": [], "src": SRC, "dist": DIST,
       "warnings": [], "errors": [], "failed_index": -1, "rolled_back": False,
       "before": len(DB.objectIds("AcDbEntity"))}

added = []
try:
    # 源实体必须在**同一个事务**里打开并求偏移：跨事务拿到的包装对象
    # 在事务结束时就关闭了，再用会直接报对象已关闭。
    with P.Db.PyDbTrans() as tr:
        target = None
        for eid in DB.objectIds("AcDbEntity"):
            o = tr.openObject(eid, 0)
            if o is None:
                continue
            if str(o.handle()).upper().lstrip("0") == SRC:
                target = o
                break
        if target is None:
            raise RuntimeError("找不到句柄 " + SRC)
        if not hasattr(target, "getOffsetCurves"):
            raise RuntimeError("该实体不支持偏移：" + str(target.className()))
        curves = target.getOffsetCurves(float(DIST))
        for c in curves:
            if not tr.addEntity(c):
                OUT["warnings"].append({"addEntity": "返回 False，偏移结果未入库"})
                continue
            added.append(c)
except Exception:
    OUT["rolled_back"] = True
    added = []
    OUT["errors"].append(traceback.format_exc().splitlines()[-1])

# 偏移曲线继承源图元的图层/颜色，这里不再单独改属性
for ent in added:
    try:
        OUT["handles"].append(str(ent.handle()))
    except Exception:
        OUT["handles"].append("?")

OUT["count"] = len([h for h in OUT["handles"] if h not in ("0", "?")])
OUT["after"] = len(DB.objectIds("AcDbEntity"))
OUT["ok"] = bool(OUT["count"] > 0) and not OUT["errors"]
result = OUT
'''

_SAVE = '''# -*- coding: utf-8 -*-
import os

PATH = __PATH__

OUT = {"ok": False, "status": None, "path": None, "bytes": 0, "errors": []}
doc = None
try:
    doc = P.Ap.PyApp.documentManager.curDocument
except Exception:
    doc = None

# 保存前库不能带任何打开的符号表：cad_batch 建层/块后都会 close，
# 若调用方自己开过表没关，这里不负责兜底（只会报 eWasOpenForWrite 或崩）
if PATH:
    st = str(P.Db.curDb().saveAs(PATH))
    OUT["path"] = PATH
elif doc is not None:
    st = str(doc.save())
    try:
        OUT["path"] = str(doc.fileName)
    except Exception:
        OUT["path"] = None
else:
    st = "eNoDocument"
    OUT["errors"].append("未命名图形且未给 path：先用 saveAs 给个路径")

OUT["status"] = st
low = str(st).lower()
OUT["ok"] = "eok" in low
tgt = OUT["path"]
if tgt and os.path.exists(tgt):
    OUT["bytes"] = os.path.getsize(tgt)
else:
    if OUT["ok"]:
        OUT["errors"].append("返回 eOk 但文件不存在：" + str(tgt))

result = OUT
'''

_STATS = '''# -*- coding: utf-8 -*-
import json

DB = P.Db.curDb()
MAX = __MAX__
ids = DB.objectIds("AcDbEntity")
out = {"ok": True, "total": len(ids), "scanned": 0,
       "by_class": {}, "by_layer": {}, "truncated": False}
# 必须走事务内 openObject：db.openObject 返回弱类型包装，className() 一律
# 是 AcDbObject、layer() 取不到（宿主实测），tr.openObject 才是强类型。
with P.Db.PyDbTrans() as tr:
    for eid in ids:
        if MAX and out["scanned"] >= MAX:
            out["truncated"] = True
            break
        o = tr.openObject(eid, 0)
        if o is None:
            continue
        out["scanned"] += 1
        try:
            c = str(o.className())
        except Exception:
            c = "?"
        out["by_class"][c] = out["by_class"].get(c, 0) + 1
        try:
            L = str(o.layer())
        except Exception:
            L = "?"
        out["by_layer"][L] = out["by_layer"].get(L, 0) + 1
try:
    out["filename"] = str(DB.getFilename())
except Exception:
    out["filename"] = ""
result = out
'''

_QUERY = '''# -*- coding: utf-8 -*-
import json

DB = P.Db.curDb()
CLS = __CLS__
LAYER = __LAYER__
LIMIT = __LIMIT__
hits = []
with P.Db.PyDbTrans() as tr:
    for eid in DB.objectIds(CLS or "AcDbEntity"):
        o = tr.openObject(eid, 0)
        if o is None:
            continue
        try:
            L = str(o.layer())
        except Exception:
            L = "?"
        if LAYER and L != LAYER:
            continue
        try:
            c = str(o.className())
        except Exception:
            c = "?"
        hits.append({"handle": str(o.handle()), "class": c, "layer": L})
        if LIMIT and len(hits) >= LIMIT:
            break
result = {"ok": True, "count": len(hits), "hits": hits}
'''

_ERASE = '''# -*- coding: utf-8 -*-
import json, traceback

DB = P.Db.curDb()
TARGETS = __HANDLES__
LAYER = __LAYER__
ALL = __ALL__
erased, matched = 0, []
with P.Db.PyDbTrans() as tr:
    for eid in DB.objectIds("AcDbEntity"):
        o = tr.openObject(eid, 1)
        if o is None:
            continue
        h = str(o.handle()).upper().lstrip("0")
        hit = False
        if ALL:
            hit = True
        elif LAYER:
            try:
                hit = (str(o.layer()) == LAYER)
            except Exception:
                hit = False
        else:
            hit = (h in TARGETS)
        if not hit:
            continue
        matched.append(str(o.handle()))
        o.erase()
        erased += 1
result = {"ok": True, "erased": erased, "handles": matched}
'''


def _py(v):
    """值 → Python 源码字面量。

    这里**不能**用 json.dumps：JSON 的 true/false/null 在 Python 里未定义，
    送进 CAD 执行直接 NameError: name 'true' is not defined（无头通道实测踩过）。
    repr 生成的是合法 Python 字面量，且保持中文原样。
    """
    return repr(v)


def build_script(calls):
    """calls: [{"tool": "line", "args": {...}}] → 在 acad.exe 内执行的完整脚本"""
    if not isinstance(calls, list) or not calls:
        raise ValueError("calls 必须是非空列表")
    code = _HEAD.replace("__CALLS__", _py(calls)) + _MAKE + _RUN
    # 生成后立刻自检，避免把语法错误送到 CAD 里（桥接通道报错信息很差）
    ast.parse(code)
    return code


def _expand_layout(lx, ly, rx, ry, a):
    """按 rect 上的 vdivide/hdivide 语义字段算出等分线。

    模型只负责认出「四等分」并填 vdivide=4，宽度和间距的除法在工具层做，
    750 宽、1875 宽这些没训过的尺寸一样精确（避免模型把 900 三等分的答案
    抄到 1000 上）。
    """
    out = []
    for key, axis in (("vdivide", "v"), ("hdivide", "h")):
        n = a.get(key)
        if n is None:
            continue
        try:
            n = int(n)
        except (TypeError, ValueError):
            continue
        if n < 2:
            continue
        for k in range(1, n):
            if axis == "v":
                x = lx + (rx - lx) * k / float(n)
                out.append({"tool": "line", "args": {"p1": [x, ly], "p2": [x, ry]}})
            else:
                y = ly + (ry - ly) * k / float(n)
                out.append({"tool": "line", "args": {"p1": [lx, y], "p2": [rx, y]}})
    for sub in out:
        for kk in ("layer", "layer_rgb", "color", "timeout_sec"):
            if a.get(kk) is not None:
                sub["args"][kk] = a[kk]
    return out


def script_for(tool, args):
    """单个原子工具 → 脚本。tool 去掉 cad_ 前缀。"""
    name = str(tool)
    if name.startswith("cad_"):
        name = name[4:]
    if name == "batch":
        ops = (args or {}).get("ops") or []
        norm = []
        for op in ops:
            t = str(op.get("tool") or "")
            if t.startswith("cad_"):
                t = t[4:]
            if t == "rect":
                # batch 的 op 白名单里原本没有 rect，但 rect 是公开工具，模型在复合
                # 指令里写 {"tool":"rect"} 完全合理。这里按顶层 cad_rect 同一口径展开成
                # 闭合 polyline（四角点），事务/图层/回滚与其它 op 一致。
                a = op.get("args") or {}
                p1 = a.get("p1") or [0, 0]
                p2 = a.get("p2") or [0, 0]
                x1, y1 = float(p1[0]), float(p1[1])
                x2, y2 = float(p2[0]), float(p2[1])
                lx, ly = min(x1, x2), min(y1, y2)
                rx, ry = max(x1, x2), max(y1, y2)
                sub = {"pts": [[lx, ly], [rx, ly], [rx, ry], [lx, ry]], "closed": True}
                for k in ("layer", "layer_rgb", "color", "timeout_sec"):
                    if a.get(k) is not None:
                        sub[k] = a[k]
                norm.append({"tool": "polyline", "args": sub})
                norm.extend(_expand_layout(lx, ly, rx, ry, a))
                continue
            if t not in _OP_NAMES:
                raise ValueError("不支持的 op：" + t + "，可用：" + "/".join(_OP_NAMES + ("rect",)))
            norm.append({"tool": t, "args": op.get("args") or {}})
        return build_script(norm)
    if name == "erase":
        a = args or {}
        return erase_script(a.get("handles"), a.get("layer") or "",
                            bool(a.get("all")), a.get("confirm") or "")
    if name == "stats":
        # 只读工具，和造图元走的不是一条路（不建事务、不改图），早先漏了分派，
        # 模型输出 cad_stats 时这里会抛「不支持的原子工具」，看着像模型不会用工具。
        return stats_script(int((args or {}).get("max_scan") or 0))
    if name == "query":
        a = args or {}
        return query_script(a.get("cls") or "", a.get("layer") or "",
                            int(a.get("limit") or 200))
    if name == "save":
        return save_script(str((args or {}).get("path") or ""))
    if name == "rect":
        # 矩形不单独写脚本：算出四角后复用 polyline，事务/图层/回滚全部一致
        a = args or {}
        p1 = a.get("p1") or [0, 0]
        p2 = a.get("p2") or [0, 0]
        x1, y1 = float(p1[0]), float(p1[1])
        x2, y2 = float(p2[0]), float(p2[1])
        lx, ly = min(x1, x2), min(y1, y2)
        rx, ry = max(x1, x2), max(y1, y2)
        sub = {"pts": [[lx, ly], [rx, ly], [rx, ry], [lx, ry]], "closed": True}
        for k in ("layer", "layer_rgb", "color", "timeout_sec"):
            if a.get(k) is not None:
                sub[k] = a[k]
        return build_script([{"tool": "polyline", "args": sub}] +
                            _expand_layout(lx, ly, rx, ry, a))
    if name == "offset":
        a = args or {}
        if not a.get("handle") or a.get("dist") is None:
            raise ValueError("cad_offset 需要 handle 和 dist")
        code = (_OFFSET.replace("__HANDLE__", _py(str(a["handle"]).upper().lstrip("0")))
                        .replace("__DIST__", repr(float(a["dist"]))))
        ast.parse(code)
        return code
    if name not in _OP_NAMES:
        raise ValueError("不支持的原子工具：" + name + "，可用：" +
                         "/".join(_OP_NAMES + ("erase", "stats", "query", "batch")))
    return build_script([{"tool": name, "args": args or {}}])


def save_script(path=""):
    code = _SAVE.replace("__PATH__", _py(path or ""))
    ast.parse(code)
    return code


def stats_script(max_scan=5000):
    code = _STATS.replace("__MAX__", str(int(max_scan or 0)))
    ast.parse(code)
    return code


def query_script(cls="", layer="", limit=200):
    code = (_QUERY.replace("__CLS__", _py(cls or ""))
                  .replace("__LAYER__", _py(layer or ""))
                  .replace("__LIMIT__", str(int(limit or 0))))
    ast.parse(code)
    return code


def erase_script(handles=None, layer="", all_=False, confirm=""):
    if all_ and confirm != "ERASE_ALL":
        raise ValueError('清空全图必须传 confirm="ERASE_ALL"')
    if not all_ and not layer and not handles:
        raise ValueError("handles / layer / all 三者至少要给一个")
    tg = sorted(set(str(h).upper().lstrip("0") for h in (handles or [])))
    code = (_ERASE.replace("__HANDLES__", _py(tg))
                  .replace("__LAYER__", _py(layer or ""))
                  .replace("__ALL__", "True" if all_ else "False"))
    ast.parse(code)
    return code


def describe():
    """给模型看的一句话能力清单（pyarx_guide 里引用）"""
    return ("原子工具：" + "、".join("cad_" + n for n in _OP_NAMES) +
            "；只读：" + "、".join(["cad_stats", "cad_query"]) +
            "；删除：cad_erase；批量：cad_batch（一次事务全有或全无）")
