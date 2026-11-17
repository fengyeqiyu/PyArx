# -*- coding: utf-8 -*-
"""真实鼠标点一遍：功能区"直线"→ 画布两击 → 截图，验证 C++ 事件派发链路。"""
import ctypes
import ctypes.wintypes as wt
import subprocess
import sys
import time

u = ctypes.windll.user32


def find_window(title_part):
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        n = u.GetWindowTextLengthW(h)
        buf = ctypes.create_unicode_buffer(n + 1)
        u.GetWindowTextW(h, buf, n + 1)
        if title_part in buf.value and u.IsWindowVisible(h):
            found.append((h, buf.value))
        return True

    u.EnumWindows(cb, 0)
    return found[0] if found else (None, None)


def client_to_screen(h, x, y):
    pt = wt.POINT(x, y)
    u.ClientToScreen(h, ctypes.byref(pt))
    return pt.x, pt.y


def click(sx, sy, hold=0.05):
    u.SetCursorPos(sx, sy)
    time.sleep(0.02)
    u.mouse_event(0x0002, 0, 0, 0, 0)   # LEFTDOWN
    time.sleep(hold)
    u.mouse_event(0x0004, 0, 0, 0, 0)   # LEFTUP
    time.sleep(0.08)


def main():
    hwnd, title = find_window("PyArx CAD")
    if hwnd is None:
        print("未找到窗口")
        return 1
    print("窗口:", title, hex(hwnd))
    u.SetForegroundWindow(hwnd)
    time.sleep(0.4)

    # 先点画布的空白处把焦点压到窗口
    click(*client_to_screen(hwnd, 560, 450))
    time.sleep(0.3)

    # 功能区"直线"按钮：截图坐标 (50, 98) 含标题栏，客户区要减 31
    click(*client_to_screen(hwnd, 50, 67))
    time.sleep(0.4)
    # 画布两点（客户区）
    click(*client_to_screen(hwnd, 400, 269))
    time.sleep(0.4)
    click(*client_to_screen(hwnd, 700, 529))
    time.sleep(0.6)
    print("done")
    return 0


if __name__ == "__main__":
    sys.exit(main())
