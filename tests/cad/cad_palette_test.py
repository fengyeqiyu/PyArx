# -*- coding: utf-8 -*-
"""cad_palette_test.py — 在 AutoCAD 进程内以"侧栏（PaletteSet）"方式铺 PyMfc XAML 界面。

在 CAD 命令行执行：PYRUNFILE → 输入本文件路径。
"""
import sys as _sys
_sys.path.insert(0, r'G:\PyArx\tests\cad')
from _cadkeep import keep as _keep
import os
import PyMfcXaml
import PyArxCore

XAML = r"G:\PyArx\tests\cad\cad_ui_palette.xaml"
LOG = r"G:\PyArx\log\cad_palette_test.log"
PAL_NAME = "py-mfc-palette"


def _log(msg):
    try:
        os.makedirs(os.path.dirname(LOG), exist_ok=True)
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def main():
    _log("[palette] 开始")
    pal = _keep(PyArxCore.Ap.getPalette(PAL_NAME, 360, 720))
    names = {}
    PyMfcXaml.guiload(XAML, None, globals(), names, 1000, pal)
    _log("[palette] 控件 ids: %s" % names)
    _log("[palette] 完成，页数=%d" % pal.pageCount)
    print("[PyArx] 侧栏方式已加载：%d 个控件，页数=%d" % (len(names), pal.pageCount))


main()
