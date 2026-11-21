# 第40轮：CSS 颜色短写 `#RGB` / `#RGBA`（HTML 里 #fff、#333 是标准简写）
# 此前 ParseColor 只认 #RRGGBB / #AARRGGBB → `background:#fff` 静默画不出底色（写了等于没写）。
# 说明：像素级验证受"容器背板由谁画"既有缺口限制（div/label 背板在 savePng 下不出像素），
# 本轮用契约层验（BackColor 原样 + 4 位短写必须解析出 BgAlpha）。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=300, h=200):
    p = os.path.join(tempfile.gettempdir(), "cs_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

BOX = '<div Name="g" Left="20" Top="20" Width="200" Height="60" style="background:%s"/>'

# 1) 3 位短写被接受并展开成 #RRGGBB（#fff = #ffffff）
win, ids = load(BOX % "#fff")
chk("#fff 展开成 #ffffff", win.get_xml_value(ids["g"], "BackColor").lower() in ("#ffffff", "ffffff"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()

# 2) 4 位短写：alpha 末位 8 → 0x88 = 136（证明 ParseColor 真解析了 #RGBA）
win, ids = load(BOX % "#0008")
chk("#0008 解析出 BgAlpha=136", win.get_xml_value(ids["g"], "BgAlpha") == "136",
    win.get_xml_value(ids["g"], "BgAlpha"))
chk("#0008 底色 #000000", win.get_xml_value(ids["g"], "BackColor").lower() in ("#000000", "000000"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()

# 3) 老 6 位写法不回归
win, ids = load(BOX % "#336699")
chk("#RRGGBB 不回归", win.get_xml_value(ids["g"], "BackColor").lower() in ("#336699", "336699"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()

print("t_color_short: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
