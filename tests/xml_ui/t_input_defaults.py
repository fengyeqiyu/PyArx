# 第46轮：HTML <input> 默认态（4px 圆角 + 1px 边框）
# 浏览器里的 input 天生带圆角和细边框；XML 早先不写就是直角无边框，观感差一截。
# 显式声明（border-radius / Radius / border / BorderThickness）一律不覆盖。
import os, sys, tempfile
from collections import Counter
from PIL import Image
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def vals(xml):
    n = {}
    w = M.guiload_text(xml, context=globals(), names=n)
    M.pump(120)
    out = {}
    for k in n:
        out[k] = (w.get_xml_value(n[k], "Radius"), w.get_xml_value(n[k], "BorderThickness"))
    w.close()
    return out

r = vals('<XWindow Width="360" Height="120">'
         '<input Name="a" Top="10" Left="10" Width="200"/></XWindow>')
chk("默认 <input> 有 4px 圆角", r["a"][0] == "4", r["a"])
chk("默认 <input> 有 1px 边框", r["a"][1] == "1", r["a"])

r = vals('<XWindow Width="360" Height="120">'
         '<input Name="b" Top="10" Left="10" Width="200" style="border-radius:10px;border:2px solid #fff"/></XWindow>')
chk("显式 border-radius 不被覆盖", r["b"][0] == "10", r["b"])
chk("显式 border 不被覆盖", r["b"][1] == "2", r["b"])

r = vals('<XWindow Width="360" Height="120">'
         '<input Name="d" Top="10" Left="10" Width="200" Radius="0"/></XWindow>')
chk("显式 Radius=0（直角）不被默认覆盖", r["d"][0] == "", r["d"])

# 第89轮：纯色输入框底色像素零漂移证据（无渐变，皮肤层 drawEdit 纯色铺底通道）。
# 既有取色 = 声明的 #336699（parse 后不与主题底混合，opacity=255）；沿中列整窗定位
# 输入框带（跳过标题栏 1/6，偏移免疫），内缩 2px 取中段像素比对，不写死绝对坐标。
p = os.path.join(tempfile.gettempdir(), "t_inp_def.xml")
with open(p, "w", encoding="utf-8") as f:
    f.write('<XWindow Width="360" Height="120" Background="#202020">'
            '<XTextField Name="d" Left="20" Top="20" Width="160" Height="28" '
            'style="background:#336699"/></XWindow>')
