import io
p = 'G:/PyArx/tests/mfc_xaml/pp_events.py'
s = io.open(p, encoding='utf-8').read()
start = s.index('def _sync_cursor(hwnd, x, y):')
end = s.index('def wheel(hwnd, delta, x, y, pump=80):')
new = '''def _screen_pt(hwnd, x, y):
    """客户坐标 → 屏幕坐标。"""
    pt = wt.POINT(int(x), int(y))
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    return pt.x, pt.y


def _client_of_screen(hwnd, sx, sy):
    """屏幕坐标 → **当下这一刻**的客户坐标。

    拖动中控件自己会移位，客户坐标的零点跟着走；所以每一步都要按当下的原点换算，
    而不是沿用开始时的换算结果。
    """
    pt = wt.POINT(int(sx), int(sy))
    user32.ScreenToClient(hwnd, ctypes.byref(pt))
    return pt.x, pt.y


def _move_cursor(sx, sy):
    user32.SetCursorPos(int(sx), int(sy))


def drag(hwnd, x1, y1, x2, y2, steps=6, pump=60):
    """左键按住从 (x1,y1) 拖到 (x2,y2)；中间插 mousemove 让控件看到移动。

    起点终点先换算成**屏幕坐标**再走：控件 SetCapture 之后，系统会把真实鼠标的
    移动也发给它，真实光标不跟着走的话会有坐标完全不同的事件挤进来；而控件在
    拖动中移位又会让客户坐标漂移。所以整条轨迹定在屏幕坐标上，每一步按控件当下
    的原点折回客户坐标发消息，并顺手把真实光标挪到同一个屏幕点上。
    """
    sx1, sy1 = _screen_pt(hwnd, x1, y1)
    sx2, sy2 = _screen_pt(hwnd, x2, y2)
    cx, cy = _client_of_screen(hwnd, sx1, sy1)
    _move_cursor(sx1, sy1)
    user32.SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, _lp(cx, cy))
    M.pump(20)
    for i in range(1, steps + 1):
        sx = int(sx1 + (sx2 - sx1) * i / steps)
        sy = int(sy1 + (sy2 - sy1) * i / steps)
        cx, cy = _client_of_screen(hwnd, sx, sy)
        _move_cursor(sx, sy)
        user32.SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, _lp(cx, cy))
        M.pump(20)
    cx, cy = _client_of_screen(hwnd, sx2, sy2)
    _move_cursor(sx2, sy2)
    user32.SendMessageW(hwnd, WM_LBUTTONUP, 0, _lp(cx, cy))
    M.pump(pump)


'''
s = s[:start] + new + s[end:]
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_events.drag 改为屏幕坐标驱动 + 每步按当下原点折回客户坐标')
