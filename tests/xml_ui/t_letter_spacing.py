# 第23轮：CSS letter-spacing（字距）落地 —— TextFormat 不支持，改走 TextLayout1::SetCharacterSpacing
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def ink_width(style, out):
    """白字（深色底）：取亮像素的最左/最右 x，跨度就是文字实际宽度。字距变大 → 跨度变大。"""
    n = {}
    w = M.guiload_text('<XWindow Width="320" Height="80">'
                       '<label Name="t" Left="10" Top="10" Width="300" Height="40" '
                       'style="color:#FFFFFF;font-size:18;' + style + '">ABCDEF</label></XWindow>', names=n)
    M.pump(200); w.save_png(out); w.close()
    im = Image.open(out).convert("L")
    xs = [x for x in range(im.size[0]) for y in range(34, 74) if im.getpixel((x, y)) > 160]
    return (min(xs), max(xs) - min(xs)) if xs else (0, 0)

w0 = ink_width("", "G:/PyArx/log/ls_0.png")
w4 = ink_width("letter-spacing:4", "G:/PyArx/log/ls_4.png")

chk("默认字距有墨迹", w0[1] > 20, w0)
chk("letter-spacing:4 文字跨度 +24（6 字符 × 4px）", w4[1] >= w0[1] + 20, (w0[1], w4[1]))

# 属性直写也认（不只 style）
n = {}
w = M.guiload_text('<XWindow Width="300" Height="80"><label Name="t" Top="10" Left="10" '
                   'Width="280" LetterSpacing="3">ABC</label></XWindow>', names=n)
M.pump(120)
v = w.get_xml_value(n["t"], "LetterSpacing")
w.close()
chk("LetterSpacing 属性可回读", v == "3", v)

print("t_letter_spacing: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))

