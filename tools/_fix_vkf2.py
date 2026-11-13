import io
for P in (r'G:\PyArx\tests\mfc_xaml\pp_grid_test.py', r'G:\PyArx\tests\mfc_xaml\_diag_grid2.py', r'G:\PyArx\tests\mfc_xaml\_diag_grid.py'):
    s = io.open(P, encoding='utf-8').read()
    s = s.replace('VK_F2, VK_INSERT, VK_DELETE = 0x72, 0x2D, 0x2E', 'VK_F2, VK_INSERT, VK_DELETE = 0x71, 0x2D, 0x2E')
    s = s.replace("('F2', 0x72)", "('F2', 0x71)")
    s = s.replace('user32.PostMessageW(lv, 0x0100, 0x72, 0)', 'user32.PostMessageW(lv, 0x0100, 0x71, 0)')
    s = s.replace('user32.PostMessageW(lv, WM_KEYDOWN, VK_F2, 0)', 'user32.PostMessageW(lv, WM_KEYDOWN, VK_F2, 0)  # VK_F2=0x71')
    io.open(P, 'w', encoding='utf-8').write(s)
print('ok')
