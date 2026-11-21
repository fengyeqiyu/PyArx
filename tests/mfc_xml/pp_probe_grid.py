# -*- coding: utf-8 -*-
"""侧板行内编辑可行性探针：39 行 × 7 个 EDIT + 1 个 ✕，平铺不滚动。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')

import PyMfcXml as M
import pp_app

ROW_BASE = 4000
COLS_W = [78, 110, 70, 130, 52, 100, 56]
DEL_W = 26
ROW_H = 16


def main():
    app = pp_app.App()
    w = app.win
    w.show()
    M.pump(30)
    nodes = app.layoutNodes()
    t0 = time.perf_counter()
    n = 0
    ids = []
    table_x = pp_app.ED_X + 10
    table_y = pp_app.ED_Y + pp_app.ED_HEAD_H + 22
    for i, node in enumerate(nodes[:39]):
        x = table_x
        y = table_y + i * ROW_H
        for j, key in enumerate(('code', 'name', 'upstream', 'cable', 'loadKw', 'voltage', 'length')):
            cid = ROW_BASE + i * 8 + j
            text = pp_app.voltText(node.get('voltage')) if key == 'voltage' else str(node.get(key, ''))
            w.add_edit(cid, text, x + 1, y, COLS_W[j] - 2, ROW_H - 1)
            w.set_control_font(cid, 'Microsoft YaHei', 11, False)
            ids.append(cid)
            n += 1
            x += COLS_W[j]
        cid = ROW_BASE + i * 8 + 7
        w.add_button(cid, '✕', x, y, DEL_W, ROW_H - 1)
        w.set_control_font(cid, 'Microsoft YaHei', 11, False)
        ids.append(cid)
        n += 1
    w.raise_many(ids)
    t1 = time.perf_counter()
    print('创建控件 %d 个，耗时 %.3f s' % (n, t1 - t0))
    M.pump(60)
    out = r'G:\PyArx\docs\_pp_probe_grid.png'
    w.save_png(out)
    print('png', os.path.getsize(out), 'B')
    t2 = time.perf_counter()
    print('首帧+截图 %.3f s' % (t2 - t1))
    time.sleep(0.3)


if __name__ == '__main__':
    main()
