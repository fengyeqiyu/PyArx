# -*- coding: utf-8 -*-
"""gen_dataset_100k.py —— 生成 10 万条 PyArx SFT 数据。

铁律（对应模型此前"背数据 / 风马牛不相及"的根因）：
  1. 只从真实来源取内容：cad_* 工具清单来自 cad_ops.py；类/方法来自 api_index.jsonl
     （源码提取）；图元样板来自 proto.py（已在 accoreconsole 无头实跑通过）。
  2. 绝不编造方法名。api_index 里没有的，一个字都不写。
  3. 不复用同一句模板刷量：每个工具 8-14 条措辞 × 参数随机 × 图层/数值随机，
     并对 (user, call) 做哈希去重，重复直接丢。
  4. 长度受 MAXLEN=384 约束：单条 user+assistant 控制在约 250 汉字内。

配比：工具调用 40000 / 类方法 35000 / 图元代码 15000 / 几何运算 8000 / 纠错 2000
"""
import json
import os
import random
import re

HERE = os.path.dirname(os.path.abspath(__file__))
CADMCP = r"G:\PyArx\tools\cadmcp"
IDX = os.path.join(CADMCP, "api_index.jsonl")
OUT = os.path.join(HERE, "sft_100k.jsonl")

SEED = 20261117
R = random.Random(SEED)

SYS_TOOL = ("你是 CAD 绘图助手。只通过 cad_* 工具操作图形，不要自己写 PyArx 代码。"
            "用户要脚本或代码时照样输出 cad_* 工具调用，由系统生成 PyArx 代码，"
            "不要改成讲架构或手写代码。坐标单位 mm，角度用度，图层不存在会自动创建。"
            "需要等分时给 cad_rect 填 vdivide=N（竖线 N 等分）或 hdivide=N（横线 N 等分），"
            "等分线坐标由工具层按宽度算，不要自己算竖线 x 值、也不要再补 line。"
            "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。"
            "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
            "失败时要读 warnings 自纠而不是重试同一调用。")

SYS_CODE = ("你是 PyArxCore 代码助手。只写真实存在的 API 调用，"
            "不要臆造方法名；不确定就先查能力矩阵。"
            "图元用 Db 子模块构造，几何用 Ge 子模块计算；"
            "开对象必须显式给模式（0=kForRead / 1=kForWrite）。"
            "新建实体先 addEntity 入模型空间再写属性。坐标单位 mm，角度用度。")

LAYERS = ["0", "LIVE_TEST", "WALL", "DIM", "TEXT", "ELEC", "CABLE", "HATCH", "FRAME"]
TEXTS = ["墙体", "标高", "配电箱", "桥架", "注意", "A", "B1", "详图 01", "DN100", "10KV"]
BLOCKS = ["_Dot", "DOOR", "WINDOW"]
VISIBLE = []


def _num(lo=0, hi=1000, step=1):
    v = R.randrange(int(lo / step), int(hi / step) + 1) * step
    return v


def _pt(lo=0, hi=1000):
    return [_num(lo, hi, 10), _num(lo, hi, 10)]


def _layer():
    return R.choice(LAYERS)


def _add(user, name, args, kind="tool", sys=SYS_TOOL):
    """一条 tool_calls 样本；重复的 (user,call) 不进集。"""
    key = (user, name, json.dumps(args, sort_keys=True, ensure_ascii=False))
    if key in _seen:
        return False
    _seen.add(key)
    VISIBLE.append({
        "messages": [
            {"role": "system", "content": sys},
            {"role": "user", "content": user},
            {"role": "assistant", "content": None,
             "tool_calls": [{"type": "function", "function": {
                 "name": name,
                 "arguments": json.dumps(args, ensure_ascii=False)}}]},
        ],
        "meta": {"kind": kind, "tool": name},
    })
    return True


def _add_code(user, code, kind="api"):
    """一条纯代码样本（无工具调用）。"""
    key = (user, code)
    if key in _seen:
        return False
    _seen.add(key)
    VISIBLE.append({
        "messages": [
            {"role": "system", "content": SYS_CODE},
            {"role": "user", "content": user},
            {"role": "assistant", "content": code},
        ],
        "meta": {"kind": kind},
    })
    return True


_seen = set()

