# -*- coding: utf-8 -*-
"""配电画布交互验收：搜索定位（selectPlan）/ 命中测试 / 缩放级别抓图。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
from pp_canvas_window import three_level


def main():
    r = M.powerplan.build(three_level(), M.powerplan.Style(), '三级配电干线图')
    win = M.Window('配电干线图 — 交互', 1180, 560)
    win.setTheme(False)
    win.addPowerCanvas(2001, 0, 0, 1180, 560)
    win.loadPowerPlan(2001, r['json'])
    win.show()
    time.sleep(0.4)

    print('fit zoom = %.4f' % win.getPlanZoom(2001))
    win.savePng(r'G:\PyArx\docs\_pp_canvas_fit.png')

    # 搜索定位：按编号选中并居中（HTML 版搜索框做的事）
    ok = win.selectPlan(2001, 'AT1-1-3')
    print('selectPlan(AT1-1-3) =', ok, '，selection =', repr(win.getPlanSelection(2001)))
    time.sleep(0.2)
    win.savePng(r'G:\PyArx\docs\_pp_canvas_select.png')

    # 命中测试：用 selectPlan 之后的卡片矩形中心去点
    rc = win.cardRectPlan(2001, 'AT1-1-3')
    print('卡片矩形（客户端）=', rc)
    if rc:
        cx, cy = rc[0] + rc[2] // 2, rc[1] + rc[3] // 2
        print('hit(%d,%d) = %r' % (cx, cy, win.hitPlan(2001, cx, cy)))

    ok2 = win.selectPlan(2001, '不存在的编号')
    print('selectPlan(不存在) =', ok2)
    win.close()


if __name__ == '__main__':
    main()
