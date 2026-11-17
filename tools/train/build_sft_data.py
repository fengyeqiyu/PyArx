# -*- coding: utf-8 -*-
"""从 cad_* 工具 schema 合成 SFT 训练样本（指令 → tool_call）。

目的：教模型「看到这类活儿该调哪个工具、参数怎么填」。
样本三源：
  1. 正向：一句中文指令 → 正确的 cad_* 调用
  2. 纠错：错误调用 → 看到 warnings/errors 后改成正确调用（这是最有价值的样本）
  3. 查询/统计：只读场景 → cad_stats / cad_query

输出 OpenAI function-calling 风格的 jsonl，训练时可直接转 chat 模板。
"""

import json
import random

OUT = r"G:\PyArx\tools\train\data\sft_cad_tools.jsonl"
PREFIX_OUT = r"G:\PyArx\tools\train\data\sft_prefix.jsonl"

SYSTEM = (
    "你是 CAD 绘图助手。只通过 cad_* 工具操作图形，不要自己写 PyArx 代码。"
    "用户要脚本或代码时照样输出 cad_* 工具调用，由系统生成 PyArx 代码，不要改成讲架构或手写代码。"
    "坐标单位 mm，角度用度，图层不存在会自动创建。"
    "需要等分时给 cad_rect 填 vdivide=N（竖线 N 等分）或 hdivide=N（横线 N 等分），"
    "等分线坐标由工具层按宽度算，不要自己算竖线 x 值、也不要再补 line。"
    "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。"
    "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
    "失败时要读 warnings 自纠而不是重试同一调用。"
)

# ---------------------------------------------------------------- 正向样本

LINE = [
    ("在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线", "cad_line",
     {"p1": [0, 0], "p2": [120, 0], "layer": "LIVE_TEST"}),
    ("画一条从 100,50 到 300,50 的水平线", "cad_line",
     {"p1": [100, 50], "p2": [300, 50]}),
    ("在轴线层画一条竖直的线，从 0,0 到 0,200", "cad_line",
     {"p1": [0, 0], "p2": [0, 200], "layer": "轴线"}),
    # 明确点名图层时 layer 必须带上，1.5B 实测会整个丢掉这个可选参数
    ("在设备层画一条从 0,0 到 200,0 的线", "cad_line",
     {"p1": [0, 0], "p2": [200, 0], "layer": "设备"}),
    ("在标注层画条从 10,10 到 10,90 的竖线", "cad_line",
     {"p1": [10, 10], "p2": [10, 90], "layer": "标注"}),
    ("在配电箱层从 5,5 画到 105,5", "cad_line",
     {"p1": [5, 5], "p2": [105, 5], "layer": "配电箱"}),
    # 矩形的两个对角点是 p1 / p2
    ("画一个 200x100 的矩形，两个对角点是 0,0 和 200,100", "cad_rect",
     {"p1": [0, 0], "p2": [200, 100]}),
    ("矩形从 50,50 到 150,120", "cad_rect",
     {"p1": [50, 50], "p2": [150, 120]}),
    # 「要脚本/代码」也必须落到 tool_call：之前的模型被问这几句会答"别写成脚本，我帮你画"，
    # 然后编造 cad_entity、把 cad_line 参数编成 start/end。代码由工具层生成，模型只出工具调用。
    ("写一段 Python 脚本，在原点画一条到 100 的线", "cad_line",
     {"p1": [0, 0], "p2": [100, 0]}),
    ("给我一段 PyArx 代码，从 0,0 到 100,0 画条线", "cad_line",
     {"p1": [0, 0], "p2": [100, 0]}),
    ("用代码画一个圆心 60,40 半径 12 的圆", "cad_circle",
     {"center": [60, 40], "radius": 12}),
    ("写个脚本把 LIVE_TEST 层的线都删掉", "cad_erase",
     {"layer": "LIVE_TEST"}),
    ("生成一段代码统计图里有多少实体", "cad_stats", {}),
    ("帮我写程序在 0,150 写 8 号字「配电箱」", "cad_text",
     {"pos": [0, 150], "text": "配电箱", "height": 8}),
    ("画个 500x300 的矩形，左下角在原点", "cad_polyline",
     {"pts": [[0, 0], [500, 0], [500, 300], [0, 300]], "closed": True}),
    ("用多段线围一个三角形 (0,0)-(100,0)-(50,80)", "cad_polyline",
     {"pts": [[0, 0], [100, 0], [50, 80]], "closed": True}),
    ("在 0,150 位置写 8 号字「配电箱」", "cad_text",
     {"pos": [0, 150], "text": "配电箱", "height": 8}),
    ("图名标注放在 0,400，字高 15，内容「一层平面图」", "cad_text",
     {"pos": [0, 400], "text": "一层平面图", "height": 15}),
    ("在 50,200 写一段多行文字，字高 5，内容「注：未尽事宜按图集施工」", "cad_mtext",
     {"pos": [50, 200], "text": "注：未尽事宜按图集施工", "height": 5}),
    ("画一个圆心在 60,40 半径 12 的圆", "cad_circle",
     {"center": [60, 40], "radius": 12}),
    ("在设备层画三个同心圆定位孔，圆心 200,100，半径分别 5、10、15", "cad_circle",
     {"center": [200, 100], "radius": 10, "layer": "设备"}),
    ("画一段圆弧，圆心 200,100，半径 30，从 0 度到 90 度", "cad_arc",
     {"center": [200, 100], "radius": 30, "start_deg": 0, "end_deg": 90}),
    ("在填充层把 (300,0)-(380,0)-(380,60)-(300,60) 这个范围填充", "cad_hatch",
     {"pts": [[300, 0], [380, 0], [380, 60], [300, 60]], "layer": "填充"}),
    ("在原点插入 3 行 2 列的表格", "cad_table",
     {"pos": [0, 0], "rows": 3, "cols": 2}),
    ("新建一个图层叫 LIVE_TEST，颜色用青色", "cad_layer",
     {"name": "LIVE_TEST", "rgb": [0, 160, 255]}),
    ("一次把线、圆和文字都画到 LIVE_TEST 层", "cad_batch",
     {"ops": [
         {"tool": "line", "args": {"p1": [0, 0], "p2": [120, 0], "layer": "LIVE_TEST"}},
         {"tool": "circle", "args": {"center": [60, 40], "radius": 12, "layer": "LIVE_TEST"}},
         {"tool": "text", "args": {"pos": [0, 20], "text": "SAVE CYCLE", "height": 8,
                                   "layer": "LIVE_TEST"}}]}),
    ("把这批图元一次画完：两条线加一个半径 25 的圆", "cad_batch",
     {"ops": [
         {"tool": "line", "args": {"p1": [0, 0], "p2": [100, 0]}},
         {"tool": "line", "args": {"p1": [0, 50], "p2": [100, 50]}},
         {"tool": "circle", "args": {"center": [50, 25], "radius": 25}}]}),
    ("统计一下当前图里有多少实体、分别在哪层", "cad_stats", {}),
    ("查一下 LIVE_TEST 层上都有什么", "cad_query", {"layer": "LIVE_TEST"}),
    ("LIVE_TEST 层上画了哪些东西", "cad_query", {"layer": "LIVE_TEST"}),
    ("列出 0 层上的图元", "cad_query", {"layer": "0"}),
    ("看看这张图 LIVE_TEST 层里有几条线", "cad_query", {"layer": "LIVE_TEST"}),
    ("列出所有圆", "cad_query", {"cls": "AcDbCircle"}),
    ("图里有哪些多段线", "cad_query", {"cls": "AcDbPolyline"}),
    ("把所有文字找出来", "cad_query", {"cls": "AcDbText"}),
    ("把句柄 698 的那个图元删掉", "cad_erase", {"handles": ["698"]}),
    ("清空 0 层上的所有图元", "cad_erase", {"layer": "0"}),
    # cad_erase 的键名是 handles（数组），cad_offset 才是 handle（单个）。
    # 1.5B 实测把两者混成 cad_erase{handle:698}，schema 直接判无效，这里把形态压死。
    ("删掉句柄 732", "cad_erase", {"handles": ["732"]}),
    ("删除句柄 733 的图元", "cad_erase", {"handles": ["733"]}),
    ("把句柄 700 清掉", "cad_erase", {"handles": ["700"]}),
    ("去掉句柄 701", "cad_erase", {"handles": ["701"]}),
    ("删掉这两个图元，句柄是 732 和 735", "cad_erase", {"handles": ["732", "735"]}),
    ("清空整张图", "cad_erase", {"all": True, "confirm": "ERASE_ALL"}),
    ("把图里所有东西都删掉", "cad_erase", {"all": True, "confirm": "ERASE_ALL"}),
    ("把这个图另存到 D:\\work\\plan.dwg", "cad_save", {"path": r"D:\work\plan.dwg"}),
    ("保存当前图形", "cad_save", {}),
]