# ---------------- 1. 工具调用：40000 ----------------
# 每个工具的措辞池都按"人真的会这么说"来写，不写"请调用 cad_line 工具"这种机器话
LINE_UTT = [
    "从 {p1} 到 {p2} 画一条线", "画线 {p1} -> {p2}", "在 {p1} 到 {p2} 之间连一条直线",
    "帮我拉一条线，起点 {p1}，终点 {p2}", "画条水平线，从 {p1} 到 {p2}",
    "{layer} 层加一条线 {p1}-{p2}", "来一条线：{p1} 到 {p2}", "这里画直线 {p1} 到 {p2}",
    "补一条线，从 {p1} 拉到 {p2}", "画线并放到 {layer} 层，{p1} 到 {p2}",
]
CIRCLE_UTT = [
    "圆心 {c}，半径 {r} 画圆", "在 {c} 画个半径 {r} 的圆", "画圆 {c} r={r}",
    "以 {c} 为圆心画半径 {r} 的圆", "{layer} 层画圆，圆心 {c}，半径 {r}",
    "来个圆：中心 {c}，半径 {r}", "画个圆，直径 {d}", "在 {c} 处画 r{r} 的圆",
]
ARC_UTT = [
    "圆心 {c} 半径 {r}，从 {a1} 度到 {a2} 度画弧", "画弧：圆心 {c}，r={r}，{a1}°→{a2}°",
    "{c} 处画一段半径 {r} 的圆弧，起始角 {a1} 度", "画个 {a1} 到 {a2} 度的弧，圆心 {c}，半径 {r}",
    "在 {c} 画弧，半径 {r}，角度 {a1} 到 {a2}", "来段圆弧 {c} r{r} {a1}°~{a2}°",
]
TEXT_UTT = [
    "在 {p} 写文字“{t}”", "{p} 处标注“{t}”", "写个字：{p}，内容“{t}”",
    "{layer} 层在 {p} 加文字“{t}”", "标一下“{t}”，位置 {p}", "在 {p} 放文字“{t}”，高 {h}",
    "写字“{t}”放到 {p}", "{p} 打上“{t}”两个字",
]
MTEXT_UTT = [
    "{p} 加多行文字“{t}”", "{p} 写多行文字：{t}", "在 {p} 放段说明文字“{t}”",
    "{layer} 层 {p} 加多行文字“{t}”", "多行文字“{t}”放在 {p}", "{p} 处写注释“{t}”，字高 {h}",
]
POLY_UTT = [
    "画多段线经过 {pts}", "连一条折线：{pts}", "多段线，顶点依次是 {pts}",
    "{layer} 层画折线 {pts}", "画个多段线，节点 {pts}", "沿 {pts} 连成多段线",
]
HATCH_UTT = [
    "把 {pts} 这个范围填充一下", "在 {pts} 围成的区域加填充", "{pts} 区域打剖面线",
    "填充 {pts} 这块，图案 SOLID", "{layer} 层给 {pts} 加填充", "{pts} 围起来的地方填充",
    "把这块 {pts} 刷上填充，比例 {s}",
]
TABLE_UTT = [
    "在 {p} 建个 {rows} 行 {cols} 列的表", "{p} 插一张 {rows}×{cols} 的表格",
    "画表：位置 {p}，{rows} 行 {cols} 列", "{layer} 层 {p} 建表 {rows} 行 {cols} 列",
    "{p} 放个 {rows} 行 {cols} 列的明细表", "在 {p} 加表格，{rows} 行 {cols} 列",
]
RECT_UTT = [
    "画个矩形 {p1} 到 {p2}", "从 {p1} 到 {p2} 画矩形", "{p1}-{p2} 画个框",
    "矩形：角点 {p1} 和 {p2}", "{layer} 层画矩形 {p1} 到 {p2}",
    "画矩形 {p1} {p2}，竖着四等分", "{p1} 到 {p2} 画矩形，横线三等分",
    "画矩形 {p1}-{p2}，vdivide={n}", "画矩形 {p1} 到 {p2}，等分 {n} 份（竖线）",
]
OFFSET_UTT = [
    "把句柄 {h} 的图元偏移 {d}", "句柄 {h} 那条线往右偏 {d}", "偏移 {h}，距离 {d}",
    "对 {h} 做偏移，{d} mm", "{h} 这个图元向内偏移 {d}",
]
QUERY_UTT = [
    "查一下 {layer} 层有什么", "看看 {layer} 层现在有几个图元", "统计 {layer} 层的图元",
    "列出 {layer} 层的内容", "{layer} 层里都有什么", "查 {layer} 层的图元数量和类型",
]
ERASE_UTT = [
    "把句柄 {h} 的图元删掉", "删掉 {h}", "删除句柄为 {h} 的对象", "清理 {h} 这个图元",
    "把 {h} 从图上移除", "去掉 {h}",
]
STATS_UTT = [
    "现在图上有多少图元", "统计当前图纸", "看一下总共有几个实体",
    "图上共多少对象", "报一下当前实体总数", "统计一下全部图元",
]
LAYER_UTT = [
    "新建一个图层 {layer}", "加个层叫 {layer}", "建图层 {layer}，颜色 {c}",
    "创建 {layer} 层", "我要一个新图层 {layer}", "新建 {layer} 图层并设成颜色 {c}",
]
SAVE_UTT = [
    "保存图纸", "存一下盘", "保存当前 dwg", "把图保存了", "落盘保存",
    "保存到文件",
]
BLOCK_UTT = [
    "在 {p} 插入块 {b}", "{p} 处插入块参照 {b}", "放一个 {b} 块到 {p}",
    "{layer} 层 {p} 插块 {b}", "插块 {b}，位置 {p}",
]
BATCH_UTT = [
    "一次画完：一条线 {p1}-{p2}，一个圆 圆心 {c} 半径 {r}，再在 {p3} 写字“{t}”",
    "顺手做三件事：画线 {p1} 到 {p2}、{c} 画半径 {r} 的圆、{p3} 写“{t}”",
    "批量：线 {p1}-{p2}；圆 {c} r{r}；文字“{t}”在 {p3}",
    "一次事务里：先画矩形 {p1}-{p2}，再在 {p2} 写“{t}”",
]
ENTITY_UTT = [
    "句柄 {h} 是什么图元", "查 {h} 的类型和图层", "{h} 这个对象的属性",
    "看看 {h} 是哪类实体，在哪个层", "{h} 是线还是圆",
]

