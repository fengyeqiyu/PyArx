# -*- coding: utf-8 -*-
"""诊断：① 手动 horiz=True 画布是否真变横向 ② ComboBox 选择事件是否回调到 Python。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

app = pp_app.App()
app.win.show()
M.pump(80)

print('初始 horiz=%s 画布 %dx%d' % (app.horiz, app.cvW, app.cvH))
print('scene dir=%s w=%s h=%s' % (app._sceneText()[:80], '-', '-'))

# ① 手动切横向
app.horiz = True
app.rebuild(fit=True)
M.pump(120)
print('手动横向后 画布 %dx%d' % (app.cvW, app.cvH))
import json
sc = json.loads(app._sceneText())
print('  scene dir=%s width=%s height=%s 节点=%d' % (sc.get('dir'), sc.get('width'), sc.get('height'), len(sc.get('nodes', [])))
)

# ② 切回纵向 + 走 ComboBox 真实路径
app.horiz = False
app.rebuild(fit=True)
M.pump(60)

hits = []
orig = app.onDirChange
def spy(*a, **k):
    hits.append(1)
    return orig(*a, **k)
app.onDirChange = spy
app.win.onChange(pp_app.C_HORIZ, spy)
M.pump(30)

# 程序化 setCurSel 不会发 CBN_SELCHANGE，这里直接看回调能不能被触发
app.win.setCurSel(pp_app.C_HORIZ, 0)
M.pump(60)
print('setCurSel(0) 后 horiz=%s 回调次数=%d' % (app.horiz, len(hits)))
print('getCurSel=%d' % app.win.getCurSel(pp_app.C_HORIZ))
