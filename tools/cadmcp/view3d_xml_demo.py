# -*- coding: utf-8 -*-
"""验证 XML 层 <XView3D>：guiload 建窗 → 取控件 id → 用 Python 接口喂几何/相机 → 出图。"""
import os
import sys
import time

sys.path.insert(0, r"G:\PyArx\X64\Release")
os.add_dll_directory(r"G:\PyArx\X64\Release")
import PyMfcXml as M

names = {}
win = M.guiload(r"G:\PyArx\docs\view3d_demo.xml", names=names)
print("names", dict(names))
vid = names["v3"]
win.show()
time.sleep(0.5)
print("camera after guiload", win.view3d_get_camera(vid))
win.save_control_png(vid, r"G:\PyArx\docs\view3d_xml.png")

# XML 里 Data 已生效；再用 Python 追加图元 + 改相机，验证两条路都通
win.view3d_append_line(vid, 2400, 0, 300, 2400, 2400, 300, 0x30507A, 2.0)

win.view3d_set_camera(vid, 55.0, 30.0, 1.0, 0, 0)
time.sleep(0.4)
print("camera now", win.view3d_get_camera(vid))
win.save_control_png(vid, r"G:\PyArx\docs\view3d_xml_py.png")
win.close()
for n in ("view3d_xml.png", "view3d_xml_py.png"):
    p = os.path.join(r"G:\PyArx\docs", n)
    print(n, os.path.getsize(p) if os.path.exists(p) else "MISSING")
