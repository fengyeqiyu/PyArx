import io
P = r'G:\PyArx\tests\mfc_xaml\pp_grid_test.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("""user32.PostMessageW(lv, WM_LBUTTONDOWN, 1, (12) | (24 + 3 * 21 << 16))   # 点第 4 行第 0 列
M.pump(60)""",
"""app.win.setTableSelectedRow(pp_app.TBL, 3)      # 等价于用户点了第 4 行
M.pump(60)""")
s = s.replace("user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 1600, 1000, 0x0001 | 0x0040)",
              "user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 1600, 1000, 0x0004 | 0x0040)")
s = s.replace("user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 1360, 900, 0x0040)",
              "user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 1360, 900, 0x0004 | 0x0040)")
s = s.replace("""app.win.hide()
print('done', flush=True)""", """print('done', flush=True)
os._exit(0)""")
io.open(P,'w',encoding='utf-8').write(s)
print('测试修正')
