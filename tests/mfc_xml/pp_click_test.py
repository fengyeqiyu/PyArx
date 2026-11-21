# -*- coding: utf-8 -*-
"""点击链路验收：普通按钮 / 复合控件按钮 / XML 声明回调 / 星号列取整。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')

import PyMfcXml as M
from MfcXmlRuntime import Runtime
import pp_events as E

u = ctypes.windll.user32
u.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]

PASS, FAIL = [0], [0]
hits = []


def ck(name, cond, detail=''):
    if cond:
        PASS[0] += 1
        print('PASS %s %s' % (name, detail), flush=True)
    else:
        FAIL[0] += 1
        print('FAIL %s %s' % (name, detail), flush=True)


# ---- XML 里 side_panel 的 BtnApply 声明的是 on_click="on_apply" ----
def on_apply(comp, name):
    hits.append(('decl', name))


class App(object):
    def __init__(self, w):
        self.win = w


def click(hw, cx=30, cy=15):
    u.SendMessageW(hw, 0x0201, 1, ctypes.c_void_p((cy << 16) | cx))
    u.SendMessageW(hw, 0x0202, 0, ctypes.c_void_p((cy << 16) | cx))
    M.pump(60)


win = M.Window('click-test', 700, 500)
win.show()
M.pump(250)
win.set_theme(False)
app = App(win)

# 1) 普通按钮：基线，确认消息路径本身没问题
PLAIN = 9911
win.add_button(PLAIN, u'普通', 20, 20, 120, 30)
win.on_click(PLAIN, lambda i=None: hits.append(('plain', i)))
M.pump(60)
hp = E.find_control(win.hwnd, PLAIN)
click(hp)
ck('普通按钮点击回调', any(h[0] == 'plain' for h in hits), str(hits))

# 2) 复合控件：XML 声明的 on_apply + 属性赋值的回调，两条都要被调到
c = Runtime.create('side_panel', app, 10, 120, 640, 360)
attr_hits = []
c.BtnApply.click = lambda comp, name: attr_hits.append(name)
M.pump(120)
hc = E.find_control(win.hwnd, c.ids['BtnApply'])
hits.clear()
click(hc)
ck('XML 声明回调被调到', any(h[0] == 'decl' and h[1] == 'BtnApply' for h in hits), str(hits))
ck('属性赋值回调被调到', attr_hits == ['BtnApply'], str(attr_hits))

# 3) 星号列取整：640 = 12 + 3*star + 12，任何一列都不该丢 1px
ck('星号列总和=640', c.rects['Table'][2] == 640, str(c.rects['Table']))
ck('按钮三段均分', c.rects['BtnApply'][2] + c.rects['BtnLoad'][2] + c.rects['BtnClose'][2] == 640 - 24,
   '%d+%d+%d' % (c.rects['BtnApply'][2], c.rects['BtnLoad'][2], c.rects['BtnClose'][2]))

print('=== %d/%d PASS ===' % (PASS[0], PASS[0] + FAIL[0]), flush=True)
os._exit(0)
