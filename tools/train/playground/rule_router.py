# -*- coding: utf-8 -*-
"""
确定性指令路由：把常见中文 CAD 指令直接映射成 cad_* 工具调用。

为什么需要它：
  1.5B + 300 条 LoRA 在“结构化调用”上能学会一部分，但在坐标换算、等分、
  层名识别这类确定性语义上会飘。这些句式用规则产出是 100% 稳定的，
  模型只留作复杂/开放语义的兜底。

用法：
    from rule_router import route
    call = route("在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线")
    # -> {"tool": "cad_line", "args": {"p1": [0,0], "p2": [120,0], "layer": "LIVE_TEST"}}
    未命中返回 None。

设计约束：只做“能确定”的句式；任何需要猜测的地方返回 None，宁可交给模型，
也不要给出错误坐标。
"""
import re

_CN_NUM = {"一": 1, "二": 2, "两": 2, "三": 3, "四": 4, "五": 5,
           "六": 6, "七": 7, "八": 8, "九": 9, "十": 10}

# 坐标：兼容 (0,0) / 0,0 / 0，0 / -5,10 / 0.5,2
_PT = r"\(\s*(-?\d+(?:\.\d+)?)\s*[,，]\s*(-?\d+(?:\.\d+)?)\s*\)"
_PT_LOOSE = r"\(?\s*(-?\d+(?:\.\d+)?)\s*[,，]\s*(-?\d+(?:\.\d+)?)\s*\)?"

# 图层名：在 XXX 层 / 于 XXX 层 / 图层 XXX；排除“这个/那个/当前”
_LAYER = r"(?:在|于|到)\s*([A-Za-z0-9_\-一-龥]{1,20}?)\s*(?:图层|层)"
_LAYER_ALT = r"([A-Za-z0-9_\-]+)\s*层"


def _num(v):
    """数字或小数字符串 -> int/float"""
    f = float(v)
    return int(f) if abs(f - int(f)) < 1e-9 else f


def _cn_or_digit(token):
    token = token.strip()
    if token.isdigit():
        return int(token)
    return _CN_NUM.get(token)


def _layer(text):
    for pat in (_LAYER, _LAYER_ALT):
        m = re.search(pat, text)
        if m:
            name = m.group(1)
            if name in ("这个", "那个", "当前", "该", "此", "某"):
                continue
            return name
    return None


def _with_layer(args, text):
    lay = _layer(text)
    if lay:
        args["layer"] = lay
    return args


# ---------------------------------------------------------------- 具体规则

def _rule_erase(text):
    """删句柄 698 / 把句柄 698 的那个图元删掉 / 删除句柄 700、701"""
    if not re.search(r"删|移除|去掉", text):
        return None
    hs = re.findall(r"句柄\s*([0-9A-Fa-f]{1,6})", text)
    if not hs:
        # 也接受 “删掉 698 那个图元” 这种把句柄直接写成数字的
        hs = re.findall(r"\b([0-9]{2,6})\b", text)
        if len(hs) != 1:
            return None
    return {"tool": "cad_erase", "args": {"handles": [str(h).upper() for h in hs]}}


def _rule_query(text):
    """查 LIVE_TEST 层有哪些图元 / 列出 0 层的线 / 有没有句柄 698 的东西"""
    if not re.search(r"查|列出|哪些|有什么|有没有|统计", text):
        return None
    if re.search(r"统计|多少", text):
        return {"tool": "cad_stats", "args": {}}
    lay = _layer(text)
    if lay:
        return {"tool": "cad_query", "args": {"layer": lay}}
    cls = None
    for k, v in (("线", "AcDbLine"), ("圆", "AcDbCircle"), ("文字", "AcDbText"),
                 ("多段线", "AcDbPolyline"), ("块", "AcDbBlockReference")):
        if k in text:
            cls = v
            break
    if cls:
        return {"tool": "cad_query", "args": {"cls": cls}}
    return None