TOOL_POOLS = [
    ("cad_line", LINE_UTT, lambda: {"p1": _pt(), "p2": _pt(),
                                    "layer": R.choice([_layer(), None])}),
    ("cad_circle", CIRCLE_UTT, lambda: {"center": _pt(), "radius": _num(5, 300, 5),
                                        "layer": R.choice([_layer(), None])}),
    ("cad_arc", ARC_UTT, lambda: {"center": _pt(), "radius": _num(5, 300, 5),
                                  "start_angle": _num(0, 350, 5),
                                  "end_angle": _num(0, 350, 5),
                                  "layer": R.choice([_layer(), None])}),
    ("cad_text", TEXT_UTT, lambda: {"pos": _pt(), "text": R.choice(TEXTS),
                                    "height": R.choice([None, 2.5, 3.5, 5]),
                                    "layer": R.choice([_layer(), None])}),
    ("cad_mtext", MTEXT_UTT, lambda: {"pos": _pt(), "text": R.choice(TEXTS),
                                      "height": R.choice([None, 2.5, 3.5]),
                                      "layer": R.choice([_layer(), None])}),
    ("cad_polyline", POLY_UTT, lambda: {"points": [_pt() for _ in range(R.randrange(3, 6))],
                                        "layer": R.choice([_layer(), None])}),
    ("cad_hatch", HATCH_UTT, lambda: {"points": [_pt() for _ in range(R.randrange(3, 5))],
                                     "pattern": "SOLID",
                                     "scale": R.choice([None, 1.0, 2.0]),
                                     "layer": R.choice([_layer(), None])}),
    ("cad_table", TABLE_UTT, lambda: {"pos": _pt(), "rows": R.randrange(2, 6),
                                      "cols": R.randrange(2, 5),
                                      "layer": R.choice([_layer(), None])}),
    ("cad_rect", RECT_UTT, lambda: {"p1": _pt(), "p2": _pt(),
                                    "vdivide": R.choice([None, None, 2, 3, 4]),
                                    "layer": R.choice([_layer(), None])}),
    ("cad_offset", OFFSET_UTT, lambda: {"handle": str(_num(100, 20000, 1)),
                                        "dist": R.choice([10, 20, 50, 100])}),
    ("cad_query", QUERY_UTT, lambda: {"layer": _layer()}),
    ("cad_erase", ERASE_UTT, lambda: {"handles": [str(_num(100, 20000, 1))]}),
    ("cad_stats", STATS_UTT, lambda: {}),
    ("cad_layer", LAYER_UTT, lambda: {"name": _layer(),
                                      "color": R.choice([None, 1, 2, 3, 4, 5, 6, 7])}),
    ("cad_save", SAVE_UTT, lambda: {}),
    ("cad_block_ref", BLOCK_UTT, lambda: {"pos": _pt(), "block": R.choice(BLOCKS),
                                          "layer": R.choice([_layer(), None])}),
    ("cad_entity", ENTITY_UTT, lambda: {"handle": str(_num(100, 20000, 1))}),
]

BATCH_MAKER = lambda: {"calls": [
    {"tool": "cad_line", "args": {"p1": _pt(), "p2": _pt()}},
    {"tool": "cad_circle", "args": {"center": _pt(), "radius": _num(5, 200, 5)}},
    {"tool": "cad_text", "args": {"pos": _pt(), "text": R.choice(TEXTS)}},
]}


