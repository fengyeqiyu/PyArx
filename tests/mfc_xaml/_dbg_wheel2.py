import ctypes, ctypes.wintypes as wt, os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M, ppApp
user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes=[wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = wt.LPARAM
user32.ClientToScreen.argtypes=[wt.HWND, ctypes.POINTER(wt.POINT)]

app = ppApp.App(); app.win.show(); time.sleep(0.6); M.pump(40)
app.showEditor(); M.pump(80)
# 包一层计数
calls = []
orig = app.on_edit_wheel
def spy(d):
    calls.append(d)
    return orig(d)
app.on_edit_wheel = spy
app.win.onWheel(spy)          # C++ 里存的是原回调对象，重新注册 spy
M.pump(20)
hwnd = app.win.hwnd
rx, ry, w, h = app._grid_body_rect()
pt = wt.POINT(rx+150, ry+150); user32.ClientToScreen(hwnd, ctypes.byref(pt))
lp = (pt.x & 0xFFFF) | ((pt.y & 0xFFFF) << 16)
user32.SendMessageW(hwnd, 0x020A, (120 & 0xFFFF) << 16, lp)   # 向上 = -1 档
M.pump(30)
print('一次向上滚轮的 Python 回调次数 =', len(calls), ' 参数 =', calls, ' 起点 =', app._scroll_row)
print('step =', app.win.getWheelStep() if hasattr(app.win, 'getWheelStep') else '(无读接口)')
