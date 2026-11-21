# -*- coding: utf-8 -*-
"""XToolBar 探针：确认控件真的被创建，并抓窗口像素。"""
import PyMfcXml as p

names = {}
w = p.guiload(r"G:\PyArx\docs\_tb_probe.xml", names=names)
w.show()
p.pump(450)
w.save_png(r"G:\PyArx\docs\_tb_new.png")
print("NAMES:", sorted(names.keys()))
for key in ("tbH", "tbH2", "tbV"):
    if key in names:
        try:
            print(key, "Items =", w.get_xml_value(names[key], "Items"),
                  "| Selected =", w.get_xml_value(names[key], "SelectedIndex"))
        except Exception as exc:
            print(key, "ERR", exc)
p.pump(300)
