# 第57轮：CSS text-align 继承（写在父容器上，子元素自动跟随）
# HTML 里 `<div style="text-align:center">` 下面所有文字都居中；XML 早先必须给每个 label 各写一遍。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def center_of(p, y0, y1):
    """返回 [y0,y1) 区间内文字亮像素的水平重心"""
    im = Image.open(p).convert("RGB")
    xs = [x for y in range(y0, y1) for x in range(0, im.size[0])
          if sum(im.getpixel((x, y))) > 330]
    return (sum(xs) / len(xs), min(xs), max(xs)) if xs else (0, 0, 0)

def shot(tag, container_align, child_align=None):
    """父容器声明对齐 + 一个不写对齐的子 label；child_align 显式写则用来验证"不覆盖" """
    n = {}
    childStyle = 'style="font-size:14;text-align:%s"' % child_align if child_align else ''
    w = M.guiload_text(
        '<XWindow Width="360" Height="150">'
        '<XStackPanel Name="box" Left="20" Top="40" Width="320" Height="80" style="%s">'
        '<label Name="t" Left="30" Top="30" Width="260" Height="24" Text="CENTER ME" %s/>'
        '</XStackPanel></XWindow>'
        % (container_align, childStyle),
        context=globals(), names=n)
    M.pump(250)
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    w.close()
    return center_of(p, 34, 150)

c_left = shot("al_inh_left", "text-align:left")
c_center = shot("al_inh_center", "text-align:center")
c_keep = shot("al_inh_keep", "text-align:center", "left")

chk("继承 center：子 label 文字重心右移", c_center[0] > c_left[0] + 20,
    (round(c_left[0]), round(c_center[0])))
chk("子元素显式写 text-align 不被父覆盖", c_keep[0] < c_center[0] - 15,
    (round(c_keep[0]), round(c_center[0])))
chk("左右帧都有文字像素", c_left[1] > 0 and c_center[1] > 0, (c_left[1], c_center[1]))
print("t_align_inherit.py: PASS=%d FAIL=%d" % (3 - len(fails), len(fails)))

