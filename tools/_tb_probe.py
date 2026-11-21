# -*- coding: utf-8 -*-
"""ToolBar 实机探针：guiload 一份只含 ToolBar 的 XML，抓窗口像素。"""
import os
import sys

XML = r"G:\PyArx\docs\_tb_probe.xml"
SHOT = r"G:\PyArx\docs\_tb_win.png"

import PyMfcXml as p  # noqa: E402

names = {}
w = p.guiload(XML, names=names)
w.show()
p.pump(400)
w.save_png(SHOT)
print("names:", sorted(names.keys()))
print("items attrs:", [k for k in names if "tb" in k.lower()])
for k in ("tbDraw", "tbMod", "tbV"):
    if k in names:
        try:
            print(k, "getXmlValue(Items) =", w.get_xml_value(names[k], "Items"))
        except Exception as e:
            print(k, "err", e)
p.pump(300)