# ---------------------------------------------------------------- 纠错样本
# 结构：(错误调用, 错误返回要点, 用户提醒, 正确调用)

FIX = [
    ({"name": "cad_line", "args": {"p1": [0, 0]}},
     {"ok": False, "errors": [{"error": "缺少参数 p2"}]},
     "报错了，补上终点",
     {"name": "cad_line", "args": {"p1": [0, 0], "p2": [120, 0]}}),
    ({"name": "cad_text", "args": {"pos": [0, 150], "text": "配电箱"}},
     {"ok": True, "count": 1, "warnings": [{"setLayer": "未生效"}],
      "handles": ["701"]},
     "字画出来了但图层不对，重新放到配电箱层",
     {"name": "cad_text", "args": {"pos": [0, 150], "text": "配电箱", "height": 8,
                                  "layer": "配电箱"}}),
    ({"name": "cad_erase", "args": {"all": True}},
     {"ok": False, "errors": [{"error": "all=True 必须同时给 confirm=ERASE_ALL"}]},
     "确认清空整张图",
     {"name": "cad_erase", "args": {"all": True, "confirm": "ERASE_ALL"}}),
    ({"name": "cad_stats", "args": {"layer": "LIVE_TEST"}},
     {"ok": True, "total": 4, "by_layer": {"0": 2, "LIVE_TEST": 2}},
     "我只要 LIVE_TEST 层的，别把全图都列出来",
     {"name": "cad_query", "args": {"layer": "LIVE_TEST"}}),
]

# ---------------------------------------------------------------- 组装

def as_call(name, args):
    return {"role": "assistant", "content": None,
            "tool_calls": [{"type": "function",
                            "function": {"name": name,
                                         "arguments": json.dumps(args, ensure_ascii=False)}}]}


