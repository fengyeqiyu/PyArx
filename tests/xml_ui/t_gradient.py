# 第30轮：CSS linear-gradient 契约（BgImage 解析/回读；纯色完全不回归）
# 注：视觉落地受"背板由谁画"限制（见 docs_notes/html-xml-map.md 缺失项）。
# 第86轮：新增容器（div）渐变的区域扫描像素断言；依赖主线刷子通道落地，
#         本轮仅静态自查，待收尾阶段随可视化套件一起运行验证。
# 第88轮：新增 XButton（OnDrawItem 自绘路径，非 WM_CTLCOLOR）渐变正向像素断言；
#         采样口径同第86轮：沿按钮内偏左 20%~30% 列定位真实按钮带，不写死绝对坐标；
#         依赖主线自绘铺底通道，本轮仅静态自查，待跑通后由主线复验。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=300, h=200):
    p = os.path.join(tempfile.gettempdir(), "t_grad.xml")
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    win = M.guiload(p, names=ids)
    return win, ids

# ---- 第86轮：容器渐变像素采样（区域式，规避 savePng 整窗偏移，不写死绝对坐标） ----
def _lum(px):
    return (px[0] * 30 + px[1] * 59 + px[2] * 11) // 100

def _profile(img, rect, axis, n=7, inset=0.12):
    """沿渐变方向在盒子内框取 n 个采样点。
    axis="v"：180deg 顶→底；axis="h"：90deg 左→右。矩形来自 getRect。
    注意 savePng 抓的是整窗矩形（含标题栏非客户区），getRect 是客户区坐标，
    两者有固定偏移；所以不直接按矩形边取点，而是沿中轴在整窗范围内定位
    真正的渐变带（黑端=亮度最低点，白端=亮度最高点），偏移免疫。"""
    x, y, w, h = rect
    cx = x + w // 2
    cy = y + h // 2
    if axis == "v":
        col = [_lum(img.getpixel((cx, yy))[:3]) for yy in range(img.size[1])]
        # 标题栏在最顶部，跳过前 1/6 高度再找黑/白端，避免抓到标题图标
        start = max(1, img.size[1] // 6)
        lo = start + min(range(len(col) - start), key=lambda i: col[start + i])
        hi = start + max(range(len(col) - start), key=lambda i: col[start + i])
        if lo > hi:
            lo, hi = hi, lo
        return [img.getpixel((cx, int(lo + (hi - lo) * i / (n - 1))))[:3]
                for i in range(n)]
    row = [_lum(img.getpixel((xx, cy))[:3]) for xx in range(img.size[0])]
    start = max(1, img.size[0] // 6)
    lo = start + min(range(len(row) - start), key=lambda i: row[start + i])
    hi = start + max(range(len(row) - start), key=lambda i: row[start + i])
    if lo > hi:
        lo, hi = hi, lo
    return [img.getpixel((int(lo + (hi - lo) * i / (n - 1)), cy))[:3]
            for i in range(n)]

def _region_colors(img, rect, step=6):
    """内框粗扫（step 像素一格），返回区域内出现过的颜色集合，用于判非纯色。"""
    x, y, w, h = rect
    x0, y0 = x + int(w * 0.1), y + int(h * 0.1)
    x1, y1 = x + w - int(w * 0.1), y + h - int(h * 0.1)
    out = set()
    for yy in range(y0, y1, step):
        for xx in range(x0, x1, step):
            out.add(img.getpixel((xx, yy))[:3])
    return out

def shot_div(inner, tag):
    p = os.path.join(tempfile.gettempdir(), "tgrad_%s.xml" % tag)
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="300" Height="200">%s</XWindow>' % inner)
    ids = {}
    win = M.guiload(p, names=ids)
    M.pump(300)
    png = os.path.join(tempfile.gettempdir(), "tgrad_%s.png" % tag)
    win.save_png(png)
    rect = list(win.get_rect(ids["g"]))
    win.close()
    return rect, Image.open(png).convert("RGB")

# 1) 180deg 渐变被完整保留
win, ids = load('<button Name="g" Left="20" Top="20" Width="200" Height="120" '
                'style="background:linear-gradient(180deg,#000000,#FFFFFF)">G</button>')
got = win.get_xml_value(ids["g"], "BgImage")
print("  debug got=", repr(got), " ids=", ids.get("g"))
chk("180deg 渐变保留原串", "gradient(180deg" in got.replace(" ", "").lower(), got)
win.close()

# 2) 90deg 写法保留
win, ids = load('<button Name="g" Left="20" Top="20" Width="200" Height="120" '
                'style="background:linear-gradient(90deg,#000,#FFF)">G</button>')
got = win.get_xml_value(ids["g"], "BgImage")
chk("90deg 渐变保留原串", "90deg" in got.lower(), got)
win.close()

# 3) to bottom 方向写法保留
win, ids = load('<button Name="g" Left="20" Top="20" Width="200" Height="120" '
                'style="background:linear-gradient(to bottom,#111,#222)">G</button>')
got = win.get_xml_value(ids["g"], "BgImage")
chk("to bottom 写法保留", "to bottom" in got.lower(), got)
win.close()

# 4) 纯色不产生 BgImage，仍走 BackColor（不回归）
win, ids = load('<button Name="g" Left="20" Top="20" Width="200" Height="120" '
                'style="background-color:#123456">G</button>')
chk("纯色不产生 BgImage", win.get_xml_value(ids["g"], "BgImage") == "",
    win.get_xml_value(ids["g"], "BgImage"))
chk("纯色仍写进 BackColor", win.get_xml_value(ids["g"], "BackColor").lower() in ("#123456", "123456"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()

# 5) background 纯色简写同样不产生 BgImage
win, ids = load('<button Name="g" Left="20" Top="20" Width="200" Height="120" '
                'style="background:#336699">G</button>')
chk("background 纯色简写不产生 BgImage", win.get_xml_value(ids["g"], "BgImage") == "",
    win.get_xml_value(ids["g"], "BgImage"))
chk("background 纯色简写写进 BackColor", win.get_xml_value(ids["g"], "BackColor").lower() in ("#336699", "336699"),
    win.get_xml_value(ids["g"], "BackColor"))
win.close()

# 6) 声明渐变不影响控件几何/可见（不崩、矩形正常）
win, ids = load('<button Name="g" Left="20" Top="20" Width="200" Height="120" '
                'style="background:linear-gradient(180deg,#000,#FFF)">G</button>')
r = win.get_rect(ids["g"])
chk("声明渐变后控件矩形正常 (20,20,200,120)", list(r) == [20, 20, 200, 120], r)
win.close()

# 7) 容器（div）linear-gradient 真出过渡色：区域扫描像素断言，而不是纯色。
#    采样口径：180deg 顶→底，内缩 12% 取 7 点；亮度单调（容差 8/255）、
#    相邻采样点平均亮度步长 > 12/255、首尾贴近声明色 #000000 / #FFFFFF；
#    内框 6px 粗扫唯一色数 > 1 判非纯色。依赖主线刷子通道，待收尾阶段运行验证。
rect, img = shot_div('<div Name="g" Left="20" Top="20" Width="200" Height="120" '
                     'style="background:linear-gradient(180deg,#000000,#FFFFFF)"/>', "v")
prof = _profile(img, rect, "v", n=7)
lum = [_lum(c) for c in prof]

colors = _region_colors(img, rect)
chk("容器渐变区域非纯色（唯一色数>1）", len(colors) > 1, (len(colors), rect))

mono = all(lum[i + 1] >= lum[i] - 8 for i in range(len(lum) - 1))
chk("180deg 沿顶→底亮度单调", mono, lum)

steps = [abs(lum[i + 1] - lum[i]) for i in range(len(lum) - 1)]
chk("相邻采样点平均亮度差超过阈值(12/255)", sum(steps) / len(steps) > 12, (lum, steps))

chk("首端贴近声明色 #000000", sum(prof[0]) <= 45, prof[0])
chk("末端贴近声明色 #FFFFFF", sum(prof[-1]) >= 210, prof[-1])

# ---- 第88轮：XButton 渐变像素采样（OnDrawItem 自绘路径，非 WM_CTLCOLOR；口径同第86轮） ----
PAGE_BTN = 0x20 * 3   # 页面底色 #202020 三通道和（与 t_box_shadow 一致）

def shot_btn(inner, tag):
    p = os.path.join(tempfile.gettempdir(), "tgrad_%s.xml" % tag)
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="300" Height="200" Background="#202020">%s</XWindow>' % inner)
    ids = {}
    win = M.guiload(p, names=ids)
    M.pump(300)
    png = os.path.join(tempfile.gettempdir(), "tgrad_%s.png" % tag)
    win.save_png(png)
    rect = list(win.get_rect(ids["g"]))
    win.close()
    return rect, Image.open(png).convert("RGB")

def locate_button_band(img, rect, fx=0.25):
    """沿按钮内偏左 fx 列在整窗内定位真实按钮带（偏移免疫，不写死绝对坐标）。
    savePng 抓整窗（含标题栏非客户区），getRect 是客户区坐标，有 ~31px 纵向偏移；
    跳过顶部 1/6（标题栏），把明显浅于页面底色 #202020 的连续行段判为按钮带。
    取样列选 25% 宽处：按钮文字水平居中，两个汉字字形落在中列一带；20%~30% 列在
    字形左侧之外，不被文字前景色污染。返回 (col, yTop, yBottom)，找不到时为 None。"""
    x, y, w, h = rect
    col = x + int(w * fx)
    start = max(1, img.size[1] // 6)
    rows = []
    for yy in range(start, img.size[1] - 4):
        if sum(img.getpixel((col, yy))[:3]) > PAGE_BTN + 30:
            rows.append(yy)
    if not rows:
        return col, None, None
    return col, rows[0], rows[-1]

# 8) XButton 180deg 渐变正向像素断言。深灰蓝 #4A5568→#2D3748，加权亮度 83→53
#    （差 30/255，明暗差足够判）；按钮 140×36（≥120×32）。阈值依据：
#    - 单调容差 8/255 同第86轮；
#    - 平均步长 > 4/255：两端亮度差 30，7 点均分每段 ≈5；纯色台阶步长≈0，取 4 为
#      "真过渡"下限（86 轮 div 色标差 255 用 12；本轮色标差小，阈值按比例收窄）；
#    - 端点逐通道容差 18/255：端点即色标理论值，软件栅格化首末行 + 边框/圆角抗锯齿
#      会平移几个色阶；18 对应亮度约 6~7/255，远小于两端 30/255 的落差。
rect, img = shot_btn('<XButton Name="g" Left="40" Top="60" Width="140" Height="36" '
                     'style="background:linear-gradient(180deg,#4A5568,#2D3748)">渐变</XButton>',
                     "btn")
col, yt, yb = locate_button_band(img, rect)
if yt is None:
    chk("按钮渐变：定位到按钮带", False, "未找到浅于页底 #202020 的按钮行段")
else:
    n = 7
    ys = [yt + 2 + (yb - yt - 4) * i // (n - 1) for i in range(n)]
    prof = [img.getpixel((col, yy))[:3] for yy in ys]
    lum = [_lum(c) for c in prof]

    # B1 按钮面非纯色：20%/30% 两列、行距 4px 粗扫唯一色数 > 1
    uniques = set()
    for yy in range(yt + 2, yb - 1, 4):
        for fx in (0.20, 0.30):
            uniques.add(img.getpixel((rect[0] + int(rect[2] * fx), yy))[:3])
    chk("按钮渐变区域非纯色（唯一色数>1）", len(uniques) > 1,
        {"uniques": len(uniques), "band": (yt, yb), "col": col})

    # B2 180deg 顶→底亮度单调递减（容差 8/255）
    mono = all(lum[i + 1] <= lum[i] + 8 for i in range(n - 1))
    chk("按钮 180deg 沿顶→底亮度单调递减", mono,
        {"prof": prof, "lum": lum, "ys": ys, "band": (yt, yb)})

    # B3 相邻采样点平均亮度步长 > 4/255（真过渡，非纯色台阶）
    steps = [abs(lum[i + 1] - lum[i]) for i in range(n - 1)]
    chk("按钮相邻采样点平均亮度步长 > 4/255", sum(steps) / len(steps) > 4,
        {"lum": lum, "steps": steps, "ys": ys, "band": (yt, yb)})

    # B4 顶端贴近 #4A5568、底端贴近 #2D3748（逐通道差 ≤18/255）
    TOP = (0x4A, 0x55, 0x68)
    BOT = (0x2D, 0x37, 0x48)
    tol = 18
    top_ok = all(abs(prof[0][k] - TOP[k]) <= tol for k in range(3))
    bot_ok = all(abs(prof[-1][k] - BOT[k]) <= tol for k in range(3))
    chk("按钮顶端贴近 #4A5568 / 底端贴近 #2D3748", top_ok and bot_ok,
        {"top": prof[0], "bottom": prof[-1], "TOP": TOP, "BOT": BOT, "tol": tol,
         "prof": prof, "ys": ys})

# ---- 第89轮：XTextField（EDIT）渐变像素断言（缺口规格，当前预期红） ----
# 背景：Edit/RICHEDIT50W 在 isSkinPaintClass 接管表内（PyMfcHost.cpp:224），
# 被换 PyMfcSkinWindowProc；WM_PAINT/WM_PRINTCLIENT 走 paintSkinControl →
# drawSkinControl → drawEdit，底色只由 pal.back 纯色铺底（PyMfcSkin.h:510），
# ap->bgImage 渐变从未参与；第88轮挂在 WM_CTLCOLOREDIT 的位图刷工厂被子类化旁路。
# 本组四条与按钮 B1–B4 同口径，作为下轮皮肤层接渐变铺底后的验收规格；
# 本轮运行时未生效，断言预期 FAIL，根因与替代落点见 mfcxml-audit-log 第89轮。
rect, img = shot_btn('<XTextField Name="g" Left="40" Top="60" Width="140" Height="28" '
                     'style="background:linear-gradient(180deg,#4A5568,#2D3748)"/>',
                     "edit")
col, yt, yb = locate_button_band(img, rect)
if yt is None:
    chk("输入框渐变：定位到输入框带", False, "未找到非页底 #202020 的输入框行段")
else:
    n = 7
    ys = [yt + 2 + (yb - yt - 4) * i // (n - 1) for i in range(n)]
    prof = [img.getpixel((col, yy))[:3] for yy in ys]
    lum = [_lum(c) for c in prof]

    # E1 输入框面非纯色：20%/30% 两列、行距 4px 粗扫唯一色数 > 1
    uniques = set()
    for yy in range(yt + 2, yb - 1, 4):
        for fx in (0.20, 0.30):
            uniques.add(img.getpixel((rect[0] + int(rect[2] * fx), yy))[:3])
    chk("输入框渐变区域非纯色（唯一色数>1）", len(uniques) > 1,
        {"uniques": len(uniques), "band": (yt, yb), "col": col})

    # E2 180deg 顶→底亮度单调递减（容差 8/255，同 B2）
    mono = all(lum[i + 1] <= lum[i] + 8 for i in range(n - 1))
    chk("输入框 180deg 沿顶→底亮度单调递减", mono,
        {"prof": prof, "lum": lum, "ys": ys, "band": (yt, yb)})

    # E3 相邻采样点平均亮度步长 > 4/255（色标加权亮度 83→53，7 点均分每段 ≈5，
    #    纯色台阶步长≈0；阈值依据同 B3）
    steps = [abs(lum[i + 1] - lum[i]) for i in range(n - 1)]
    chk("输入框相邻采样点平均亮度步长 > 4/255", sum(steps) / len(steps) > 4,
        {"lum": lum, "steps": steps, "ys": ys, "band": (yt, yb)})

    # E4 顶端贴近 #4A5568、底端贴近 #2D3748（逐通道差 ≤18/255，容差依据同 B4）
    TOP = (0x4A, 0x55, 0x68)
    BOT = (0x2D, 0x37, 0x48)
    tol = 18
    top_ok = all(abs(prof[0][k] - TOP[k]) <= tol for k in range(3))
    bot_ok = all(abs(prof[-1][k] - BOT[k]) <= tol for k in range(3))
    chk("输入框顶端贴近 #4A5568 / 底端贴近 #2D3748", top_ok and bot_ok,
        {"top": prof[0], "bottom": prof[-1], "TOP": TOP, "BOT": BOT, "tol": tol,
         "prof": prof, "ys": ys})

print("t_gradient: PASS=%d FAIL=%d" % (20 - len(fails), len(fails)))
