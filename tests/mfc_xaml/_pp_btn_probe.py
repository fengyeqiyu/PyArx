# -*- coding: utf-8 -*-
"""按钮事件 + 文件通道端到端探针：
递归找按钮子窗口（类 Button，控件 id 就是 B_* 常量），真发 BM_CLICK，
走宿主命令分发 → Python 回调 → 写/读文件。fileDialog 换固定路径，避免模态框阻塞。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

user32 = ctypes.WinDLL('user32', use_last_error=True)
PROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
user32.EnumChildWindows.argtypes = [wt.HWND, PROC, wt.LPARAM]
user32.GetWindowLongW.argtypes = [wt.HWND, ctypes.c_int]
user32.GetWindowLongW.restype = ctypes.c_long
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = wt.LPARAM

BM_CLICK = 0x00F5
OUTX = r'G:\PyArx\docs\_btn_out.xlsx'
OUTC = r'G:\PyArx\docs\_btn_out.csv'
SRC = r'G:\PyArx\docs\_dual.xlsx'

calls = []
pp_app.fileDialog = lambda save, title, flt, default_name='': (
    calls.append((save, title)) or (OUTX if 'xlsx' in (flt or '').lower() or save else SRC))


def findBtn(cid):
    hit = []

    def cb(h, l):
        if (user32.GetWindowLongW(h, -12) & 0xFFFF) == cid:
            hit.append(h)
        return True
    user32.EnumChildWindows(app.win.hwnd, PROC(cb), 0)
    return hit[0] if hit else 0


app = pp_app.App()
app.win.show()
M.pump(250)
app.onDualSource()
M.pump(250)

for f in (OUTX, OUTC):
    if os.path.exists(f):
        os.remove(f)


def click(cid, name):
    h = findBtn(cid)
    print('%-8s id=%d hwnd=%s' % (name, cid, h or '未找到'), flush=True)
    if not h:
        return None
    user32.SendMessageW(h, BM_CLICK, 0, 0)
    M.pump(400)
    return h


print('=== 导出 XLSX ===', flush=True)
click(pp_app.B_EXLSX, '导出XLSX')
print('fileDialog 调用:', calls[-1] if calls else None, flush=True)
print('文件:', os.path.exists(OUTX), os.path.getsize(OUTX) if os.path.exists(OUTX) else 0, flush=True)
if os.path.exists(OUTX):
    from openpyxl import load_workbook
    ws = load_workbook(OUTX).active
    print('openpyxl 回读: %s %dx%d A1=%s C7=%s E7=%r'
          % (ws.title, ws.max_row, ws.max_column, ws['A1'].value, ws['C7'].value, ws['E7'].value), flush=True)

print('=== 导出 CSV ===', flush=True)
pp_app.fileDialog = lambda save, title, flt, default_name='': (calls.append((save, title)) or OUTC)
click(pp_app.B_ECSV, '导出CSV')
print('文件:', os.path.exists(OUTC), flush=True)
if os.path.exists(OUTC):
    raw = open(OUTC, 'rb').read()
    print('BOM:', raw[:3], '首行:', raw.decode('utf-8-sig').splitlines()[0], flush=True)

print('=== 导入 XLSX ===', flush=True)
pp_app.fileDialog = lambda save, title, flt, default_name='': (calls.append((save, title)) or SRC)
app.nodes = []                       # 先清空，证明导入真的换了数据
click(pp_app.B_XLSX, '导入XLSX')
print('节点数:', len(app.nodes), flush=True)
print('双电源:', [(n['code'], n['upstream']) for n in app.nodes
                 if (n.get('upstream') or '') == 'TR1|TR2'], flush=True)
print('标题:', app.win.getText(pp_app.E_TITLE), flush=True)
print('探针结束', flush=True)
