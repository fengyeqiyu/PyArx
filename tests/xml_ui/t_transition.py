# -*- coding: utf-8 -*-
"""颜色过渡的地基：宿主逐帧写入的颜色必须真的画出来（CSS `transition` 的前置条件）。

审计结论（45 轮）：`animateColor` 把插值写进 m_ctrlBack，但绘制端（owner-draw 背板）
从来不读它 —— 动画在算、屏幕不动，这也是 CSS `transition` 一直落不了地的根因。

断言方式：动画中途取样，像素必须严格介于起止两色之间（既不是起点也不是终点）。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

FAILS = []
CHECKS = [0]
FROM_RGB = (0x2E, 0x7D, 0x32)
TO_RGB = (0x15, 0x65, 0xC0)
ID = 1000


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def pixel(png, x, y):
    from PIL import Image
    img = Image.open(png).convert('RGB')
    return img.getpixel((x, y))


def main():
    path = os.path.join(ROOT, 'log', '_t_tr.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write('<window Width="360" Height="220">'
                '<label Name="a" Text="" '
                'style="left:20;top:20;width:200;height:120;background:#2E7D32"/>'
                '</window>')
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(300)

    p0 = os.path.join(ROOT, 'log', '_t_tr_from.png')
    win.save_png(p0)
    c0 = pixel(p0, 60, 60)     # 截图偏移 x+8 / y+31
    check(all(abs(c0[i] - FROM_RGB[i]) <= 8 for i in range(3)), '起始底色是声明的 #2E7D32', c0)

    # 300ms 动画：跑 120ms 取中间帧（animateColor 收的是 Win32 COLORREF = 0x00BBGGRR）
    cref = TO_RGB[0] | (TO_RGB[1] << 8) | (TO_RGB[2] << 16)
    win.animate_color(ID, cref, 1200, 'linear', 0)
    M.pump(150)
    p1 = os.path.join(ROOT, 'log', '_t_tr_mid.png')
    win.save_png(p1)
    c1 = pixel(p1, 60, 60)
    inrange = all(min(FROM_RGB[i], TO_RGB[i]) - 4 <= c1[i] <= max(FROM_RGB[i], TO_RGB[i]) + 4
                  for i in range(3))
    moved = sum(abs(c1[i] - FROM_RGB[i]) for i in range(3)) > 12
    check(inrange and moved and c1 != FROM_RGB and c1 != TO_RGB,
          '动画中途：像素在起止之间（真的逐帧重绘）', c1)

    M.pump(320)
    p2 = os.path.join(ROOT, 'log', '_t_tr_end.png')
    win.save_png(p2)
    c2 = pixel(p2, 60, 60)
    check(all(abs(c2[i] - TO_RGB[i]) <= 8 for i in range(3)), '动画结束落到目标色', c2)

    win.close()

    # ---- CSS transition：hover 切换不再硬切 ----
    import ctypes
    from ctypes import wintypes
    u32 = ctypes.windll.user32
    path = os.path.join(ROOT, 'log', '_t_tr2.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write('<window Width="360" Height="220">'
                '<label Name="b" Text="" style="left:20;top:20;width:200;height:120;'
                'background:#2E7D32;hover-background:#1565C0;transition:1500ms linear"/>'
                '</window>')
    win = M.guiload(path, context={}, names={})
    win.show()
    M.pump(300)
    def find_ctl(parent, tid):
        h = u32.GetWindow(parent, 5)   # GW_CHILD
        while h:
            if u32.GetDlgCtrlID(h) == tid:
                return h
            r = find_ctl(h, tid)
            if r:
                return r
            h = u32.GetWindow(h, 2)    # GW_HWNDNEXT
        return 0

    u32.GetDlgCtrlID.argtypes = [wintypes.HWND]
    u32.GetDlgCtrlID.restype = ctypes.c_int
    hctl = find_ctl(wintypes.HWND(int(win.hwnd)), 1000)
    check(hctl != 0, '找到控件窗口（hover 消息要投到真实子窗口）', hctl)
    # 直接投递鼠标消息（无头环境里 SetCursorPos 未必把 WM_MOUSEMOVE 送到控件）
    u32.SendMessageW(hctl, 0x0200, 0, 0)   # WM_MOUSEMOVE
    M.pump(140)
    p3 = os.path.join(ROOT, 'log', '_t_tr_hover_mid.png')
    win.save_png(p3)
    c3 = pixel(p3, 60, 60)
    mid = all(min(FROM_RGB[i], TO_RGB[i]) - 4 <= c3[i] <= max(FROM_RGB[i], TO_RGB[i]) + 4
              for i in range(3))
    moved2 = sum(abs(c3[i] - FROM_RGB[i]) for i in range(3)) > 10
    check(mid and moved2, 'transition 声明：hover 中途是插值色（不是硬切）', c3)
    for _ in range(6): M.pump(400)
    p4 = os.path.join(ROOT, 'log', '_t_tr_hover_end.png')
    win.save_png(p4)
    c4 = pixel(p4, 60, 60)
    check(all(abs(c4[i] - TO_RGB[i]) <= 8 for i in range(3)), 'hover 结束停在 hover 底色', c4)

    u32.SetCursorPos(0, 0)   # 移开 → 回到常态色
    u32.SendMessageW(hctl, 0x02A3, 0, 0)   # WM_MOUSELEAVE
    for _ in range(6):
        M.pump(400)
    p5 = os.path.join(ROOT, 'log', '_t_tr_leave.png')
    win.save_png(p5)
    c5 = pixel(p5, 60, 60)
    back_home = all(abs(c5[i] - FROM_RGB[i]) <= 8 for i in range(3))
    going_back = (c5[2] < TO_RGB[2] and c5[2] > FROM_RGB[2] and c5[0] > TO_RGB[0])
    check(back_home or going_back, '离开 hover 开始往常态色过渡（不是硬跳）', c5)
    win.close()

    print('t_transition: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())




