# 第39轮：CSS `text-decoration:underline`（XML 侧此前完全不支持）
# 第5轮补修：下划线落点按 HTML 语义贴文字基线。早先实现用"盒底内缩 3px"，
# 盒子一高（或文字改垂直居中/顶对齐）整条线就脱离文字；同时旧测试窗口只有 90px 高，
# 40px 的 label 底部被窗口裁掉，像素层永远拍不到，才长期挂 XFAIL —— 是探针口径问题。
# HTML 对照：文字 24px、盒子 40px 或 70px，下划线都紧贴文字最后一行下方。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image, ImageChops

fails = []
total = 0
def chk(name, cond, got=""):
    global total
    total += 1
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=300, h=120):
    p = os.path.join(tempfile.gettempdir(), "td_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return p, M.guiload(p, names=ids), ids

# 1) 契约：text-decoration:underline → TextDecoration=underline
p1, win, ids = load('<label Name="a" Left="10" Top="10" style="text-decoration:underline">下划线</label>')
chk("underline 回读", win.get_xml_value(ids["a"], "TextDecoration").lower() == "underline",
    win.get_xml_value(ids["a"], "TextDecoration"))
win.close()

# 2) text-decoration: none 不误开
p2, win, ids = load('<label Name="b" Left="10" Top="10" style="text-decoration:none">普通</label>')
chk("none 不误开", win.get_xml_value(ids["b"], "TextDecoration").lower() != "underline",
    win.get_xml_value(ids["b"], "TextDecoration"))
win.close()

# 2b) 删除线：HTML 的 <del> 默认就是 line-through
p2b, win, ids = load('<label Name="s" Left="10" Top="10" style="text-decoration:line-through">删除</label>')
chk("line-through 回读", win.get_xml_value(ids["s"], "TextDecoration").lower() == "line-through",
    win.get_xml_value(ids["s"], "TextDecoration"))
win.close()

# 3) 视觉：窗口够高，label 完整落在客户区里（旧版 90px 高的窗口把盒底裁掉了）
def shot(styled, box_h):
    xml = ('<label Left="20" Top="20" Width="200" Height="%d" '
           'style="font-size:24px;color:#ffffff;%s">UNDER</label>'
           % (box_h, 'text-decoration:underline' if styled else ''))
    pth, win, ids = load(xml, 260, 140)
    png = os.path.join(tempfile.gettempdir(), "td_%s.png" % uuid.uuid4().hex[:6])
    win.save_png(png)
    win.close()
    return png

# label 占 x∈[20,220]；标题栏约 30px，客户区从 ~31 开始
def rows_info(png):
    im = Image.open(png).convert("RGB"); px = im.load(); w, h = im.size
    return im, px, w, h

def text_span(png):
    """无装饰那张里，label 区域内文字在纵向所占的行区间（近白像素）。"""
    im, px, w, h = rows_info(png)
    ys = [y for y in range(30, h) if any(sum(px[x, y]) > 600 for x in range(20, 220))]
    return (ys[0], ys[-1]) if ys else (0, 0)

def diff_band(pa, pb):
    """两张图的差异像素所在行区间（下划线本体）。"""
    ia = Image.open(pa).convert("RGB"); ib = Image.open(pb).convert("RGB")
    diff = ImageChops.difference(ia, ib)
    ys = []
    for y in range(diff.size[1]):
        row = [diff.getpixel((x, y)) for x in range(diff.size[0])]
        if sum(1 for px in row if sum(px) > 24) >= 4:
            ys.append(y)
    return (ys[0], ys[-1]) if ys else None

for box_h, tag in ((40, "标准盒"), (70, "加高盒")):
    pa = shot(True, box_h); pb = shot(False, box_h)
    band = diff_band(pa, pb)
    tb = text_span(pb)[1]
    chk("%s 有下划线像素" % tag, band is not None, band)
    if band is not None:
        # 下划线应紧贴文字最后一行下方 1..8px（HTML 基线位置）。
        # 若退回"盒底 - 3px"实现，加高盒的线会落到文字下方 30px 开外，这里会红。
        chk("%s 下划线贴基线" % tag, 1 <= band[0] - tb <= 8, (tb, band))
        chk("%s 下划线是细线" % tag, band[1] - band[0] <= 3, band)

# 3e) 删除线：线要穿过字面中部，而不是贴在文字下面
def shot_del(styled):
    xml = ('<label Left="20" Top="20" Width="200" Height="40" '
           'style="font-size:24px;color:#ffffff;%s">STRIKE</label>'
           % ('text-decoration:line-through' if styled else ''))
    pth, win, ids = load(xml, 260, 140)
    png = os.path.join(tempfile.gettempdir(), "td_lt_%s.png" % uuid.uuid4().hex[:6])
    win.save_png(png)
    win.close()
    return png

pa, pb = shot_del(True), shot_del(False)
band = diff_band(pa, pb)
top, bottom = text_span(pb)
chk("line-through 有像素", band is not None, band)
if band is not None:
    # 删除线要落在字面上下沿之间（穿过文字），与"文字下方"的下划线严格区分
    chk("line-through 穿过字面", top < band[0] and band[1] < bottom + 1, (top, bottom, band))

# 4) 不写 text-decoration 不回归
p4, win, ids = load('<label Name="d" Left="10" Top="10">x</label>')
chk("未写不回归", win.get_xml_value(ids["d"], "TextDecoration") == "",
    repr(win.get_xml_value(ids["d"], "TextDecoration")))
win.close()

print("t_text_decoration: PASS=%d FAIL=%d" % (total - len(fails), len(fails)))
