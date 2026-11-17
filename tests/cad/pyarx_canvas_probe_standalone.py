# -*- coding: utf-8 -*-
"""独立进程 XPowerCanvas 1:1 探针：不依赖 CAD 父 HWND。"""
import json
import os
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as p

XML = r"G:\PyArx\tests\cad\canvas_probe.xml"
OUT = r"G:\PyArx\log\pyarx_canvas_probe.json"
PNG = r"G:\PyArx\log\canvas_probe_1344x104.png"
SVG = ('<svg xmlns="http://www.w3.org/2000/svg" width="1344" height="104">'
       '<rect x="0" y="0" width="1344" height="104" fill="#FF0000"/></svg>')

R = {"ok": False, "names": None, "size": None, "png": PNG, "err": None}
os.makedirs(os.path.dirname(OUT), exist_ok=True)

try:
    names = {}
    win = p.guiload(XML, names=names)
    win.show()
    p.pump(200)
    try:
        win.reflowXml()
    except Exception:
        pass
    p.pump(120)
    cid = names["cv"]
    win.loadPowerSvg(cid, SVG)
    size = win.getSize(cid)
    saved = win.saveControlPng(cid, PNG)
    R["names"] = names
    R["size"] = list(size)
    R["saved"] = bool(saved)
    R["ok"] = bool(saved) and os.path.exists(PNG)
except Exception:
    R["err"] = traceback.format_exc()[-500:]
finally:
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1)
    print(json.dumps(R, ensure_ascii=False))
