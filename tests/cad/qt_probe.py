# -*- coding: utf-8 -*-
"""探针：AutoCAD 进程里的 Qt 6.2.6 能不能摸到、QCoreApplication 是否已存在。

只做三件不会改状态的事：
  1. LoadLibrary(CAD 自带的 Qt6Core.dll)，看能不能进本进程；
  2. 取 QCoreApplication::instance()（静态函数，未初始化时返回 nullptr，不创建任何东西）；
  3. 试 LoadLibrary Qt6Widgets / Qt6WebEngineWidgets，看依赖是否完整。
"""
import ctypes
import os
import time

ROOT = r'C:\Program Files\Autodesk\AutoCAD 2024'
LOG = r'G:\PyArx\docs\_qt_probe.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


log('---- 探针开始 ----')
os.add_dll_directory(ROOT)

try:
    qt = ctypes.WinDLL(os.path.join(ROOT, 'Qt6Core.dll'))
    log('Qt6Core.dll 载入成功')
except OSError as e:
    log('Qt6Core.dll 载入失败: %s' % e)
    raise

# QCoreApplication::instance() —— 静态成员函数，MSVC x64 用统一调用约定
try:
    inst = qt['?instance@QCoreApplication@@SAPEAV1@XZ']
    inst.restype = ctypes.c_void_p
    p = inst()
    log('QCoreApplication::instance() = %s' % ('0x%X' % p if p else 'nullptr（Qt 尚未初始化）'))
except OSError as e:
    log('取 instance 符号失败: %s' % e)

for name in ('Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6WebEngineWidgets.dll', 'Qt6WebEngineCore.dll'):
    try:
        ctypes.WinDLL(os.path.join(ROOT, name))
        log('%s 载入成功' % name)
    except OSError as e:
        log('%s 载入失败: %s' % (name, e))

log('---- 探针结束 ----')
print('[qt_probe] done')
