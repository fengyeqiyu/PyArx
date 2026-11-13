import ctypes as C, ctypes.wintypes as wt, time
u = C.windll.user32
u.FindWindowW.restype = wt.HWND
h = u.FindWindowW(None, u'配电规划图 — MFCXaml')
u.SetWindowPos.argtypes=[wt.HWND,wt.HWND,C.c_int,C.c_int,C.c_int,C.c_int,wt.UINT]
u.SetWindowPos(h, wt.HWND(-1), 100, 100, 0, 0, 0x0001|0x0040)
u.SetForegroundWindow.argtypes=[wt.HWND]; u.SetForegroundWindow(h)
time.sleep(1.2)
print('hwnd', h)
