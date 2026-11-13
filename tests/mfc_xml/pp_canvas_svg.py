# -*- coding: utf-8 -*-
"""SVG 进 XML 画布：在窗口里显示矢量 SVG，并验证缩放后的清晰度。

输入用真实导出的配电 SVG（1036×2578，122 图元 + 122 段文字）。
"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M

SVG = r'G:\PyArx\docs\_pyarx_qml_powerplan.svg'


def main():
    with open(SVG, 'r', encoding='utf-8') as f:
        svg = f.read()
    print('SVG 长度 %d' % len(svg))

    info = M.svg.parse(SVG)
    shapes = info['shapes']
    print('解析：%dx%d shapes=%d paths=%s warnings=%s'
          % (info['width'], info['height'],
             len(shapes) if isinstance(shapes, (list, tuple)) else shapes,
             info['paths'], info['warnings']))

    win = M.Window('SVG 矢量化画布', 900, 640)
    win.setTheme(False)
    win.addPowerCanvas(3001, 0, 0, 900, 640)
    win.loadPowerSvg(3001, svg)
    win.show()
    time.sleep(0.4)

    print('fit zoom = %.4f' % win.getPlanZoom(3001))
    win.savePng(r'G:\PyArx\docs\_pp_svg_canvas_fit.png')

    # 1:1 看细节
    win.setPlanView(3001, 1.0, -40, -80)
    time.sleep(0.2)
    win.savePng(r'G:\PyArx\docs\_pp_svg_canvas_100.png')

    win.close()


if __name__ == '__main__':
    main()
