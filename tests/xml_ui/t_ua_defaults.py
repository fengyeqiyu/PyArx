# 第 15 轮：<button> 的 HTML 默认"背板"（底色/边框/字色）+ CSS border 简写真正画得出来
# 浏览器 UA 给 button 的是浅灰底 + 2px 灰框 + 黑字；XML 早先只补了圆角，底色/边框全空，
# 同一段界面里 HTML 是"一块按钮"，XML 只剩浮在背景上的几个字。
# 同时修掉一个真 bug：style="border:2px solid #f00" 解析出的 BorderStyle 是 "solid"，
# 而绘制端只认老写法 Single/Sunken/Raised —— 简写"解析成功、画不出来"。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def shot(inner, tag):
    p = os.path.join(tempfile.gettempdir(), "ua_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="300" Height="240">%s</XWindow>' % inner)
    ids = {}
    win = M.guiload(p, names=ids)
    M.pump(150)
    png = os.path.join(tempfile.gettempdir(), "ua_%s_%s.png" % (tag, uuid.uuid4().hex[:6]))
    win.save_png(png)
    win.close()
    return png, ids, p

def count(png, pred):
    im = Image.open(png).convert("RGB"); px = im.load(); w, h = im.size
    return sum(1 for y in range(30, h) for x in range(w) if pred(px[x, y]))

LIGHT = lambda c: min(c) > 200
GRAY767 = lambda c: abs(c[0] - 118) < 12 and abs(c[1] - 118) < 12 and abs(c[2] - 118) < 12
RED = lambda c: c[0] > 150 and c[1] < 90 and c[2] < 90

# 1) 默认 <button>：浅灰背板 + 灰边框（HTML UA 外观）
png, ids, _ = shot('<button Left="20" Top="20" Text="OK"/>', "btn")
chk("默认 button 有浅灰背板", count(png, LIGHT) > 500, count(png, LIGHT))
chk("默认 button 有灰边框", count(png, GRAY767) > 50, count(png, GRAY767))

# 2) 默认 <input>：1px 灰边框
png, ids, _ = shot('<input Left="20" Top="20" Text="abc"/>', "inp")
chk("默认 input 有灰边框", count(png, GRAY767) > 50, count(png, GRAY767))

# 3) CSS border 简写真的画出来（真 bug 修复）
png, ids, _ = shot('<button Left="20" Top="20" Text="OK" '
                   'style="border:2px solid #ff0000;background:#ffffff;color:#000000"/>', "brd")
chk("border:2px solid #f00 画出红框", count(png, RED) > 50, count(png, RED))

# 4) 作者显式写 background 时默认背板不覆盖
png, ids, _ = shot('<button Left="20" Top="20" Text="OK" style="background:#ff0000"/>', "ovr")
chk("显式 background 覆盖默认浅灰", count(png, RED) > 500, count(png, RED))

# 5) 不加凭空边框：只声明背景的 <div> 不该长出灰框
png, ids, _ = shot('<div Left="20" Top="20" Width="80" Height="30" style="background:#eeeeee"/>', "plain")
chk("只声明背景的 div 没有凭空灰框", count(png, GRAY767) < 10, count(png, GRAY767))

# 6) 属性回读：默认背板/边框落到外观层
p = os.path.join(tempfile.gettempdir(), "ua_rb_%s.xml" % uuid.uuid4().hex[:8])
with open(p, "w", encoding="utf-8") as f:
    f.write('<XWindow Width="300" Height="240"><button Name="b" Left="20" Top="20" Text="OK"/></XWindow>')
ids = {}
win = M.guiload(p, names=ids)
M.pump(120)
chk("默认 button Background=#efefef", win.get_xml_value(ids["b"], "Background").lower() == "#efefef",
    win.get_xml_value(ids["b"], "Background"))
chk("默认 button BorderThickness=2", win.get_xml_value(ids["b"], "BorderThickness") == "2",
    win.get_xml_value(ids["b"], "BorderThickness"))
win.close()

print("t_ua_defaults: PASS=%d FAIL=%d" % (8 - len(fails), len(fails)))
