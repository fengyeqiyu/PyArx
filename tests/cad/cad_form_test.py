# -*- coding: utf-8 -*-
"""cad_form_test.py — 在 AutoCAD 进程内以"嵌入窗体"方式铺 PyMfc XAML 界面。

在 CAD 命令行执行：PYRUNFILE → 输入本文件路径。
"""
import os
import sys as _sys
_sys.path.insert(0, r'G:\PyArx\tests\cad')
from _cadkeep import keep as _keep
import PyMfcXaml
try:
    import PyArxCore
except Exception as e:  # noqa
    PyArxCore = None
    _ARX_ERR = e

XAML = r"G:\PyArx\tests\cad\cad_ui.xaml"
LOG = r"G:\PyArx\log\cad_form_test.log"


def _log(msg):
    try:
        os.makedirs(os.path.dirname(LOG), exist_ok=True)
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def main():
    _log("[form] 开始")
    parent = PyArxCore.Ap.PyApp.mainWindow
    _log("[form] parent_hwnd=0x%X" % (parent or 0))
    w = _keep(PyMfcXaml.create_embedded(parent, "PyArx CAD 窗体测试", 540, 460))
    names = {}
    PyMfcXaml.guiload(XAML, w, globals(), names)
    _log("[form] 控件 ids: %s" % names)
    gv = names.get("gv")
    if gv:
        for i in range(12):
            w.addListItem(gv, "工程 %02d|子项 %d|%d" % (i + 1, i, i))
        w.setListSelected(gv, 3)
    # ProgressBar 的值走 XAML 的 Value 属性，运行期没有再暴露 setter（保持接口面最小）
    _log("[form] 完成，窗口已显示")
    print("[PyArx] 窗体方式已加载：%d 个控件" % len(names))


main()
