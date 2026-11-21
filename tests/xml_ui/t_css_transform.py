# -*- coding: utf-8 -*-
"""CSS transform（4×4 矩阵）端到端测试：style 里的 transform 必须真的改变画面。

量法是截图取像素，而不是问 XmlCore 的 layout —— transform 只影响绘制，
不影响布局（CSS 同样如此：transform 不参与文档流）。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402
from PIL import Image  # noqa: E402

FAILS = []
CHECKS = [0]


def check(cond, what, got=None):
    CHECKS[0] += 1
    line = ('PASS ' if cond else 'FAIL ') + what
    if got is not None:
        line += '  got=%r' % (got,)
    print(line)
    if not cond:
        FAILS.append(what)
    return bool(cond)


def shot(xml, png, w=460, h=260):
    path = os.path.join(ROOT, 'log', '_t_css_transform.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    names = {}
    win = M.guiload(path, context={}, names=names)
    win.show()
    M.pump(400)
    win.save_png(png)
    win.close()
    return Image.open(png).convert('RGB')


def bbox(img, rgb, tol=40, region=None):
    """返回匹配颜色的包围盒（无匹配返回 None）。region=(x0,y0,x1,y1) 限定搜索范围。"""
    px = img.load()
    rx0, ry0, rx1, ry1 = region or (0, 0, img.width, img.height)
    x0 = y0 = 10 ** 9
    x1 = y1 = -1
    for y in range(max(0, ry0), min(ry1, img.height)):
        for x in range(max(0, rx0), min(rx1, img.width)):
            r, g, b = px[x, y]
            if abs(r - rgb[0]) <= tol and abs(g - rgb[1]) <= tol and abs(b - rgb[2]) <= tol:
                x0 = min(x0, x)
                y0 = min(y0, y)
                x1 = max(x1, x)
                y1 = max(y1, y)
    if x1 < 0:
        return None
    return (x0, y0, x1 + 1, y1 + 1)


def main():
    # 1. scale(2)：绿色盒子必须变成两倍宽（盒子放在窗口中央，避免被窗口边裁掉）
    img = shot('<window Width="460" Height="260">'
               '<label Name="a" Text="" style="left:150;top:20;width:100;height:40;background:#00FF00"/>'
               '</window>', os.path.join(ROOT, 'log', '_t_xf_plain.png'))
    b0 = bbox(img, (0, 255, 0))
    check(b0 is not None and b0[2] - b0[0] == 100, '基线盒子宽 100', b0)

    img = shot('<window Width="460" Height="260">'
               '<label Name="a" Text="" style="left:150;top:20;width:100;height:40;'
               'background:#00FF00;transform:scale(2)"/>'
               '</window>', os.path.join(ROOT, 'log', '_t_xf_scale.png'))
    b1 = bbox(img, (0, 255, 0))
    check(b1 is not None and 195 <= b1[2] - b1[0] <= 205, 'scale(2) 盒子宽约 200', b1)
    check(b1 is not None and abs((b1[0] + b1[2]) - (b0[0] + b0[2])) <= 3,
          'scale 绕盒中心、水平中心不变', (b0, b1))

    # 2. translate：整块右移 40 / 下移 20
    img = shot('<window Width="460" Height="260">'
               '<label Name="a" Text="" style="left:150;top:20;width:100;height:40;'
               'background:#00FF00;transform:translate(40px,20px)"/>'
               '</window>', os.path.join(ROOT, 'log', '_t_xf_translate.png'))
    b2 = bbox(img, (0, 255, 0))
    check(b2 is not None and b2[0] - b0[0] == 40 and b2[1] - b0[1] == 20,
          'translate(40px,20px) 位移精确', (b0, b2))

    # 3. rotate(45deg)：轴对齐盒子的高度必须明显变高（旋转后外接框 = 对角）
    img = shot('<window Width="460" Height="260">'
               '<label Name="a" Text="" style="left:40;top:40;width:120;height:40;'
               'background:#00FF00;transform:rotate(45deg)"/>'
               '</window>', os.path.join(ROOT, 'log', '_t_xf_rotate.png'))
    b3 = bbox(img, (0, 255, 0))
    check(b3 is not None and b3[3] - b3[1] > 60, 'rotate(45deg) 外接框高 > 60', b3)
    check(b3 is not None and b3[2] - b3[0] > 100, 'rotate(45deg) 外接框宽 > 100', b3)

    # 4. 声明了 transform 的控件只能画一次：文字不能出现"变换过的 + 没变换的"两份
    img = shot('<window Width="460" Height="260">'
               '<label Name="txt" Text="XXXXXXXXXX" style="left:20;top:20;width:300;height:40;'
               'background:#000000;color:#FFFFFF;font-size:20;transform:scale(2)"/>'
               '</window>', os.path.join(ROOT, 'log', '_t_xf_text.png'))
    white = bbox(img, (255, 255, 255), tol=60)
    check(white is not None, '缩放后文字仍然可见（没被未变换盒裁掉）', white)

    # 5. 容器（div）自身 transform：背板与子树文字必须一起转，且只画一次
    div = ('<div Name="card" style="left:40;top:40;width:180;height:100;background:#2E7D32;'
           'display:flex;align-items:center;justify-content:center;%s">'
           '<label Name="tt" Text="inside card" style="width:160;height:32;color:#FFFFFF;font-size:18"/>'
           '</div>')
    img_plain = shot('<window Width="460" Height="260">' + (div % '') + '</window>',
                     os.path.join(ROOT, 'log', '_t_xf_div_plain.png'))
    bd0 = bbox(img_plain, (46, 125, 50), region=(25, 45, 300, 230))
    check(bd0 is not None and bd0[2] - bd0[0] == 180 and bd0[3] - bd0[1] == 100,
          '容器基线背板 180x100', bd0)
    white0 = bbox(img_plain, (255, 255, 255), tol=60, region=(25, 40, 280, 200))
    check(white0 is not None and white0[3] - white0[1] < 35,
          '容器内子 label 文字可见且是水平一行', white0)

    img_rot = shot('<window Width="460" Height="260">' + (div % 'transform:rotate(-15deg);') + '</window>',
                   os.path.join(ROOT, 'log', '_t_xf_div_rot.png'))
    bd1 = bbox(img_rot, (46, 125, 50))
    check(bd1 is not None and bd1[3] - bd1[1] > 120,
          '容器背板随 transform 旋转（外接框变高）', bd1)
    white1 = bbox(img_rot, (255, 255, 255), tol=60, region=(25, 40, 280, 200))
    check(white1 is not None and white1[3] - white1[1] > 35,
          '容器内子 label 文字一起旋转（外接框变高）', white1)

    print('t_css_transform: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        print('failed: ' + '; '.join(FAILS))
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
