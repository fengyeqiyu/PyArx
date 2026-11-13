import io
P = r'G:\PyArx\tests\mfc_xaml\_diag_children.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("""u = ctypes.WinDLL('user32', use_last_error=True)""",
"""u = ctypes.WinDLL('user32', use_last_error=True)
u.GetWindow.argtypes = [wt.HWND, wt.UINT]
u.GetWindow.restype = wt.HWND
u.GetClassNameW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]
u.GetWindowLongPtrW.argtypes = [wt.HWND, ctypes.c_int]
u.GetWindowLongPtrW.restype = ctypes.c_ssize_t""")
s = s.replace("gid = ctypes.c_long(u.GetWindowLong(child, -12)).value     # GWL_ID",
              "gid = int(u.GetWindowLongPtrW(child, -12))                 # GWL_ID")
io.open(P, 'w', encoding='utf-8').write(s)
print('ok')
