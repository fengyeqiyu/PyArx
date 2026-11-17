# -*- coding: utf-8 -*-
"""gen_shapes.py —— 形态样本：联动 / 多步 / 自纠。

三类结构，每类多组模板（30 组），配合参数随机化，样本唯一性由
「模板 × 坐标/半径/文字/图层」共同决定，不会像单工具样本那样很快撞满。

  multi_call  一句话触发多个工具调用（建层→画线→标注）
  multi_turn  分步：动手→看返回→再动手
  repair      故意给会失败的参数→读 errors→改正

只使用 18 个真实工具、真实块名（_Dot/DOOR/WINDOW），工具返回严格按
cad_ops.py 的 OUT 结构。跨批去重读已有文件的指纹。

用法：python gen_shapes.py [条数]       默认 3000
      从 gen_stream.py 调用 generate(n)
"""
import json
import os
import random
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gen_dataset_100k as G

OUT = os.path.join(HERE, "sft_shapes.jsonl")
LOG = os.path.join(HERE, "logs", "gen_shapes.log")
MAXCHARS = 700

SYS_SHORT = ("你是 CAD 绘图助手。只用 cad_* 工具操作图形，不写 PyArx 代码。"
             "坐标单位 mm，图层不存在会自动建。一次事务全有或全无；"
             "工具返回 ok/count/handles/warnings/errors，失败时要读 errors 自纠，不要重试同一调用。")

REAL_BLOCKS = ["_Dot", "DOOR", "WINDOW"]
BAD_BLOCKS = ["ELEC_BOX", "PANEL_1", "VALVE_2", "MCC_CABINET", "SWITCH_3"]


def _n(lo=0, hi=1000, step=10):
    return random.randrange(int(lo), int(hi) + 1, step)


def _pt(lo=0, hi=1000):
    return [_n(lo, hi), _n(lo, hi)]


def _layer():
    return random.choice(G.LAYERS)


