import io
P = r'G:\PyArx\tests\mfc_xaml\pp_grid_test.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]",
              "user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]")
io.open(P,'w',encoding='utf-8').write(s)
print('ok')