def build():
    rows = []
    for utt, name, args in LINE:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": utt},
            as_call(name, args),
        ], "meta": {"kind": "positive", "tool": name}})

    for bad, obs, hint, good in FIX:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": "帮我把这个图画一下"},
            as_call(bad["name"], bad["args"]),
            {"role": "tool", "content": json.dumps(obs, ensure_ascii=False)},
            {"role": "user", "content": hint},
            as_call(good["name"], good["args"]),
        ], "meta": {"kind": "repair", "tool": good["name"]}})

    random.seed(20261116)
    LAYERS = ["0", "轴线", "设备", "标注", "LIVE_TEST", "配电箱"]
    WORDS = ["配电箱", "开关柜", "电缆桥架", "接地", "照明", "插座", "图名", "说明"]
    HANDLES = ["732", "733", "734", "735", "698", "700", "701"]
    extra = []

    def add(utt, name, args):
        extra.append((utt, name, args))

    # 每个工具都要有随机变体：之前只随机了 line/circle，结果 246/304 全压在这两个上，
    # 其余 15 个工具几乎没有样本，模型只会画线画圆。
    for _ in range(6):
        x1, y1 = random.randint(0, 600), random.randint(0, 400)
        x2, y2 = random.randint(0, 600), random.randint(0, 400)
        layer = random.choice(LAYERS)
        add(f"在 {layer} 层画一条从 {x1},{y1} 到 {x2},{y2} 的线", "cad_line",
            {"p1": [x1, y1], "p2": [x2, y2], "layer": layer})

    for _ in range(6):
        cx, cy = random.randint(0, 600), random.randint(0, 400)
        r = random.choice([5, 8, 10, 12, 20, 25])
        layer = random.choice(LAYERS)
        add(f"在 {cx},{cy} 画半径 {r} 的圆，放到 {layer} 层", "cad_circle",
            {"center": [cx, cy], "radius": r, "layer": layer})

    for _ in range(6):
        x1, y1 = random.randint(0, 300), random.randint(0, 200)
        w, h = random.choice([(100, 50), (200, 80), (60, 60), (300, 120)])
        layer = random.choice(LAYERS)
        add(f"画一个 {w}x{h} 的矩形，左下角在 {x1},{y1}，图层 {layer}", "cad_rect",
            {"p1": [x1, y1], "p2": [x1 + w, y1 + h], "layer": layer})

    for _ in range(5):
        n = random.randint(3, 5)
        pts = [[random.randint(0, 400), random.randint(0, 300)] for _ in range(n)]
        add("用多段线连起这些点：" + "、".join(f"({p[0]},{p[1]})" for p in pts),
            "cad_polyline", {"pts": pts, "closed": random.choice([True, False])})

    for _ in range(5):
        x, y = random.randint(0, 400), random.randint(0, 300)
        hh = random.choice([3, 5, 8, 10, 15])
        w = random.choice(WORDS)
        add(f"在 {x},{y} 写 {hh} 号字「{w}」", "cad_text",
            {"pos": [x, y], "text": w, "height": hh})

    for _ in range(4):
        x, y = random.randint(0, 400), random.randint(0, 300)
        hh = random.choice([3, 5, 8])
        w = random.choice(WORDS) + "，" + random.choice(WORDS)
        add(f"在 {x},{y} 写一段多行文字，字高 {hh}，内容「{w}」", "cad_mtext",
            {"pos": [x, y], "text": w, "height": hh})

    for _ in range(5):
        cx, cy = random.randint(0, 400), random.randint(0, 300)
        r = random.choice([10, 20, 30, 45])
        a1 = random.choice([0, 30, 45, 90])
        a2 = random.choice([180, 270, 315, 360])
        add(f"画一段圆弧，圆心 {cx},{cy}，半径 {r}，从 {a1} 度到 {a2} 度", "cad_arc",
            {"center": [cx, cy], "radius": r, "start_deg": a1, "end_deg": a2})

    for _ in range(4):
        x, y = random.randint(0, 300), random.randint(0, 300)
        w, h = random.choice([(80, 40), (120, 60), (60, 60)])
        add(f"把 ({x},{y}) 到 ({x + w},{y + h}) 的范围填充", "cad_hatch",
            {"pts": [[x, y], [x + w, y], [x + w, y + h], [x, y + h]]})

    for _ in range(4):
        rows_n, cols_n = random.randint(2, 6), random.randint(2, 5)
        x, y = random.randint(0, 300), random.randint(0, 300)
        add(f"在 {x},{y} 插入 {rows_n} 行 {cols_n} 列的表格", "cad_table",
            {"pos": [x, y], "rows": rows_n, "cols": cols_n})

    for _ in range(4):
        nm = random.choice(["轴线", "设备", "标注", "配电箱", "TEST_LAYER"])
        rgb = random.choice([[0, 160, 255], [255, 0, 0], [0, 255, 0], [255, 255, 0]])
        add(f"新建一个图层叫 {nm}，颜色用 {rgb}", "cad_layer",
            {"name": nm, "rgb": rgb})

    # 读数据：查图元和统计原本各只有 1-2 条
    for _ in range(3):
        layer = random.choice(LAYERS)
        add(f"{layer} 层上都有什么", "cad_query", {"layer": layer})
    for _ in range(3):
        cls = random.choice(["AcDbLine", "AcDbCircle", "AcDbPolyline", "AcDbText"])
        add(f"把所有 {cls} 找出来", "cad_query", {"cls": cls})
    add("统计一下这张图的实体数", "cad_stats", {})
    add("看看全图有多少个图元、分别在哪层", "cad_stats", {})
    add("数一下当前图纸里一共有多少个图元", "cad_stats", {})

    # 用句柄操作
    for _ in range(3):
        hd = random.choice(HANDLES)
        d = random.choice([5, 10, 15, 20])
        add(f"把句柄 {hd} 的图元偏移 {d}", "cad_offset", {"handle": hd, "dist": d})
    for _ in range(5):
        hd = random.choice(HANDLES)
        add(f"删掉句柄 {hd} 的图元", "cad_erase", {"handles": [hd]})

    for utt, name, args in extra:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": utt},
            as_call(name, args),
        ], "meta": {"kind": "positive", "tool": name}})

    # ---------------------------------------------------------------- 链式样本
    # 「先查再动手」：模型必须学会用上一步返回的句柄，而不是自己编一个。
    for i in range(4):
        layer = random.choice(LAYERS)
        hd = HANDLES[i]
        obs = {"ok": True, "count": 2,
               "hits": [{"handle": hd, "class": "AcDbLine", "layer": layer},
                        {"handle": HANDLES[i + 1], "class": "AcDbLine", "layer": layer}]}
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"把 {layer} 层上第一条线删掉"},
            as_call("cad_query", {"layer": layer, "cls": "AcDbLine", "limit": 5}),
            {"role": "tool", "content": json.dumps(obs, ensure_ascii=False)},
            as_call("cad_erase", {"handles": [hd]}),
        ], "meta": {"kind": "chain", "tool": "cad_erase"}})

        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"把 {layer} 层上的线各偏移 10"},
            as_call("cad_query", {"layer": layer, "cls": "AcDbLine", "limit": 5}),
            {"role": "tool", "content": json.dumps(obs, ensure_ascii=False)},
            as_call("cad_offset", {"handle": hd, "dist": 10}),
        ], "meta": {"kind": "chain", "tool": "cad_offset"}})

    rows.append({"messages": [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": "看看这张图里有什么，然后把 0 层清掉"},
        as_call("cad_stats", {}),
        {"role": "tool", "content": json.dumps(
            {"ok": True, "total": 6, "by_layer": {"0": 3, "LIVE_TEST": 3},
             "by_class": {"AcDbLine": 4, "AcDbCircle": 2}}, ensure_ascii=False)},
        as_call("cad_erase", {"layer": "0"}),
    ], "meta": {"kind": "chain", "tool": "cad_erase"}})

    # ---------------------------------------------------------- 自动纠错（无用户提示）
    # 这是「会用 PyArxCore」最核心的一类：模型自己读 tool 返回的 ok=false，
    # 不等人说"报错了"就改。现有数据里这类是 0 条，全靠用户提示才纠正。
    AUTO_FIX = [
        # 缺参数
        ("在原点画一条到 120 的线",
         as_call("cad_line", {"p1": [0, 0]}),
         {"ok": False, "errors": [{"error": "缺少参数 p2"}]},
         as_call("cad_line", {"p1": [0, 0], "p2": [120, 0]})),
        # 句柄不存在 -> 先查
        ("把句柄 699 删掉",
         as_call("cad_erase", {"handles": ["699"]}),
         {"ok": False, "errors": [{"error": "图上无此句柄: 699"}],
          "count": 0, "handles": []},
         as_call("cad_query", {"limit": 10})),
        # 键名单数 -> 改复数
        ("删掉句柄 700",
         as_call("cad_erase", {"handle": "700"}),
         {"ok": False, "errors": [{"error": "未知参数：handle"}]},
         as_call("cad_erase", {"handles": ["700"]})),
        # 层不存在 -> 先建层再画
        ("在母线层画一条从 0,0 到 200,0 的线",
         as_call("cad_line", {"p1": [0, 0], "p2": [200, 0], "layer": "母线"}),
         {"ok": False, "errors": [{"error": "图层不存在: 母线"}]},
         as_call("cad_batch", {"ops": [
             {"tool": "layer", "args": {"name": "母线"}},
             {"tool": "line", "args": {"p1": [0, 0], "p2": [200, 0]}}]})),
        # 块名不存在 -> 不重试，改成画图元
        ("在这里插一个不存在的块",
         as_call("cad_block_ref", {"name": "不存在的块", "pos": [0, 0]}),
         {"ok": False, "errors": [{"error": "块定义不存在: 不存在的块"}]},
         as_call("cad_rect", {"p1": [0, 0], "p2": [10, 6]})),
        # 半径非法 -> 修真值
        ("画一个半径 -5 的圆",
         as_call("cad_circle", {"center": [0, 0], "radius": -5}),
         {"ok": False, "errors": [{"error": "radius 必须为正数"}]},
         as_call("cad_circle", {"center": [0, 0], "radius": 5})),
        # 清空全图漏 confirm -> 补
        ("把图清空",
         as_call("cad_erase", {"all": True}),
         {"ok": False, "errors": [{"error": "all=true must come with confirm=ERASE_ALL"}]},
         as_call("cad_erase", {"all": True, "confirm": "ERASE_ALL"})),
        # 批量里某一步失败 -> 整批回滚，改单步重试
        ("一次画三条线到 LIVE_TEST",
         as_call("cad_batch", {"ops": [
             {"tool": "line", "args": {"p1": [0, 0], "p2": [50, 0]}},
             {"tool": "line", "args": {"p1": [0, 10]}}]}),
         {"ok": False, "failed_index": 1,
          "errors": [{"error": "第 1 步缺少参数 p2"}], "rolled_back": True},
         as_call("cad_batch", {"ops": [
             {"tool": "line", "args": {"p1": [0, 0], "p2": [50, 0]}},
             {"tool": "line", "args": {"p1": [0, 10], "p2": [50, 10]}},
             {"tool": "line", "args": {"p1": [0, 20], "p2": [50, 20]}}],
             "layer": "LIVE_TEST"})),
    ]
    for utt, bad, obs, good in AUTO_FIX:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": utt},
            bad,
            {"role": "tool", "content": json.dumps(obs, ensure_ascii=False)},
            good,
        ], "meta": {"kind": "auto_fix", "tool": good["tool_calls"][0]["function"]["name"]}})

    # ---------------------------------------------------------- 块参照（之前 0 条）
    for nm, pos, sc in [("配电箱符号", [0, 0], 1.0), ("开关", [100, 0], 1.0),
                        ("插座", [200, 40], 1.0), ("灯", [0, 100], 0.5),
                        ("桥架", [300, 0], 2.0)]:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"在 {pos[0]},{pos[1]} 插入 {nm} 块，比例 {sc}"},
            as_call("cad_block_ref", {"name": nm, "pos": pos, "scale": sc}),
        ], "meta": {"kind": "positive", "tool": "cad_block_ref"}})
    # 旋转插入
    rows.append({"messages": [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": "把配电箱符号插到 50,50，旋转 90 度"},
        as_call("cad_block_ref", {"name": "配电箱符号", "pos": [50, 50], "rotation_deg": 90}),
    ], "meta": {"kind": "positive", "tool": "cad_block_ref"}})

    # ---------------------------------------------------------- 批量（之前 3 条）
    BATCH = [
        ("画一台配电柜：外框 300x600，里面三条竖线，标题写「柜A」",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [300, 600]}},
             {"tool": "line", "args": {"p1": [100, 0], "p2": [100, 600]}},
             {"tool": "line", "args": {"p1": [200, 0], "p2": [200, 600]}},
             {"tool": "text", "args": {"pos": [0, 620], "text": "柜A", "height": 20}}]}),
        ("一次画三条水平线，间距 50，长 200",
         {"ops": [
             {"tool": "line", "args": {"p1": [0, 0], "p2": [200, 0]}},
             {"tool": "line", "args": {"p1": [0, 50], "p2": [200, 50]}},
             {"tool": "line", "args": {"p1": [0, 100], "p2": [200, 100]}}]}),
        ("画一个 3 行 3 列的表格，标题行写「序号」「名称」「数量」",
         {"ops": [{"tool": "table", "args": {"pos": [0, 0], "rows": 3, "cols": 3,
                                             "texts": [["序号", "名称", "数量"]]}}]}),
        ("在 LIVE_TEST 层画三角形，三段线",
         {"ops": [
             {"tool": "line", "args": {"p1": [0, 0], "p2": [100, 0]}},
             {"tool": "line", "args": {"p1": [100, 0], "p2": [50, 80]}},
             {"tool": "line", "args": {"p1": [50, 80], "p2": [0, 0]}}],
          "layer": "LIVE_TEST"}),
        # ---- 复合指令（一个句子含多种图元）必须整体走 cad_batch
        # 实测事故：说「外框+三条竖线+标题」时模型只吐 cad_rect（ops[0]），
        # 前 4 条各只有一个样本，模式没立起来。这里换成同结构不同措辞/不同元素数。
        ("画一台配电柜：外框 300x600，里面三条竖线，标题写柜A",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [300, 600]}},
             {"tool": "line", "args": {"p1": [75, 0], "p2": [75, 600]}},
             {"tool": "line", "args": {"p1": [150, 0], "p2": [150, 600]}},
             {"tool": "line", "args": {"p1": [225, 0], "p2": [225, 600]}},
             {"tool": "text", "args": {"pos": [0, 620], "text": "柜A", "height": 20}}]}),
        ("画一台电箱：外框 400x800，中间两条横线，右上角写箱A",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [400, 800]}},
             {"tool": "line", "args": {"p1": [0, 300], "p2": [400, 300]}},
             {"tool": "line", "args": {"p1": [0, 600], "p2": [400, 600]}},
             {"tool": "text", "args": {"pos": [350, 820], "text": "箱A", "height": 20}}]}),
        ("画一台机柜：外框 600x1000，四等分竖线，标题写机柜1",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [600, 1000]}},
             {"tool": "line", "args": {"p1": [150, 0], "p2": [150, 1000]}},
             {"tool": "line", "args": {"p1": [300, 0], "p2": [300, 1000]}},
             {"tool": "line", "args": {"p1": [450, 0], "p2": [450, 1000]}},
             {"tool": "text", "args": {"pos": [0, 1020], "text": "机柜1", "height": 20}}]}),
        ("画一个窗户：外框 1200x600，中间一条竖线、两条横线",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1200, 600]}},
             {"tool": "line", "args": {"p1": [600, 0], "p2": [600, 600]}},
             {"tool": "line", "args": {"p1": [0, 200], "p2": [1200, 200]}},
             {"tool": "line", "args": {"p1": [0, 400], "p2": [1200, 400]}}]}),
        ("画一扇门：外框 900x2100，中间一条竖线，标注门A",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [900, 2100]}},
             {"tool": "line", "args": {"p1": [450, 0], "p2": [450, 2100]}},
             {"tool": "text", "args": {"pos": [0, 2130], "text": "门A", "height": 20}}]}),
        ("画一个配电盘：外框 500x700，三条竖线，标题盘1",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [500, 700]}},
             {"tool": "line", "args": {"p1": [125, 0], "p2": [125, 700]}},
             {"tool": "line", "args": {"p1": [250, 0], "p2": [250, 700]}},
             {"tool": "line", "args": {"p1": [375, 0], "p2": [375, 700]}},
             {"tool": "text", "args": {"pos": [0, 720], "text": "盘1", "height": 20}}]}),
        ("画一张桌子：桌面 1200x600，四条腿，标注桌1",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1200, 600]}},
             {"tool": "line", "args": {"p1": [50, 0], "p2": [50, -150]}},
             {"tool": "line", "args": {"p1": [1150, 0], "p2": [1150, -150]}},
             {"tool": "line", "args": {"p1": [50, 600], "p2": [50, 750]}},
             {"tool": "line", "args": {"p1": [1150, 600], "p2": [1150, 750]}},
             {"tool": "text", "args": {"pos": [0, 780], "text": "桌1", "height": 20}}]}),
        ("画一个抽屉柜：外框 500x800，四个抽屉用横线分隔，标题柜B",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [500, 800]}},
             {"tool": "line", "args": {"p1": [0, 200], "p2": [500, 200]}},
             {"tool": "line", "args": {"p1": [0, 400], "p2": [500, 400]}},
             {"tool": "line", "args": {"p1": [0, 600], "p2": [500, 600]}},
             {"tool": "text", "args": {"pos": [0, 820], "text": "柜B", "height": 20}}]}),
        ("画一个控制箱：外框 350x500，内部两条竖线一条横线，标题箱C",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [350, 500]}},
             {"tool": "line", "args": {"p1": [117, 0], "p2": [117, 500]}},
             {"tool": "line", "args": {"p1": [234, 0], "p2": [234, 500]}},
             {"tool": "line", "args": {"p1": [0, 250], "p2": [350, 250]}},
             {"tool": "text", "args": {"pos": [0, 520], "text": "箱C", "height": 20}}]}),
        # ---- 空间关系：等分交给工具层的 vdivide/hdivide 算坐标
        # 教训：让模型自己算「600 宽四等分 = 150/300/450」，换成没训过的 750 宽
        # 实测输出 x=375（退化成一刀两半），1000 三等分则抄了 900 的答案 300。
        # 改成模型只认「四等分」这个语义并填 vdivide=4，宽度除法由 cad_ops 做，
        # 任意尺寸（750/1875）都精确。样本里刻意不给 line，避免模型重复画线。
        ("画一台 600 宽 1000 高的机柜，竖线四等分，标题写机柜1",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [600, 1000], "vdivide": 4}},
             {"tool": "text", "args": {"pos": [0, 1020], "text": "机柜1", "height": 20}}]}),
        ("画一个 900 宽的柜体，三等分竖线，标题柜2",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [900, 600], "vdivide": 3}},
             {"tool": "text", "args": {"pos": [0, 620], "text": "柜2", "height": 20}}]}),
        ("画一个 1200 宽 500 高的盘面，五等分竖线",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1200, 500], "vdivide": 5}}]}),
        ("在 600x400 的框里居中放一个半径 100 的圆",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [600, 400]}},
             {"tool": "circle", "args": {"center": [300, 200], "radius": 100}}]}),
        ("画一个 400x400 的方框，左右各一条对称竖线，距边 60",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [400, 400]}},
             {"tool": "line", "args": {"p1": [60, 0], "p2": [60, 400]}},
             {"tool": "line", "args": {"p1": [340, 0], "p2": [340, 400]}}]}),
        ("画一台 800 宽 1000 高的机柜，竖线四等分",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [800, 1000], "vdivide": 4}}]}),
        ("画一台 1000 宽 800 高的柜子，竖线四等分，标题写柜A",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1000, 800], "vdivide": 4}},
             {"tool": "text", "args": {"pos": [0, 820], "text": "柜A", "height": 20}}]}),
        ("画一台 1200 宽 600 高的机柜，竖线四等分",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1200, 600], "vdivide": 4}}]}),
        ("画一个 600 宽 400 高的柜体，三等分竖线",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [600, 400], "vdivide": 3}}]}),
        ("画一个 1200 宽 500 高的盘面，三等分竖线，标题盘3",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1200, 500], "vdivide": 3}},
             {"tool": "text", "args": {"pos": [0, 520], "text": "盘3", "height": 20}}]}),
        ("画一个 1000 宽 400 高的配电盘，五等分竖线",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1000, 400], "vdivide": 5}}]}),
        ("画一个 1500 宽 600 高的盘面，五等分竖线",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [1500, 600], "vdivide": 5}}]}),
        ("画一个 800x400 的框，居中放一个半径 80 的圆",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [800, 400]}},
             {"tool": "circle", "args": {"center": [400, 200], "radius": 80}}]}),
        ("画一个 800 长 600 高的柜，横向三等分，第一格标题写主回路",
         {"ops": [
             {"tool": "rect", "args": {"p1": [0, 0], "p2": [800, 600], "hdivide": 3}},
             {"tool": "text", "args": {"pos": [10, 410], "text": "主回路", "height": 20}}]}),
    ]
    for utt, args in BATCH:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": utt},
            as_call("cad_batch", args),
        ], "meta": {"kind": "positive", "tool": "cad_batch"}})

    # ---------------------------------------------------------- 统计后决策
    rows.append({"messages": [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": "图里有几个实体，主要在哪层"},
        as_call("cad_stats", {}),
    ], "meta": {"kind": "positive", "tool": "cad_stats"}})
    for layer in ["0", "LIVE_TEST", "设备"]:
        rows.append({"messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"{layer} 层上有多少东西"},
            as_call("cad_query", {"layer": layer, "limit": 20}),
        ], "meta": {"kind": "positive", "tool": "cad_query"}})

    return rows


