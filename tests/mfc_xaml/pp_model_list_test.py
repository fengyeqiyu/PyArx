# -*- coding: utf-8 -*-
# ItemView 的 model 直接接 Python list：一维 / 二维、改一次重建一次行
import os
import sys
import tempfile

sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXaml as pm  # noqa: E402

XAML = r"""<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" title="模型列表" Width="520" Height="360">
  <XItemView x:Name="V1" itemHeight="52" padding="8" layout.fillWidth="true" layout.fillHeight="true" model="占位|第 0 行|空闲">
    <XStackPanel orientation="Horizontal" padding="8" spacing="8">
      <XLabel x:Name="T{index}" text="{0}" Width="140"/>
      <XLabel x:Name="S{index}" text="{1}" Width="180"/>
      <XButton x:Name="B{index}" text="{2}" Width="80" Height="30"/>
    </XStackPanel>
  </XItemView>
</XWindow>"""

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


path = os.path.join(tempfile.gettempdir(), "pyarx_model_list.xaml")
with open(path, "w", encoding="utf-8") as f:
    f.write(XAML)

names = {}
win = pm.guiload(path, names=names, context=globals())
win.show()

# 初始一行
check("初始行 T0", win.getText(names["T0"]) == "占位", win.getText(names["T0"]))

# 二维 list：3 行 × 3 字段
rows = [["AT1-1", "回路一", "正常"], ["AT1-2", "回路二", "告警"], ["AT1-3", "回路三", "断开"]]
win.setXamlValue("V1", "model", rows)

check("二维 list 第1行 T0", win.getText(names["T0"]) == "AT1-1", win.getText(names["T0"]))
check("二维 list 第1行 S0", win.getText(names["S0"]) == "回路一", win.getText(names["S0"]))
check("二维 list 第2行 T1", win.getText(names["T1"]) == "AT1-2", win.getText(names["T1"]))
check("二维 list 第2行 B1", win.getText(names["B1"]) == "告警", win.getText(names["B1"]))
check("二维 list 第3行 S2", win.getText(names["S2"]) == "回路三", win.getText(names["S2"]))

# 一维 list：每行一个字段，{index} 跟着变
win.setXamlValue("V1", "model", ["甲", "乙"])
check("一维 list 第1行", win.getText(names["T0"]) == "甲", win.getText(names["T0"]))
check("一维 list 第2行", win.getText(names["T1"]) == "乙", win.getText(names["T1"]))

# 空 list：行清空（不留旧行控件）
win.setXamlValue("V1", "model", [])
check("空 list 无残留行", "T0" not in names, "T0 还在")

win.close()
print("=== %d/%d PASS ===" % (ok, ok + fail))
sys.exit(0 if fail == 0 else 1)