def fill(utt, args):
    """把参数填进措辞模板；模板里没有的占位符忽略。"""
    p1 = args.get("p1") or args.get("center") or args.get("pos") or [0, 0]
    p2 = args.get("p2") or args.get("center") or args.get("pos") or [0, 0]
    p3 = args.get("pos") or p1
    c = args.get("center") or args.get("pos") or [0, 0]
    d = args.get("dist") or 10
    r = args.get("radius") or 10
    return (utt
            .replace("{p1}", _fmt(p1)).replace("{p2}", _fmt(p2))
            .replace("{p3}", _fmt(p3)).replace("{p}", _fmt(args.get("pos") or p1))
            .replace("{c}", _fmt(c)).replace("{r}", str(r))
            .replace("{d}", str(d))
            .replace("{a1}", str(args.get("start_angle", 0)))
            .replace("{a2}", str(args.get("end_angle", 90)))
            .replace("{t}", str(args.get("text", "文字")))
            .replace("{h}", str(args.get("handle") or (args.get("handles") or ["0"])[0]))
            .replace("{layer}", str(args.get("layer") or "0"))
            .replace("{rows}", str(args.get("rows", 2)))
            .replace("{cols}", str(args.get("cols", 2)))
            .replace("{n}", str(args.get("vdivide") or 4))
            .replace("{s}", str(args.get("scale") or 1))
            .replace("{b}", str(args.get("block") or "_Dot"))
            .replace("{pts}", "、".join(_fmt(q) for q in (args.get("points") or [])))
            .replace("{d}", str(d)))


def _fmt(p):
    if isinstance(p, (list, tuple)):
        return "(%s,%s)" % (p[0], p[1])
    return str(p)


def gen_tools(n_target):
    made = 0
    guard = 0
    while made < n_target and guard < n_target * 40:
        guard += 1
        if R.random() < 0.08:
            args = BATCH_MAKER()
            if _add(R.choice(BATCH_UTT), "cad_batch", args):
                made += 1
            continue
        name, pool, maker = R.choice(TOOL_POOLS)
        args = maker()
        args = {k: v for k, v in args.items() if v is not None}
        utt = R.choice(pool)
        u = fill(utt, args)
        if _add(u, name, args):
            made += 1
    return made


# ---------------- 2. 类方法：35000 ----------------
VARNAME = {
    "PyDbDatabase": "db", "PyDbTrans": "tr", "PyDbTransAction": "act",
    "PyDbBlockTable": "bt", "PyDbLayerTable": "lt",
    "PyDbBlockTableRecord": "btr", "PyDbSymbolTableRecord": "rec",
    "PyDbLayerTableRecord": "rec", "PyDbDimStyleTableRecord": "rec",
    "PyDbTextStyleTableRecord": "rec", "PyDbLinetypeTableRecord": "rec",
}


def _varname(cls):
    if cls in VARNAME:
        return VARNAME[cls]
    if cls.startswith("AcGe"):
        return "g"
    return "e"


def _parse_sig(sig):
    """PyDbCircle.setRadius(r: double) -> (cls, method, [(name,type)])"""
    m = re.match(r"^(\w+)\.(\w+)\((.*)\)$", sig or "")
    if not m:
        m2 = re.match(r"^(\w+)\.(\w+)$", sig or "")
        if not m2:
            return None
        return m2.group(1), m2.group(2), []
    cls, meth, raw = m.group(1), m.group(2), m.group(3).strip()
    if not raw:
        return cls, meth, []
    params = []
    for part in raw.split(","):
        p = part.strip()
        p = re.sub(r"^const\s+|&\s*$|\*\s*$", "", p).strip()
        if ":" in p:
            nm, tp = p.split(":", 1)
            params.append((nm.strip(), tp.strip()))
    return cls, meth, params


def _typ_arg(tp):
    """把 C++ 形参类型转成可写的实参表达式；认不出就返回 None（宁可不出样本）。"""
    t = tp.lower()
    if "acgepoint3d" in t:
        return "Ge.AcGePoint3d(%s, %s, 0.0)" % (_num(0, 900, 10), _num(0, 900, 10))
    if "acgepoint2d" in t:
        return "Ge.AcGePoint2d(%s, %s)" % (_num(0, 900, 10), _num(0, 900, 10))
    if "acgevector3d" in t:
        return "Ge.AcGeVector3d(1.0, 0.0, 0.0)"
    if "acgevector2d" in t:
        return "Ge.AcGeVector2d(1.0, 0.0)"
    if "acgematrix3d" in t:
        return "Ge.AcGeMatrix3d()"
    if "std::string" in t or t.endswith("string&") or "string" in t:
        return '"%s"' % R.choice(LAYERS)
    if "bool" in t:
        return R.choice(["True", "False"])
    if "double" in t or "float" in t:
        return str(R.choice([1, 2.5, 10, R.randrange(1, 500)]))
    if "int" in t or "uint" in t or "short" in t:
        return str(R.randrange(0, 100))
    return None