# ---------------------------------------------------------------- 现状前缀样本
# 前缀由 playground 后端拼在 user 消息最前面（不是 system），训练样本同口径：
# user content = 现状前缀 + 空格 + 用户指令。
# 这批行性质上仍是工具样本，单独落 sft_prefix.jsonl，由 build_distill_data.py 并入 sft_all。

P1 = "[现状 17:31 实体6 层:0=3,LIVE_TEST=3 图层:LIVE_TEST,0 src=com 12ms]"
P2 = "[现状 09:12 实体14 层:轴线=6,设备=5,0=3 图层:0,轴线,设备 src=com 28ms]"
P3 = "[现状 21:05 实体9 层:标注=4,配电箱=3,0=2 图层:0,标注,配电箱 src=headless 19ms]"
P4 = "[现状 10:47 实体21 层:0=12,TEST_LAYER=9 图层:0,TEST_LAYER src=com 35ms]"
P5 = "[现状 16:20 取图失败 src=none 340ms]"
P6 = "[现状 13:58 实体2 层:照明=2 图层:照明 src=headless 8ms]"


def _plain(prefix, utt, name, args, group):
    return {"messages": [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": prefix + " " + utt},
        as_call(name, args),
    ], "meta": {"kind": "prefix", "group": group, "tool": name}}