def _rule_line(text):
    """画线：从 (0,0) 到 (120,0) / 从 100,50 到 300,50 画水平线"""
    if not re.search(r"线|连线|连起", text):
        return None
    m = re.search(_PT_LOOSE + r"\s*(?:到|至|~|-)\s*" + _PT_LOOSE, text)
    if not m:
        return None
    p1 = [_num(m.group(1)), _num(m.group(2))]
    p2 = [_num(m.group(3)), _num(m.group(4))]
    if p1 == p2:
        return None
    return {"tool": "cad_line", "args": _with_layer({"p1": p1, "p2": p2}, text)}


def _rule_polyline(text):
    """用多段线连起这些点：(325,58)、(155,197)、(205,268)"""
    if "多段线" not in text and "polyline" not in text.lower():
        return None
    pts = re.findall(_PT_LOOSE, text)
    if len(re.findall(_PT, text)) < 2 and len(pts) < 2:
        return None
    arr = [[_num(x), _num(y)] for x, y in pts]
    if len(arr) < 2:
        return None
    closed = "闭合" in text or "closed" in text.lower()
    return {"tool": "cad_polyline", "args": _with_layer({"pts": arr, "closed": closed}, text)}


def _rule_circle(text):
    """画圆，圆心 50,50，半径 30 / 以 (100,100) 为圆心半径 20 画圆"""
    if "圆" not in text or "圆弧" in text or "圆柱" in text:
        return None
    m = re.search(r"圆心\s*" + _PT_LOOSE, text) or re.search(_PT_LOOSE + r"\s*为?圆心", text)
    if not m:
        return None
    r = re.search(r"半径\s*(?:为|=|:)?\s*(-?\d+(?:\.\d+)?)", text)
    if not r:
        return None
    c = [_num(m.group(1)), _num(m.group(2))]
    return {"tool": "cad_circle", "args": _with_layer(
        {"center": c, "radius": _num(r.group(1))}, text)}


def _rule_arc(text):
    """画圆弧，圆心 50,50，半径 30，从 0 度到 90 度"""
    if "圆弧" not in text and "弧" not in text:
        return None
    m = re.search(r"圆心\s*" + _PT_LOOSE, text)
    r = re.search(r"半径\s*(?:为|=|:)?\s*(-?\d+(?:\.\d+)?)", text)
    if not m or not r:
        return None
    a = re.search(r"从\s*(-?\d+(?:\.\d+)?)\s*度?\s*(?:到|至)\s*(-?\d+(?:\.\d+)?)\s*度", text)
    args = {"center": [_num(m.group(1)), _num(m.group(2))], "radius": _num(r.group(1))}
    if a:
        args["start_deg"] = _num(a.group(1))
        args["end_deg"] = _num(a.group(2))
    return {"tool": "cad_arc", "args": _with_layer(args, text)}


def _rule_rect(text):
    """800x600 的矩形 / 500 长 600 高的柜 / 矩形 300x600 三等分"""
    if not re.search(r"矩形|方框|柜|框", text):
        return None
    m = re.search(r"(\d+(?:\.\d+)?)\s*(?:x|X|×|\*)\s*(\d+(?:\.\d+)?)", text)
    if m:
        w, h = _num(m.group(1)), _num(m.group(2))
    else:
        m = re.search(r"(\d+(?:\.\d+)?)\s*(?:长|宽)\s*(\d+(?:\.\d+)?)\s*高", text)
        if not m:
            return None
        w, h = _num(m.group(1)), _num(m.group(2))
    # 基准点：给了左下角/原点坐标就用，否则按原点
    base = [0, 0]
    bm = re.search(r"(?:左下角|起点|基点|位置|在)\s*(?:在|是|为)?\s*" + _PT_LOOSE, text)
    if bm:
        base = [_num(bm.group(1)), _num(bm.group(2))]
    p1 = [base[0], base[1]]
    p2 = [_num(base[0] + w), _num(base[1] + h)]
    args = {"p1": p1, "p2": p2}
    # 等分：三等分/四等分/横向 N 等分/竖向 N 等分
    dv, dh = None, None
    for m2 in re.finditer(r"(横向|竖向|竖直|垂直)?\s*([一二两三四五六七八九十])?\s*等分", text):
        direction, cn = m2.group(1), m2.group(2)
        n = _cn_or_digit(cn) if cn else None
        if not n:
            nm = re.search(r"(\d+)\s*等分", text)
            n = _num(nm.group(1)) if nm else None
        if not n:
            continue
        if direction in ("竖向", "竖直", "垂直"):
            dv = n
        else:
            dh = n          # 横向等分 -> 竖线分隔，对应 vdivide
    if dv or dh:
        # 横向 N 等分 = 画 N-1 条竖线 = vdivide
        nd = dv or dh
        if nd and nd >= 2:
            args["vdivide"] = nd
    return {"tool": "cad_rect", "args": _with_layer(args, text)}


