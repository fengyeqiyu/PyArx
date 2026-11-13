# -*- coding: utf-8 -*-
"""按 HTML 版式抓几张实图，供与浏览器里的 power-plan.html 并排比对。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

DOCS = r'G:\PyArx\docs'


def shot(app, name):
    app.win.savePng(os.path.join(DOCS, name))
    p = os.path.join(DOCS, name)
    print('  截图', name, os.path.getsize(p), 'B')


def main():
    app = pp_app.App()
    app.win.show()
    time.sleep(0.6)
    M.pump(40)

    print('[初始] %dx%d 节点 %d' % (app.view['width'], app.view['height'], len(app.view['nodes'])))
    z, ox, oy = app.win.getPlanView(pp_app.CV)
    print('  视口 zoom %.3f ox %d oy %d 画布 %dx%d' % (z, ox, oy, pp_app.CV_W, pp_app.CV_H))
    print('  状态:', app.win.getText(pp_app.L_STATUS))
    shot(app, '_pp_one_main.png')

    # 选中一个中间节点 → 详情浮层
    code = ''
    for n in app.view['nodes']:
        if n['code'].count('-') == 1:
            code = n['code']
            break
    if code:
        app.win.selectPlan(pp_app.CV, code)
        app.curSel = code
        app.fillDetail(code)
        M.pump(40)
        print('  详情:', app.win.getText(pp_app.DT_TITLE), '|', app.win.getText(pp_app.DT_TIP))
        shot(app, '_pp_one_detail.png')

    # 编辑侧板
    app.showEditor()
    M.pump(40)
    print('  侧板 zoom %.0f%%  行数 %d  控件 %d' % (
        app.win.getPlanView(pp_app.CV)[0] * 100, app._grid_rows, len(app._gridIds)))
    shot(app, '_pp_one_editor.png')
    app.hideEditor()
    M.pump(40)

    # 横向布局
    app.win.setCurSel(pp_app.C_HORIZ, 0)
    app.onDirChange()
    M.pump(40)
    print('[横向] %dx%d zoom %.0f%%' % (app.view['width'], app.view['height'],
                                        app.win.getPlanView(pp_app.CV)[0] * 100))
    shot(app, '_pp_one_horiz.png')

    print('done')


if __name__ == '__main__':
    main()
