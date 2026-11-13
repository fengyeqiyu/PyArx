# -*- coding: utf-8 -*-
import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M
import pp_app
import pp_events as E

app = pp_app.App(); app.win.show(); M.pump(120); app.showEditor(); M.pump(300)
cv = E.find_control(app.win.hwnd, pp_app.CV)
z0 = app.win.getPlanView(pp_app.CV)[0]
traj = [('start', z0)]
for i in range(5):
    E.wheel(cv, 120, 300, 300, pump=30)
    traj.append(('up%d' % (i+1), app.win.getPlanView(pp_app.CV)[0]))
for i in range(5):
    E.wheel(cv, -120, 300, 300, pump=30)
    traj.append(('down%d' % (i+1), app.win.getPlanView(pp_app.CV)[0]))
for name, z in traj:
    print('%s %.6f' % (name, z), flush=True)
print('delta=%.6f' % (traj[-1][1] - z0), flush=True)
os._exit(0)
