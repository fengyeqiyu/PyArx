import ctypes, sys, time
from ctypes import wintypes
import PyMfcXaml as p

u = ctypes.windll.user32
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]


class RECT(ctypes.Structure):
    _fields_ = [('left', ctypes.c_long), ('top', ctypes.c_long),
                ('right', ctypes.c_long), ('bottom', ctypes.c_long)]


def by_id(hwnd, cid):
    got = []
    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(h, _):
        if u.GetDlgCtrlID(h) == cid:
            got.append(h)
        return True
    u.EnumChildWindows(hwnd, EP(cb), 0)
    return got[0] if got else None


names = {}
w = p.guiload(r'G:\PyArx\tests\mfc_controls\_fix1.xaml', names=names)
w.show()
p.pump(300)
u.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, ctypes.c_int, ctypes.c_uint]
u.SetWindowPos(w.hwnd, ctypes.c_void_p(-1), 40, 40, 660, 940, 0x0040 | 0x0002)
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]
u.SetForegroundWindow(w.hwnd)
p.pump(400)

r = RECT()
u.GetWindowRect(by_id(w.hwnd, names['prgLoad']), ctypes.byref(r))
print('PRG %d %d %d %d' % (r.left, r.top, r.right, r.bottom), flush=True)
r2 = RECT()
u.GetWindowRect(by_id(w.hwnd, names['chkFlag']), ctypes.byref(r2))
print('CHK %d %d %d %d' % (r2.left, r2.top, r2.right, r2.bottom), flush=True)

w.setPos(names['prgLoad'], 5)
w.setPos(names['sldZoom'], 5)
w.setCheck(names['chkFlag'], False)
p.pump(300)
print('STATE_A', flush=True)
time.sleep(6)

w.setPos(names['prgLoad'], 95)
w.setPos(names['sldZoom'], 95)
w.setCheck(names['chkFlag'], True)
p.pump(400)
print('STATE_B', flush=True)
time.sleep(6)

w.close()
p.pump(200)
p.shutdown()
