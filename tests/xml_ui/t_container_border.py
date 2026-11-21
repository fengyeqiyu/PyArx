# 第 16 轮：容器（<div>/面板）自己的 CSS 边框终于画得出来 + 容器位图刷 R/B 互换真 bug
# 浏览器里 <div style="border:2px solid red"> 是一圈红框；XML 里这圈框永远消失，
# 因为容器是空 STATIC、宿主 DC 的描边又被 WS_CLIPCHILDREN 裁掉。
# 顺带发现：容器位图刷写 DIB 时把 COLORREF(0x00BBGGRR) 直接当 0x00RRGGBB 用，
# linear-gradient(#ff0000) 在 XML 里会画成一片纯蓝 —— 同一处修复。
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
    p = os.path.join(tempfile.gettempdir(), "cb_%s.xml" % uuid.uuid4().hex[:6])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="300" Height="200">%s</XWindow>' % inner)
    ids = {}
    win = M.guiload(p, names=ids)
    M.pump(200)
    png = os.path.join(tempfile.gettempdir(), "cb_%s_%s.png" % (tag, uuid.uuid4().hex[:6]))
    win.save_png(png)
    win.close()
    return png

def count(png, pred, box=None):
    im = Image.open(png).convert("RGB"); px = im.load(); w, h = im.size
    x0, y0, x1, y1 = box if box else (0, 30, w, h)
    return sum(1 for y in range(y0, y1) for x in range(x0, x1) if pred(px[x, y]))

RED = lambda c: c[0] > 170 and c[1] < 80 and c[2] < 80
BLUE = lambda c: c[2] > 170 and c[0] < 80 and c[1] < 80

# 1) <div> 写 CSS 边框：真画出来（15 轮遗留缺口）
png = shot('<div Left="20" Top="20" Width="120" Height="80" '
           'style="background:#eeeeee;border:2px solid #ff0000"/>', "bd")
chk("div border:2px solid #f00 画出红框", count(png, RED) > 300, count(png, RED))

# 2) 没写 border 的 div 不会凭空长框（拿 #ff0000 作哨兵：不该出现红像素）
png = shot('<div Left="20" Top="20" Width="120" Height="80" style="background:#eeeeee"/>', "nb")
chk("无 border 声明的 div 没有红框像素", count(png, RED) == 0, count(png, RED))

# 3) 老 XML 写法 BorderThickness + BorderColor 同样生效（两条通路合流）
png = shot('<div Left="20" Top="20" Width="120" Height="80" '
           'BorderThickness="2" BorderColor="#ff0000" style="background:#ffffff"/>', "old")
chk("BorderThickness+BorderColor 老写法也出框", count(png, RED) > 300, count(png, RED))

# 4) 圆角容器照样有边框（边框按角心环带判定）
png = shot('<div Left="20" Top="20" Width="120" Height="80" '
           'style="background:#ffffff;border-radius:12px;border:2px solid #ff0000"/>', "rad")
chk("圆角容器边框仍然可见", count(png, RED) > 250, count(png, RED))

# 5) 真 bug：容器 linear-gradient(#ff0000) 之前画成纯蓝（DIB 里 R/B 互换）
png = shot('<div Left="20" Top="20" Width="120" Height="80" '
           'style="background:linear-gradient(#ff0000,#ff0000)"/>', "gd")
chk("linear-gradient(#ff0000) 是红色而非蓝色", count(png, RED) > 5000 and count(png, BLUE) == 0,
    (count(png, RED), count(png, BLUE)))

# 6) 渐变仍在（两色渐变两端各自成色）
png = shot('<div Left="20" Top="20" Width="120" Height="80" '
           'style="background:linear-gradient(#ff0000,#0000ff)"/>', "gd2")
chk("红蓝渐变两端各自成色",
    count(png, RED, (20, 40, 140, 60)) > 500 and count(png, BLUE, (20, 90, 140, 112)) > 500,
    (count(png, RED, (20, 40, 140, 60)), count(png, BLUE, (20, 90, 140, 112))))

# ---- 第 17 轮：线型（dashed/dotted/double/groove/ridge）与圆角外角 ----
# 17 轮前 BorderStyle 会被归一成 Single，浏览器里一条虚线在 XML 里永远是实线。
BOX = 'Left="20" Top="20" Width="120" Height="80"'
def row(png, y=70):
    im = Image.open(png).convert("RGB"); px = im.load()
    return [px[x, y] for x in range(28, 40)]   # 截图相对客户区固定偏移 +8/+31

solid = shot('<div %s style="background:#eeeeee;border:6px solid #ff0000"/>' % BOX, "s6")
dashed = shot('<div %s style="background:#eeeeee;border:6px dashed #ff0000"/>' % BOX, "d6")
chk("dashed 比 solid 明显少画（真画成虚线）", count(dashed, RED) < count(solid, RED) * 0.7,
    (count(dashed, RED), count(solid, RED)))
def edge_line(png, y=53, x0=28, x1=146):
    im = Image.open(png).convert("RGB"); px = im.load()
    return [px[x, y] for x in range(x0, x1)]   # 顶边框那一行，扫整条边

chk("dashed 沿边既有实线段也有空隙段",
    any(RED(c) for c in edge_line(dashed)) and any(not RED(c) for c in edge_line(dashed)),
    (sum(1 for c in edge_line(dashed) if RED(c)), len(edge_line(dashed))))
dotted = shot('<div %s style="background:#eeeeee;border:6px dotted #ff0000"/>' % BOX, "t6")
chk("dotted 比 solid 明显少画（真画成点线）", count(dotted, RED) < count(solid, RED) * 0.7,
    (count(dotted, RED), count(solid, RED)))

dbl = shot('<div %s style="background:#eeeeee;border:6px double #ff0000"/>' % BOX, "db6")
r = row(dbl)
chk("double 是两条线中间留缝", RED(r[0]) and RED(r[1]) and not RED(r[2]) and RED(r[4]),
    [(c[0]) for c in r[:6]])

gr = shot('<div %s style="background:#eeeeee;border:6px groove #ff0000"/>' % BOX, "gr6")
ri = shot('<div %s style="background:#eeeeee;border:6px ridge #ff0000"/>' % BOX, "ri6")
rg, rr = row(gr), row(ri)
chk("groove 外暗内亮", rg[0][0] < 200 and rg[3][0] > rg[0][0], (rg[0], rg[3]))
chk("ridge 外亮内暗", rr[0][0] > 200 and rr[3][0] < rr[0][0], (rr[0], rr[3]))

# 圆角外角：CSS 里这块不属于盒子，必须露页面底色（之前一律涂成盒子底色）
rad2 = shot('<div %s style="background:#ffffff;border-radius:16px"/>' % BOX, "rad2")
im = Image.open(rad2).convert("RGB"); px = im.load()
corner_out = px[13, 44]     # 盒子左上角外侧（半径圆外）
corner_in = px[60, 70]      # 盒子中心
chk("圆角外角露出页面底色", abs(corner_out[0] - 49) < 10 and abs(corner_out[2] - 66) < 10
    and corner_in == (255, 255, 255), (corner_out, corner_in))

print("t_container_border: PASS=%d FAIL=%d" % (13 - len(fails), len(fails)))

