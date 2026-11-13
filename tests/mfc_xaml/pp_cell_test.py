# -*- coding: utf-8 -*-
"""原地单元格编辑验收：双击某格 → EDIT 盖到该格 → 回车提交 / Esc 取消 / 换格失焦提交。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')

import PyMfcXaml as M
import pp_app
import pp_events as E

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]

ok_n, bad = 0, 0


def chk(name, cond, detail=''):
    global ok_n, bad
    if cond:
        ok_n += 1
        print('PASS %s %s' % (name, detail), flush=True)
    else:
        bad += 1
        print('FAIL %s %s' % (name, detail), flush=True)


def rect_of(hwnd):
    r = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    return (r.left, r.top, r.right - r.left, r.bottom - r.top)


app = pp_app.App()
app.win.show()
M.pump(200)
app.showEditor()
M.pump(400)

tbl = E.find_control(app.win.hwnd, pp_app.TBL)
row, col = 5, 1                     # 第 6 行「名称」列
name_before = app.nodes[row]['name']

rc = app.tblCellRect(row, col)
chk('能取到单元格矩形', bool(rc) and rc[2] > 30 and rc[3] > 10, str(rc))

cx, cy = rc[0] + rc[2] // 2, rc[1] + rc[3] // 2
E.click(tbl, cx, cy)
M.pump(120)
E.double_click(tbl, cx, cy)
M.pump(200)

ce = getattr(app, '_ce', None)
chk('双击后进入原地编辑', bool(ce), repr(ce))
if ce:
    eid = ce[2]
    chk('编辑框 id 与列对应', eid == pp_app.E_F0 + col, 'eid=%s col=%d' % (eid, col))
    eh = E.find_control(app.win.hwnd, eid)
    chk('编辑框可见', eh is not None and bool(user32.IsWindowVisible(eh)), repr(eh))
    if eh:
        tx, ty = app._tblPos
        e_rc = rect_of(eh)
        t_rc = rect_of(tbl)
        want_x = t_rc[0] + rc[0]
        want_y = t_rc[1] + rc[1]
        chk('编辑框精确盖在单元格上',
            abs(e_rc[0] - want_x) <= 2 and abs(e_rc[1] - want_y) <= 2,
            'EDIT=(%d,%d) 期望=(%d,%d)' % (e_rc[0], e_rc[1], want_x, want_y))
        chk('编辑框初值是该格原值',
            (app.win.getText(eid) or '') == (name_before or ''),
            '%r vs %r' % (app.win.getText(eid), name_before))

        # 输入新值 + 回车提交
        app.win.setText(eid, '')
        M.pump(60)
        E.type_text(eh, 'INPLACE-1')
        M.pump(120)
        print('输入后 EDIT 文本 = %r' % (app.win.getText(eid),), flush=True)
        E.key(eh, 0x0D)             # VK_RETURN
        M.pump(200)
        chk('回车后退出编辑态', getattr(app, '_ce', None) is None, repr(app._ce))
        chk('回车后数据已写回', app.nodes[row]['name'] == 'INPLACE-1',
            repr(app.nodes[row]['name']))
        chk('回车后表格格已刷新',
            (app.tblGet(row, col) or '') == 'INPLACE-1',
            repr(app.tblGet(row, col)))

        # 再双击，Esc 取消
        rc2 = app.tblCellRect(row, col)
        E.double_click(tbl, rc2[0] + rc2[2] // 2, rc2[1] + rc2[3] // 2)
        M.pump(200)
        ce2 = getattr(app, '_ce', None)
        chk('再次双击重新进入编辑', bool(ce2), repr(ce2))
        if ce2:
            e2 = ce2[2]
            eh2 = E.find_control(app.win.hwnd, e2)
            app.win.setText(e2, 'SHOULD-NOT-SAVE')
            M.pump(80)
            E.key(eh2, 0x1B)        # VK_ESCAPE
            M.pump(200)
            chk('Esc 取消不写回', app.nodes[row]['name'] == 'INPLACE-1',
                repr(app.nodes[row]['name']))
            chk('Esc 后表格值未被改',
                (app.tblGet(row, col) or '') == 'INPLACE-1',
                repr(app.tblGet(row, col)))

        # 双击另一列 → 编辑框跟着换
        rc3 = app.tblCellRect(row, 3)
        E.double_click(tbl, rc3[0] + rc3[2] // 2, rc3[1] + rc3[3] // 2)
        M.pump(220)
        ce3 = getattr(app, '_ce', None)
        chk('双击换格后编辑框跟着换列', bool(ce3) and ce3[1] == 3, repr(ce3))
        if ce3:
            e3 = ce3[2]
            eh3 = E.find_control(app.win.hwnd, e3)
            app.win.setText(e3, 'YJV22-4x70')
            M.pump(60)
            # 点表格另一行 → 失焦提交
            rc4 = app.tblCellRect(row + 1, 3)
            E.click(tbl, rc4[0] + rc4[2] // 2, rc4[1] + rc4[3] // 2)
            M.pump(220)
            chk('点别处（失焦）自动提交',
                (app.nodes[row]['cable'] or '') == 'YJV22-4x70',
                repr(app.nodes[row]['cable']))
            chk('失焦提交后编辑框已隐藏',
                not bool(user32.IsWindowVisible(eh3)) if eh3 else True, '')

print('=== %d/%d PASS ===' % (ok_n, ok_n + bad), flush=True)
if bad == 0:
    try:
        app.win.savePng(r'G:\PyArx\docs\_cell_edit.png')
        print('截图 _cell_edit.png', flush=True)
    except Exception as e:
        print('截图失败 %s' % e, flush=True)
os._exit(0)
