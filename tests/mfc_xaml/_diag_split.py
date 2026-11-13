import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
print('直接调用: down(3) / move(-57)')
app.onSplitDrag('down', 3); app.onSplitDrag('move', -57)
print(' edW =', app._edW)
orig = app.onSplitDrag
def spy(phase, x):
    print('  回调 phase=%r x=%r edW=%d' % (phase, x, app._edW))
    orig(phase, x)
app.onSplitDrag = spy
sp = E.find_control(app.win.hwnd, ppApp.SPLIT)
app.win.onDrag(ppApp.SPLIT, spy); M.pump(50)
print('真实拖动 -60')
E.drag(sp, 3, 200, 3-60, 200, steps=3); M.pump(120)
print('拖后 edW =', app._edW)
os._exit(0)
