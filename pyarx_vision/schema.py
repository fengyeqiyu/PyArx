# -*- utf-8 -*-
"""pyarx_vision.schema —— 统一实体模型

不同图纸的图元导出格式千差万别（数组式 / 对象式 / 按 handle 分桶），
识别算法只认这一套中立结构，转换放在 adapters.py。

EntitySet = {
  "source": str,
  "frame":  (x0, y0, x1, y1) or None,
  "lines":    [{"a": (x, y), "b": (x, y), "layer": str}],
  "plines":   [{"pts": [(x, y), ...], "closed": bool, "layer": str}],
  "texts":    [{"pos": (x, y), "text": str, "height": float, "rotation": float, "layer": str}],
  "circles":  [{"center": (x, y), "r": float, "layer": str}],
}
"""


def empty(source=""):
    return {"source": source, "frame": None, "lines": [], "plines": [],
            "texts": [], "circles": []}


def add_line(es, a, b, layer=""):
    es["lines"].append({"a": (float(a[0]), float(a[1])),
                        "b": (float(b[0]), float(b[1]),), "layer": str(layer)})


def add_pline(es, pts, closed, layer=""):
    es["plines"].append({"pts": [(float(p[0]), float(p[1])) for p in pts],
                         "closed": bool(closed), "layer": str(layer)})


def add_text(es, pos, text, height, rotation=0.0, layer=""):
    es["texts"].append({"pos": (float(pos[0]), float(pos[1]),), "text": str(text),
                        "height": float(height or 1.0),
                        "rotation": float(rotation or 0.0), "layer": str(layer)})


def add_circle(es, center, r, layer=""):
    es["circles"].append({"center": (float(center[0]), float(center[1]),),
                          "r": float(r), "layer": str(layer)})


def bbox(es):
    """全部图元外包围盒；无图元返回 None"""
    xs, ys = [], []
    for l in es["lines"]:
        xs += [l["a"][0], l["b"][0]]
        ys += [l["a"][1], l["b"][1]]
    for p in es["plines"]:
        for q in p["pts"]:
            xs.append(q[0])
            ys.append(q[1])
    for t in es["texts"]:
        xs.append(t["pos"][0])
        ys.append(t["pos"][1])
    for c in es["circles"]:
        xs += [c["center"][0] - c["r"], c["center"][0] + c["r"]]
        ys += [c["center"][1] - c["r"], c["center"][1] + c["r"]]
    if not xs:
        return None
    return (min(xs), min(ys), max(xs), max(ys))


def validate(es):
    """结构自检，返回问题列表（空 = 通过）"""
    problems = []
    if not any(es[k] for k in ("lines", "plines", "texts", "circles")):
        problems.append("实体集为空")
    for i, l in enumerate(es["lines"]):
        if len(l["a"]) != 2 or len(l["b"]) != 2:
            problems.append("line[%d] 坐标维度异常" % i)
            break
    for i, p in enumerate(es["plines"]):
        if len(p["pts"]) < 2:
            problems.append("pline[%d] 顶点不足" % i)
            break
    for i, t in enumerate(es["texts"]):
        if not t["text"]:
            problems.append("text[%d] 内容为空" % i)
            break
    return problems
