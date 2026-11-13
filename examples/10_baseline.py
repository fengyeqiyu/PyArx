# -*- coding: utf-8 -*-
r"""示例 10：文字基线对齐（anchors.baseline / Qt.AlignBaseline）。

运行：
    $env:PYARX_D2D_RT='software'
    $env:PYTHONPATH='G:\PyArx\X64\Release'
    python examples\10_baseline.py

对应 QML：
    RowLayout {
        Label { text: "回路一"; font.pixelSize: 22; Layout.alignment: Qt.AlignBaseline }
        Label { text: "（说明 9pt）"; Layout.alignment: Qt.AlignBaseline }
    }
"""
import sys

sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M  # noqa: E402

SAMPLE = r"G:\PyArx\UIDesign\samples\15_qml_baseline.xml"
rects = M.xmlLayout(SAMPLE, 440, 260)

big = rects["Big1"]
small = rects["Small1"]
t2 = rects["T2"]
s2 = rects["S2"]
t3 = rects["T3"]
p3 = rects["P3"]

print("AlignBaseline   16pt y=%d  9pt y=%d  差=%d" % (big[1], small[1], small[1] - big[1]))
print("anchors.baseline=\"T2\"  y=%d  T2 y=%d  差=%d" % (s2[1], t2[1], s2[1] - t2[1]))
print("anchors.top=\"T3.baseline\"  P3 y=%d  T3 y=%d  差=%d" % (p3[1], t3[1], p3[1] - t3[1]))
print("三组差值应当一致（同一套字体度量）")
