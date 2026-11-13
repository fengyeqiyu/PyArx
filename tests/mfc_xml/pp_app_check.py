# -*- coding: utf-8 -*-
"""pp_app.py 的自动化验收：逐项触发 HTML 的每一条功能并取证。

不开交互循环（App.main 是循环），这里直接构造 App 再逐项点按钮，
把每一步的界面抓图落到 docs\\_pp_app_*.png，导出文件落到 docs\\。
"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M
import pp_app

DOCS = r'G:\PyArx\docs'


def shot(app, name):
    app.win.savePng(os.path.join(DOCS, name))
    print('  截图', name, os.path.getsize(os.path.join(DOCS, name)), 'B')


def main():
    app = pp_app.App()
    app.win.show()
    time.sleep(0.5)
    M.pump(30)

    r = app.view
    print('[初始] 三级示例 %dx%d 节点 %d 交叉 %d' %
          (r['width'], r['height'], len(r['nodes']), r['crossings']))
    print('  状态栏:', app.win.getText(app.win.__class__ and pp_app.L_STATUS))
    shot(app, '_pp_app_main.png')

    # 1) 横向布局
    app.win.setCheck(pp_app.C_HORIZ, True)
    app.on_dir_toggle()
    time.sleep(0.2); M.pump(30)
    r = app.view
    print('[横向] %dx%d 交叉 %d' % (r['width'], r['height'], r['crossings']))
    shot(app, '_pp_app_horiz.png')

    # 2) 连线标注关
    app.win.setCheck(pp_app.C_HORIZ, False)
    app.on_dir_toggle()
    time.sleep(0.2); M.pump(30)
    app.win.setCheck(pp_app.C_CABLE, False)
    app.onCableToggle()
    time.sleep(0.2); M.pump(30)
    shot(app, '_pp_app_nocable.png')
    app.win.setCheck(pp_app.C_CABLE, True)
    app.onCableToggle()

    # 3) 搜索定位
    app.win.setText(pp_app.E_SEARCH, 'AT2-1-3')
    app.doSearch()
    time.sleep(0.3); M.pump(30)
    print('[搜索] 选中 = %r 视口 = %s' %
          (app.win.getPlanSelection(pp_app.CV), app.win.getPlanView(pp_app.CV)))
    shot(app, '_pp_app_search.png')

    # 4) 折叠：双击 AT2-1 的判定靠 selection，这里直接驱动状态机
    sel = 'AT2-1'
    app.curSel = sel
    app.win.selectPlan(pp_app.CV, sel)
    app.onCanvasDblclick()
    time.sleep(0.3); M.pump(30)
    nodes_now = [n['code'] for n in app.view['nodes']]
    print('[折叠] AT2-1 后节点数 =', len(nodes_now), ' 还在 =', 'AT2-1' in nodes_now,
          ' 下游被隐藏 =', ('AT2-1-1' not in nodes_now))
    shot(app, '_pp_app_collapsed.png')

    # 展开回来
    app.onCanvasDblclick()
    time.sleep(0.2); M.pump(30)
    print('[展开] 节点数 =', len(app.view['nodes']))

    # 5) 文档示例
    app.onSampleDoc()
    time.sleep(0.3); M.pump(30)
    print('[文档示例] 节点 %d 尺寸 %dx%d' %
          (len(app.view['nodes']), app.view['width'], app.view['height']))
    shot(app, '_pp_app_doc.png')

    # 6) 编辑面板
    app.onOpenEditor()
    time.sleep(0.3); M.pump(30)
    print('[编辑面板] 表格行数 =', app.win.getTableRowCount(pp_app.TBL))
    shot(app, '_pp_app_editor.png')

    # 6b) 编辑一行：把 AT1-1 的负载从 9 改 12
    app.win.setTableSelectedRow(pp_app.TBL, 2)
    app.on_load_row()
    app.win.setText(pp_app.E_F0 + 4, '12')
    app.on_apply_row()
    time.sleep(0.2); M.pump(30)
    print('[编辑一行] 改后 节点数 =', len(app.view['nodes']),
          ' AT1-1 电缆行 =', [n.get('loadKw') for n in app.view['nodes']
                             if n['code'] == 'AT1-1'])
    app.onCloseEditor()
    time.sleep(0.2); M.pump(30)

    # 7) 导出四种
    out = {}
    p = os.path.join(DOCS, '_pp_app_export.json')
    pp_app.writeText(p, app.view['json'])
    out['json'] = os.path.getsize(p)
    p = os.path.join(DOCS, '_pp_app_export.csv')
    pp_app.writeText(p, app.view['csv'], bom=True)
    out['csv'] = os.path.getsize(p)
    p = os.path.join(DOCS, '_pp_app_export.svg')
    pp_app.writeText(p, app.view['svg'])
    out['svg'] = os.path.getsize(p)
    p = os.path.join(DOCS, '_pp_app_export.png')
    size = M.powerplan.renderPng(p, app.layoutNodes(), app.style(), 2, app.horiz)
    out['png'] = '%dx%d %d B' % (size[0], size[1], os.path.getsize(p))
    print('[导出]', out)

    # 8) 小地图跳转
    app.win.setPlanView(pp_app.CV, 1.0, -400, -100)
    app.curSel = 'TR1'
    app.centerOn('TR1')
    time.sleep(0.2); M.pump(30)
    print('[小地图/居中] TR1 后 视口 =', app.win.getPlanView(pp_app.CV))
    shot(app, '_pp_app_mini.png')

    app.win.close()
    print('验收完成')


if __name__ == '__main__':
    main()
