import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app, pp_events as E

orig = pp_app.App.onSplitDrag
def traced(self, phase, x):
    print('drag %s x=%d  ed_w_pref=%d eff=%d split_from=%s' % (
        phase, x, self._edW, self._sideWidth(), getattr(self, '_split_from', None)), flush=True)
    orig(self, phase, x)
pp_app.App.onSplitDrag = traced

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
sp = E.find_control(app.win.hwnd, pp_app.SPLIT)
print('bar screen x0 =', end=' ', flush=True)
import ctypes as C
u32 = C.windll.user32
class RC(C.Structure):
    _fields = [('l', C.c_long), ('t', C.c_long), ('r', C.c_long), ('b', C.c_long)]
rc = RC(); u32.GetWindowRect(C.cVoidP(sp), C.byref(rc)); print(rc.l, flush=True)
E.drag(sp, 3, 200, 3 - 60, 200, steps=3)
M.pump(200)
print('after: pref=%d eff=%d cvW=%d' % (app._edW, app._sideWidth(), app.cvW), flush=True)
os._exit(0)
