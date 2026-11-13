# -*- coding: utf-8 -*-
r"""示例 09：运行时换 delegate 模板（QML 里就是换 delegate 那个 Component）。

运行：
    $env:PYARX_D2D_RT='software'
    $env:PYTHONPATH='G:\PyArx\X64\Release'
    python examples\09_delegate_swap.py

对应 QML：
    ListView { model: [...] ; delegate: wideRow }
    // 运行中切成另一套行外观
    listView.delegate = compactRow
"""
import os
import sys

os.add_dll_directory(r"G:\PyArx\X64\Release")
sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M  # noqa: E402

names = {}
win = M.guiload(r"G:\PyArx\UIDesign\samples\14_qml_delegate_swap.xml", names=names)
win.show()

print("宽行模板 ->", win.getText(names["W0"]), "|", win.getText(names["S0"]))
win.setXmlValue("LV", "delegate", "RowCompact")
print("紧凑模板 ->", win.getText(names["K0"]), win.getText(names["N1"]), win.getText(names["B1"]))
win.setXmlValue("LV", "delegate", "RowWide")
print("换回宽行 ->", win.getText(names["W0"]), "|", win.getText(names["A0"]))
win.close()
