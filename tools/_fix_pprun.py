import io
P = r'G:\PyArx\tests\mfc_xaml\pp_run.py'
s = io.open(P, encoding='utf-8-sig').read()
s = s.replace("""print('窗口已打开：侧板一屏 %d 行 / 共 %d 行，滚轮试滚动；关闭窗口即退出。'
      % (app._grid_vis, app._grid_rows), flush=True)""",
"""print('窗口已打开：表格共 %d 行（原生 ListView，自带滚动条），关闭窗口即退出。'
      % app._grid_rows, flush=True)""")
io.open(P,'w',encoding='utf-8-sig',newline='').write(s)
print('pp_run 已修')
