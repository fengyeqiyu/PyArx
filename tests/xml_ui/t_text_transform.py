# 第42轮：CSS text-transform（uppercase / lowercase / capitalize）
# HTML 里"标签全大写"只需一个声明；XML 早先只能把文案本身写成大写（改数据不改渲染）。
# 判据：契约回读 + 像素跨度（大写比小写宽）。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def render(style, out):
    n = {}
    w = M.guiload_text('<XWindow Width="320" Height="80">'
                       '<label Name="t" Left="10" Top="10" Width="300" Height="40" '
                       'style="color:#FFFFFF;font-size:18;' + style + '">abc def</label></XWindow>', names=n)
    M.pump(200)
    v = w.get_xml_value(n["t"], "TextTransform")
    w.save_png(out); w.close()
    im = Image.open(out).convert("L")
    xs = [x for x in range(im.size[0]) for y in range(34, 74) if im.getpixel((x, y)) > 160]
    return v, ((min(xs), max(xs) - min(xs)) if xs else (0, 0))

v0, w0 = render("", "G:/PyArx/log/tt_0.png")
vu, wu = render("text-transform:uppercase", "G:/PyArx/log/tt_u.png")
vc, wc = render("text-transform:capitalize", "G:/PyArx/log/tt_c.png")
vl, wl = render("text-transform:lowercase", "G:/PyArx/log/tt_l.png")

chk("uppercase 回读", vu == "uppercase", vu)
chk("capitalize 回读", vc == "capitalize", vc)
chk("lowercase 回读", vl == "lowercase", vl)
chk("uppercase 文字更宽（大写比小写宽 ≥3px）", wu[1] >= w0[1] + 3, (w0[1], wu[1]))
chk("未写不产生值", v0 == "", repr(v0))

# 非法值不落（避免"写了却画错"）
n = {}
w = M.guiload_text('<XWindow Width="200" Height="60"><label Name="t" Top="10" Left="10" '
                   'Width="180" style="text-transform:blink">hi</label></XWindow>', names=n)
M.pump(120)
vb = w.get_xml_value(n["t"], "TextTransform")
w.close()
chk("非法值被忽略", vb == "", repr(vb))

print("t_text_transform: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))
