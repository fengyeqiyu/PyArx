"""验证 exe 侧「框选导出」真的落盘到 viewer\\export\\。"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

u32 = ctypes.windll.user32
PY = r"G:\PyArx\X64\Release\python.exe"
SHOT = r"G:\PyArx\viewer\shot.py"
DOWN, UP, SHIFT, KEYUP, GA_ROOT = 0x0002, 0x0004, 0x10, 0x0002, 2
OUT = r"G:\PyArx\viewer\export"

hwnd = u32.FindWindowW(None, "PyArx Viewer")
if not hwnd:
    print("窗口没开")
    sys.exit(1)

u32.ShowWindow(hwnd, 9)
try:
    u32.SetForegroundWindow(hwnd)
except Exception:
    pass
time.sleep(1.0)

r = wt.RECT()
u32.GetWindowRect(hwnd, ctypes.byref(r))


def move(x, y):
    pt = wt.POINT(r.left + x, r.top + y)
    h = u32.WindowFromPoint(pt)
    if u32.GetAncestor(h, GA_ROOT) != hwnd:
        raise SystemExit("目标不在 PyArxViewer 上，放弃 (%d,%d)" % (x, y))
    u32.SetCursorPos(r.left + x, r.top + y)


def click(x, y):
    move(x, y)
    time.sleep(0.35)
    move(x + 1, y)
    time.sleep(0.12)
    u32.mouse_event(DOWN, 0, 0, 0, 0)
    time.sleep(0.16)
    u32.mouse_event(UP, 0, 0, 0, 0)
    time.sleep(1.3)


def drag(x1, y1, x2, y2):
    move(x1, y1)
    u32.keybd_event(SHIFT, 0, 0, 0)
    time.sleep(0.25)
    u32.mouse_event(DOWN, 0, 0, 0, 0)
    for i in range(1, 13):
        u32.SetCursorPos(r.left + int(x1 + (x2 - x1) * i / 12),
                         r.top + int(y1 + (y2 - y1) * i / 12))
        time.sleep(0.06)
    time.sleep(0.15)
    u32.mouse_event(UP, 0, 0, 0, 0)
    u32.keybd_event(SHIFT, 0, KEYUP, 0)
    time.sleep(1.4)


before = set(os.listdir(OUT)) if os.path.isdir(OUT) else set()

drag(205, 320, 455, 420)                       # 框选第一排左两台柜
subprocess.run([PY, SHOT, r"G:\PyArx\viewer\_t8_panel.png", "--screen"], check=False)
click(1100, 304)                               # 「导出 PNG」
time.sleep(5.0)

subprocess.run([PY, SHOT, r"G:\PyArx\viewer\_t9_export.png", "--screen"], check=False)

after = set(os.listdir(OUT)) if os.path.isdir(OUT) else set()
new = sorted(after - before)
print("export 目录:", OUT)
if new:
    for f in new:
        p = os.path.join(OUT, f)
        print("  新增:", f, round(os.path.getsize(p) / 1024), "KB")
else:
    print("  新增：（无）")
print("done")
