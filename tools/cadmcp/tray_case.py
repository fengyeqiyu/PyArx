# -*- coding: utf-8 -*-
"""对照实验：XPowerCanvas.Data 分别喂 小 SVG / 无 viewBox SVG / 带 viewBox SVG"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXml as M

OUT = r'G:\PyArx\docs\_tray_case.xml'


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
             .replace('"', "&quot;"))


def run(name, svg, w=900, h=400):
    xml = ('<?xml version="1.0" encoding="utf-8"?>\n'
           '<XWindow Name="Case" Width="%d" Height="%d" Title="%s">\n'
           '  <XPowerCanvas Name="CV" Data="%s"/>\n'
           '</XWindow>\n') % (w, h, name, esc(svg.replace("\n", " "))
                              )
    open(OUT, "w", encoding="utf-8").write(xml)
    win = M.guiload(OUT)
    win.show()
    time.sleep(0.6)
    png = r'G:\PyArx\docs\_tray_case_%s.png' % name
    win.savePng(png)
    win.close()
    print(name, os.path.getsize(png))


# A: 极小 SVG（无 viewBox）
run("A_small", '<svg xmlns="http://www.w3.org/2000/svg" width="300" height="120">'
               '<rect x="10" y="10" width="280" height="100" fill="#8ec0ea"/></svg>', 320, 160)

# B: 带 viewBox 的小 SVG
run("B_viewbox", '<svg xmlns="http://www.w3.org/2000/svg" width="300" height="120" viewBox="0 0 300 120">'
                 '<rect x="10" y="10" width="280" height="100" fill="#eda23c"/></svg>', 320, 160)

# C: 真实桥架 SVG（有 width/height/viewBox/font-family）
svg = open(r'G:\PyArx\docs\tray_plan.svg', encoding='utf-8').read()
run("C_tray", svg, 1520, 700)
print("done")
