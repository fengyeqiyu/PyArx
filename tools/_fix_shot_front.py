import io
P = r'G:\PyArx\tests\mfc_xaml\pp_scroll_test.py'
s = io.open(P, encoding='utf-8-sig').read()
old = '''def front(hwnd):
    """savePng 抓的是屏幕，窗口被别的程序压住就会抓到别人，截图前先提到前台。"""
    user32.SetForegroundWindow.argtypes = [wt.HWND]
    user32.SetForegroundWindow(hwnd)
    user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                    ctypes.c_int, ctypes.c_int, wt.UINT]
    user32.SetWindowPos(hwnd, None, 60, 40, 0, 0, 0x0001 | 0x0004)   # NOSIZE|NOZORDER
    time.sleep(0.35)
'''
new = '''HWND_TOPMOST = -1
HWND_NOTOPMOST = -2


def front(hwnd, topmost=True):
    """savePng 抓的是屏幕，窗口被别的程序压住就会抓到别人。
    只 SetForegroundWindow 抢不过浏览器/Word，直接临时置顶最可靠。"""
    user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                    ctypes.c_int, ctypes.c_int, wt.UINT]
    user32.SetWindowPos.restype = wt.BOOL
    user32.SetForegroundWindow.argtypes = [wt.HWND]
    user32.SetForegroundWindow(hwnd)
    flag = HWND_TOPMOST if topmost else HWND_NOTOPMOST
    user32.SetWindowPos(hwnd, flag, 60, 40, 0, 0, 0x0001 | 0x0002 | 0x0040)  # NOSIZE|NOMOVE? 用 NOSIZE 保持尺寸
    time.sleep(0.45)
'''
assert s.count(old) == 1
s = s.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(s)
print('ok')
