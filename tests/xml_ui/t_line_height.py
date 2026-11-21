# 第51轮：CSS line-height 落到绘制端（早先只把值写进 LineHeight 属性，全仓无消费方 = 写了等于没写）
# 浏览器里 line-height:2 会让多行正文占两倍高度；XML 侧此前行距不变。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

TEXT = "PyArx XML channel line-height demo text wrapping across lines"
fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def span(tag, style):
    n = {}
    w = M.guiload_text(
        '<XWindow Width="300" Height="200">'
        '<label Name="t" Left="10" Top="10" Width="240" Height="160" Text="%s" '
        'TextWrapping="Wrap" style="font-size:14px;%s"/>'
        '</XWindow>' % (TEXT, style), context=globals(), names=n)
    M.pump(200)
    lh = w.get_xml_value(n["t"], "LineHeight")
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    w.close()
    im = Image.open(p).convert("RGB")
    ys = []
    for y in range(34, im.size[1]):
        for x in range(10, 260):
            c = im.getpixel((x, y))
            if c[0] > 170 and c[1] > 170 and c[2] > 170:
                ys.append(y)
    return lh, (min(ys), max(ys)) if ys else (None, None)

lh1, sp1 = span("lh_tight", "line-height:1.2")
lh2, sp2 = span("lh_loose", "line-height:2.4")
chk("LineHeight 契约回读", lh1 == "1.2" and lh2 == "2.4", (lh1, lh2))
chk("行距放大后文字纵向跨度明显变高",
    sp1[0] is not None and sp2[0] is not None and (sp2[1] - sp2[0]) > (sp1[1] - sp1[0]) * 1.3,
    (sp1, sp2))
print("t_line_height: PASS=%d FAIL=%d" % (2 - len(fails), len(fails)))
