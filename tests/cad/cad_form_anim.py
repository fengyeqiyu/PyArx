# -*- coding: utf-8 -*-
"""cad_form_anim.py — CAD 内窗体：动画时间线（宽度 / 数值 / 颜色）+ 页容器切页。

CAD 命令行：PYRUNFILE → 输入本文件路径。
"""
import sys as _sys
_sys.path.insert(0, r'G:\PyArx\tests\cad')
from _cadkeep import keep as _keep
import os
import PyMfcXaml as p
import PyArxCore

LOG = r"G:\PyArx\log\cad_form_anim.log"
XAML = r'''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="PyArx CAD 动画" Width="520" Height="400">
  <Canvas>
    <TextBlock x:Name="lb" Text="宽度 / 数值 / 颜色动画 + StackView 切页" Width="470" Height="22" Canvas.Left="14" Canvas.Top="8" />
    <Button x:Name="ab" Text="动画按钮" Width="160" Height="36" Canvas.Left="14" Canvas.Top="40" />
    <SpinBox x:Name="asp" Width="100" Height="30" Canvas.Left="190" Canvas.Top="42" Value="10" />
    <ProgressBar x:Name="apg" Width="300" Height="18" Canvas.Left="14" Canvas.Top="92" />
    <StackView x:Name="sv" Width="360" Height="180" Canvas.Left="14" Canvas.Top="126" />
    <TextBlock x:Name="pg1" Text="第一页：文字" Width="360" Height="150" Canvas.Left="14" Canvas.Top="126" />
    <QCanvas x:Name="pg2" Width="360" Height="150" Canvas.Left="14" Canvas.Top="126"
             Commands="Clear #313842; Rect 12,12,90,40,#00A8E8; Ellipse 120,10,50,50,#57C84D; Text 12,96,第二页:画命令,9,#FFFFFF" />
  </Canvas>
</Window>'''


def _log(msg):
    try:
        os.makedirs(os.path.dirname(LOG), exist_ok=True)
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def main():
    _log("[anim] 开始")
    path = os.path.join(os.path.dirname(LOG), "_cad_form_anim.xaml")
    with open(path, "w", encoding="utf-8") as f:
        f.write(XAML)

    parent = PyArxCore.Ap.PyApp.mainWindow
    w = _keep(p.create_embedded(parent, "PyArx CAD 动画", 520, 400))
    names = {}
    p.guiload(path, w, globals(), names)
    _log("[anim] ids=%s" % names)

    ab = names.get("ab")
    if ab:
        w.animate(ab, "Width", 300, 600, "outCubic")
        _log("[anim] 宽度动画 160→300 已启动")

    asp = names.get("asp")
    if asp:
        w.animate(asp, "Value", 90, 800, "outBack")
        _log("[anim] 数值动画 10→90 已启动")

    apg = names.get("apg")
    if apg:
        try:
            w.setRange(apg, 0, 100)
            w.setPos(apg, 20)                       # 有值才不是一条空轨道
            w.animate(apg, "Value", 100, 900, "outCubic")
            w.animateColor(apg, 0x00A8E8, 700, "linear")
            _log("[anim] 进度 20→100 + 颜色动画已启动")
        except Exception as e:
            _log("[anim] 进度/颜色动画跳过：%s" % e)

    sv = names.get("sv")
    if sv:
        w.addPage(sv, [names["pg1"]])
        w.addPage(sv, [names["pg2"]])
        w.setViewIndex(sv, 1)
        _log("[anim] 页容器 pages=%d index=%d" % (w.getViewCount(sv), w.getViewIndex(sv)))

    _log("[anim] 完成")
    print("[PyArx] 动画窗体已加载：%s" % names)


main()
