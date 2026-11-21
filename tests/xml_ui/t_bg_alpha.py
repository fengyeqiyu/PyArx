# 第31轮：CSS 半透明底色 background:rgba(r,g,b,a) / #RRGGBBAA
# HTML 里这是最常用的半透明写法；XML 早先只是把原串塞进 BackColor，绘制端认不出（等于无效色）。
# 像素验证用 <div> 容器（容器背板走 DrawBackplate → Blend 真生效）；
# STATIC 控件（label/button）底色仍由 CTLCOLOR 纯色刷子决定，属"背板归属"既有缺口。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def wrap(inner, w=300, h=200):
    p = os.path.join(tempfile.gettempdir(), "bgx_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    return p

def load(inner):
    ids = {}
    return M.guiload(wrap(inner), names=ids), ids

def shot(inner, w=300, h=200):
    png = os.path.join(tempfile.gettempdir(), "bgx_%s.png" % uuid.uuid4().hex[:8])
    win = M.guiload(wrap(inner, w, h))
    M.pump(360)
    win.save_png(png)
    win.close()
    return Image.open(png).convert("RGB")

BOX = '<div Name="g" Left="20" Top="20" Width="200" Height="60" style="%s"/>'

# 1) rgba(0,0,0,0.5) → 不透明底色 + BgAlpha=128
win, ids = load(BOX % "background:rgba(0,0,0,0.5)")
bc = win.get_xml_value(ids["g"], "BackColor").lower()
chk("rgba 拆成 #000000", bc in ("#000000", "000000"), bc)
chk("rgba 的 alpha 落到 BgAlpha=128", win.get_xml_value(ids["g"], "BgAlpha") == "128",
    win.get_xml_value(ids["g"], "BgAlpha"))
win.close()

# 2) #RRGGBBAA（CSS 标准：alpha 在末尾）
win, ids = load(BOX % "background:#33669980")
bc = win.get_xml_value(ids["g"], "BackColor").lower()
chk("#RRGGBBAA 拆成 #336699", bc in ("#336699", "336699"), bc)
chk("#RRGGBBAA 的 alpha 落到 BgAlpha=128", win.get_xml_value(ids["g"], "BgAlpha") == "128",
    win.get_xml_value(ids["g"], "BgAlpha"))
win.close()

# 3) rgba a=1 等于纯色；纯色不产生半透明
win, ids = load(BOX % "background:rgba(255,0,0,1)")
chk("rgba a=1 底色 #FF0000", win.get_xml_value(ids["g"], "BackColor").lower() in ("#ff0000", "ff0000"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()
win, ids = load(BOX % "background:#336699")
chk("纯色不产生半透明", win.get_xml_value(ids["g"], "BgAlpha") in ("", "255"),
    repr(win.get_xml_value(ids["g"], "BgAlpha")))
chk("纯色 BackColor 正常", win.get_xml_value(ids["g"], "BackColor").lower() in ("#336699", "336699"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()

# 4) 文字/边框不受背景 alpha 影响（HTML 语义：只有背景半透明）
win, ids = load('<div Name="g" Left="20" Top="20" Width="200" Height="60" '
                'style="background:rgba(0,0,0,0.5);color:#FFFFFF;border:1px solid #FF0000"/>')
chk("背景 rgba 不影响文字色", win.get_xml_value(ids["g"], "Foreground").lower() in ("#ffffff", "ffffff"),
    win.get_xml_value(ids["g"], "Foreground"))
chk("背景 rgba 不影响边框色", win.get_xml_value(ids["g"], "BorderColor").lower() in ("#ff0000", "ff0000"),
    win.get_xml_value(ids["g"], "BorderColor"))
win.close()

# 5) 老写法 opacity:0.5 不回归（仍是整块 0~1 → 128）
win, ids = load('<div Name="g" Left="20" Top="20" Width="200" Height="60" '
                'style="background:#000000;opacity:0.5"/>')
chk("opacity:0.5 仍映射到 128", win.get_xml_value(ids["g"], "Opacity") == "128",
    win.get_xml_value(ids["g"], "Opacity"))
win.close()

print("t_bg_alpha: PASS=%d FAIL=%d" % (8 - len(fails), len(fails)))
