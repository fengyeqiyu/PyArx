# -*- coding: utf-8 -*-
"""cad2d 界面层测试：全原生控件外壳（cad_shell.xml）的结构、文案、点击派发。

跑法（在 examples\\cad2d 下）：
    set PYTHONPATH=G:\\PyArx\\X64\\Release
    python tests\\test_ui_shell.py

覆盖点：
  1 外壳声明的控件全部存在（工具条 14 / 特性行 4+8 + 3 / 状态栏 / 命令行 / 表格）
  2 工具条 14 个按钮真实点击 → 命令派发（FIT 走视图）
  3 状态栏两个胶囊翻转 + 文案跟随
  4 命令行输入 → 执行按钮 → 派发（命令名 / 英文别名 / 中文命令名）
  5 特性行点键名进入编辑（颜色走下拉、几何值走输入框）+ 写回 + 撤销
  6 提示行、视图行、图层表内容刷新
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                      # examples\cad2d
BUILD = os.path.join(os.path.dirname(os.path.dirname(ROOT)), 'X64', 'Release')
for path in (ROOT, HERE, BUILD):
    if path not in sys.path:
        sys.path.insert(0, path)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import app as cadapp      # noqa: E402
import uiauto             # noqa: E402

p = cadapp.p

# 工具条按钮 → 期望的命令名（FIT 是视图动作，单独测）
TOOLBAR = [
    ('tbLINE', 'LINE'), ('tbPLINE', 'PLINE'), ('tbCIRCLE', 'CIRCLE'),
    ('tbARC', 'ARC'), ('tbRECT', 'RECT'), ('tbHATCH', 'HATCH'),
    ('tbTEXT', 'TEXT'), ('tbDIM', 'DIM'), ('tbMOVE', 'MOVE'),
    ('tbCOPY', 'COPY'), ('tbROTATE', 'ROTATE'), ('tbERASE', 'ERASE'),
    ('tbSELECT', 'SELECT'),
]

NEEDED = (
    # 功能区 / 画布（容器本身不登记进 names，这里只验叶子控件）
    'cvRibbon', 'cvMain',
    # 特性面板
    'lblPropTitle', 'grpGeneral', 'grpGeo', 'grpView',
    'propCombo', 'propEdit', 'propApply',
    'lblLayerTitle', 'tblLayers',
    # 命令行 + 状态栏
    'lblCmd', 'txtCmd', 'btnRun', 'lblHint',
    'stlModel', 'btnGrid', 'btnMode', 'stlSpacer',
    'stlCoord', 'stlCount', 'stlZoom',
) + tuple(k for k, _ in TOOLBAR) + ('tbFIT',) \
  + tuple('pr%d%s' % (i, s) for i in range(4) for s in ('k', 'v')) \
  + tuple('ge%d%s' % (i, s) for i in range(8) for s in ('k', 'v')) \
  + tuple('vw%d%s' % (i, s) for i in range(3) for s in ('k', 'v'))


def run(show_shot=False):
    a = cadapp.App()
    ck = uiauto.Checker()
    ids = a.names

    # ---- 1 结构：外壳里的控件一个都不许少
    missing = [n for n in NEEDED if n not in ids]
    ck.add('外壳控件齐备', not missing, '缺 %s' % missing if missing else '%d 个控件' % len(NEEDED))

    # 工具条容器里确实是 14 个图标按钮（+2 分隔线）
    ck.add('工具条 14 个按钮', all(k in ids for k, _ in TOOLBAR) and 'tbFIT' in ids,
           '共 %d 个命令入口 + 缩放' % len(TOOLBAR))

    # ---- 2 工具条：逐个真实点击
    for name, cmd in TOOLBAR:
        a.cmds.cancel()
        uiauto.click(a.win, ids[name])
        p.pump(80)
        active = a.cmds.active.name if a.cmds.active else None
        ck.add('点击 %s → %s' % (name, cmd), active == cmd, 'active=%s' % active)
        a.cmds.cancel()

    a.cmds.cancel()
    uiauto.click(a.win, ids['tbFIT'])
    p.pump(120)
    ck.add('点击 tbFIT 缩放全图',
           a.win.get_text(ids['stlZoom']).endswith('%'),
           'chip=%s' % a.win.get_text(ids['stlZoom']))

    # ---- 3 状态栏胶囊
    g0 = a.grid_on
    uiauto.click(a.win, ids['btnGrid'])
    p.pump(120)
    ck.add('btnGrid 翻转栅格', a.grid_on != g0,
           'chip=%s' % a.win.get_text(ids['btnGrid']))
    m0 = a.point_mode
    uiauto.click(a.win, ids['btnMode'])
    p.pump(120)
    ck.add('btnMode 翻转左键模式', a.point_mode != m0,
           'chip=%s' % a.win.get_text(ids['btnMode']))
    a.point_mode = True

    # ---- 4 命令行：命令名 / 别名 / 中文名
    for typed, cmd in (('CIRCLE', 'CIRCLE'), ('L', 'LINE'), ('直线', 'LINE')):
        a.cmds.cancel()
        a.win.set_text(ids['txtCmd'], typed)
        uiauto.click(a.win, ids['btnRun'])
        p.pump(100)
        active = a.cmds.active.name if a.cmds.active else None
        hint = a.win.get_text(ids['lblHint'])
        ck.add('命令行输入 %s → %s' % (typed, cmd), active == cmd,
               'active=%s hint=%s' % (active, hint))
        a.cmds.cancel()
    a.win.set_text(ids['txtCmd'], '')

    # ---- 5 特性行：点键名进入编辑
    line = next((e for e in a.doc.entities if e.kind == 'line'), None)
    assert line is not None, '示例图里没有直线'
    a.selection = [line]
    a._show_props(line)
    p.pump(60)

    ck.eq('常规行键名', a.win.get_text(ids['pr0k']), '颜色')
    ck.eq('几何行键名', a.win.get_text(ids['ge0k']), '类型')
    ck.add('几何值已填', a.win.get_text(ids['ge1v']) != '', 'ge1v=%s' % a.win.get_text(ids['ge1v']))
    ck.eq('视图行键名', a.win.get_text(ids['vw0k']), '缩放')

    uiauto.click(a.win, ids['pr0k'])
    p.pump(100)
    ck.add('点颜色行 → 下拉编辑态',
           a.prop_field == 'color' and a.win.get_visible(ids['propCombo'])
           and a.win.get_visible(ids['propApply']),
           'field=%s combo=%s apply=%s' % (a.prop_field,
                                           a.win.get_visible(ids['propCombo']),
                                           a.win.get_visible(ids['propApply'])))

    # 下拉选色 → 应用 → 撤销
    a.win.set_cur_sel(ids['propCombo'], 3)
    uiauto.click(a.win, ids['propApply'])
    p.pump(80)
    ck.add('颜色写回 3', line.color == 3, 'color=%s' % line.color)
    a._undo()
    a._refresh_props()
    ck.add('颜色撤销还原', line.color == 0, 'color=%s' % line.color)

    # 几何值：点"起点"行走输入框
    uiauto.click(a.win, ids['ge1k'])
    p.pump(100)
    ck.add('点几何行 → 输入框编辑态',
           a.win.get_visible(ids['propEdit']) and a.prop_field is not None,
           'field=%s' % a.prop_field)
    a.win.set_text(ids['propEdit'], '1234.5')
    uiauto.click(a.win, ids['propApply'])
    p.pump(80)
    ck.add('几何数值写回 1234.5', abs(line.x1 - 1234.5) < 1e-6, 'x1=%.2f' % line.x1)
    a._undo()
    a._refresh_props()
    ck.add('几何数值撤销还原', abs(line.x1 - 1000.0) < 1e-6, 'x1=%.2f' % line.x1)

    # 编辑态收尾：没选择时下拉/输入框/应用按钮都要收起来
    a._show_props(None)
    p.pump(60)
    ck.add('无选择时编辑区收起',
           not a.win.get_visible(ids['propCombo']) and not a.win.get_visible(ids['propEdit'])
           and not a.win.get_visible(ids['propApply']),
           'combo=%s edit=%s apply=%s' % (a.win.get_visible(ids['propCombo']),
                                          a.win.get_visible(ids['propEdit']),
                                          a.win.get_visible(ids['propApply'])))

    # ---- 6 提示行 / 视图行 / 图层表
    a.cmds.cancel()
    a._hint('已创建 hatch')
    ck.eq('提示行文字', a.win.get_text(ids['lblHint']), '已创建 hatch')

    a.redraw()
    p.pump(60)
    ck.eq('视图行缩放值', a.win.get_text(ids['vw0k']), '缩放')
    rows = a.win.get_table_row_count(ids['tblLayers'])
    ck.add('图层表有数据行', rows >= 1, 'rows=%s' % rows)
    first = a.win.get_table_cell(ids['tblLayers'], 0, 0)
    ck.add('图层表首行名称', bool(first), 'cell=%s' % first)

    # ---- 7 功能区回传（XRibbonButton 没独立 HWND，验 Python 回调这一端）
    a.cmds.cancel()
    a._on_ribbon('ERASE')
    p.pump(80)
    ck.add('Ribbon 回传 ERASE', a.cmds.active is not None and a.cmds.active.name == 'ERASE',
           'active=%s' % (a.cmds.active.name if a.cmds.active else None))
    a.cmds.cancel()

    if show_shot:
        a.win.save_png(os.path.join(ROOT, 'cad2d_ui_test.png'))

    return ck.done()


if __name__ == '__main__':
    run(show_shot='--shot' in sys.argv)
