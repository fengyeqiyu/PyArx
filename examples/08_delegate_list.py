# -*- coding: utf-8 -*-
r"""示例 08：ItemView 的 model 直接喂 Python list（不用拼 "a|b;c|d" 字符串）。

运行：
    $env:PYARX_D2D_RT='software'
    $env:PYTHONPATH='G:\PyArx\X64\Release'
    python examples\08_delegate_list.py

对应 QML：
    ListView {
        model: [["AT1-1", "回路一", "正常"], ["AT1-2", "回路二", "告警"]]
        delegate: RowLayout { Text { text: modelData } ... }
    }
"""
import os
import sys

os.add_dll_directory(r"G:\PyArx\X64\Release")
sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M  # noqa: E402

names = {}
win = M.guiload(r"G:\PyArx\UIDesign\samples\13_qml_model_list.xml", names=names)
win.show()

# 二维 list：一行三个字段，对应模板里的 {0} {1} {2}
rows = [["AT1-1", "回路一", "正常"], ["AT1-2", "回路二", "告警"], ["AT1-3", "回路三", "断开"]]
win.set_xml_value("LV", "model", rows)
print("二维 list ->", win.get_text(names["Txt1"]), "|", win.get_text(names["Sub2"]))

# 一维 list：每个元素一行，{modelData} 取整行文字
win.set_xml_value("LV", "model", ["甲", "乙"])
print("一维 list ->", win.get_text(names["Txt0"]), "|", win.get_text(names["Txt1"]))

# 空 list：清空全部行
win.set_xml_value("LV", "model", [])
print("空 list   -> 行控件全销毁")

win.close()
