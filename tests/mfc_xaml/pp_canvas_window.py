# -*- coding: utf-8 -*-
"""XAML 配电画布控件：开窗显示 + 缩放平移 + 截图取证。

window.addPowerCanvas(id, x, y, w, h) → 自绘窗口（滚轮缩放 / 拖动平移 / 单击选中）
window.loadPowerPlan(id, json)        → 喂布局结果，自动 fit
window.savePng(path)                  → 抓真屏幕（含 D2D 自绘子窗）
"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M


def three_level():
    nodes = [{'code': 'TR1', 'name': '1号变压器', 'upstream': '',
              'cable': 'YJV22-4×95+1×50', 'loadKw': 0.0}]
    for i in (1, 2):
        nodes.append({'code': 'AT%d' % i, 'name': '%d号柜' % i, 'upstream': 'TR1',
                      'cable': 'YJV-4×35+1×16', 'loadKw': 0.0})
        for j in (1, 2, 3):
            nodes.append({'code': 'AT%d-%d' % (i, j),
                          'name': '%d号柜%d号回路' % (i, j), 'upstream': 'AT%d' % i,
                          'cable': 'YJV-4×35+1×16', 'loadKw': 0.0})
            for k in (1, 2, 3, 4, 5):
                nodes.append({'code': 'AT%d-%d-%d' % (i, j, k),
                              'name': '%d号柜%d-%d末端' % (i, j, k),
                              'upstream': 'AT%d-%d' % (i, j),
                              'cable': 'BV-3×4', 'loadKw': float(2 + k)})
    return nodes


def main():
    nodes = three_level()
    plan = M.powerplan.build(nodes, M.powerplan.Style(), '三级配电干线图')
    print('布局 %dx%d，节点 %d' % (plan['width'], plan['height'], len(nodes)))

    win = M.Window('配电干线图 — XAML 画布', 1200, 640)
    win.setTheme(False)
    win.addPowerCanvas(2001, 0, 0, 1200, 640)
    win.loadPowerPlan(2001, plan['json'])
    win.show()
    time.sleep(0.4)

    zoom0 = win.getPlanZoom(2001)
    print('fit zoom = %.3f' % zoom0)

    out = r'G:\PyArx\docs'
    win.savePng(os.path.join(out, '_pp_canvas_fit.png'))
    print('已抓 _pp_canvas_fit.png（fit 全图）')

    # 放大到 1.0 看细节
    win.setPlanView(2001, 1.0, -300, -20)
    time.sleep(0.25)
    win.savePng(os.path.join(out, '_pp_canvas_zoom100.png'))
    print('已抓 _pp_canvas_zoom100.png（1:1）')

    # 再放大到 2.0，验证文字是"重画"而不是"拉位图"
    win.setPlanView(2001, 2.0, -900, -60)
    time.sleep(0.25)
    win.savePng(os.path.join(out, '_pp_canvas_zoom200.png'))
    print('已抓 _pp_canvas_zoom200.png（2:1）')

    # 选中一张卡片，确认命中测试
    win.setPlanView(2001, zoom0, 0, 0)
    time.sleep(0.2)
    print('选中前 selection = %r' % win.getPlanSelection(2001))
    win.close()


if __name__ == '__main__':
    main()
