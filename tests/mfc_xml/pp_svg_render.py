# -*- coding: utf-8 -*-
"""SVG 导入实试：nanosvg 解析 → MFCXml 的 D2D 栈离屏出 PNG。

覆盖 path 的 M/L/C/Q/Z、rect/circle/ellipse/polygon、fill/stroke/opacity、
group transform，并顺带 parse 一份真实导出 SVG（QML 版配电图）看能解析出多少。
"""
import os
import sys

REL = r'G:\PyArx\X64\Release'
sys.path.insert(0, REL)
os.add_dll_directory(REL)

import PyMfcXml as M

DOCS = r'G:\PyArx\docs'

TEST_SVG = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="640" height="400" viewBox="0 0 640 400">
  <rect x="16" y="16" width="608" height="368" fill="#FFFFFF" stroke="#CFD6DF" stroke-width="2"/>

  <!-- 圆角电柜卡片：fill + stroke -->
  <g transform="translate(40,40)">
    <rect x="0" y="0" width="160" height="64" rx="6" fill="#EAF2FB" stroke="#2F6FBA" stroke-width="2"/>
    <path d="M 24 44 C 48 8, 112 8, 136 44" fill="none" stroke="#E15759" stroke-width="3"/>
  </g>

  <!-- 二次贝塞尔 + 折线 -->
  <path d="M 240 72 Q 300 8 360 72 T 480 72" fill="none" stroke="#0F766E" stroke-width="3"/>
  <polyline points="240,110 300,80 360,110 420,80 480,110" fill="none" stroke="#B54708" stroke-width="3"/>

  <!-- 带填充的多边形与圆 -->
  <polygon points="240,150 300,150 270,200" fill="#FEF3F2" stroke="#A8071A" stroke-width="2"/>
  <circle cx="420" cy="175" r="32" fill="#E6FFFA" stroke="#0F766E" stroke-width="2"/>
  <ellipse cx="540" cy="175" rx="48" ry="28" fill="#FEF6E7" stroke="#B54708" stroke-width="2"/>

  <!-- 闭合路径（三次贝塞尔拼出的近似椭圆） -->
  <path d="M 120 300 C 160 240, 240 240, 280 300 C 240 360, 160 360, 120 300 Z"
        fill="#F3F5F8" stroke="#8A94A6" stroke-width="2"/>

  <!-- 阶梯折线：验证多段 + 圆角笔宽 -->
  <path d="M 340 300 L 380 300 L 380 340 L 420 340 L 420 300 L 460 300"
        fill="none" stroke="#1F64B0" stroke-width="4"/>

  <!-- 半透明叠层 -->
  <rect x="500" y="280" width="80" height="80" fill="#2F6FBA" opacity="0.35"/>
</svg>
"""


def main():
    svg_path = os.path.join(DOCS, '_pp_svg_test.svg')
    with open(svg_path, 'w', encoding='utf-8') as f:
        f.write(TEST_SVG)

    info = M.svg.parse(svg_path)
    print('测试 SVG: %.0fx%.0f, shapes=%d paths=%d warnings=%d' % (
        info['width'], info['height'], info['shapes'], info['paths'], len(info['warnings'])))
    for w in info['warnings']:
        print('  警告:', w)

    for scale in (1, 2):
        out = os.path.join(DOCS, '_pp_svg_test_%dx.png' % scale)
        w, h = M.svg.renderPng(svg_path, out, scale, '#FFFFFF')
        print('  scale=%d -> %dx%d, %d 字节' % (scale, w, h, os.path.getsize(out)))

    # 真实导出 SVG（QML 版配电图）：主要看几何能进来多少
    real = os.path.join(DOCS, '_pyarx_qml_powerplan.svg')
    if os.path.exists(real):
        try:
            info2 = M.svg.parse(real)
            print('配电 SVG: %.0fx%.0f, shapes=%d paths=%d warnings=%d' % (
                info2['width'], info2['height'], info2['shapes'],
                info2['paths'], len(info2['warnings'])))
            out = os.path.join(DOCS, '_pp_svg_powerplan_1x.png')
            w, h = M.svg.renderPng(real, out, 1, '#FFFFFF')
            print('  渲染 -> %dx%d, %d 字节' % (w, h, os.path.getsize(out)))
        except Exception as e:
            print('配电 SVG 失败:', e)


if __name__ == '__main__':
    main()
