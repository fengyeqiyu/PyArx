# -*- coding: utf-8 -*-
# 基线对齐：Layout.alignment=Qt.AlignBaseline / anchors.baseline=A / anchors.top=A.baseline
# 用 xamlLayout 直接量算好的矩形（不开窗口，纯布局数学）
import sys

sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXaml as M  # noqa: E402

SAMPLE = r"G:\PyArx\UIDesign\samples\15_qml_baseline.xaml"
ok = 0
fail = 0


def check(name, cond, extra=""):
    global ok, fail
    if cond:
        ok += 1
        print("PASS", name)
    else:
        fail += 1
        print("FAIL", name, extra)


rects = M.xamlLayout(SAMPLE, 440, 260)
for k, v in rects.items():
    print("  rect", k, v)


def y(n):
    return rects[n][1]


def h(n):
    return rects[n][3]


def w(n):
    return rects[n][2]


def x(n):
    return rects[n][0]


# 1) 行内基线对齐：小字号要往下让出 ascent 差（16pt 比 9pt 高约 8~12px）
dy1 = y("Small1") - y("Big1")
check("AlignBaseline 小字号下移", 4 <= dy1 <= 18, "dy=%d" % dy1)

# 同一条基线的必要条件：dy 等于两条 ascent 之差；用另一组字号验算一致性
# 2) anchors.baseline="T2"：S2 基线 = T2 基线 → S2 顶边比 T2 低 ascent 差
dy2 = y("S2") - y("T2")
check("anchors.baseline 生效", 4 <= dy2 <= 18, "dy=%d" % dy2)
check("anchors.baseline 与 AlignBaseline 同一套度量", abs(dy2 - dy1) <= 2,
      "dy1=%d dy2=%d" % (dy1, dy2))
check("anchors.left 仍然生效（S2 在 T2 右侧）", x("S2") >= x("T2") + w("T2"), "x=%d" % x("S2"))

# 3) anchors.top="T3.baseline"：P3 顶边 = T3 文字基线
dy3 = y("P3") - y("T3")
check("anchors.top=A.baseline 生效", 10 <= dy3 <= 30, "dy=%d" % dy3)
check("baseline 边比底边更靠上", dy3 < h("T3"), "dy=%d h=%d" % (dy3, h("T3")))
check("P3 在 T3 右侧", x("P3") >= x("T3") + w("T3"), "x=%d" % x("P3"))

# 4) 字体族 / 字重不同也按同一套度量对齐（Consolas 12pt vs YaHei Bold 9pt）
dy4 = y("Bold4") - y("Mono4")
check("不同字体族/字重的基线对齐", 0 <= dy4 <= 18, "dy=%d" % dy4)
check("字体差异不算进同一行外的其它行", abs(y("Mono4") - y("Small1")) > 0, "mono_y=%d" % y("Mono4"))

print("=== %d/%d PASS ===" % (ok, ok + fail))
sys.exit(0 if fail == 0 else 1)
