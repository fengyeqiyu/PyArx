# 第56轮：CSS text-align:justify（两端对齐）
# HTML 里块级元素写一行 `text-align:justify` 就得到两端对齐；XML 早先只有 Left/Center/Right，
# 多行文本的右端参差不齐。这里校验"中间行右端被撑到接近右边界"。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

TEXT = "这是一段用来验证两端对齐的中文段落，它必须足够长以致于在窄盒子里折成多行才行。"
fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def rows(p):
    """返回每行文字的 (minx, maxx)，按 y 聚类，只统计含亮像素的行"""
    im = Image.open(p).convert("RGB")
    hits = []
    for y in range(34, im.size[1]):
        xs = [x for x in range(0, im.size[0])
              if sum(im.getpixel((x, y))) > 330]   # 亮像素=文字
        if xs: hits.append((y, min(xs), max(xs)))
    out = []
    for y, a, b in hits:
        if out and y - out[-1][0] <= 1:
            out[-1] = (y, min(out[-1][1], a), max(out[-1][2], b))
        else:
            out.append((y, a, b))
    return [r for r in out]

def shot(tag, align, wrap=False):
    n = {}
    w = M.guiload_text(
        '<XWindow Width="320" Height="180">'
        '<label Name="t" Left="20" Top="60" Width="240" Height="90" Text="%s" %s '
        'style="font-size:14;text-align:%s"/></XWindow>'
        % (TEXT, 'TextWrapping="Wrap"' if wrap else '', align),
        context=globals(), names=n)
    M.pump(250)
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    v = w.get_xml_value(n["t"], "TextAlign")
    w.close()
    return v, rows(p)

v0, r0 = shot("ta_nowrap", "justify")
v1, r1 = shot("ta_left", "left", wrap=True)
v2, r2 = shot("ta_just", "justify")
chk("TextAlign 契约回读", v1 == "Left" and v2 == "Justify" and v0 == "Justify", (v1, v2, v0))
chk("justify 不写 text-wrapping 也自动折行（对齐 HTML 块级默认折行）", len(r0) >= 2, len(r0))
chk("两种写法都折成多行", len(r1) >= 2 and len(r2) >= 2, (len(r1), len(r2)))
if len(r1) >= 2 and len(r2) >= 2:
    # 取前几行（末行两端对齐按 HTML 规则不撑），比较右端
    head_l = max(x[2] for x in r1[:-1])
    head_j = max(x[2] for x in r2[:-1])
    chk("justify 中间行右端更靠右", head_j > head_l + 4, (head_l, head_j))
else:
    chk("justify 中间行右端更靠右", False, "行数不足")
print("t_text_align.py: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
