# -*- coding: utf-8 -*-
"""_cadkeep.py — 让 CAD 内加载的窗口活过脚本生命周期。

PYRUNFILE 执行完，脚本模块的局部/全局变量都会被回收；Python 端 Window 对象一旦被销毁，
对应的 PyMfcNativeWindow HWND 也就没了（用户看到"窗口一闪就没"）。
把对象挂到常驻的 sys 模块上，引用计数就一直在。
"""
import sys


def keep(obj):
    lst = getattr(sys, "_pyarx_windows", None)
    if lst is None:
        lst = []
        sys._pyarx_windows = lst
    lst.append(obj)
    return obj
