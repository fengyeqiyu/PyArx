# -*- coding: utf-8 -*-
"""排查：真实点击 → XAML 声明回调 这条链路，逐步打点。"""
import os, sys, ctypes as C, ctypes.wintypes as wt, traceback
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
from MfcXamlRuntime import Runtime
import pp_events as E

u = C.windll.user32
u.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, C.cVoidP]

hits = []
def on_apply(comp, name):
    hits.append(name)

class App:
    def __init__(self, w): self.win = w

win = M.Window('click-probe', 660, 420); win.show(); M.pump(250); win.setTheme(False)
print('T1 window ok', flush=True)
c = Runtime.create('side_panel', App(win), 10, 10, 640, 380)
print('T2 composite built, decl=%s' % (c._decl,), flush=True)
hw = E.find_control(win.hwnd, c.ids['BtnApply'])
print('T3 btn hwnd=%s' % hw, flush=True)

def down_up():
    u.SendMessageW(hw, 0x0201, 1, C.cVoidP(0x00200020))   # WM_LBUTTONDOWN, MK_LBUTTON, (32,32)
    u.SendMessageW(hw, 0x0202, 0, C.cVoidP(0x00200020))   # WM_LBUTTONUP

try:
    print('T4 before click', flush=True)
    down_up()
    print('T5 after click', flush=True)
    M.pump(60)
    print('T6 after pump', flush=True)
    print('hits=%s' % (hits,), flush=True)
except Exception:
    traceback.print_exc()
print('RESULT=%s' % ('HIT' if hits else 'NOHIT'), flush=True)
os._exit(0)
