# -*- coding: utf-8 -*-
"""工具条自测：三态图标 + 双通道派发 + 截图取证。

截图里应能看到：I_line 悬停（亮白）、I_rect 选中（蓝底），其余为常态。
"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\plugins')

import PyMfcXml as M
import draw_toolbar


def main():
    hits = []
    win = M.Window('绘图工具条', 360, 900)
    win.set_theme(True)
    bar = draw_toolbar.ToolBar(
        win, 8, 8,
        on_cmd=lambda c: hits.append(('py', c)),
        cad_cmd=lambda c: hits.append(('cad', c)))
    win.show()
    M.pump(2000)

    # 程序化摆出两种态
    bar.hover('I_line')
    bar.set_active('I_rect')
    M.pump(800)
    win.save_png(r'C:\temp\toolbar_win.png')

    # 走一遍真实点击链路（直接调事件处理）
    bar._clicked(bar.ids['I_circle'])
    bar._clicked(bar.ids['Sep1'])          # 分隔无 cmd，应被忽略
    M.pump(400)
    win.save_png(r'C:\temp\toolbar_win2.png')

    print('units=%d ids=%d hot=%s' % (len(bar.items), len(bar.ids), bar.hot))
    print('cmds=%s' % [c for _, c, _ in bar.items if c])
    print('hits=%s' % hits)
    print('cmd=%s' % bar.cmd)


main()
