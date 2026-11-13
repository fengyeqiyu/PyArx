# -*- coding: utf-8 -*-
"""只验一件事：XAML 里声明的 on_click 在点击时真的回到 Python。"""
import os, sys, traceback
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, pp_plugin, pp_events as E

hits = []
def on_apply(comp, name):
    hits.append(name)

class Fake:
    def __init__(self, w): self.win = w

win = M.Window('cb-test', 660, 420); win.show(); M.pump(300); win.setTheme(False)
c = pp_plugin.import_plugin('side_panel', Fake(win), 10, 10, 640, 380)
M.pump(200)
hw = E.find_control(win.hwnd, c.ids['BtnApply'])
print('btn hwnd=%s rect=%s' % (hw, c.rects['BtnApply']), flush=True)
try:
    E.click(hw, 40, 16, pump=0)
    M.pump(120)
except Exception:
    traceback.print_exc()
print('hits=%s' % (hits,), flush=True)
print('RESULT=%s' % ('OK' if hits else 'MISS'), flush=True)
os._exit(0)
