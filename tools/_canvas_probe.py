# -*- coding: utf-8 -*-
"""画布视口探针：看 XDwgCanvas 实际渲染区域、zoom/offset，以及单独导出画布位图。"""
import PyMfcXml as p

names = {}
w = p.guiload(r"G:\PyArx\examples\cad2d\cad_shell.xml", names=names)
w.show()
p.pump(500)

for key in ("cvRibbon", "cvTools", "cvMain"):
    if key not in names:
        print(key, "MISSING")
        continue
    cid = names[key]
    for call, args in (("dwgProbe", (cid,)), ("getDwgView", (cid,))):
        try:
            print(key, call, getattr(w, call)(*args))
        except Exception as exc:
            print(key, call, "ERR", exc)
    try:
        out = r"G:\PyArx\docs\_cv_%s.png" % key
        w.save_control_png(cid, out)
        print(key, "saveControlPng ->", out)
    except Exception as exc:
        print(key, "saveControlPng ERR", exc)
p.pump(200)
