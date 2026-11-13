# -*- coding: utf-8 -*-
"""ppApp 第二批验收：CSV / XLSX / JSON 导入、标题改名、缩放按钮、折叠位、剪贴板只读。

文件对话框（GetOpenFileName）无法自动点，所以对话框之后的逻辑直接喂内容测；
对话框本身单独用 ctypes 调一次验证"能返回路径即可"不在本脚本范围。
"""
import ctypes
import os
import sys
import time
import zipfile

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M
import pp_app

DOCS = r'G:\PyArx\docs'
NS = 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'


def make_xlsx(path):
    """写一个最小 xlsx（inlineStr 单元格），Excel 能打开、本工程 reader 也能读。"""
    rows = [
        ['编号', '名称', '上游', '电缆型号', '负载kW', '电压', '长度m'],
        ['TR9', '9号变压器', '', '', '0', '10kV', '0'],
        ['AT9', '9号柜', 'TR9', 'YJV22-4×95', '0', '380', '15'],
        ['AT9-1', '9号柜1路', 'AT9', 'YJV22-4×35', '9', '380', '6'],
        ['AT9-1-1', '末端', 'AT9-1', 'BV-3×4', '3', '220', '4'],
    ]
    xml = ['<?xml version="1.0" encoding="UTF-8" standalone="yes"?>',
           '<worksheet xmlns="%s"><sheetData>' % NS]
    for r, row in enumerate(rows, start=1):
        cells = []
        for c, v in enumerate(row):
            ref = '%s%d' % (chr(ord('A') + c), r)
            cells.append('<c r="%s" t="inlineStr"><is><t>%s</t></is></c>' % (ref, v))
        xml.append('<row r="%d">%s</row>' % (r, ''.join(cells)))
    xml.append('</sheetData></worksheet>')
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
        z.writestr('xl/worksheets/sheet1.xml', ''.join(xml))


def main():
    app = pp_app.App()
    app.win.show()
    time.sleep(0.4)
    M.pump(30)

    # 1) CSV 导入
    csv_text = ('编号,名称,上游,电缆型号,负载kW,电压,长度m\r\n'
                'TR7,7号变压器,,,0,10kV,0\r\n'
                'AT7,7号柜,TR7,YJV22-4×95,0,380,10\r\n'
                'AT7-1,7号柜1路,AT7,YJV22-4×35,8,380,5\r\n'
                'AT7-1-1,末端1,AT7-1,BV-3×4,2,220,3\r\n')
    app.onTitleChange.__self__  # 占位，保持引用
    app.loadNodes = app.loadNodes
    app.importCsvText(csv_text, 'CSV 导入')
    time.sleep(0.3); M.pump(30)
    print('[CSV]  节点 %d  尺寸 %dx%d  标题 %r' %
          (len(app.view['nodes']), app.view['width'], app.view['height'],
           app.win.getText(pp_app.E_TITLE)))
    app.win.savePng(os.path.join(DOCS, '_pp_app_csv.png'))

    # 2) XLSX 导入
    xp = os.path.join(DOCS, '_pp_app_test.xlsx')
    make_xlsx(xp)
    try:
        rows = pp_app.readXlsx(xp)
        print('[XLSX] 读到 %d 行，首行 = %s' % (len(rows), rows[0] if rows else '空'))
        app.importCsvText(pp_app.rowsToCsv(rows), 'XLSX 导入')
        time.sleep(0.3); M.pump(30)
        print('       节点 %d  尺寸 %dx%d' % (len(app.view['nodes']),
                                            app.view['width'], app.view['height']))
        app.win.savePng(os.path.join(DOCS, '_pp_app_xlsx.png'))
    except Exception as e:
        print('[XLSX] 失败：%s' % e)

    # 3) JSON 导入（对话框之后走的就是这条）
    app.importJsonText(app.view['json'], 'JSON 回灌')
    time.sleep(0.3); M.pump(30)
    print('[JSON] 节点 %d  尺寸 %dx%d' % (len(app.view['nodes']),
                                       app.view['width'], app.view['height']))

    # 4) 标题改名 → rebuild
    app.win.setText(pp_app.E_TITLE, '改过名的配电图')
    app.onTitleChange()
    time.sleep(0.2); M.pump(30)
    print('[改名] 标题 %r  节点 %d' % (app.title(), len(app.view['nodes'])))

    # 5) 缩放按钮（B_ZIN / B_ZOUT / B_Z100 / 适应窗口 的回调体）
    before = app.win.getPlanView(pp_app.CV)[0]
    app.zoomAt(1.25)
    after_in = app.win.getPlanView(pp_app.CV)[0]
    app.zoomAt(1.0 / 1.25)
    app.setZoom(1.0)
    app.fitView()
    print('[缩放] %.3f -> 放大 %.3f -> 100%% -> 适应 %.3f' %
          (before, after_in, app.win.getPlanView(pp_app.CV)[0]))

    # 6) 剪贴板只读（不改用户剪贴板）
    txt = pp_app.clipboardText()
    print('[剪贴板] 读到 %d 字符（只读，未改写）：%r' % (len(txt), txt[:40]))

    # 7) 折叠位：折叠 TR9（全树）后应该只剩 1 个节点
    app.curSel = 'TR9'
    app.win.selectPlan(pp_app.CV, 'TR9')
    app.onCanvasDblclick()
    time.sleep(0.3); M.pump(30)
    print('[折叠根] 节点数 %d（应 1）' % len(app.view['nodes']))
    app.win.savePng(os.path.join(DOCS, '_pp_app_collapse_root.png'))
    app.onCanvasDblclick()
    time.sleep(0.2); M.pump(30)
    print('[展开根] 节点数 %d' % len(app.view['nodes']))

    app.win.close()
    print('第二批验收完成')


if __name__ == '__main__':
    main()
