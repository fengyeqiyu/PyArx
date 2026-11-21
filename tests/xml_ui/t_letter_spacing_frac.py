# 第41轮：letter-spacing 小数/负值全链路（HTML 里 -.5px / 1.5px 是常用写法）
# 此前解析用 _wtof 其实能读小数，但回读与绘制都按 int 截断 → 0.5px 静默变 0（写了等于没写）。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def ink(style, out):
    n = {}
    w = M.guiload_text('<XWindow Width="320" Height="80">'
                       '<label Name="t" Left="10" Top="10" Width="300" Height="40" '
                       'style="color:#FFFFFF;font-size:18;' + style + '">ABCDEF</label></XWindow>', names=n)
    M.pump(200)
    v = w.get_xml_value(n["t"], "LetterSpacing")
    w.save_png(out); w.close()
    im = Image.open(out).convert("L")
    xs = [x for x in range(im.size[0]) for y in range(34, 74) if im.getpixel((x, y)) > 160]
    return v, ((min(xs), max(xs) - min(xs)) if xs else (0, 0))

v0, w0 = ink("", "G:/PyArx/log/lsf_0.png")
v1, w1 = ink("letter-spacing:1.5px", "G:/PyArx/log/lsf_15.png")
v2, w2 = ink("letter-spacing:4px", "G:/PyArx/log/lsf_4.png")

chk("1.5px 回读保留小数", v1 == "1.5", v1)
chk("未写不产生值", v0 == "", repr(v0))
chk("4px 回读为整数 4", v2 == "4", v2)
chk("1.5px 与 4px 不被截成同值", v1 != v2, (v1, v2))
chk("1.5px 真让文字变宽（宽度差 ≥ 3px）", w1[1] >= w0[1] + 3, (w0[1], w1[1]))

# 负值：HTML 里 `.letter-tight{letter-spacing:-.5px}` 很常见
n = {}
w = M.guiload_text('<XWindow Width="300" Height="80"><label Name="t" Top="10" Left="10" '
                   'Width="280" style="letter-spacing:-0.5px">ABC</label></XWindow>', names=n)
M.pump(120)
vneg = w.get_xml_value(n["t"], "LetterSpacing")
w.close()
chk("负值 -0.5px 回读", vneg == "-0.5", vneg)

print("t_letter_spacing_frac: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))