def _call(name, args):
    return {"type": "function",
            "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}


def _asst(*calls):
    return {"role": "assistant", "content": None, "tool_calls": [_call(n, a) for n, a in calls]}


def _tool(obj):
    return {"role": "tool", "content": json.dumps(obj, ensure_ascii=False)}


def _h():
    return "%X" % random.randrange(0x100, 0xFFFF)


def _ok(count=1, handles=None, before=17):
    return {"ok": True, "count": count, "handles": handles if handles is not None else [_h()],
            "before": before, "after": before + count, "warnings": [], "errors": []}


def _fail(msg, tool, i=0, before=17):
    return {"ok": False, "count": 0, "handles": [], "steps": [{"i": k, "tool": tool, "ok": k != i}
                                                             for k in range(i + 1)],
            "before": before, "after": before, "warnings": [],
            "errors": [{"i": i, "tool": tool, "msg": msg}],
            "failed_index": i, "rolled_back": True}


def _chars(s):
    n = sum(len(m.get("content") or "") for m in s["messages"])
    for m in s["messages"]:
        for c in (m.get("tool_calls") or []):
            n += len(c["function"]["arguments"])
    return n


def _row(msgs, kind, tool=None):
    meta = {"kind": kind}
    if tool:
        meta["tool"] = tool
    return {"messages": msgs, "meta": meta}


# ==================== A. 联动：一次多个调用（12 组）====================
def mc_layer_line_text():
    layer, p1, p2, pos = _layer(), _pt(), _pt(), _pt()
    t = random.choice(G.TEXTS)
    return ("新建 %s 层，在它上面从 (%d,%d) 到 (%d,%d) 画一条线，再在 (%d,%d) 写文字“%s”"
            % (layer, p1[0], p1[1], p2[0], p2[1], pos[0], pos[1], t),
            [("cad_layer", {"name": layer, "color": random.choice([1, 2, 3, 4, 5, 6, 7])}),
             ("cad_line", {"p1": p1, "p2": p2, "layer": layer}),
             ("cad_text", {"pos": pos, "text": t, "height": random.choice([2.5, 3.5, 5]),
                           "layer": layer})], "cad_layer+cad_line+cad_text")


def mc_rect_vdivide_text():
    p1, w, h = _pt(100, 300), _n(200, 400), _n(200, 400)
    p2 = [p1[0] + w, p1[1] + h]
    nv = random.choice([2, 3, 4, 5])
    return ("画一个 %d 长 %d 高的柜，横向 %d 等分，第一格标题写主回路" % (w, h, nv),
            [("cad_rect", {"p1": p1, "p2": p2, "vdivide": nv}),
             ("cad_text", {"pos": [p1[0] + w // (2 * nv), p1[1] + h // 2], "text": "主回路"})],
            "cad_rect+cad_text")


def mc_rect_grid_text():
    p1 = _pt(100, 300)
    w, h = _n(200, 400), _n(200, 400)
    p2 = [p1[0] + w, p1[1] + h]
    nv, nh = random.choice([2, 3]), random.choice([2, 3])
    return ("画 %d×%d 的网格框（宽 %d 高 %d），横向 %d 等分、纵向 %d 等分"
            % (nv, nh, w, h, nv, nh),
            [("cad_rect", {"p1": p1, "p2": p2, "vdivide": nv, "hdivide": nh})],
            "cad_rect")


def mc_poly_hatch():
    pts = [_pt(100, 600) for _ in range(random.randrange(3, 6))]
    return ("画一个 %d 边形的封闭轮廓，内部 SOLID 填充" % len(pts),
            [("cad_polyline", {"points": pts, "layer": _layer()}),
             ("cad_hatch", {"points": pts, "pattern": "SOLID", "layer": _layer()})],
            "cad_polyline+cad_hatch")


def mc_circle_cross():
    c, r = _pt(200, 500), _n(20, 120)
    layer = _layer()
    return ("在 (%d,%d) 画半径 %d 的圆，再沿圆心画一条水平引线" % (c[0], c[1], r),
            [("cad_circle", {"center": c, "radius": r, "layer": layer}),
             ("cad_line", {"p1": [c[0] - r - 10, c[1]], "p2": [c[0] + r + 10, c[1]], "layer": layer})],
            "cad_circle+cad_line")


def mc_three_phase():
    c, r = _pt(200, 500), _n(30, 90)
    layer = _layer()
    calls = [("cad_circle", {"center": c, "radius": r, "layer": layer})]
    for i in range(3):
        ang = i * 120
        calls.append(("cad_text", {"pos": [c[0] + r + 5, c[1] + i * 30], "text": "L%d" % (i + 1),
                                   "layer": layer}))
    return ("画一个三相符号：半径 %d 的圆加 L1/L2/L3 三个标注" % r, calls,
            "cad_circle+cad_text")


def mc_block_text():
    p, b, t = _pt(), random.choice(REAL_BLOCKS), random.choice(G.TEXTS)
    layer = _layer()
    return ("在 (%d,%d) 插入 %s 块，旁边写“%s”，图层 %s" % (p[0], p[1], b, t, layer),
            [("cad_block_ref", {"pos": p, "block": b, "layer": layer}),
             ("cad_text", {"pos": [p[0] + 15, p[1] + 15], "text": t, "layer": layer})],
            "cad_block_ref+cad_text")


def mc_table_header():
    p = _pt(100, 400)
    rows, cols = random.randrange(2, 6), random.randrange(2, 5)
    return ("在 (%d,%d) 建 %d 行 %d 列的表，表头写“序号”" % (p[0], p[1], rows, cols),
            [("cad_table", {"pos": p, "rows": rows, "cols": cols}),
             ("cad_text", {"pos": [p[0] + 5, p[1] - 5], "text": "序号", "height": 2.5})],
            "cad_table+cad_text")


def mc_arc_text():
    c, r = _pt(200, 500), _n(30, 150)
    a1 = _n(0, 300, 5)
    a2 = a1 + _n(30, 90, 5)
    layer = _layer()
    return ("在 (%d,%d) 画半径 %d 的弧，从 %d° 到 %d°，中点位置标“R%d”"
            % (c[0], c[1], r, a1, a2, r),
            [("cad_arc", {"center": c, "radius": r, "start_angle": a1, "end_angle": a2,
                          "layer": layer}),
             ("cad_text", {"pos": [c[0] + r, c[1] + r + 5], "text": "R%d" % r, "layer": layer})],
            "cad_arc+cad_text")


def mc_polyline_vertices():
    pts = [_pt(100, 700) for _ in range(random.randrange(3, 6))]
    layer = _layer()
    calls = [("cad_polyline", {"points": pts, "layer": layer})]
    for i, p in enumerate(pts[:2]):
        calls.append(("cad_text", {"pos": [p[0] + 5, p[1] + 5], "text": "P%d" % (i + 1),
                                   "layer": layer}))
    return ("画 %d 点折线，前两个节点标 P1、P2" % len(pts), calls, "cad_polyline+cad_text")


def mc_nested_rects():
    p1 = _pt(100, 250)
    w, h = _n(300, 500), _n(250, 450)
    p2 = [p1[0] + w, p1[1] + h]
    inset = 20
    p3, p4 = [p1[0] + inset, p1[1] + inset], [p2[0] - inset, p2[1] - inset]
    layer = _layer()
    return ("画外框 (%d,%d)-(%d,%d)，再内缩 %d 画内框，都放 %s 层"
            % (p1[0], p1[1], p2[0], p2[1], inset, layer),
            [("cad_rect", {"p1": p1, "p2": p2, "layer": layer}),
             ("cad_rect", {"p1": p3, "p2": p4, "layer": layer})], "cad_rect+cad_rect")


def mc_line_hatch_band():
    p1 = _pt(100, 600)
    w, h = _n(200, 400), _n(20, 60)
    p2 = [p1[0] + w, p1[1] + h]
    p3, p4 = [p1[0], p1[1] + h], [p2[0], p2[1]]
    return ("画一条 %d 宽的带状剖面：上边线、下边线，中间填充" % w,
            [("cad_line", {"p1": p1, "p2": p2}),
             ("cad_line", {"p1": p3, "p2": p4}),
             ("cad_hatch", {"points": [p1, p2, p4, p3], "pattern": "SOLID"})],
            "cad_line+cad_line+cad_hatch")


MULTI_CALL_TPLS = [mc_layer_line_text, mc_rect_vdivide_text, mc_rect_grid_text, mc_poly_hatch,
                   mc_circle_cross, mc_three_phase, mc_block_text, mc_table_header,
                   mc_arc_text, mc_polyline_vertices, mc_nested_rects, mc_line_hatch_band]


# ==================== B. 多步（8 组）====================
def tt_layer_then_shape():
    layer, p = _layer(), _pt()
    if random.random() < 0.5:
        r = _n(20, 150)
        second = ("cad_circle", {"center": p, "radius": r, "layer": layer},
                  "在 (%d,%d) 画半径 %d 的圆" % (p[0], p[1], r), "cad_circle")
    else:
        p2 = _pt()
        second = ("cad_line", {"p1": p, "p2": p2, "layer": layer},
                  "从 (%d,%d) 到 (%d,%d) 画一条线" % (p[0], p[1], p2[0], p2[1]), "cad_line")
    t = random.choice(G.TEXTS)
    user = "先新建 %s 层，再%s，最后在 (%d,%d) 标注“%s”" % (layer, second[2], p[0], p[1] - 30, t)
    steps = [(_asst(("cad_layer", {"name": layer,
                                   "color": random.choice([1, 2, 3, 4, 5, 6, 7])})),
              _tool({"ok": True, "count": 0, "handles": [], "exists": random.choice([True, False]),
                     "before": 17, "after": 17, "warnings": [], "errors": []})),
             (_asst((second[0], second[1])), _tool(_ok()))]
    final = _asst(("cad_text", {"pos": [p[0], p[1] - 30], "text": t, "layer": layer}))
    return user, steps, final, "cad_layer+" + second[3] + "+cad_text"


def tt_query_then_draw():
    layer = _layer()
    p1, p2 = _pt(), _pt()
    q = _ok(0, [], 17)
    q["items"] = []
    user = "先查一下 %s 层现在有什么，再在这层补一条线 (%d,%d)→(%d,%d)" % (
        layer, p1[0], p1[1], p2[0], p2[1])
    steps = [(_asst(("cad_query", {"layer": layer})), _tool(q))]
    final = _asst(("cad_line", {"p1": p1, "p2": p2, "layer": layer}))
    return user, steps, final, "cad_query+cad_line"


def tt_stats_then_update():
    p = _pt()
    t = random.choice(G.TEXTS)
    s = _ok(0, [], 17)
    s.update({"total": 17, "byClass": {"AcDbLine": 8, "AcDbCircle": 5}, "file": "model.dwg"})
    user = "先统计当前图纸，然后画一个圆 (%d,%d) r%d，并在旁边写“%s”" % (p[0], p[1], 40, t)
    steps = [(_asst(("cad_stats", {})), _tool(s))]
    final = _asst(("cad_circle", {"center": p, "radius": 40, "layer": _layer()}),
                  ("cad_text", {"pos": [p[0] + 45, p[1]], "text": t}))
    return user, steps, final, "cad_stats+cad_circle+cad_text"


def tt_query_then_erase():
    layer = _layer()
    hs = [_h(), _h()]
    q = _ok(0, [], 17)
    q["items"] = [{"handle": hs[0], "class": "AcDbLine", "layer": layer},
                  {"handle": hs[1], "class": "AcDbText", "layer": layer}]
    user = "查一下 %s 层的图元，然后把查到的都删掉" % layer
    steps = [(_asst(("cad_query", {"layer": layer})), _tool(q))]
    final = _asst(("cad_erase", {"handles": hs}))
    return user, steps, final, "cad_query+cad_erase"


def tt_draw_then_offset():
    p1, p2 = _pt(), _pt()
    d = random.choice([10, 20, 50])
    user = "从 (%d,%d) 到 (%d,%d) 画线，然后向右偏移 %d" % (p1[0], p1[1], p2[0], p2[1], d)
    steps = [(_asst(("cad_line", {"p1": p1, "p2": p2})), _tool(_ok()))]
    final = _asst(("cad_offset", {"handle": _h(), "dist": d}))
    return user, steps, final, "cad_line+cad_offset"


def tt_rect_then_vdivide():
    p1, w, h = _pt(100, 300), _n(200, 400), _n(200, 400)
    p2 = [p1[0] + w, p1[1] + h]
    user = "先画 %d×%d 的矩形 (%d,%d) 起，再看一眼确定没问题，最后横向三等分" % (
        w, h, p1[0], p1[1])
    steps = [(_asst(("cad_rect", {"p1": p1, "p2": p2})), _tool(_ok(1, [_h()])),
              ),
             (_asst(("cad_query", {"layer": "0"})), _tool(_ok(0, [])))]
    final = _asst(("cad_rect", {"p1": p1, "p2": p2, "vdivide": 3}))
    return user, steps, final, "cad_rect+cad_query+cad_rect"


def tt_layer_block_text():
    layer, p, b, t = _layer(), _pt(), random.choice(REAL_BLOCKS), random.choice(G.TEXTS)
    user = "建 %s 层，插 %s 块到 (%d,%d)，并在块上方标“%s”" % (layer, b, p[0], p[1], t)
    steps = [(_asst(("cad_layer", {"name": layer})),
              _tool({"ok": True, "count": 0, "handles": [], "exists": False,
                     "before": 17, "after": 17, "warnings": [], "errors": []}),
              ),
             (_asst(("cad_block_ref", {"pos": p, "block": b, "layer": layer})), _tool(_ok()))]
    final = _asst(("cad_text", {"pos": [p[0], p[1] + 20], "text": t, "layer": layer}))
    return user, steps, final, "cad_layer+cad_block_ref+cad_text"


def tt_poly_then_hatch_then_text():
    pts = [_pt(100, 600) for _ in range(random.randrange(3, 5))]
    t = random.choice(G.TEXTS)
    user = "按 %d 个点画封闭折线，再填充，最后在第一个点旁标“%s”" % (len(pts), t)
    steps = [(_asst(("cad_polyline", {"points": pts}),), _tool(_ok()))]
    final = _asst(("cad_hatch", {"points": pts, "pattern": "SOLID"}),
                  ("cad_text", {"pos": [pts[0][0] + 8, pts[0][1] + 8], "text": t}))
    return user, steps, final, "cad_polyline+cad_hatch+cad_text"


MULTI_TURN_TPLS = [tt_layer_then_shape, tt_query_then_draw, tt_stats_then_update,
                   tt_query_then_erase, tt_draw_then_offset, tt_rect_then_vdivide,
                   tt_layer_block_text, tt_poly_then_hatch_then_text]


# ==================== C. 自纠（10 组）====================
def rp_bad_block():
    bad, good, p, layer = random.choice(BAD_BLOCKS), random.choice(REAL_BLOCKS), _pt(), _layer()
    user = "在 (%d,%d) 插入 %s 块，图层 %s" % (p[0], p[1], bad, layer)
    bad_calls = [("cad_block_ref", {"pos": p, "block": bad, "layer": layer})]
    bad_res = _fail("块定义不存在：%s（本图里没有这个块，需先在模板图定义）" % bad, "block_ref")
    good_calls = [("cad_block_ref", {"pos": p, "block": good, "layer": layer})]
    return user, bad_calls, bad_res, good_calls, "cad_block_ref"


def rp_bad_handle_offset():
    p1, p2, d = _pt(), _pt(), random.choice([10, 20, 50])
    bad_h = str(_n(1000, 99999))
    user = "用句柄 %s 把图元向右偏移 %d" % (bad_h, d)
    bad_calls = [("cad_offset", {"handle": bad_h, "dist": d})]
    bad_res = _fail("句柄不存在：%s（先用 cad_query 查真实句柄）" % bad_h, "offset")
    good_calls = [("cad_line", {"p1": p1, "p2": p2}),
                  ("cad_offset", {"handle": _h(), "dist": d})]
    return user, bad_calls, bad_res, good_calls, "cad_offset"


def rp_bad_layer_name():
    good_layer = _layer()
    typo = good_layer + random.choice(["L", "1", "_X"])
    p = _pt()
    user = "把圆画在 %s 层，圆心 (%d,%d) 半径 %d" % (typo, p[0], p[1], 50)
    bad_calls = [("cad_circle", {"center": p, "radius": 50, "layer": typo})]
    bad_res = _ok(1, [_h()])
    # 实机行为：setLayer 恒返回 False，实体落 0 层，并在 warnings 里显式记录
    bad_res["warnings"] = [{"i": 0, "setLayer": "False"}]
    good_calls = [("cad_layer", {"name": good_layer}),
                  ("cad_circle", {"center": p, "radius": 50, "layer": good_layer})]
    return user, bad_calls, bad_res, good_calls, "cad_layer+cad_circle"


def rp_missing_arg():
    p = _pt()
    user = "从 (%d,%d) 画一条线" % (p[0], p[1])
    bad_calls = [("cad_line", {"p1": p})]
    bad_res = _fail("缺参数：p2（cad_line 需要 p1 和 p2 两个点）", "line")
    good_calls = [("cad_line", {"p1": p, "p2": _pt()})]
    return user, bad_calls, bad_res, good_calls, "cad_line"


def rp_string_int():
    p1, w, h = _pt(100, 300), _n(200, 400), _n(200, 400)
    p2 = [p1[0] + w, p1[1] + h]
    user = "画矩形 (%d,%d)-(%d,%d)，横向四等分" % (p1[0], p1[1], p2[0], p2[1])
    bad_calls = [("cad_rect", {"p1": p1, "p2": p2, "vdivide": "4"})]
    bad_res = _fail("参数不合法：vdivide 需为整数，收到字符串 \"4\"", "rect")
    good_calls = [("cad_rect", {"p1": p1, "p2": p2, "vdivide": 4})]
    return user, bad_calls, bad_res, good_calls, "cad_rect"


def rp_bad_table_size():
    p = _pt(100, 400)
    user = "在 (%d,%d) 建一个 100 行 100 列的表" % (p[0], p[1])
    bad_calls = [("cad_table", {"pos": p, "rows": 100, "cols": 100})]
    bad_res = _fail("参数不合法：rows/cols 超出上限（建议 ≤10）", "table")
    good_calls = [("cad_table", {"pos": p, "rows": 5, "cols": 3})]
    return user, bad_calls, bad_res, good_calls, "cad_table"


def rp_bad_batch():
    p, p2, layer = _pt(), _pt(), _layer()
    user = "批量执行：先画一条线，再用 %d 号句柄做偏移基准" % 9999
    bad_calls = [("cad_batch", {"calls": [
        {"tool": "cad_line", "args": {"p1": p, "p2": p2, "layer": layer}},
        {"tool": "cad_offset", "args": {"handle": "9999", "dist": 10}}]})]
    bad_res = {"ok": False, "count": 0, "handles": [],
               "steps": [{"i": 0, "tool": "line", "ok": True},
                         {"i": 1, "tool": "offset", "ok": False}],
               "before": 17, "after": 17, "warnings": [],
               "errors": [{"i": 1, "tool": "offset",
                           "msg": "句柄不存在：9999（先 cad_query 查真实句柄）"}],
               "failed_index": 1, "rolled_back": True}
    good_calls = [("cad_query", {"layer": layer})]
    return user, bad_calls, bad_res, good_calls, "cad_batch"


def rp_zero_radius():
    p = _pt()
    user = "在 (%d,%d) 画半径 0 的圆" % (p[0], p[1])
    bad_calls = [("cad_circle", {"center": p, "radius": 0})]
    bad_res = _fail("参数不合法：radius 必须为正数", "circle")
    good_calls = [("cad_circle", {"center": p, "radius": 40})]
    return user, bad_calls, bad_res, good_calls, "cad_circle"


def rp_bad_text_height():
    p = _pt()
    t = random.choice(G.TEXTS)
    user = "在 (%d,%d) 写“%s”，字高 0" % (p[0], p[1], t)
    bad_calls = [("cad_text", {"pos": p, "text": t, "height": 0})]
    bad_res = _fail("参数不合法：height 必须为正数", "text")
    good_calls = [("cad_text", {"pos": p, "text": t, "height": 2.5})]
    return user, bad_calls, bad_res, good_calls, "cad_text"


def rp_erase_then_requery():
    layer = _layer()
    stale = _h()
    user = "把 %s 层的 %s 号图元删掉" % (layer, stale)
    bad_calls = [("cad_erase", {"handles": [stale]})]
    bad_res = _fail("没有匹配的图元：%s（句柄可能已过期）" % stale, "erase")
    good_calls = [("cad_query", {"layer": layer}), ("cad_erase", {"handles": [_h()]})]
    return user, bad_calls, bad_res, good_calls, "cad_erase"


REPAIR_TPLS = [rp_bad_block, rp_bad_handle_offset, rp_bad_layer_name, rp_missing_arg,
               rp_string_int, rp_bad_table_size, rp_bad_batch, rp_zero_radius,
               rp_bad_text_height, rp_erase_then_requery]


# ==================== 组装与落盘 ====================
def _build(kind):
    if kind == "multi_call":
        user, calls, label = random.choice(MULTI_CALL_TPLS)()
        return _row([{"role": "system", "content": SYS_SHORT},
                     {"role": "user", "content": user}, _asst(*calls)], kind, label)
    if kind == "multi_turn":
        user, steps, final, label = random.choice(MULTI_TURN_TPLS)()
        msgs = [{"role": "system", "content": SYS_SHORT}, {"role": "user", "content": user}]
        for a, t in steps:
            msgs += [a, t]
        msgs.append(final)
        return _row(msgs, kind, label)
    user, bad, bad_res, good, label = random.choice(REPAIR_TPLS)()
    return _row([{"role": "system", "content": SYS_SHORT},
                 {"role": "user", "content": user},
                 _asst(*bad), _tool(bad_res), _asst(*good)], kind, label)


def _fingerprint(r):
    return re.sub(r"\s+", "", json.dumps(r["messages"], ensure_ascii=False, sort_keys=True))


def _existing():
    fps = set()
    if not os.path.exists(OUT):
        return fps
    size = os.path.getsize(OUT)
    with open(OUT, "rb") as f:
        f.seek(max(0, size - 12 * 1024 * 1024))
        chunk = f.read().decode("utf-8", "ignore")
    for line in chunk.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            fps.add(_fingerprint(json.loads(line)))
        except Exception:
            pass
    return fps


def generate(n=3000, kinds=None):
    """生成并追加 n 条形态样本，返回实际入库条数。"""
    kinds = kinds or ["multi_call", "multi_turn", "repair"]
    plan = {"multi_call": 0.4, "multi_turn": 0.33, "repair": 0.27}
    hist = _existing()
    fresh = []
    long_n = dup = 0
    for kind in kinds:
        want = int(n * plan[kind]) if len(kinds) > 1 else n
        made = 0
        guard = 0
        while made < want and guard < want * 30:
            guard += 1
            r = _build(kind)
            if _chars(r) > MAXCHARS:
                long_n += 1
                continue
            fp = _fingerprint(r)
            if fp in hist:
                dup += 1
                continue
            hist.add(fp)
            fresh.append(r)
            made += 1
    with open(OUT, "a", encoding="utf-8") as f:
        for r in fresh:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    kinds_cnt = {}
    for r in fresh:
        kinds_cnt[r["meta"]["kind"]] = kinds_cnt.get(r["meta"]["kind"], 0) + 1
    msg = ("[shapes] 目标=%d 入库=%d 重复剔除=%d 超长剔除=%d | kind=%s"
           % (n, len(fresh), dup, long_n, json.dumps(kinds_cnt, ensure_ascii=False)))
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("%s %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S"), msg))
    print(msg)
    return len(fresh)


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
    t0 = time.time()
    got = generate(n)
    try:
        with open(OUT, encoding="utf-8") as f:
            total = sum(1 for _ in f)
    except Exception:
        total = -1
    print("文件总行=%d 耗时=%.1fs" % (total, time.time() - t0))
    print("%s [result] %s" % (time.strftime("%Y-%m-%d %H:%M:%S"),
                              "PASS" if got >= int(n * 0.9) else "LOW"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
