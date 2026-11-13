import ctypes as C, ctypes.wintypes as wt, os
u = C.windll.user32
u.FindWindowW.restype = wt.HWND
u.FindWindowW.argtypes = [wt.LPCWSTR, wt.LPCWSTR]
u.GetWindowRect.argtypes = [wt.HWND, C.POINTER(wt.RECT)]
h = u.FindWindowW(None, u'配电规划图 — MFCXaml')
print('hwnd', h)
rc = wt.RECT(); u.GetWindowRect(h, C.byref(rc))
wd, ht = rc.right-rc.left, rc.bottom-rc.top
print('rect', rc.left, rc.top, wd, ht)
import subprocess
