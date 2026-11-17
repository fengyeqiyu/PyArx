# -*- coding: utf-8 -*-
"""_svg_probe.py —— 判定 PowerCanvas 到底接受哪些 SVG 元素。

逐个往同一块画布喂"只有一种元素"的 SVG，每次抓一张 PNG。
"""
import os
import sys
import time

import PyMfcXml as p

OUT = r'G:\PyArx\docs'
XML = r'''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="svg-probe"
         Width="640" Height="480">
  <XStackPanel>
    <XPowerCanvas Name="cv" Width="620" Height="440"/>
  </XStackPanel>
</XWindow>'''
XMLP = os.path.join(OUT, '_svg_probe.xml')
open(XMLP, 'w', encoding='utf-8').write(XML)

CASES = {
    'line': '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300" width="400" height="300">'
            '<line x1="40" y1="40" x2="360" y2="260" stroke="#FF0000" stroke-width="4"/></svg>',
    'path': '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300" width="400" height="300">'
            '<path d="M 40 40 L 360 260 Z" stroke="#00AA00" stroke-width="4" fill="none"/></svg>',
    'circle': '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300" width="400" height="300">'
              '<circle cx="200" cy="150" r="100" stroke="#0000FF" stroke-width="4" fill="none"/></svg>',
    'rect': '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300" width="400" height="300">'
            '<rect x="40" y="40" width="320" height="220" stroke="#FF00FF" stroke-width="4" fill="none"/></svg>',
    'text': '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300" width="400" height="300">'
            '<text x="40" y="160" fill="#FFFFFF" font-size="28">SVG TEXT</text></svg>',
    'mixed': '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300" width="400" height="300">'
             '<line x1="20" y1="20" x2="380" y2="20" stroke="#FF0000" stroke-width="4"/>'
             '<rect x="20" y="40" width="140" height="100" stroke="#00FF00" stroke-width="3" fill="none"/>'
             '<circle cx="300" cy="90" r="60" stroke="#00AAFF" stroke-width="3" fill="none"/>'
             '<path d="M 20 260 Q 200 160 380 260" stroke="#FFAA00" stroke-width="4" fill="none"/>'
             '<text x="20" y="290" fill="#FFFFFF" font-size="24">MIXED</text></svg>',
}


def main():
    names = {}
    w = p.guiload(XMLP, names=names)
    w.show()
    p.pump(300)
    cv = names['cv']
    for key, svg in CASES.items():
        w.loadPowerSvg(cv, svg)
        w.setPlanView(cv, 1.0, 10, 10)
        p.pump(400)
        shot = os.path.join(OUT, '_svgprobe_%s.png' % key)
        w.saveControlPng(cv, shot)
        z, ox, oy = w.getPlanView(cv)
        print('%-8s zoom=%.2f ox=%d oy=%d -> %s' % (key, z, ox, oy, os.path.basename(shot)))
    p.pump(200)


if __name__ == '__main__':
    main()
