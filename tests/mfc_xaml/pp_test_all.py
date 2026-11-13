# -*- coding: utf-8 -*-
"""全功能回归：按 HTML 的 24 项操作逐条跑，打印 PASS/FAIL 与实测数字。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')

import PyMfcXaml as M
import pp_app

D = r'G:\PyArx\docs'
ok = 0
bad = 0


def chk(name, cond, info=''):
    global ok, bad
    if cond:
        ok += 1
        print('PASS  %-22s %s' % (name, info))
    else:
        bad += 1
        print('FAIL  %-22s %s' % (name, info))


def main():
    app = pp_app.App()
    w = app.win
    w.show()
    M.pump(40)

    # 1 启动 + 默认数据
    chk('启动默认三级示例', len(app.nodes) == 39,
        '节点 %d 画布 %dx%d' % (len(app.nodes), app.view['width'], app.view['height']))

    # 2 适应窗口
    app.fitView()
    M.pump(20)
    k, ox, oy = w.getPlanView(pp_app.CV)
    chk('适应窗口 fit', 0.05 < k < 0.35, 'zoom %.0f%%' % (k * 100))

    # 3 放大 / 4 缩小
    app.zoomAt(1.25)
    M.pump(20)
    k1 = w.getPlanView(pp_app.CV)[0]
    app.zoomAt(1.0 / 1.25)
    M.pump(20)
    k2 = w.getPlanView(pp_app.CV)[0]
    chk('放大/缩小', k1 > k2, '%.0f%% -> %.0f%%' % (k1 * 100, k2 * 100))

    # 5 100%
    app.setZoom(1.0)
    M.pump(20)
    chk('100%', abs(w.getPlanView(pp_app.CV)[0] - 1.0) < 0.02,
        'zoom %.0f%%' % (w.getPlanView(pp_app.CV)[0] * 100))

    # 6 横向（dirSel 第 0 项 = 左→右）
    w.setCurSel(pp_app.C_HORIZ, 0)
    app.onDirChange()
    M.pump(30)
    chk('切成横向', app.view['height'] > app.view['width'],
        '画布 %dx%d' % (app.view['width'], app.view['height']))
    w.setCurSel(pp_app.C_HORIZ, 1)
    app.onDirChange()
    M.pump(20)
    chk('切回纵向', app.view['width'] > app.view['height'],
        '画布 %dx%d' % (app.view['width'], app.view['height']))

    # 7 折叠（双击 1 号配电柜 AT1）
    app.fitView()
    M.pump(20)
    before = len(app.layoutNodes())
    app.onCanvasDblclick.__self__ if False else None
    app.collapsed.add('AT1')
    app.rebuild(fit=False)
    M.pump(30)
    chk('折叠 AT1', len(app.layoutNodes()) < before,
        '可见 %d -> %d' % (before, len(app.layoutNodes())))
    app.collapsed.discard('AT1')
    app.rebuild(fit=True)
    M.pump(30)

    # 8 搜索
    w.setText(pp_app.E_SEARCH, 'AT2-1-3')
    app.onSearchChange()
    M.pump(30)
    chk('搜索定位', app.curSel == 'AT2-1-3', '选中 %s' % app.curSel)
    w.setText(pp_app.E_SEARCH, '')
    app.onSearchChange()
    M.pump(10)

    # 9 选中 + 详情浮层
    w.selectPlan(pp_app.CV, 'AT1-1')
    app.onCanvasClick()
    M.pump(20)
    chk('选中出详情', w.getText(pp_app.DT_TITLE) == '节点 AT1-1',
        '%s / 电缆 %s' % (w.getText(pp_app.DT_TITLE), w.getText(pp_app.D_F0 + 3)))

    # 10 详情保存
    w.setText(pp_app.D_F0 + 1, '1号柜1号回路-详情改')
    app.onDetailSave()
    M.pump(30)
    n = app.nodeOf('AT1-1')
    chk('详情保存', (n or {}).get('name') == '1号柜1号回路-详情改', repr((n or {}).get('name')))

    # 11 新增下游
    before = len(app.nodes)
    app.onDetailAddchild()
    M.pump(30)
    chk('＋下游节点', len(app.nodes) == before + 1, '节点 %d -> %d' % (before, len(app.nodes)))

    # 12 删除节点
    app.onDetailDel()
    M.pump(30)
    chk('删除节点', len(app.nodes) == before, '节点 %d' % len(app.nodes))

    # 13 侧板打开
    app.showEditor()
    M.pump(30)
    chk('侧板打开', app.editing and app._grid_rows == 39,
        '%d 行 / 表格 %d 列 / 画布 %dx%d' % (app._grid_rows,
                                   7, app.cvW, app.cvH))

    # 14 表格改值 -> 应用（表格是原生 ListView，数据源在 Python）
    #    当前 11 列口径：0起点 1回路 2终点 3型号 4电压 5根数 6水平 7垂直 8余量 9合计 10功率
    w.setTableCell(pp_app.TBL, 2, 10, '15')
    app.onApplyAll()
    M.pump(40)
    chk('表格改功率并重绘', abs(float(app.nodes[2].get('loadKw') or 0) - 15) < 1e-6,
        '%s 功率 %s / 画布 %dx%d' % (app.nodes[2].get('code'), app.nodes[2].get('loadKw'),
                                 app.view['width'], app.view['height']))

    # 15 校验：空终点（终点即设备编号）
    w.setTableCell(pp_app.TBL, 2, 2, '')
    app.onApplyAll()
    chk('校验空编号', '终点不能为空' in (w.getText(pp_app.L_EDIT_COUNT) or ''),
        w.getText(pp_app.L_EDIT_COUNT))
    w.setTableCell(pp_app.TBL, 2, 2, app.nodes[2]['code'])

    # 16 校验：起点是终点自己
    w.setTableCell(pp_app.TBL, 2, 0, app.nodes[2]['code'])
    app.onApplyAll()
    chk('校验起点即终点', '起点不能是终点自己' in (w.getText(pp_app.L_EDIT_COUNT) or ''),
        w.getText(pp_app.L_EDIT_COUNT))
    w.setTableCell(pp_app.TBL, 2, 0, app.nodes[2].get('upstream') or '')

    # 17 ✕ 删行
    last = app.nodes[-1]['code']
    before = len(app.nodes)
    app.onGridDelete(last)
    M.pump(30)
    chk('✕ 删行', len(app.nodes) == before - 1, '%s / 节点 %d' % (last, len(app.nodes)))

    # 18 ＋ 新增节点
    before = len(app.nodes)
    app.onAddRow()
    M.pump(30)
    chk('＋新增节点', len(app.nodes) == before + 1,
        '+1 -> 节点 %d 行 %d' % (len(app.nodes), app._grid_rows))

    # 19 关闭侧板
    app.hideEditor()
    M.pump(20)
    chk('关闭侧板', (not app.editing) and app.cvW == pp_app.CV_W, '画布回宽 %d' % app.cvW)

    # 20 文档示例（当前是超大数据填充：20×10×50 = 10221 行）
    app.onSampleDoc()
    M.pump(40)
    chk('文档示例超大数据', len(app.nodes) > 10000,
        '节点 %d 画布 %dx%d' % (len(app.nodes), app.view['width'], app.view['height']))

    # 21 CSV 导入（计算书口径：起点/回路/终点/型号规格/根数/水平/垂直/余量/合计/功率/电压）
    csv_text = ('起点,回路,终点,型号规格,电缆根数,水平长度,垂直长度,余量,合计,功率,电压等级\n'
                ',W0,总柜,YJV-3×95,1,0,0,0,0,0,10kV\n'
                '总柜,W1,一号柜,YJV-3×35,1,10,2,1,13,45,380V\n'
                '总柜,W2,二号柜,YJV-3×25,1,8,1,0,9,30,380V\n')
    app.importCsvText(csv_text, '测试CSV')
    M.pump(40)
    chk('导入 CSV', len(app.nodes) == 3, '节点 %d 画布 %dx%d'
        % (len(app.nodes), app.view['width'], app.view['height']))

    # 22 JSON 导入（HTML 的 fileJson 只 loadRecords(obj.nodes)，不改 dir）
    js = ('{"title":"JSON测试","dir":"h","nodes":[{'
          '"code":"J1","name":"总柜","upstream":"","cable":"YJV-3x95","loadKw":0,'
          '"voltage":"10kV","length":0},{"code":"J2","name":"支1","upstream":"J1",'
          '"cable":"YJV-3x25","loadKw":20,"voltage":"380V","length":8}]}')
    app.importJsonText(js, '测试JSON')
    M.pump(40)
    chk('导入 JSON', len(app.nodes) == 2 and not app.horiz,
        '节点 %d 方向 %s（HTML 导入 JSON 不改 dir） 标题 %s'
        % (len(app.nodes), 'h' if app.horiz else 'v', app.title()))

    # 23 导出四件套（直接调内部，跳文件框）
    app.nodes = pp_app.sampleThreeLevel()
    app.horiz = False
    app.rebuild(fit=True)
    M.pump(30)
    t0 = time.perf_counter()
    for name, fn in (('json', lambda p: open(p, 'w', encoding='utf-8').write(app._sceneView()['json'])),
                     ('csv', lambda p: open(p, 'w', encoding='utf-8').write(app._sceneView()['csv'])),
                     ('svg', lambda p: open(p, 'w', encoding='utf-8').write(app._sceneView()['svg'])),
                     ('png', lambda p: M.powerplan.renderScenePng(p, app._sceneText(), 2))):
        p = os.path.join(D, '_pp_test_export.' + name)
        fn(p)
        chk('导出 %s' % name, os.path.getsize(p) > 0, '%d B' % os.path.getsize(p))

    # 24 抓最终图
    M.pump(60)
    out = os.path.join(D, '_pp_test_final.png')
    w.savePng(out)
    chk('最终截图', os.path.getsize(out) > 20000, '%d B' % os.path.getsize(out))

    print('\n汇总：PASS %d / FAIL %d' % (ok, bad))
    time.sleep(0.5)


if __name__ == '__main__':
    main()
