# -*- coding: utf-8 -*-
"""XToolBar 点击探针：真实鼠标点第一个条目，验证回调能到 Python。"""
import ctypes
import time

import PyMfcXml as p

XML = r"G:\PyArx\docs\_tb_probe.xml"
names = {}
w = p.guiload(XML, names=names)
w.show()
p.pump(400)

hits = []


def on_click(*args):
    sel = -1
    try:
        sel = int(w.get_xml_value(names["tbRoot"], "SelectedIndex"))
    except Exception as exc:
        print("read sel ERR", exc)
    hits.append((args, sel))
    print("CLICK args=%s selected=%d" % (args, sel))


w.on_click(names["tbRoot"], on_click)
p.pump(300)

u = ctypes.windll.user32
hwnd = u.FindWindowW(None, "toolbar probe")
if not hwnd:
    print("NO WINDOW")
else:
    class RECT(ctypes.Structure):
        _fields_ = [("l", ctypes.c_long), ("t", ctypes.c_long),
                    ("r", ctypes.c_long), ("b", ctypes.c_long)]

    rc = RECT()
    u.GetWindowRect(hwnd, ctypes.byref(rc))
    # 窗口左边框约 8px、标题栏约 30px，第一条目中心 ≈ clientX 41 / clientY 26
    x = rc.l + 8 + 41
    y = rc.t + 30 + 26
    u.SetCursorPos(x, y)
    time.sleep(0.15)
    u.mouse_event(2, 0, 0, 0, 0)
    time.sleep(0.06)
    u.mouse_event(4, 0, 0, 0, 0)
    p.pump(500)
    w.save_png(r"G:\PyArx\docs\_tb_click.png")

print("HITS:", hits)
p.pump(200)