def _deny_handle(prefix, utt, query_args, obs, answer, group):
    return {"messages": [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": prefix + " " + utt},
        as_call("cad_query", query_args),
        {"role": "tool", "content": json.dumps(obs, ensure_ascii=False)},
        {"role": "assistant", "content": answer, "tool_calls": None},
    ], "meta": {"kind": "prefix", "group": group, "tool": "cad_query"}}


def build_prefix():
    rows = []

    # A. 现状里已有该层 → 不再 cad_layer，直接在该层画（≥3 种措辞）
    rows.append(_plain(P1, "在 LIVE_TEST 层画一条从 (0,0) 到 (120,0) 的线",
                        "cad_line", {"p1": [0, 0], "p2": [120, 0], "layer": "LIVE_TEST"},
                        "layer_exists"))
    rows.append(_plain(P1, "把圆画到 LIVE_TEST 上，圆心 60,40，半径 12",
                        "cad_circle", {"center": [60, 40], "radius": 12,
                                       "layer": "LIVE_TEST"}, "layer_exists"))
    rows.append(_plain(P2, "轴线层上补一条竖直线，从 0,0 到 0,200",
                        "cad_line", {"p1": [0, 0], "p2": [0, 200], "layer": "轴线"},
                        "layer_exists"))
    rows.append(_plain(P3, "在配电箱层 50,200 写 8 号字「进线柜」",
                        "cad_text", {"pos": [50, 200], "text": "进线柜", "height": 8,
                                     "layer": "配电箱"}, "layer_exists"))

    # B. 现状里没有的句柄 → 先 cad_query 确认，查不到就答复无此图元，不直接 erase
    rows.append(_deny_handle(
        P1, "把句柄 999 删掉", {},
        {"ok": True, "count": 3, "hits": [
            {"handle": "732", "class": "AcDbLine", "layer": "0"},
            {"handle": "733", "class": "AcDbCircle", "layer": "LIVE_TEST"},
            {"handle": "734", "class": "AcDbText", "layer": "0"}]},
        "图上没有句柄 999，未删除。现有句柄：732、733、734。", "handle_missing"))
    rows.append(_deny_handle(
        P2, "帮我删除句柄 88 那个图元", {"layer": "0"},
        {"ok": True, "count": 3, "hits": [
            {"handle": "700", "class": "AcDbLine", "layer": "0"},
            {"handle": "701", "class": "AcDbCircle", "layer": "轴线"}]},
        "查询后未发现句柄 88，图上不存在该图元，未做删除。", "handle_missing"))
    rows.append(_deny_handle(
        P3, "句柄 9001 能删掉吗，删了它", {},
        {"ok": True, "count": 4, "hits": [
            {"handle": "698", "class": "AcDbLine", "layer": "标注"},
            {"handle": "735", "class": "AcDbText", "layer": "配电箱"}]},
        "句柄 9001 不在当前图里，无法删除，没有执行 cad_erase。", "handle_missing"))
    rows.append(_deny_handle(
        P1, "找一下句柄 777 在不在，不在就别删", {},
        {"ok": True, "count": 2, "hits": [
            {"handle": "732", "class": "AcDbLine", "layer": "0"},
            {"handle": "733", "class": "AcDbCircle", "layer": "LIVE_TEST"}]},
        "未查到句柄 777，图上不存在，未执行删除。", "handle_missing"))

    # C. 统计类问题：现状数字一致时直接引现状，不再调 cad_stats
    rows.append(_plain(P1, "现在图里一共几个实体？", None, None, "stats_from_prefix"))
    rows[-1]["messages"][-1] = {"role": "assistant",
        "content": "共 6 个实体：0 层 3 个，LIVE_TEST 层 3 个。", "tool_calls": None}
    rows.append(_plain(P2, "当前图纸分几层、每层各有几个", None, None, "stats_from_prefix"))
    rows[-1]["messages"][-1] = {"role": "assistant",
        "content": "共 3 层、14 个实体：轴线 6 个、设备 5 个、0 层 3 个。",
        "tool_calls": None}
    rows.append(_plain(P3, "数一下标注层上有多少图元", None, None, "stats_from_prefix"))
    rows[-1]["messages"][-1] = {"role": "assistant",
        "content": "标注层 4 个图元（全图共 9 个：标注 4、配电箱 3、0 层 2）。",
        "tool_calls": None}
    rows.append(_plain(P5, "这张图现在有几个实体、各在哪层", None, None, "stats_from_prefix"))
    rows[-1]["messages"][-1] = {"role": "assistant",
        "content": "现状取图失败（src=none），我无法从现状判断实体数量和层分布，不能猜。"
                   "需要时用 cad_stats 做只读统计。", "tool_calls": None}

    # D. 普通绘图指令带前缀变体
    rows.append(_plain(P1, "画一条 100,50 到 300,50 的水平线",
                        "cad_line", {"p1": [100, 50], "p2": [300, 50]}, "plain"))
    rows.append(_plain(P2, "设备层画圆心 200,100、半径 15 的圆",
                        "cad_circle", {"center": [200, 100], "radius": 15,
                                       "layer": "设备"}, "plain"))
    rows.append(_plain(P3, "在 30,30 画一个 100x60 的矩形",
                        "cad_rect", {"p1": [30, 30], "p2": [130, 90], "layer": "0"},
                        "plain"))
    rows.append(_plain(P4, "用多段线闭合围起 (0,0)、(80,0)、(80,50)、(0,50)",
                        "cad_polyline", {"pts": [[0, 0], [80, 0], [80, 50], [0, 50]],
                                         "closed": True}, "plain"))
    rows.append(_plain(P1, "在 0,150 位置写 8 号字「配电箱」",
                        "cad_text", {"pos": [0, 150], "text": "配电箱", "height": 8},
                        "plain"))
    rows.append(_plain(P2, "把句柄 610 偏移 12",
                        "cad_offset", {"handle": "610", "dist": 12}, "plain"))
    rows.append(_plain(P4, "批量画两条线加一个半径 25 的圆到 0 层",
                        "cad_batch", {"ops": [
                            {"tool": "line", "args": {"p1": [0, 0], "p2": [100, 0], "layer": "0"}},
                            {"tool": "line", "args": {"p1": [0, 50], "p2": [100, 50], "layer": "0"}},
                            {"tool": "circle", "args": {"center": [50, 25], "radius": 25,
                                                        "layer": "0"}}]}, "plain"))
    rows.append(_plain(P3, "把当前图形另存到 D:\\work\\plan.dwg",
                        "cad_save", {"path": r"D:\work\plan.dwg"}, "plain"))
    rows.append(_plain(P1, "查一下 LIVE_TEST 层上都有什么",
                        "cad_query", {"layer": "LIVE_TEST"}, "plain"))
    # 建新层：必须针对现状里不存在的层名
    rows.append(_plain(P2, "新建一个标注图层，颜色用红色",
                        "cad_layer", {"name": "标注", "rgb": [255, 0, 0]}, "plain"))
    rows.append(_plain(P4, "加一个照明图层",
                        "cad_layer", {"name": "照明", "rgb": [0, 255, 0]}, "plain"))
    rows.append(_plain(P3, "在 50,200 写一段多行文字，字高 5，内容「注：按图集施工」",
                        "cad_mtext", {"pos": [50, 200], "text": "注：按图集施工", "height": 5},
                        "plain"))
    rows.append(_plain(P3, "画一段圆弧，圆心 200,100，半径 30，从 0 度到 90 度",
                        "cad_arc", {"center": [200, 100], "radius": 30,
                                    "start_deg": 0, "end_deg": 90}, "plain"))
    rows.append(_plain(P2, "把全图所有圆找出来",
                        "cad_query", {"cls": "AcDbCircle"}, "plain"))
    rows.append(_plain(P1, "把句柄 732 的图元删掉",
                        "cad_erase", {"handles": ["732"]}, "plain"))
    rows.append(_plain(P4, "填充 (300,0)-(380,0)-(380,60)-(300,60) 这个范围",
                        "cad_hatch", {"pts": [[300, 0], [380, 0], [380, 60], [300, 60]]},
                        "plain"))

    return rows


if __name__ == "__main__":
    import os
    rows = build()
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    print("rows=%d path=%s" % (len(rows), OUT))

    prefix_rows = build_prefix()
    with open(PREFIX_OUT, "w", encoding="utf-8") as f:
        for r in prefix_rows:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    print("prefix_rows=%d path=%s" % (len(prefix_rows), PREFIX_OUT))
