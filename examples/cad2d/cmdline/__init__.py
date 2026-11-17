# -*- coding: utf-8 -*-
"""cmdline —— 命令行（模块 7）

AutoCAD 的"键盘入口"：输入命令名 / 别名 / 坐标都能用。
这一层只做解析，不认识文档也不认识画布 —— 解析结果交给 App 派发。
"""
import interact

# 额外命令（交互模块之外的动作）
EXTRA = {
    "Z": "ZOOM", "ZOOM": "ZOOM",
    "ZE": "ZOOM_E", "EXTENTS": "ZOOM_E",
    "ZI": "ZOOM_IN", "ZOOMIN": "ZOOM_IN",
    "ZO": "ZOOM_OUT", "ZOOMOUT": "ZOOM_OUT",
    "U": "UNDO", "UNDO": "UNDO",
    "REDO": "REDO",
    "OPEN": "OPEN", "打开": "OPEN",
    "RE": "REDRAW", "REDRAW": "REDRAW",
    "LA": "LAYER", "LAYER": "LAYER",
    "SV": "SAVE", "SAVE": "SAVE",
    "Q": "QUIT", "QUIT": "QUIT",
    "T": "TEXT", "TEXT": "TEXT", "DT": "TEXT",
    "?:": "HELP", "HELP": "HELP",
}

# 命令的中文别名（用户敲中文也能进）
CN = {
    "直线": "LINE", "线": "LINE",
    "圆": "CIRCLE", "圆弧": "ARC", "弧": "ARC",
    "矩形": "RECT",
    "多段线": "PLINE",
    "删除": "ERASE",
    "选择": "SELECT",
    "移动": "MOVE", "复制": "COPY", "旋转": "ROTATE", "缩放": "SCALE",
    "缩放": "ZOOM", "全图": "ZOOM_E",
    "文字": "TEXT",
    "图层": "LAYER",
    "保存": "SAVE",
    "成块": "BLOCK", "定义块": "BLOCK",
    "插入块": "INSERT",
    "标注": "DIM",
    "填充": "HATCH",
}

# 命令 → 中文说明（HELP 用）
HELP_TEXT = {
    "LINE": "直线：点起点，之后每点一次连一段，回车结束",
    "PLINE": "多段线：连续点击，回车收尾成一条多段线",
    "CIRCLE": "圆：点圆心，再点半径点",
    "ARC": "圆弧：起点 → 弧上一点 → 端点",
    "RECT": "矩形：两个对角点",
    "TEXT": "文字：点插入点，输入内容",
    "ERASE": "删除：点选对象后删除",
    "SELECT": "选择：点选对象",
    "ZOOM": "缩放：ZE 全图 / ZI 放大 / ZO 缩小",
    "UNDO": "撤销上一步",
    "SAVE": "保存图纸",
    "LAYER": "图层：列出所有图层",
    "BLOCK": "成块：选对象后指定基点，写入块表记录（原图保留）",
    "INSERT": "插入块：落点放置最近定义的块参照",
    "DIM": "线性标注：第一原点 → 第二原点 → 尺寸线位置",
    "HATCH": "区域填充：点选闭合边界，生成剖面线",
}


class ParseResult(object):
    """解析结果：action 是要执行什么，point/args 是附带参数。"""

    def __init__(self, action, point=None, args=None, echo="", command=None):
        self.action = action
        self.point = point
        self.args = args
        self.echo = echo
        self.command = command or args

    def __repr__(self):
        return "<%s %s>" % (self.action, self.point or self.args or "")


def parse_point(text):
    """支持 10,20 / 10 20 / @100<45（相对极坐标）三种定点写法，失败返回 None。"""
    s = str(text).strip().replace("，", ",")
    if not s:
        return None
    try:
        if s.startswith("@"):
            body = s[1:]
            if "<" in body:
                dist, ang = body.split("<", 1)
                return ("polar", float(dist), float(ang))
            parts = body.replace(",", " ").split()
            return ("rel", float(parts[0]), float(parts[1]))
        if "<" in s:
            dist, ang = s.split("<", 1)
            return ("polar_abs", float(dist), float(ang))
        parts = s.replace(",", " ").replace(";", " ").split()
        if len(parts) == 2:
            return ("abs", float(parts[0]), float(parts[1]))
    except (TypeError, ValueError):
        return None
    return None


def parse(line):
    """一行输入 → ParseResult。认不出就返回 None（上层报"未知命令"）。"""
    raw = (line or "").strip()
    if not raw:
        return None

    # 1) 纯坐标：直接当定点
    pt = parse_point(raw)
    if pt is not None:
        return ParseResult("POINT", point=pt, echo=raw)

    # 2) 命令名 / 别名 / 中文
    word = raw.split()[0].upper()
    cn = CN.get(raw) or CN.get(word)
    if cn:
        word = cn
    if word in interact.COMMANDS:
        return ParseResult("CMD", command=word, args=raw[len(word):].strip())
    if word in EXTRA:
        return ParseResult(EXTRA[word], args=raw[len(word):].strip())
    return None


def help_text():
    lines = ["可用命令："]
    for k in sorted(HELP_TEXT):
        lines.append("  %-7s %s" % (k, HELP_TEXT[k]))
    return "\n".join(lines)


if __name__ == "__main__":
    assert parse("L").action == "CMD"
    assert parse("直线").args == "LINE"
    assert parse("10,20").point == ("abs", 10.0, 20.0)
    assert parse("@100<45").point == ("polar", 100.0, 45.0)
    assert parse("ZE").action == "ZOOM_E"
    assert parse("不存在") is None
    print("cmdline 自检通过")
