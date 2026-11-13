import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
print('edW 初始', app._edW)
sp = E.find_control(app.win.hwnd, ppApp.SPLIT)
for tag, x1, x2 in (('左拖60', 3, -60), ('右拖120', 3, 120)):
    print(tag); sys.stdout.flush()
    E.drag(sp, x1, 200, x2, 200, steps=3); M.pump(120)
    print('  edW =', app._edW, 'cvW =', app.cvW, 'edX =', app._edRc[0])
    sys.stdout.flush()
os._exit(0)
