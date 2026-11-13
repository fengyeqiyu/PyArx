# -*- coding: utf-8 -*-
"""cad_form_overlay.py — CAD 内窗体：浮层族（菜单 / 菜单栏 / 提示 / 弹出层）。

CAD 命令行：PYRUNFILE → 输入本文件路径。
"""
import sys as _sys
_sys.path.insert(0, r'G:\PyArx\tests\cad')
from _cadkeep import keep as _keep
import os
import PyMfcXaml as p
import PyArxCore

LOG = r"G:\PyArx\log\cad_form_overlay.log"
XAML = r'''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="PyArx CAD 浮层" Width="560" Height="360">
  <XCanvas>
    <XTextBlock x:Name="lb" Text="MenuStrip / Menu / ToolTip / Popup 在 CAD 进程内" Width="500" Height="22" Canvas.Left="14" Canvas.Top="8" />
    <XMenuStrip x:Name="mb" Width="520" Height="30" Canvas.Left="14" Canvas.Top="36" />
    <XButton x:Name="btn" Text="打开菜单" Width="130" Height="34" Canvas.Left="14" Canvas.Top="86" />
    <XButton x:Name="btn2" Text="显示提示" Width="130" Height="34" Canvas.Left="154" Canvas.Top="86" />
    <XButton x:Name="btn3" Text="弹出层" Width="130" Height="34" Canvas.Left="294" Canvas.Top="86" />
    <XCheckBox x:Name="s1" Text="深色皮肤" Width="150" Height="26" Canvas.Left="14" Canvas.Top="140" />
    <XToggleButton x:Name="s2" Text="自绘开关" Width="150" Height="34" Canvas.Left="14" Canvas.Top="176" />
    <XProgressBar x:Name="pg" Value="80" Width="300" Height="18" Canvas.Left="14" Canvas.Top="228" />
  </XCanvas>
</XWindow>'''


def _log(msg):
    try:
        os.makedirs(os.path.dirname(LOG), exist_ok=True)
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def main():
    _log("[overlay] 开始")
    path = os.path.join(os.path.dirname(LOG), "_cad_form_overlay.xaml")
    with open(path, "w", encoding="utf-8") as f:
        f.write(XAML)

    parent = PyArxCore.Ap.PyApp.mainWindow
    w = _keep(p.create_embedded(parent, "PyArx CAD 浮层", 560, 360))
    names = {}
    p.guiload(path, w, globals(), names)
    _log("[overlay] ids=%s" % names)

    # 菜单栏 / 菜单 / 提示 / 弹出层：接口存在则调用，缺失不阻断整体加载
    # 这些接口收的是控件 id（int），不是 x:Name 字符串
    for fn, args in (
        ("addMenuBarItem", (names.get("mb"), "文件")),
        ("addMenuBarItem", (names.get("mb"), "编辑")),
        ("addMenuBarItem", (names.get("mb"), "视图")),
        ("showMenu", (names.get("btn"), ["新建", "打开", "退出"], 20, 40)),
        ("showToolTip", (names.get("btn2"), "这是一个 CAD 内的自绘提示", 3000)),
        ("showPopup", (names.get("btn3"), "弹出层标题", ["PyMfc 在 CAD 进程内绘制的浮层", "正文第二行"], 20, 40, 320, 180)),
    ):
        try:
            getattr(w, fn)(*args)
            _log("[overlay] %s%s ok" % (fn, args))
        except Exception as e:
            _log("[overlay] %s 跳过：%s" % (fn, e))

    _log("[overlay] 完成")
    print("[PyArx] 浮层窗体已加载：%s" % names)


main()
