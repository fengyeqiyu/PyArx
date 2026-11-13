# -*- coding: utf-8 -*-
"""真文件浏览器探针：用真 comdlg32 对话框（不替换 fileDialog）。
1) 真点「导出 XLSX」按钮 → 应弹出保存对话框（外部脚本截图取证后关掉）
2) 直接调 fileDialog 再弹一次，打印返回值（应为空=取消）"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

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
print('app ready', flush=True)


def stage(msg):
    print('[%s] %s' % (time.strftime('%H:%M:%S'), msg), flush=True)


h = findBtn(pp_app.B_EXLSX)
stage('点按钮 hwnd=%s（对话框应弹出，等待外部关闭）' % h)
user32.SendMessageW(h, BM_CLICK, 0, 0)      # 这里会阻塞在 GetSaveFileNameW 里
stage('按钮回调已返回（对话框被关闭/确认）')
print('状态栏:', app.win.getText(pp_app.L_STATUS), flush=True)

stage('第二次：直接弹「导入 CSV」浏览器，5 秒内请外部关掉')
p = pp_app.fileDialog(False, '导入 CSV', 'CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*',
                      owner=app.win.hwnd)
stage('直接调用返回: %r' % p)
print('探针结束', flush=True)
