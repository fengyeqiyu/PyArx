# -*- coding: utf-8 -*-
# 基线对齐：行容器 Align="baseline"（CSS align-items:baseline），旧 QML anchors 已删除
# 用 xmlLayout 直接量算好的矩形（不开窗口，纯布局数学）
import sys

sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M  # noqa: E402

SAMPLE = r"G:\PyArx\UIDesign\samples\15_qml_baseline.xml"
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


rects = M.xml_layout(SAMPLE, 440, 260)
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
# 2) Row2 Align="baseline"：S2 基线 = T2 基线 → S2 顶边比 T2 低 ascent 差
dy2 = y("S2") - y("T2")
check("Align=baseline 生效", 4 <= dy2 <= 18, "dy=%d" % dy2)
check("与 AlignBaseline 写法同一套度量", abs(dy2 - dy1) <= 2,
      "dy1=%d dy2=%d" % (dy1, dy2))
check("S2 排在 T2 右侧", x("S2") >= x("T2") + w("T2"), "x=%d" % x("S2"))

# 3) Row3 无文字方块：按基线行落位（CSS 下取合成基线）
dy3 = y("P3") - y("T3")
check("无文字方块随基线行落位", 10 <= dy3 <= 30, "dy=%d" % dy3)
check("基线位比底边更靠上", dy3 < h("T3"), "dy=%d h=%d" % (dy3, h("T3")))
check("P3 在 T3 右侧", x("P3") >= x("T3") + w("T3"), "x=%d" % x("P3"))

# 4) 字体族 / 字重不同也按同一套度量对齐（Consolas 12pt vs YaHei Bold 9pt）
dy4 = y("Bold4") - y("Mono4")
check("不同字体族/字重的基线对齐", 0 <= dy4 <= 18, "dy=%d" % dy4)
check("字体差异不算进同一行外的其它行", abs(y("Mono4") - y("Small1")) > 0, "mono_y=%d" % y("Mono4"))

# 5) 文字垂直居中的控件（按钮 32 高）：基线在盒中线附近，不是贴顶
dy5 = y("Lbl5") - y("Btn5")
check("居中控件的基线落到盒中线", 3 <= dy5 <= 14, "dy=%d" % dy5)
check("居中规则生效（不再按贴顶算）", dy5 >= 2, "dy=%d" % dy5)

# 6) 多行文本取首行基线：折行标签与单行标签同高、同基线
dy6 = y("Single6") - y("Multi6")
check("多行文本按首行基线对齐", -1 <= dy6 <= 1, "dy=%d" % dy6)

print("=== %d/%d PASS ===" % (ok, ok + fail))
sys.exit(0 if fail == 0 else 1)