def _rule_text(text):
    """在 10,10 写文字「柜A」/ 标题写主回路"""
    m = re.search(r"(?:写|标注|写上|标题写?)\s*[「“\"']?([^「」“”\"'，,。]{1,30})[」”\"']?", text)
    if not m:
        return None
    content = m.group(1).strip()
    if not content:
        return None
    pm = re.search(_PT_LOOSE, text)
    pos = [_num(pm.group(1)), _num(pm.group(2))] if pm else [0, 0]
    hm = re.search(r"字高\s*(?:为|=)?\s*(\d+(?:\.\d+)?)", text)
    args = {"pos": pos, "text": content}
    if hm:
        args["height"] = _num(hm.group(1))
    return {"tool": "cad_text", "args": _with_layer(args, text)}


def _rule_block(text):
    """在 0,0 插入配电箱 块"""
    if not re.search(r"插入|放置|放一个", text):
        return None
    m = re.search(r"(?:插入|放置|放一个?)\s*[「“\"']?([A-Za-z0-9_\u4e00-\u9fa5]{1,24}?)[」”\"']?\s*(?:块|符号|图块)", text)
    if not m:
        return None
    pm = re.search(_PT_LOOSE, text)
    pos = [_num(pm.group(1)), _num(pm.group(2))] if pm else [0, 0]
    return {"tool": "cad_block_ref", "args": _with_layer({"name": m.group(1), "pos": pos}, text)}


def _rule_save(text):
    if re.search(r"保存|存盘", text):
        return {"tool": "cad_save", "args": {}}
    return None


_RULES = [_rule_erase, _rule_query, _rule_arc, _rule_circle, _rule_block,
          _rule_polyline, _rule_rect, _rule_line, _rule_text, _rule_save]


def route(text):
    """返回 {"tool":..., "args":...} 或 None（未命中交给模型）"""
    if not text:
        return None
    t = text.strip()
    for fn in _RULES:
        try:
            r = fn(t)
        except Exception:
            r = None
        if r:
            r["args"].setdefault("layer", None)
            if r["args"].get("layer") is None:
                r["args"].pop("layer", None)
            return r
    return None


# ---------------------------------------------------------------- 自测
_CASES = [
    "在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线",
    "把句柄 698 的那个图元删掉",
    "画一个 800 长 600 高的柜，横向三等分，第一格标题写主回路",
    "画一条从 100,50 到 300,50 的水平线",
    "在轴线层画一条竖直的线，从 0,0 到 0,200",
    "画个 500x300 的矩形，左下角在原点",
    "画圆，圆心 50,50，半径 30",
    "画一段圆弧，圆心 327,52，半径 10，从 90 度到 360 度",
    "在 0,0 插入配电箱 块",
    "查 LIVE_TEST 层有哪些图元",
    "统计当前图纸有多少图元",
    "用多段线连起这些点：(325,58)、(155,197)、(205,268)",
]

if __name__ == "__main__":
    import json
    ok = 0
    for c in _CASES:
        r = route(c)
        print(("OK   " if r else "MISS ") + c)
        if r:
            ok += 1
            print("      -> " + json.dumps(r, ensure_ascii=False))
    print("\n命中 %d/%d" % (ok, len(_CASES)))
