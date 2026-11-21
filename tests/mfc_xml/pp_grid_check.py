# -*- coding: utf-8 -*-
"""侧板行内编辑链路验证：改值 -> 应用并重绘 -> 校验拦截 -> 删除行。"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')

import PyMfcXml as M
import pp_app

DOC = r'G:\PyArx\docs'


def main():
    app = pp_app.App()
    w = app.win
    w.show()
    M.pump(30)

    app.showEditor()
    M.pump(20)
    print('侧板行数', app._grid_rows, '控件', len(app._gridIds))

    # 1) 改第 2 行（AT1）的名称 -> 应用
    base = pp_app.GRID_BASE + 1 * 8
    w.set_text(base + 1, '1号配电柜-改名')
    M.pump(10)
    app.onApplyAll()
    M.pump(30)
    print('改后 nodes[1] name =', app.nodes[1].get('name'))

    # 2) 校验：编号清空 -> 应拒绝且不重绘
    w.set_text(base + 0, '')
    M.pump(10)
    app.onApplyAll()
    M.pump(20)
    print('空编号提示 =', repr(w.get_text(pp_app.L_EDIT_COUNT)))
    w.set_text(base + 0, 'AT1')
    M.pump(10)

    # 3) 校验：负载填 abc
    w.set_text(base + 4, 'abc')
    M.pump(10)
    app.onApplyAll()
    print('非法负载提示 =', repr(w.get_text(pp_app.L_EDIT_COUNT)))
    w.set_text(base + 4, '11')

    # 4) 删除一个末端节点（AT2-3-5 是最后一行）
    last_code = app.nodes[-1]['code']
    app.onGridDelete(last_code)
    M.pump(20)
    print('删 %s 后 节点数 %d 行数 %d 提示 %s'
          % (last_code, len(app.nodes), app._grid_rows, repr(w.get_text(pp_app.L_EDIT_COUNT))))

    app.onApplyAll()
    M.pump(40)
    out = os.path.join(DOC, '_pp_grid_applied.png')
    w.save_png(out)
    print('截图', out, os.path.getsize(out), 'B')
    print('画布', app.view['width'], 'x', app.view['height'], '节点', len(app.view['nodes']))
    print('状态', w.get_text(pp_app.L_STATUS))


if __name__ == '__main__':
    main()
