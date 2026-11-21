# -*- coding: utf-8 -*-
"""临时审计探针（目标2 渲染机制与性能）。
通路：
  A. DWG 画布直注 loadDwgXml = ShxText rebuild 末端同一条 setDwgXml 通路；
     测 1 次 vs 7 次场景重灌（解析+入场景+失效）耗时与重建次数。
  B. ItemView model：一次 setXmlValue(model) 整表行控件销毁重建；测 1 次 vs 7 次。
  C. Button BackColor：外观属性逐个推送，测 1 次 vs 7 次（子控件整窗失效通路）。
ShxText 宿主附着路径本轮实测崩溃（见 _wip_perf.md C 节），其 7 属性×rebuild 次数
以静态代码行号取证，动态耗时以上述同通路代理值给出。
收尾必须删除本文件与 log/ 下产物。"""
import os
import re
import sys
import statistics
import time

ROOT = r'G:\PyArx'
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
os.environ['PYARX_MFC_DEBUG'] = '1'
os.environ['PYARX_LOG_DIR'] = os.path.join(ROOT, 'tests', '_audit_tmp', 'log')

import PyMfcXml as M  # noqa: E402

LOG = os.path.join(os.environ['PYARX_LOG_DIR'], 'pymfc_shutdown.log')


def scene(n_poly):
    out = ["<XScene Width='700' Height='80'><XGroup>"]
    for i in range(n_poly):
        y = 6.0 + i * 2.5
        pts = '20,%.1f 350,%.1f 680,%.1f' % (y, y + 1.0, y + 0.5)
        out.append("<XPolyline Points='%s' Stroke='#1A1A1A' Width='1.2'/>" % pts)
    out.append('</XGroup></XScene>')
    return ''.join(out)


XML_DWG = ('<XWindow Name="root" Width="760" Height="120">'
           '<XDwgCanvas Name="cv" x="20" y="20" w="700" h="80"/></XWindow>')

XML_BTN = ('<XWindow Name="root" Width="300" Height="90">'
           '<XButton Name="b" x="20" y="20" Width="120" Height="36" Text="OK"/></XWindow>')

XML_LIST = (
    '<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" '
    'Width="520" Height="360">'
    '<XItemView x:Name="V1" itemHeight="30" padding="8" model="a|b|c">'
    '<XStackPanel orientation="Horizontal" padding="8" spacing="8">'
    '<XLabel x:Name="T{index}" text="{0}" Width="140"/>'
    '<XLabel x:Name="S{index}" text="{1}" Width="180"/>'
    '</XStackPanel></XItemView></XWindow>')


def log_size():
    return os.path.getsize(LOG) if os.path.exists(LOG) else 0


def count_rebuilds(cid, offset):
    if not os.path.exists(LOG):
        return 0
    with open(LOG, 'rb') as f:
        f.seek(offset)
        data = f.read().decode('utf-8', 'replace')
    return len(re.findall(r'\[dwg-cv\] id=%d xml shapes=' % cid, data))


def load(xml):
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    try:
        win.show()
    except Exception:
        pass
    M.pump(40)
    return win, names


def close(win):
    try:
        win.close()
    except Exception:
        pass


def dwg_one():
    win, names = load(XML_DWG)
    cid = names['cv']
    off = log_size()
    t0 = time.perf_counter()
    win.load_dwg_xml(cid, scene(26))
    t1 = time.perf_counter()
    M.pump(20)
    n = count_rebuilds(cid, off)
    close(win)
    return (t1 - t0) * 1000.0, n


def dwg_seven():
    win, names = load(XML_DWG)
    cid = names['cv']
    off = log_size()
    t0 = time.perf_counter()
    for k in range(7):
        win.load_dwg_xml(cid, scene(26))
    t1 = time.perf_counter()
    M.pump(20)
    n = count_rebuilds(cid, off)
    close(win)
    return (t1 - t0) * 1000.0, n


def btn_one():
    win, names = load(XML_BTN)
    t0 = time.perf_counter()
    win.set_xml_value('b', 'BackColor', '#F1F5F9')
    t1 = time.perf_counter()
    M.pump(20)
    close(win)
    return (t1 - t0) * 1000.0


def btn_seven():
    win, names = load(XML_BTN)
    t0 = time.perf_counter()
    for k, col in enumerate(['#F1F5F9', '#E2E8F0', '#F8FAFC', '#EEF2F7',
                             '#F1F5F9', '#E2E8F0', '#F8FAFC']):
        win.set_xml_value('b', 'BackColor', col)
    t1 = time.perf_counter()
    M.pump(20)
    close(win)
    return (t1 - t0) * 1000.0


def rows(n):
    return [['AT%d-1' % i, '回路 %d' % i] for i in range(n)]


def list_one():
    win, names = load(XML_LIST)
    t0 = time.perf_counter()
    win.set_xml_value('V1', 'model', rows(20))
    t1 = time.perf_counter()
    M.pump(20)
    close(win)
    return (t1 - t0) * 1000.0


def list_seven():
    win, names = load(XML_LIST)
    t0 = time.perf_counter()
    for k in range(7):
        win.set_xml_value('V1', 'model', rows(20))
    t1 = time.perf_counter()
    M.pump(20)
    close(win)
    return (t1 - t0) * 1000.0


def med(xs):
    return round(statistics.median(xs), 3)


def main():
    d1, d7, b1, b7, l1, l7 = [], [], [], [], [], []
    for _ in range(5):
        d1.append(dwg_one())
        d7.append(dwg_seven())
        b1.append(btn_one())
        b7.append(btn_seven())
        l1.append(list_one())
        l7.append(list_seven())

    def row2(tag, rows_):
        ms = [r[0] for r in rows_]
        ns = [r[1] for r in rows_]
        print('%s ms_median=%s ms_all=%s rebuilds=%s' %
              (tag, med(ms), [round(x, 2) for x in ms], ns))

    def row1(tag, rows_):
        ms = list(rows_)
        print('%s ms_median=%s ms_all=%s' % (tag, med(ms), [round(x, 2) for x in ms]))

    row2('dwg_scene_1', d1)
    row2('dwg_scene_7', d7)
    row1('btn_backcolor_1', b1)
    row1('btn_backcolor_7', b7)
    row1('itemview_model_1x20rows', l1)
    row1('itemview_model_7x20rows', l7)
    print('dwg seven_per_call=%s ms' % med([r[0] / 7 for r in d7]))
    print('btn seven_per_call=%s ms' % med([x / 7 for x in b7]))
    print('list seven_per_call=%s ms' % med([x / 7 for x in l7]))


if __name__ == '__main__':
    main()
