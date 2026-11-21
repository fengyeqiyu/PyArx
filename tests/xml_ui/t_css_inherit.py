# 第 13 轮：CSS 继承属性（HTML 能、XML 不能的典型点）
# 浏览器里 color / font-size / text-align / line-height / white-space 写在父元素上，
# 子元素自动生效；XML 早先每条属性只作用于声明那一层，
# <div style="text-align:right"><label Text="X"/></div> 里的文字永远左对齐、永远黑字。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=420, h=220):
    p = os.path.join(tempfile.gettempdir(), "inh_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

def shot(inner, name):
    win, ids = load(inner)
    M.pump(180)
    png = os.path.join(tempfile.gettempdir(), "inh_%s_%s.png" % (name, uuid.uuid4().hex[:6]))
    win.save_png(png)
    win.close()
    return png

def bright_bbox(png):
    im = Image.open(png).convert("RGB"); px = im.load(); w, h = im.size
    xs = []; ys = []
    for y in range(30, h):
        for x in range(w):
            r, g, b = px[x, y]
            if r + g + b > 600:
                xs.append(x); ys.append(y)
    return (min(xs), max(xs), min(ys), max(ys)) if xs else None

def red_pixels(png):
    im = Image.open(png).convert("RGB"); px = im.load(); w, h = im.size
    return sum(1 for y in range(30, h) for x in range(w)
               if px[x, y][0] > 150 and px[x, y][1] < 90 and px[x, y][2] < 90)

# 1) 属性回读：父元素的 text-align / font-size 落到了子 label 上
win, ids = load('<div Left="10" Top="10" Width="380" Height="80" style="text-align:right">'
                '<label Name="t" Text="WWWW" Width="360" Height="40"/></div>')
chk("text-align 继承到子 label",
    win.get_xml_value(ids["t"], "TextAlign").lower() == "right",
    win.get_xml_value(ids["t"], "TextAlign"))
win.close()

win, ids = load('<div Left="10" Top="10" Width="380" Height="100" style="font-size:28px">'
                '<label Name="t" Text="WWWW" Width="360" Height="50"/></div>')
chk("font-size 继承到子 label",
    win.get_xml_value(ids["t"], "FontSize") == "28",
    win.get_xml_value(ids["t"], "FontSize"))
win.close()

# 2) 子元素自己的声明优先（CSS 的"就近覆盖"）
win, ids = load('<div Left="10" Top="10" Width="380" Height="80" style="text-align:right">'
                '<label Name="t" Text="WWWW" Width="360" Height="40" style="text-align:left"/></div>')
chk("子元素 text-align:left 覆盖父继承",
    win.get_xml_value(ids["t"], "TextAlign").lower() == "left",
    win.get_xml_value(ids["t"], "TextAlign"))
win.close()

# 3) 像素：文字真的靠右（父 text-align:right）vs 默认靠左
a = shot('<div Left="10" Top="10" Width="380" Height="80" style="text-align:right">'
         '<label Name="t" Text="WWWW" Width="360" Height="40"/></div>', "a")
b = shot('<div Left="10" Top="10" Width="380" Height="80">'
         '<label Name="t" Text="WWWW" Width="360" Height="40"/></div>', "b")
ba, bb = bright_bbox(a), bright_bbox(b)
chk("父 text-align:right 后文字靠右", ba is not None and ba[1] > 320, ba)
chk("未声明时文字靠左", bb is not None and bb[0] < 40, bb)
chk("两者相差 200px 以上", ba is not None and bb is not None and (ba[0] - bb[0]) > 200,
    (bb[0] if bb else None, ba[0] if ba else None))

# 4) 像素：color 从父元素继承下来（红字）
c = shot('<div Left="10" Top="10" Width="380" Height="80" style="color:#ff0000">'
         '<label Name="t" Text="WWWW" Width="360" Height="40"/></div>', "c")
chk("父 color:#ff0000 让子文字变红", red_pixels(c) > 50, red_pixels(c))
chk("默认父容器下没有红字", red_pixels(b) == 0, red_pixels(b))

# 5) text-decoration 不是"继承属性"但会传播到在流后代（CSS 规定）：
#    <div style="text-decoration:underline"> 子树里的文字全带下划线
BASE = ('<div Left="10" Top="10" Width="320" Height="60" %s>'
        '<label Name="t" Text="UNDER" Width="300" Height="40" '
        'style="font-size:24px;color:#ffffff%s"/></div>')

win, ids = load(BASE % ('style="text-decoration:underline"', ""))
chk("父 text-decoration:underline 传到子 label",
    win.get_xml_value(ids["t"], "TextDecoration") == "underline",
    win.get_xml_value(ids["t"], "TextDecoration"))
win.close()

win, ids = load('<div Left="10" Top="10" Width="320" Height="60" style="text-decoration:underline">'
                '<label Name="t" Text="UNDER" Width="300" Height="40" '
                'style="font-size:24px;color:#ffffff;text-decoration:none"/></div>')
chk("子元素 text-decoration:none 取消传播",
    win.get_xml_value(ids["t"], "TextDecoration") in ("", "none"),
    win.get_xml_value(ids["t"], "TextDecoration"))
win.close()

def diff_band(pa, pb):
    from PIL import ImageChops
    ia = Image.open(pa).convert("RGB"); ib = Image.open(pb).convert("RGB")
    d = ImageChops.difference(ia, ib)
    ys = [y for y in range(d.size[1])
          if sum(1 for x in range(d.size[0]) if sum(d.getpixel((x, y))) > 24) >= 4]
    return (ys[0], ys[-1]) if ys else None

pa = shot(BASE % ('style="text-decoration:underline"', ""), "du")
pb = shot(BASE % ('style=""', ""), "dp")
pc = shot('<div Left="10" Top="10" Width="320" Height="60" style="text-decoration:underline">'
          '<label Name="t" Text="UNDER" Width="300" Height="40" '
          'style="font-size:24px;color:#ffffff;text-decoration:none"/></div>', "dn")
b1 = diff_band(pa, pb)
chk("子 label 真的画出下划线", b1 is not None and b1[1] - b1[0] <= 4, b1)
chk("text-decoration:none 子元素无下划线", diff_band(pc, pb) is None, diff_band(pc, pb))

print("t_css_inherit: PASS=%d FAIL=%d" % (12 - len(fails), len(fails)))
