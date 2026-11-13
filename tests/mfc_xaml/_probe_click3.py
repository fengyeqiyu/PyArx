# -*- coding: utf-8 -*-
"""对比：普通按钮 vs 复合控件里的按钮，同样的 SendMessage 点击，看回调是否触发。"""
import os, sys, ctypes as C, ctypes.wintypes as wt
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
class App:
    def __init__(self, w): self.win = w

win = M.Window('click-probe2', 700, 500); win.show(); M.pump(250); win.setTheme(False)

# 普通按钮
PLAIN = 9911
win.addButton(PLAIN, u'普通', 20, 20, 120, 30)
win.onClick(PLAIN, lambda i=None: hits.append(('plain', i)))

# 复合控件里的按钮
c = Runtime.create('side_panel', App(win), 10, 120, 640, 360)
c.BtnApply.click = lambda comp, name: hits.append(('composite', name))
M.pump(120)

def click(hw, cx=30, cy=15):
    u.SendMessageW(hw, 0x0201, 1, C.cVoidP((cy << 16) | cx))
    u.SendMessageW(hw, 0x0202, 0, C.cVoidP((cy << 16) | cx))
    M.pump(40)

hp = E.find_control(win.hwnd, PLAIN)
hc = E.find_control(win.hwnd, c.ids['BtnApply'])
print('hwnd plain=%s composite=%s' % (hp, hc), flush=True)
click(hp); print('after plain hits=%s' % (hits,), flush=True)
click(hc); print('after composite hits=%s' % (hits,), flush=True)
print('RESULT plain=%s comp=%s' % (any(h[0]=='plain' for h in hits), any(h[0]=='composite' for h in hits)), flush=True)
os._exit(0)
