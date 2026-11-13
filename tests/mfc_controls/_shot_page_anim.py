# -*- coding: utf-8 -*-
"""_shot_page_anim.py — 页+动画四拍取证（离屏合成，不受窗口遮挡影响）。"""
import os

import PyMfcXaml as p
from demo_page_anim import PageAnim, build
from _verify_flickable import capture


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(250)
    sv = names['sv']
    w.addPage(sv, [names['s1'], names['p1t'], names['p1b'], names['p1p']])
    w.addPage(sv, [names['s2'], names['p2t'], names['p2s']])
    w.addPage(sv, [names['s3'], names['p3t'], names['p3d']])
    pa = PageAnim(w, names)

    def run(ms):
        spent = 0
        while spent < ms:
            p.pump(20)
            pa.tick()
            spent += 20

    out = r'G:\PyArx\docs'
    run(200)
    capture(w.hwnd, os.path.join(out, '_pa_1_off.bmp'))

    w.setSwitch(pa.sw[0], 1)
    run(420)
    capture(w.hwnd, os.path.join(out, '_pa_1_on.bmp'))

    w.setViewIndex(sv, 1)
    run(150)
    w.setSwitch(pa.sw[1], 1)
    run(380)
    capture(w.hwnd, os.path.join(out, '_pa_2_on.bmp'))

    w.setViewIndex(sv, 2)
    run(150)
    w.setSwitch(pa.sw[2], 1)
    run(380)
    capture(w.hwnd, os.path.join(out, '_pa_3_on.bmp'))
    print('shots done')
    w.close()


if __name__ == '__main__':
    main()
