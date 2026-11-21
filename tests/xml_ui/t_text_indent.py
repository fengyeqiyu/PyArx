# 第52轮：CSS text-indent —— 首行缩进（中文段落 `text-indent:2em` 是标准写法）
# HTML 一行 style 就能让段落首行缩两字；XML 此前完全没有这个概念。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

TEXT = "首行缩进测试文本内容足够长以便在窄框里折行形成第二行"
fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def rows(tag, style):
    n = {}
    w = M.guiload_text(
        '<XWindow Width="300" Height="180">'
        '<label Name="t" Left="10" Top="10" Width="200" Height="150" Text="%s" '
        'TextWrapping="Wrap" style="font-size:14px;%s"/>'
        '</XWindow>' % (TEXT, style), context=globals(), names=n)
    M.pump(200)
    ind = w.get_xml_value(n["t"], "TextIndent")
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    w.close()
    im = Image.open(p).convert("RGB")
    # 按 y 行聚合亮像素，取每行的最左 x（行与行之间空 y 段分界）
    line_x = []
    cur = None
    for y in range(34, im.size[1]):
        xs = [x for x in range(10, 220)
              if all(c > 170 for c in im.getpixel((x, y)))]
        if xs:
            lo = min(xs)
            if cur is None or y - cur[1] > 3:
                cur = [lo, y]
                line_x.append(cur)
            else:
                cur[0] = min(cur[0], lo)
                cur[1] = y
    return ind, line_x

ind0, l0 = rows("ti_off", "line-height:1.8")
ind1, l1 = rows("ti_on", "line-height:1.8;text-indent:2em")
chk("TextIndent 契约回读", ind0 == "" and ind1 == "2em", (ind0, ind1))
chk("首行比次行缩进（≥ 1.5 字号）",
    len(l1) >= 2 and (l1[0][0] - l1[1][0]) >= 20, (l0[:2], l1[:2]))
print("t_text_indent: PASS=%d FAIL=%d" % (2 - len(fails), len(fails)))
