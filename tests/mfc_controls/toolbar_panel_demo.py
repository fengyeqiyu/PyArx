# -*- coding: utf-8 -*-
"""宿主二：独立 Draw 面板（工具条 + 深色绘图区 + 命令回显）。

与 toolbar_demo 的区别：这里演示"工具条挂在一个真实宿主里"，
点击后既有 Python 回调（更新状态行），也有 CAD 通道回调（此处打日志）。
"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\plugins')

import PyMfcXml as M
import draw_toolbar

LBL = 1201
BACK = 1202


def main():
    log = []
    win = M.Window('Draw 面板 — 工具条宿主', 520, 900)
    win.set_theme(True)

    def on_py(cmd):
        log.append(cmd)
        try:
            win.set_text(LBL, '当前命令：%s    已选 %d 次' % (cmd.upper(), len(log)))
        except Exception:
            pass

    def on_cad(cmd):
        print('CAD 通道 →', cmd)

    win.add_label(LBL, '当前命令：无', 60, 10, 440, 24)
    # 深色绘图区背板
    win.add_shadow_box(BACK, 56, 44, 452, 840, 0, 4, 0, 0, 0,
                     '#00000000', '#1E2530', '#1E2530', '#33404F', 1)

    bar = draw_toolbar.ToolBar(win, 8, 8, on_cmd=on_py, cad_cmd=on_cad)
    win.show()
    M.pump(1500)

    for name in ('I_line', 'I_circle', 'I_text'):
        bar._clicked(bar.ids[name])
        M.pump(250)
    bar.hover('I_arc')
    M.pump(600)
    win.save_png(r'C:\temp\toolbar_panel.png')
    print('log=%s cmd=%s' % (log, bar.cmd))


main()