# 与"画图"无关的模块：渲染器/框架/编辑器/界面等，不进绘图数据集，避免污染分布
EXCLUDE_PREFIX = ("AcGi", "AcRx", "AcAp", "AcUi", "AcGs", "AcCm", "AcSt",
                  "AcPl", "AcEd", "AcLy", "AcBr", "AcModeler", "AcTr", "AcDMM",
                  "AcDbAssoc", "AcDbDwgFiler", "AcDbDxfFiler",
                  # 配置/样式类：绘图场景里用不到，混进来只会教模型乱调
                  "PyDbVisualStyle", "PyDbTableStyle", "PyDbObjectContext",
                  "PyDbObjectId", "PyDbDwgFiler", "PyDbDictionary",
                  # LISP 桥接类不是 Python 侧绘图 API
                  "Ads")
SET_VERBS = ("set", "add", "append", "insert", "remove", "erase", "delete",
             "attach", "put", "update")
GET_VERBS = ("get", "is", "has", "num", "count", "find", "contains", "can",
             "to", "bound")

SET_UTT = ["{cls} 怎么设置 {m}", "{cls} 的 {m} 怎么用", "对 {cls} 调用 {m} 设置值",
           "{cls}.{m} 要传什么参数", "{cls} 用 {m} 改属性"]
GET_UTT = ["{cls} 怎么读 {m}", "{cls} 的 {m} 返回什么", "取 {cls} 的 {m} 的值",
           "{cls}.{m} 怎么取值", "查 {cls} 的 {m}"]
ACT_UTT = ["{cls} 的 {m} 怎么调", "{cls}.{m} 的用法", "对 {cls} 执行 {m}",
           "{cls} 的 {m} 要什么参数", "{cls}.{m} 的调用写法"]


def _api_call(cls, meth, params, v):
    """拼出真实调用表达式；形参类型认不出就返回 None（这条不出样本）。"""
    if not params:
        return "%s.%s()" % (v, meth), False
    args = []
    for _nm, tp in params:
        a = _typ_arg(tp)
        if a is None:
            return None, False
        args.append(a)
    return "%s.%s(%s)" % (v, meth, ", ".join(args)), False


def _utt_for(cls, meth, shape):
    if meth.startswith(SET_VERBS):
        pool = SET_UTT
    elif meth.startswith(GET_VERBS):
        pool = GET_UTT
    else:
        pool = ACT_UTT
    return pool[shape % len(pool)].format(cls=cls, m=meth)


