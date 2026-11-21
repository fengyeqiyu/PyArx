# -*- coding: utf-8 -*-
"""金丝雀：把 plugins/*.xml 逐个 guiload 一遍，确认既有界面没被布局/糖层改动弄坏。"""
import glob, os, sys, traceback
BIN = os.environ.get('PYARX_BIN') or r'G:\PyArx\X64\Release'
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

fails = 0
for path in sorted(glob.glob(r'G:\PyArx\plugins\*.xml')):
    names = {}
    try:
        w = M.guiload(path, context=globals(), names=names)
        w.show()
        M.pump(120)
        print('OK   %-22s controls=%d' % (os.path.basename(path), len(names)))
        w.close()
    except Exception as exc:
        fails += 1
        print('FAIL %-22s %s: %s' % (os.path.basename(path), type(exc).__name__, exc))
        traceback.print_exc()
print('failed =', fails)
sys.exit(1 if fails else 0)
