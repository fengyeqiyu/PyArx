# -*- coding: utf-8 -*-
r"""示例 07：Python 侧按 QML 习惯喂 model（list / list[list]）。

运行：
    $env:PYARX_D2D_RT='software'
    $env:PYTHONPATH='G:\PyArx\X64\Release'
    python examples\07_model_python.py

对应 QML：
    ListView  { model: ["甲", "乙", "丙"] }
    表格类控件 { model: [["回路", 1], ["负载", 2]] }
"""
import os
import sys

os.add_dll_directory(r"G:\PyArx\X64\Release")
sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M  # noqa: E402


def dump(win, cid):
    n = win.countStrings(cid)
    return [win.getString(cid, i) for i in range(n)]


names = {}
win = M.guiload(r"G:\PyArx\UIDesign\samples\06_qml_layout.xml", names=names)
lid = names["Right"]

win.setXmlValue("Right", "model", ["甲", "乙", "丙"])
print("一维 list  ->", dump(win, lid))

win.setXmlValue("Right", "model", [["回路", 1], ["负载", 2]])
print("二维 list  ->", dump(win, lid))

win.setXmlValue("Right", "Items", "A|B")
print("字符串写法 ->", dump(win, lid))