def gen_api(n_target):
    if not os.path.isfile(IDX):
        return 0
    sigs = []
    with open(IDX, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            r = json.loads(line)
            sig = r.get("sig") or ""
            if sig.startswith(EXCLUDE_PREFIX):
                continue
            sigs.append(sig)
    R.shuffle(sigs)
    made = 0
    for sig in sigs:
        if made >= n_target:
            break
        parsed = _parse_sig(sig)
        if not parsed:
            continue
        cls, meth, params = parsed
        if meth.startswith("_"):
            continue
        v = _varname(cls)
        call, _ = _api_call(cls, meth, params, v)
        if call is None:
            continue
        for shape in range(5):
            if made >= n_target:
                break
            utt = _utt_for(cls, meth, shape)
            if shape == 0:
                code = "```python\n%s\n```" % call
            elif shape == 1:
                code = ("```python\nwith P.Db.PyDbTrans() as tr:\n    %s\n```" % call)
            elif shape == 2:
                code = "```python\n# %s 为已打开对象\n%s\n```" % (v, call)
            elif shape == 3:
                if meth.startswith(GET_VERBS):
                    code = "```python\nval = %s\n```" % call
                else:
                    # 非取值方法不写 val = ...，避免暗示返回值
                    code = "```python\n%s\n```" % call
            else:
                code = "```python\n# %s.%s\n%s\n```" % (cls, meth, call)
            if _add_code(utt, code, kind="api"):
                made += 1
    return made


# ---------------- 3. 图元代码：15000 ----------------
def _lit(v):
    if isinstance(v, str):
        return json.dumps(v, ensure_ascii=False)
    if isinstance(v, bool):
        return "True" if v else "False"
    if isinstance(v, (int, float)):
        return str(v)
    return json.dumps(v, ensure_ascii=False)


def _p3(v):
    return "Ge.AcGePoint3d(%r, %r, 0.0)" % (float(v[0]), float(v[1]))


def _p2(v):
    return "Ge.AcGePoint2d(%r, %r)" % (float(v[0]), float(v[1]))


def concretize(code, sample):
    """把样板里的 a['k'] / _p3(...) / _rad(...) 全部替换成字面量。
    返回 None 表示有无法安全展开的残留（这条就不出样本，宁缺勿造假）。"""
    import math
    code = re.sub(r"_p3\(a\['(\w+)'\]\)", lambda m: _p3(sample[m.group(1)]), code)
    code = re.sub(r"_p2\(a\['(\w+)'\]\)", lambda m: _p2(sample[m.group(1)]), code)
    code = re.sub(r"_p3\(_q\)", "Ge.AcGePoint3d(_q[0], _q[1], 0.0)", code)
    code = re.sub(r"_p2\(_q\)", "Ge.AcGePoint2d(_q[0], _q[1])", code)
    code = re.sub(r"_rad\(a\.get\('(\w+)',\s*([\d.]+)\)\)",
                  lambda m: str(round(math.radians(float(sample.get(m.group(1), m.group(2)))), 6)), code)
    code = re.sub(r"_rad\(a\['(\w+)'\]\)",
                  lambda m: str(round(math.radians(float(sample[m.group(1)])), 6)), code)
    code = re.sub(r"float\(a\.get\('(\w+)',\s*([\d.]+)\)\)",
                  lambda m: str(float(sample.get(m.group(1), m.group(2)))), code)
    code = re.sub(r"float\(a\['(\w+)'\]\)", lambda m: str(float(sample[m.group(1)]))
                  if not isinstance(sample[m.group(1)], str) else _lit(sample[m.group(1)]), code)
    code = re.sub(r"int\(a\.get\('(\w+)',\s*(\d+)\)\)",
                  lambda m: str(int(sample.get(m.group(1), m.group(2)))), code)
    code = re.sub(r"str\(a\['(\w+)'\]\)", lambda m: _lit(str(sample[m.group(1)])) , code)
    code = re.sub(r"bool\(a\.get\('(\w+)',\s*(True|False)\)\)",
                  lambda m: "True" if bool(sample.get(m.group(1), m.group(2) == "True")) else "False",
                  code)
    code = re.sub(r"a\['(\w+)'\]", lambda m: _lit(sample[m.group(1)]) , code)
    code = re.sub(r"a\.get\('(\w+)',\s*([^)]*)\)",
                  lambda m: _lit(sample.get(m.group(1), _safe_eval(m.group(2)))), code)
    # 残留说明仍有猜的成分，直接丢弃
    if re.search(r"_p[23]\(|_rad\(|a\[|a\.get\(", code):
        return None
    return code


def _safe_eval(s):
    s = s.strip()
    try:
        return eval(s)
    except Exception:
        return s.strip("'\"")


def gen_entity(n_target):
    try:
        import sys as _s
        _s.path.insert(0, CADMCP)
        import proto
    except Exception:
        return 0
    made = 0
    kinds = proto.kinds()
    guard = 0
    while made < n_target and guard < n_target * 30:
        guard += 1
        k = R.choice(kinds)
        spec = proto.ENTITY[k]
        sample = dict(spec.get("sample") or {})
        for key, val in list(sample.items()):
            if (isinstance(val, (list, tuple)) and len(val) == 2
                    and all(isinstance(x, (int, float)) for x in val)):
                sample[key] = _pt(0, 400)
            elif isinstance(val, (list, tuple)):
                sample[key] = [_pt(0, 400) for _ in (val or [1])]
            elif isinstance(val, (int, float)) and not isinstance(val, bool):
                sample[key] = R.choice([10, 20, 30, 40, 50, 80, 120])
        if "tr.addEntity" in (spec.get("build") or ""):
            # 该候选是给验证器用的（依赖 ns['added']），不适合当模型样板
            continue
        body = proto.build_code(k)
        if not body:
            continue
        body = concretize(body, sample)
        if not body:
            continue
        code = (body.strip()
                + "\nwith P.Db.PyDbTrans() as tr:\n"
                  "    tr.addEntity(e)\n"
                  "print(e.handle())")
        utt = R.choice([
            "用 PyArxCore 画一个{k_cn}，参数自己定",
            "写代码建{k_cn}，最后提交事务",
            "在模型空间加一个{k_cn}",
            "{k_cn} 怎么用代码画出来",
            "给我{k_cn}的完整创建代码",
            "用事务建{k_cn}并取句柄",
        ]).format(k_cn=CN.get(k, k))
        if _add_code(utt, "```python\n" + code + "\n```", kind="entity"):
            made += 1
    return made


CN = {
    "line": "直线", "polyline": "多段线", "circle": "圆", "arc": "圆弧",
    "text": "单行文字", "mtext": "多行文字", "point": "点", "solid": "实体填充四边形",
    "xline": "无限长线", "ray": "射线", "ellipse": "椭圆", "spline": "样条曲线",
    "hatch": "图案填充", "table": "表格", "mline": "多线", "leader": "引线",
    "wipeout": "遮挡", "rotated_dim": "转角标注", "block_ref": "块参照",
}

# ---------------- 4. 几何运算：8000 ----------------
def gen_geo(n_target):
    made = 0
    guard = 0
    while made < n_target and guard < n_target * 20:
        guard += 1
        c = R.randrange(3)
        a, b = _num(-200, 300, 5), _num(-200, 300, 5)
        if c == 0:
            # 向量模
            x, y, z = _num(-100, 100, 5), _num(-100, 100, 5), R.choice([0, 5, 10])
            L = round((x * x + y * y + z * z) ** 0.5, 4)
            utt = R.choice(["算这个向量的长度：(%d,%d,%d)" % (x, y, z),
                            "(%d,%d,%d) 的模是多少" % (x, y, z),
                            "用 AcGeVector3d 求 (%d,%d,%d) 的长度" % (x, y, z)])
            code = ("```python\nv = Ge.AcGeVector3d(%d.0, %d.0, %d.0)\n"
                    "print(v.length())  # -> %s\n```" % (x, y, z, L))
        elif c == 1:
            # 两点距离
            x1, y1, x2, y2 = a, b, _num(-200, 300, 5), _num(-200, 300, 5)
            L = round(((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5, 4)
            utt = R.choice(["(%d,%d) 到 (%d,%d) 的距离" % (x1, y1, x2, y2),
                            "求两点 (%d,%d)、(%d,%d) 之间多远" % (x1, y1, x2, y2),
                            "用 LineSeg3d 求 (%d,%d) 到 (%d,%d) 的长度" % (x1, y1, x2, y2)])
            code = ("```python\nseg = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(%d.0, %d.0, 0.0),\n"
                    "                       Ge.AcGePoint3d(%d.0, %d.0, 0.0))\n"
                    "print(seg.length())  # -> %s\n```" % (x1, y1, x2, y2, L))
        else:
            # 旋转矩阵作用于点
            ang = R.choice([30, 45, 60, 90, 180])
            x1, y1 = _num(-100, 200, 5), _num(-100, 200, 5)
            ca, sa = round(__import__("math").cos(__import__("math").radians(ang)), 4), \
                     round(__import__("math").sin(__import__("math").radians(ang)), 4)
            nx, ny = round(x1 * ca - y1 * sa, 3), round(x1 * sa + y1 * ca, 3)
            utt = R.choice(["把点 (%d,%d) 绕原点转 %d 度后在哪" % (x1, y1, ang),
                            "用 AcGeMatrix3d 旋转 (%d,%d) %d°" % (x1, y1, ang),
                            "点 (%d,%d) 旋转 %d 度的结果" % (x1, y1, ang)])
            code = ("```python\nm = Ge.AcGeMatrix3d()\nm.setToRotation(%s, Ge.AcGeVector3d(0.0,0.0,1.0))\n"
                    "p = Ge.AcGePoint3d(%d.0, %d.0, 0.0)\nq = p.transformBy(m)\n"
                    "print(q.x, q.y)  # -> %s, %s\n```"
                    % (round(__import__("math").radians(ang), 6), x1, y1, nx, ny))
        if _add_code(utt, code, kind="geo"):
            made += 1
    return made


# ---------------- 5. 纠错：2000 ----------------
CORRECT_PAIRS = [
    ("用 e.getBoundingPoints() 取包围盒", "e.bounds() 返回 (min, max) 两个 AcGePoint3d，没有 getBoundingPoints"),
    ("PyDbEntity.openObjectByIdString('731') 打开图元", "没有这个方法；要在事务内遍历 db.objectIds('AcDbEntity')、用 tr.openObject(eid, 0) 比对 handle()"),
    ("用 e.isA() 判断类名", "实体上没有 isA，用 e.className()"),
    ("Ge.AcGePoint3dArray() 装点数组", "Ge 没导出该数组类型；样条改用 setFitPointAt/insertFitPointAt"),
    ("PyDbWipeout 用 appendLoopPoints 设边界", "只有 append；且传 list 后 addEntity 仍返回 False，遮挡暂不可用"),
    ("PyDbMline 用 appendSegments", "真名是 appendSeg（不加 s）"),
    ("e = Db.PyDbPoint(pt) 直接带点构造", "PyDbPoint() 后 setPosition(pt)"),
    ("e = Db.PyDbSolid(p1,p2,p3,p4)", "PyDbSolid() 后 setPointAt(i, pt) 四次"),
    ("Db.PyDbXline(pt, vec) 建无限线", "PyDbXline() 后 setBasePoint + setUnitDir"),
    ("ellipse.setMajorAxis(vec) 设主轴", "没有 setMajorAxis；用 setCenter/setMajorRadius/setRadiusRatio（但当前绑定下半径 setter 无效）"),
    ("tr.openObject(eid) 不写模式", "必须显式给 0=kForRead 或 1=kForWrite，省略会拿到未定义行为"),
    ("图层表用完不 close", "符号表一个进程只能开一次且必须 close()，否则带写锁存盘直接崩"),
    ("tr.addEntity(e) 之后不判断返回值", "addEntity 返回 bool，失败不抛异常，必须判返回值"),
    ("e = Db.PyDbLine(...) 之后先 setLayer 再 addEntity", "入库前对象未 setDatabaseDefaults，属性写入可能丢失；先 addEntity 再写属性"),
    ("在事务里 getLayerTable 新建图层", "图层要在事务外建，开完必须 close"),
    ("cad_layer 用来查某层有什么", "cad_layer 只用于新建图层，查询用 cad_query"),
    ("矩形等分自己算竖线 x 坐标再补 line", "给 cad_rect 填 vdivide=N 就行，坐标由工具层 _expand_layout 算"),
    ("cad_erase 传 handle 单值", "cad_erase 收的是 handles 数组，形如 [\"698\"]"),
    ("offset 用 setOffset 改源图元", "偏移应调用 getOffsetCurves(dist) 生成新曲线，不改源对象"),
    ("cad_rect 传 hdivide 想要竖线", "vdivide 是竖线等分（沿 x 切），hdivide 是横线等分（沿 y 切）"),
    ("用 e.bounds() 判断实体是否有效", "半无限线（xline/ray）本就没有有限盒，别拿盒退化当失败"),
    ("保存后不校验文件大小", "saveAs 之后要读回字节数确认，失败可能静默"),
    ("事务里 openObject 拿到的对象带出事务用", "对象随事务结束关闭，跨事务复用会报对象已关闭"),
]
CORRECT_UTT = [
    "我写了 {bad}，对吗？",
    "{bad} 这样能跑吗",
    "同事给的代码是 {bad}，有问题吗",
    "{bad} 报错了，正确的怎么写",
    "{bad} 看着没问题，为什么出错",
    "{bad} 这么写有什么坑",
]


def gen_correct(n_target):
    made = 0
    guard = 0
    while made < n_target and guard < n_target * 30:
        guard += 1
        bad, good = R.choice(CORRECT_PAIRS)
        utt = R.choice(CORRECT_UTT).format(bad=bad)
        code = "有问题。" + good + "。"
        if _add_code(utt, code, kind="correct"):
            made += 1
    return made


def main():
    n_tool = gen_tools(40000)
    n_api = gen_api(35000)
    n_ent = gen_entity(15000)
    n_geo = gen_geo(10000)
    n_cor = gen_correct(300)
    # 保证总量正好 100000；差额由几何池补（参数空间足够，不制造重复）
    target = 100000
    need = target - len(VISIBLE)
    if need > 0:
        gen_geo(need + 200)
    VISIBLE[:] = VISIBLE[:target]
    total = len(VISIBLE)
    with open(OUT, "w", encoding="utf-8") as f:
        for s in VISIBLE:
            f.write(json.dumps(s, ensure_ascii=False) + "\n")
    kinds = {}
    tools = {}
    for s in VISIBLE:
        k = s["meta"].get("kind")
        kinds[k] = kinds.get(k, 0) + 1
        t = s["meta"].get("tool")
        if t:
            tools[t] = tools.get(t, 0) + 1
    print("目标 100000，实际 %d 条（去重后）" % total)
    print("  工具调用 %d / 类方法 %d / 图元代码 %d / 几何 %d / 纠错 %d"
          % (n_tool, n_api, n_ent, n_geo, n_cor))
    print("  kind 分布：%s" % json.dumps(kinds, ensure_ascii=False))
    print("  工具分布：%s" % json.dumps(tools, ensure_ascii=False))
    print("  文件：%s (%.1f MB)" % (OUT, os.path.getsize(OUT) / 1048576.0))


if __name__ == "__main__":
    main()