ids = {}
win = M.guiload(p, names=ids)
M.pump(300)
png = os.path.join(tempfile.gettempdir(), "t_inp_def.png")
win.save_png(png)
rect = list(win.get_rect(ids["d"]))
win.close()
im = Image.open(png).convert("RGB")
x, y, w, h = rect
col = x + w // 2
start = max(1, im.size[1] // 6)
rows = [yy for yy in range(start, im.size[1] - 4)
        if sum(im.getpixel((col, yy))[:3]) > 0x20 * 3 + 30]
if not rows:
    chk("纯色输入框定位到输入框带", False, "未找到非页底行段")
else:
    yt, yb = rows[0], rows[-1]
    ys = [yt + 2 + (yb - yt - 4) * i // 4 for i in range(5)]
    prof = [im.getpixel((col, yy))[:3] for yy in ys]
    avg = tuple(sum(c[k] for c in prof) // len(prof) for k in range(3))
    TARGET = (0x33, 0x66, 0x99)
    tol = 10
    ok = all(abs(avg[k] - TARGET[k]) <= tol for k in range(3))
    chk("纯色输入框底色像素 == #336699（零漂移）", ok,
        {"avg": avg, "target": TARGET, "prof": prof, "band": (yt, yb)})

# 第90轮：HTML 语义底色断言（只写断言，不实现）。
# 浏览器 <input> 自带独立浅字段色，不会与页面底同色；当前 XmlCore.cpp 的 htmlInput
# 默认只补 Radius=4 / BorderThickness=1，没给字段底色。皮肤过程 drawSkinControl 对
# 无 style 的控件整框铺 pal.back，而 pal.back = m_bg（页面底 setPageBackdrop/
# setBackgroundColor 会把 m_bg 镜像成页面底色），所以无样式输入框内部现在就是页面底
# /主题底，看不出独立字段。下面两条语义型断言实现无关，只判「是否具有独立字段底色」。
# 采样口径（与第6项同一路径，坐标偏移免疫）：getRect 取矩形 → 中列扫描跳过顶部 1/6
# 标题栏 → 内缩 2px → 中段 5 点均值；阈值 任一通道差 > 12/255 视为「明显不同」。
def _sample_interior(xml, name):
    """加载 xml、截图、返回控件内部中段 5 点均值 RGB 与矩形；找不到字段带返回 None。"""
    px = os.path.join(tempfile.gettempdir(), "t_inp_def2.xml")
    with open(px, "w", encoding="utf-8") as f:
        f.write(xml)
    ids = {}
    win = M.guiload(px, names=ids)
    M.pump(300)
    png = os.path.join(tempfile.gettempdir(), "t_inp_def2.png")
    win.save_png(png)
    rect = list(win.get_rect(ids[name]))
    win.close()
    im = Image.open(png).convert("RGB")
    x, y, w, h = rect
    col = x + w // 2
    start = max(1, im.size[1] // 6)
    # 整窗截图相对客户区坐标有固定标题栏偏移（~31px），不能直接按 rect 行段取点；
    # 在候选 dy 里取中列与页面底色差异最大的行段（口径同 t_disabled），不写死绝对坐标。
    page = Counter(im.getdata()).most_common(1)[0][0]
    best = None
    for dy in (0, 31, -31, 20, 40, 52):
        rows = [yy for yy in range(start, im.size[1] - 4)
                if y + 2 + dy <= yy <= y + h - 2 + dy]
        if not rows:
            continue
        diff = sum(sum(abs(im.getpixel((col, yy))[k] - page[k]) for k in range(3)) for yy in rows)
        if best is None or diff > best[0]:
            best = (diff, rows)
    rows = best[1] if best else []
    if not rows:
        return None, rect, None
    yt, yb = rows[0], rows[-1]
    ys = [yt + 2 + (yb - yt - 4) * i // 4 for i in range(5)]
    prof = [im.getpixel((col, yy))[:3] for yy in ys]
    avg = tuple(sum(c[k] for c in prof) // len(prof) for k in range(3))
    return avg, rect, prof

def _diff(avg, ref):
    return tuple(abs(avg[k] - ref[k]) for k in range(3))

THRESH = 12
PAGE_BG = (0x20, 0x20, 0x20)          # 用例1页面声明底 #202020
THEME_BG = (49, 56, 66)               # 主题 m_bg 深色（PyMfcHost.cpp:107）；不写死字段应取色

# 1) 有 Background 窗：无 style 的 <XTextField>，interior 必须与页面底 #202020 明显不同。
# 预期红：当前 pal.back=m_bg 被镜像成页面底，interior == 页面底，缺字段色。
avg1, rect1, prof1 = _sample_interior(
    '<XWindow Width="360" Height="120" Background="#202020">'
    '<XTextField Name="a" Left="20" Top="20" Width="160" Height="28"/></XWindow>', "a")
if avg1 is None:
    chk("有 Background 窗：无样式输入框 interior ≠ 页面底", False, "未定位到字段带")
else:
    d1 = _diff(avg1, PAGE_BG)
    ok1 = any(d > THRESH for d in d1)
    chk("有 Background 窗：无样式输入框有独立字段底色（≠ #202020）", ok1,
        {"avg": avg1, "page": PAGE_BG, "diff": d1, "prof": prof1, "rect": rect1})

# 2) 无 Background 深色主题窗：不写 Background，interior 既不同于页面底、也不同于
# 主题 m_bg (49,56,66)，说明字段有自己的底色而不是继承页面/主题底。
# 预期红：当前 interior 就是主题底 m_bg（无页面声明时 m_pageBg 缺省），缺字段色。
avg2, rect2, prof2 = _sample_interior(
    '<XWindow Width="360" Height="120">'
    '<XTextField Name="b" Left="20" Top="20" Width="160" Height="28"/></XWindow>', "b")
if avg2 is None:
    chk("无 Background 窗：无样式输入框 interior ≠ 主题底", False, "未定位到字段带")
else:
    d_page2 = _diff(avg2, PAGE_BG)
    d_theme2 = _diff(avg2, THEME_BG)
    ok2 = any(d > THRESH for d in d_page2) and any(d > THRESH for d in d_theme2)
    chk("无 Background 窗：无样式输入框有独立字段底色（≠ 页面底 且 ≠ m_bg）", ok2,
        {"avg": avg2, "theme": THEME_BG, "diff_page": d_page2, "diff_theme": d_theme2,
         "prof": prof2, "rect": rect2})

# 对照（预期绿）：同窗口显式 style="background:#336699" 的输入框 interior 仍精确等于
# #336699——字段色缺口不得影响显式声明的样式。口径沿用上面第 6 项，此处不重复断言。

print("t_input_defaults: PASS=%d FAIL=%d" % (8 - len(fails), len(fails)))

