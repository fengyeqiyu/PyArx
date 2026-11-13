# -*- coding: utf-8 -*-
"""cad_form_list.py — CAD 内窗体：虚拟化列表 + 表格 + 若干数值控件。

CAD 命令行：PYRUNFILE → 输入本文件路径。解释器常驻，可反复加载。
"""
import sys as _sys
_sys.path.insert(0, r'G:\PyArx\tests\cad')
from _cadkeep import keep as _keep
import os
import PyMfcXaml as p
import PyArxCore

LOG = r"G:\PyArx\log\cad_form_list.log"
XAML = r'''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="PyArx CAD 列表与表格" Width="560" Height="470">
  <Canvas>
    <TextBlock x:Name="lb" Text="VirtualList 21 行 / TableView 3 行 / 数值控件" Width="490" Height="22" Canvas.Left="14" Canvas.Top="8" />
    <VirtualList x:Name="lv" Items="%s" ItemHeight="44" Width="260" Height="230" Canvas.Left="14" Canvas.Top="36" />
    <TableView x:Name="tv" Width="260" Height="230" Canvas.Left="288" Canvas.Top="36" />
    <SpinBox x:Name="sp" Width="90" Height="28" Canvas.Left="14" Canvas.Top="282" Value="42" />
    <RangeSlider x:Name="rs" Width="180" Height="28" Canvas.Left="118" Canvas.Top="282" LowValue="20" HighValue="70" />
    <Dial x:Name="dl" Width="90" Height="90" Canvas.Left="14" Canvas.Top="320" Value="40" />
    <Tumbler x:Name="tb" Width="90" Height="90" Canvas.Left="118" Canvas.Top="320" />
    <ScrollBar x:Name="sb" Width="24" Height="90" Canvas.Left="228" Canvas.Top="320" Value="35" />
    <Switch x:Name="sw" Width="56" Height="28" Canvas.Left="288" Canvas.Top="282" Checked="true" />
    <QCanvas x:Name="qc" Width="230" Height="90" Canvas.Left="288" Canvas.Top="320"
             Commands="Clear #313842; Rect 10,10,80,30,#00A8E8; Ellipse 110,8,44,44,#57C84D; Line 10,72,220,72,#FFFFFF,2; Text 10,50,列表页,9,#FFFFFF" />
  </Canvas>
</Window>''' % ';'.join('工程 %02d|负责：张%d|%d%%' % (i + 1, (i % 9) + 1, (i * 7) % 100) for i in range(21))


def _log(msg):
    try:
        os.makedirs(os.path.dirname(LOG), exist_ok=True)
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def main():
    _log("[list] 开始")
    path = os.path.join(os.path.dirname(LOG), "_cad_form_list.xaml")
    with open(path, "w", encoding="utf-8") as f:
        f.write(XAML)

    parent = PyArxCore.Ap.PyApp.mainWindow
    w = _keep(p.create_embedded(parent, "PyArx CAD 列表与表格", 560, 480))
    names = {}
    p.guiload(path, w, globals(), names)
    _log("[list] ids=%s" % names)

    lv = names.get("lv")
    if lv:
        w.setListSelected(lv, 6)
        _log("[list] list count=%d sel=%d scroll=%d"
             % (w.getListItemCount(lv), w.getListSelected(lv), w.getListScroll(lv)))

    tv = names.get("tv")
    if tv:
        w.setTableColumns(tv, "零件\t数量\t单价")
        for row in ("接头\t12\t3.50", "法兰\t8\t12.00", "垫片\t40\t0.80"):
            w.addTableRow(tv, row)
        w.setTableSelectedRow(tv, 1)
        _log("[list] table 已填 3 行")

    _log("[list] 完成")
    print("[PyArx] 列表/表格窗体已加载：%s" % names)


main()
