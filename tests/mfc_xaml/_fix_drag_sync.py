import io
p = 'G:/PyArx/tests/mfc_xaml/pp_events.py'
s = io.open(p, encoding='utf-8').read()
old = '''def drag(hwnd, x1, y1, x2, y2, steps=6, pump=60):
    """左键按住从 (x1,y1) 拖到 (x2,y2)；中间插 mousemove 让控件看到移动。"""
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, _lp(x1, y1))
    M.pump(20)
    for i in range(1, steps + 1):
        mx = int(x1 + (x2 - x1) * i / steps)
        my = int(y1 + (y2 - y1) * i / steps)
        user32.SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, _lp(mx, my))
        M.pump(20)
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(x2, y2))
    M.pump(pump)'''
new = '''def _sync_cursor(hwnd, x, y):
    """把真实鼠标也挪到模拟点上。

    控件一旦 SetCapture，系统会把**真实鼠标**的移动也发给它。模拟拖动时如果
    只发消息不动光标，真实光标（落在别处）的抖动会带着一个完全不同的坐标挤进来，
    把手势算出来的值冲掉。让真实光标跟着走，两路事件才是同一个坐标。
    """
    pt = wt.POINT(int(x), int(y))
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    user32.SetCursorPos(pt.x, pt.y)


def drag(hwnd, x1, y1, x2, y2, steps=6, pump=60):
    """左键按住从 (x1,y1) 拖到 (x2,y2)；中间插 mousemove 让控件看到移动。"""
    _sync_cursor(hwnd, x1, y1)
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, _lp(x1, y1))
    M.pump(20)
    for i in range(1, steps + 1):
        mx = int(x1 + (x2 - x1) * i / steps)
        my = int(y1 + (y2 - y1) * i / steps)
        _sync_cursor(hwnd, mx, my)
        user32.SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, _lp(mx, my))
        M.pump(20)
    _sync_cursor(hwnd, x2, y2)
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(x2, y2))
    M.pump(pump)'''
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_events.drag 加了真实光标同步')
