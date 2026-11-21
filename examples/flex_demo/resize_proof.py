# -*- coding: utf-8 -*-
"""flex 容器的缩放实测证据（P0 验收第 7 条）：窗口 720 -> 520，子项自己重排。

用法：python resize_proof.py
产物：resize_720.png / resize_520.png + 两个尺寸下的子项矩形对照。
"""
import ctypes, ctypes.wintypes as wt, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M
from PIL import Image

user32 = ctypes.windll.user32
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, ctypes.c_uint]
SWP_NOMOVE, SWP_NOZORDER = 0x0002, 0x0004


def rects_of(xml_path, w, h):
    r = M.xml_layout(xml_path, w, h)
    return {k: r[k] for k in ('c1', 'c2', 'c3', 'tools', 'inp', 'ok') if k in r}


def main():
    xml_path = os.path.join(HERE, 'flex_demo_html.xml')
    names = {}
    win = M.guiload(xml_path, context=globals(), names=names)
    win.show()
    M.pump(400)

    shot_big = os.path.join(HERE, 'resize_720.png')
    win.save_png(shot_big)
    big = rects_of(xml_path, 704, 404)

    # 客户区 520 宽（外框 +16 边框）
    user32.SetWindowPos(win.hwnd, None, 0, 0, 536, 420, SWP_NOMOVE | SWP_NOZORDER)
    M.pump(300)
    shot_small = os.path.join(HERE, 'resize_520.png')
    win.save_png(shot_small)
    small = rects_of(xml_path, 504, 404)

    print('%-6s %-22s %-22s' % ('name', '720 宽', '520 宽'))
    for k in ('c1', 'c2', 'c3', 'tools', 'inp', 'ok'):
        if k in big and k in small:
            print('%-6s %-22s %-22s' % (k, big[k], small[k]))

    px = Image.open(shot_big).convert('RGB'), Image.open(shot_small).convert('RGB')
    print('shots', px[0].size, px[1].size)
    win.close()
    M.pump(50)
    return 0


if __name__ == '__main__':
    sys.exit(main())
