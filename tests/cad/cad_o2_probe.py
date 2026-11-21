# -*- coding: utf-8 -*-
"""cad_o2_probe.py — /O2 构建在 AutoCAD 进程内的冒烟：解释器 + pyd + 嵌入窗体 + P0 原语。

在 CAD 命令行执行：PYRUNFILE → 输入本文件路径（单行，不重启 CAD）。
结果写 G:\\PyArx\\log\\cad_o2_probe.log，窗口留在原地不关。
"""
import sys as _sys
_sys.path.insert(0, r'G:\PyArx\tests\cad')
from _cadkeep import keep as _keep
import os
import sys
import traceback

LOG = r"G:\PyArx\log\cad_o2_probe.log"
XML = r"G:\PyArx\tests\cad\cad_o2.xml"


def _log(msg):
    try:
        os.makedirs(os.path.dirname(LOG), exist_ok=True)
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


def main():
    _log("[o2] start python=%s" % sys.version.split()[0])
    import PyMfcXml
    _log("[o2] PyMfcXml from %s" % getattr(PyMfcXml, "__file__", "?"))
    try:
        import PyArxCore
        _log("[o2] PyArxCore from %s" % getattr(PyArxCore, "__file__", "?"))
    except Exception as e:
        _log("[o2] PyArxCore import failed: %r" % (e,))

    parent = PyArxCore.Ap.PyApp.mainWindow
    _log("[o2] parent_hwnd=0x%X" % (parent or 0))

    w = _keep(PyMfcXml.create_embedded(parent, "PyArx O2 冒烟", 560, 400))
    names = {}
    PyMfcXml.guiload(XML, w, globals(), names)
    _log("[o2] controls=%s" % names)

    # 控件级真 opacity 走一遍，确认 /O2 下离屏合成路径没被优化坏
    qcv = names.get("qcv")
    if qcv:
        w.set_layer_opacity(qcv, 255)
        _log("[o2] getLayerOpacity=%d" % w.get_layer_opacity(qcv))

    _log("[o2] ok，窗口已显示，保留不关")
    print("[PyArx] O2 冒烟完成：%d 个控件" % len(names))


try:
    main()
except Exception:
    _log("[o2] FAILED\n" + traceback.format_exc())
    print("[PyArx] O2 冒烟失败，详见 " + LOG)
