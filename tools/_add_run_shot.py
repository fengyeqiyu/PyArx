import io
P = r'G:\PyArx\tests\mfc_xaml\pp_run.py'
s = io.open(P, encoding='utf-8-sig').read()
s = s.replace("""while user32.IsWindow(hwnd):
    M.pump(50)""",
"""if '--shot' in sys.argv:
    time.sleep(0.3)
    user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
    M.pump(400)
    app.win.savePng(r'G:\\PyArx\\docs\\_pp_live.png')
    M.pump(80)
    print('已截图 docs/_pp_live.png', flush=True)

while user32.IsWindow(hwnd):
    M.pump(50)""")
io.open(P,'w',encoding='utf-8-sig',newline='').write(s)
print('ok')
