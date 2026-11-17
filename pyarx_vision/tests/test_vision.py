# -*- utf-8 -*-
"""pyarx_vision 单元测试。既可用 tests/run_tests.py 跑，也可被 pytest 直接收集。"""
import os
import sys

sys.path.insert(0, r"G:\PyArx")
import pyarx_vision as V
from pyarx_vision import geometry as G, detect, semantics, schema


# ---------------------------------------------------------------- 几何原语
def test_rect_of_accepts_axis_aligned():
    r = G.rect_of([(0, 0), (100, 0), (100, 50), (0, 50)], tol=1.0)
    assert r == (0, 0, 100, 50)


def test_rect_of_rejects_skewed():
    # 一个顶点跑到内部 → 不是轴对齐矩形
    assert G.rect_of([(0, 0), (100, 0), (50, 20), (0, 50)], tol=1.0) is None


def test_rect_of_rejects_triangle():
    assert G.rect_of([(0, 0), (100, 0), (0, 50)], tol=1.0) is None


def test_bucket_groups_close_sizes():
    assert G.bucket(1598, 50) == 1600
    assert G.bucket(1603, 50) == 1600
    assert G.bucket(1701, 50) == 1700


def test_median_even_odd():
    assert G.median([1, 2, 3]) == 2
    assert G.median([1, 2, 3, 4]) == 2.5


def test_split_by_gap_keeps_close_intervals():
    segs = G.split_by_gap([(0, 100), (120, 300)], gap=50)
    assert len(segs) == 1 and segs[0] == (0, 300)


def test_split_by_gap_breaks_on_large_gap():
    segs = G.split_by_gap([(0, 100), (250, 300)], gap=50)
    assert len(segs) == 2


def test_equidistant_detects_regular_series():
    ok, step = G.equidistant([0, 100, 200, 300], max_dev=0.15)
    assert ok and abs(step - 100) < 1e-6


def test_equidistant_rejects_irregular():
    ok, _ = G.equidistant([0, 100, 450, 470], max_dev=0.15)
    assert not ok


# ---------------------------------------------------------------- 结构发现
def _rect(x, y, w, h):
    return {"r": (x, y, x + w, y + h), "layer": "X"}


def test_find_units_detects_row():
    # 一行 10 个 1000×2000 矩形，间距 2500 → 应识别为一个阵列
    rects = [_rect(i * 2500, 0, 1000, 2000) for i in range(10)]
    units = detect.find_units(rects, min_count=8)
    assert units and units[0]["count"] == 10
    assert abs(units[0]["step"] - 2500) < 1e-6


def test_find_units_ignores_scattered_sizes():
    rects = [_rect(i * 3000, 0, 500 + i * 10, 900) for i in range(8)]
    assert detect.find_units(rects, min_count=8) == []


def test_find_buses_splits_same_row_by_gap():
    es = schema.empty()
    # 同一条 y 上两段，空档 3000 → 必须拆成两条
    schema.add_line(es, (0, 1000), (40000, 1000))
    schema.add_line(es, (43000, 1000), (70000, 1000))
    schema.add_line(es, (0, 900), (40000, 900))
    schema.add_line(es, (43000, 900), (70000, 900))
    buses = detect.find_buses(es, frame_w=70000, min_len_ratio=0.15, gap=2000)
    assert len(buses) == 2
    assert buses[0]["x0"] == 0 and buses[1]["x1"] == 70000


def test_assign_bus_uses_x_when_same_y():
    buses = [{"x0": 0, "x1": 40000, "top": 1000},
             {"x0": 43000, "x1": 70000, "top": 1000}]
    assert detect.assign_bus((20000, 2000), buses) == 0
    assert detect.assign_bus((50000, 2000), buses) == 1


# ---------------------------------------------------------------- 语义
def test_classify_variants():
    assert semantics.classify("400A") == "cap"
    assert semantics.classify("800/5") == "ct"
    assert semantics.classify("YJV-0.6/1KV 3x70+2x35") == "cable"
    assert semantics.classify("多功能电表(带通讯)") == "meter"
    assert semantics.classify("x1备用") == "spare"
    assert semantics.classify("1#铝合金母线槽 5000A") == "bus"
    assert semantics.classify("中试车间变电站3") == "station"
    assert semantics.classify("Kx=0.57 Ijs=4379.6A") == "load"
    assert semantics.classify("外壳防护等级") == "other"


def test_assign_is_one_to_one():
    # 两条容量文字落在同一列区间，必须一回路一条，不许共用
    loops = [{"x": 0, "y": 0}, {"x": 900, "y": 0}]
    pools = {"cap": [{"pos": (0, 3000), "text": "400A", "height": 100, "rotation": 0, "layer": ""},
                     {"pos": (900, 3000), "text": "630A", "height": 100, "rotation": 0, "layer": ""}]}
    stats = semantics.assign(loops, pools)
    assert loops[0]["cap"] == "400"
    assert loops[1]["cap"] == "630"
    assert stats["cap"] == (2, 2)


