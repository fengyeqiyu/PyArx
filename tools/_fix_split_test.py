import io
P = r'G:\PyArx\tests\mfc_xaml\pp_split_test.py'
s = io.open(P, encoding='utf-8').read()
head = """ok_n, bad = 0, 0"""
head_new = """import ctypes as C

u32 = C.windll.user32


class _RC(C.Structure):
    _fields_ = [('l', C.c_long), ('t', C.c_long), ('r', C.c_long), ('b', C.c_long)]


def rect(hwnd):
    \"\"\"控件矩形，相对主窗客户区（x, y, w, h）。\"\"\"
    rc = _RC()
    u32.GetWindowRect(C.c_void_p(hwnd), C.byref(rc))
    pt = (C.c_long(rc.l), C.c_long(rc.t))
    u32.ScreenToClient(C.c_void_p(app.win.hwnd), C.byref(pt[0]))
    u32.ScreenToClient(C.c_void_p(app.win.hwnd), C.byref(pt[1]))
    return (pt[0].value, pt[1].value, rc.r - rc.l, rc.b - rc.t)


ok_n, bad = 0, 0"""
assert s.count(head) == 1
s = s.replace(head, head_new)

s = s.replace("sx, sy, sw, sh = w.getPos(pp_app.SPLIT)", "sx, sy, sw, sh = rect(sp)")
s = s.replace("sx2 = w.getPos(pp_app.SPLIT)[0]", "sx2 = rect(sp)[0]")
s = s.replace("sx3, sy3, sw3, sh3 = w.getPos(pp_app.SPLIT)", "sx3, sy3, sw3, sh3 = rect(sp)")
s = s.replace("p1 = w.getPos(pp_app.CV)", "p1 = rect(E.find_control(w.hwnd, pp_app.CV))")
io.open(P, 'w', encoding='utf-8').write(s)
print('测试改用 GetWindowRect')
