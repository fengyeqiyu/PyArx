# -*- coding: utf-8 -*-
# 运行时换 delegate 模板：Delegate 指向 ControlTemplate 的键，换键即换整行模板
import os
import sys

sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as pm  # noqa: E402

SAMPLE = r"G:\PyArx\UIDesign\samples\14_qml_delegate_swap.xml"

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


names = {}
win = pm.guiload(SAMPLE, names=names, context=globals())
win.show()

# 1) 加载时 Delegate="RowWide"：模板里的 W0 / S0 / A0
check("加载用 RowWide 的 W0", win.get_text(names["W0"]) == "AT1-1", win.get_text(names["W0"]))
check("加载用 RowWide 的 S0", win.get_text(names["S0"]) == "第 0 行 · 回路一", win.get_text(names["S0"]))
check("RowWide 第2行 A1", win.get_text(names["A1"]) == "告警", win.get_text(names["A1"]))

# 2) 换 RowCompact：旧行控件必须消失，新模板控件出现
win.set_xml_value("LV", "delegate", "RowCompact")
check("换模板后 W0 已销毁", "W0" not in names, "W0 还在")
check("RowCompact 的 K0", win.get_text(names["K0"]) == "0.", win.get_text(names["K0"]))
check("RowCompact 的 N1", win.get_text(names["N1"]) == "AT1-2", win.get_text(names["N1"]))
check("RowCompact 的 B1", win.get_text(names["B1"]) == "告警", win.get_text(names["B1"]))

# 3) 换回 RowWide：再换一次仍然正常（模板可来回切）
win.set_xml_value("LV", "delegate", "RowWide")
check("切回后 K0 已销毁", "K0" not in names, "K0 还在")
check("切回后 W0 回来", win.get_text(names["W0"]) == "AT1-1", win.get_text(names["W0"]))

# 4) 换模板后再换 model：两者可以叠加
win.set_xml_value("LV", "delegate", "RowCompact")
win.set_xml_value("LV", "model", [["AT1-9", "回路九", "断开"]])
check("换模板+换 model 后的 N0", win.get_text(names["N0"]) == "AT1-9", win.get_text(names["N0"]))
check("换模板+换 model 后的 B0", win.get_text(names["B0"]) == "断开", win.get_text(names["B0"]))

win.close()
print("=== %d/%d PASS ===" % (ok, ok + fail))
sys.exit(0 if fail == 0 else 1)