def test_assign_drops_duplicate_text():
    # 只有一条文字、两个回路候选 → 只能给最近的一个
    loops = [{"x": 0, "y": 0}, {"x": 500, "y": 0}]
    pools = {"ct": [{"pos": (100, 300), "text": "500/5", "height": 100,
                     "rotation": 0, "layer": ""}]}
    semantics.assign(loops, pools)
    assert loops[0]["ct"] == "500/5" and loops[1]["ct"] == ""


# ---------------------------------------------------------------- 端到端
def _build_mini_system():
    """合成一张迷你干线图：2 条母线（同排、x 分开）+ 各 6 回 + 容量/CT/备用"""
    es = schema.empty("mini")
    es["frame"] = (0, -10000, 70000, 6000)
    # 母线槽（上下边线，各段长 30000/32000，空档 4000）
    for y in (1000, 900):
        schema.add_line(es, (0, y), (30000, y), "BUS")
        schema.add_line(es, (34000, y), (66000, y), "BUS")
    # 左段 6 个断路器（下方，y=3000）
    for i in range(6):
        x = 2000 + i * 5000
        schema.add_pline(es, [(x, 3000), (x + 1600, 3000), (x + 1600, 5000),
                              (x, 5000), (x, 3000)], True, "DQ")
        schema.add_text(es, (x + 800, 2400), "%dA" % (400 + i * 100), 100)
        schema.add_text(es, (x + 800, 1800), "500/5", 80)
    # 右段 6 个
    for i in range(6):
        x = 36000 + i * 5000
        schema.add_pline(es, [(x, 3000), (x + 1600, 3000), (x + 1600, 5000),
                              (x, 5000), (x, 3000)], True, "DQ")
        schema.add_text(es, (x + 800, 2400), "%dA" % (500 + i * 50), 100)
        schema.add_text(es, (x + 800, 1800), "800/5", 80)
        if i < 2:
            schema.add_text(es, (x + 800, 1200), "x1备用", 80)
    # 母线规格文字
    schema.add_text(es, (500, 1150), "1#铝合金母线槽 5000A 3P+N+PE", 350)
    schema.add_text(es, (34500, 1150), "2#铝合金母线槽 4000A 3P+N+PE", 350)
    schema.add_text(es, (30000, 3500), "4000A", 400)   # 母线额定量（大字高）
    return es


def test_recognize_mini_system_end_to_end():
    res = V.recognize(_build_mini_system())
    assert res.kind == "trunk", res.summary()
    assert res.ok, res.problems
    assert len(res.buses) == 2
    assert res.buses[0]["count"] == 6 and res.buses[1]["count"] == 6
    assert res.stats["spare"] == 2
    assert all(l["cap"] for l in res.loops)


def test_recognize_reports_unassignable():
    es = _build_mini_system()
    # 删掉一个容量文字 → 必须报"缺整定容量"，不能静默
    es["texts"] = [t for t in es["texts"]
                   if not (t["text"] == "400A")]
    res = V.recognize(es)
    assert not res.ok
    assert any("缺整定容量" in p for p in res.problems)


def test_recognize_plan_kind():
    es = schema.empty("plan")
    for i in range(20):
        schema.add_line(es, (i * 1000, 0), (i * 1000, 2000), "TRAY")
        schema.add_text(es, (i * 1000, 2500), "桥架 %d" % i, 50)
    res = V.recognize(es)
    assert res.kind == "plan"
    assert res.stats["lines"] == 20


def test_to_design_shape():
    res = V.recognize(_build_mini_system())
    d = res.to_design()
    assert d["doc_type"] == "power-trunk-design"
    assert len(d["busbars"]) == 2 and len(d["breakers"]) == 12
    assert len(d["edges"]) == 24
    assert d["self_check"]["ok"]


def test_validate_catches_empty():
    assert schema.validate(schema.empty()) == ["实体集为空"]


# ---------------------------------------------------------------- runner
def run_all():
    fns = [(k, v) for k, v in sorted(globals().items())
           if k.startswith("test_") and callable(v)]
    failed = []
    for name, fn in fns:
        try:
            fn()
            print("PASS  %s" % name)
        except AssertionError as e:
            failed.append(name)
            print("FAIL  %s  %s" % (name, e))
        except Exception as e:
            failed.append(name)
            print("ERR   %s  %r" % (name, e))
    print("\n%d/%d passed" % (len(fns) - len(failed), len(fns)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(run_all())
