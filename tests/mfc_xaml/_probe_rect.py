import os, sys, ctypes, ctypes.wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
u=ctypes.windll.user32; app=ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(300); app.fillDetail(app.nodes[0]['code']); M.pump(300)
hw=E.find_control(app.win.hwnd, ppApp.D_VOL); rc=wt.RECT(); u.GetWindowRect(hw, ctypes.byref(rc)); pt=wt.POINT(rc.left,rc.top); u.MapWindowPoints(0, app.win.hwnd, ctypes.byref(pt), 1); print('hwnd',hw,'screen',rc.left,rc.top,rc.right,rc.bottom,'parent',pt.x,pt.y,'w,h',rc.right-rc.left,rc.bottom-rc.top)

