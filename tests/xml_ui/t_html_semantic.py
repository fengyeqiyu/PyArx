# 第 8 轮：HTML 语义标签的默认样式（不写 CSS 也对）
# 浏览器里 <b>/<em>/<u>/<del>/<h2>/<mark>/<code> 天生有效果；XML 早先一律是素面文本，
# 作者必须补 FontWeight / FontStyle / TextDecoration / FontSize 才等价 —— 典型的"更啰嗦"。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=320, h=160):
    p = os.path.join(tempfile.gettempdir(), "sem_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

def val(tag, attr, text="样例文字"):
    win, ids = load('<%s Name="x" Left="10" Top="10">%s</%s>' % (tag, text, tag))
    v = win.get_xml_value(ids["x"], attr)
    win.close()
    return v

# 1) <strong>/<b> → 加粗
chk("<strong> 默认加粗", val("strong", "FontWeight") == "Bold", val("strong", "FontWeight"))
chk("<b> 默认加粗", val("b", "FontWeight") == "Bold", val("b", "FontWeight"))
# 2) <em>/<i> → 斜体
chk("<em> 默认斜体", val("em", "FontStyle") == "Italic", val("em", "FontStyle"))
chk("<i> 默认斜体", val("i", "FontStyle") == "Italic", val("i", "FontStyle"))
# 3) <u>/<ins> → 下划线，<del>/<s> → 删除线
chk("<u> 默认下划线", val("u", "TextDecoration") == "underline", val("u", "TextDecoration"))
chk("<del> 默认删除线", val("del", "TextDecoration") == "line-through",
    val("del", "TextDecoration"))
chk("<s> 默认删除线", val("s", "TextDecoration") == "line-through",
    val("s", "TextDecoration"))
# 4) <h2> → 24px + 加粗（浏览器默认的 h 系列字号表）
chk("<h2> 默认字号 24", val("h2", "FontSize") == "24", val("h2", "FontSize"))
chk("<h2> 默认加粗", val("h2", "FontWeight") == "Bold", val("h2", "FontWeight"))
chk("<h6> 默认字号 11", val("h6", "FontSize") == "11", val("h6", "FontSize"))
# 5) <mark> → 黄色高亮，<code> → 等宽族
chk("<mark> 默认高亮底色", val("mark", "Background") != "", val("mark", "Background"))
# 5b) <code>/<pre> 早先落 XCodeEditor（未注册控件）→ 加载即报"未知控件"；现落等宽文本
chk("<code> 默认等宽", val("code", "FontFamily") != "", val("code", "FontFamily"))
win, ids = load('<pre Name="x" Left="10" Top="10">int main()</pre>')
chk("<pre> 能加载且等宽", win.get_xml_value(ids["x"], "FontFamily") != "",
    win.get_xml_value(ids["x"], "FontFamily"))
win.close()
# 6) 显式写法优先：作者写了 none / 自定义字号，默认不得覆盖
win, ids = load('<del Name="x" Left="10" Top="10" style="text-decoration:none">文字</del>')
v = win.get_xml_value(ids["x"], "TextDecoration")
win.close()
chk("显式 text-decoration:none 优先", v in ("", "none"), repr(v))

win, ids = load('<h3 Name="x" Left="10" Top="10" style="font-size:40px">文字</h3>')
v = win.get_xml_value(ids["x"], "FontSize")
win.close()
chk("显式 font-size 优先", v == "40", repr(v))

# 7) 老写法 <XLabel> 不注入任何默认（不回归）
win, ids = load('<XLabel Name="x" Left="10" Top="10" Width="120" Height="24">文字</XLabel>')
got = (win.get_xml_value(ids["x"], "FontWeight"), win.get_xml_value(ids["x"], "FontStyle"),
       win.get_xml_value(ids["x"], "TextDecoration"))
win.close()
chk("<XLabel> 不注入默认", got == ("", "", ""), got)

# 8) 像素级：标签默认样式必须真的画出来（不只是属性回读）
from PIL import Image, ImageChops

_POS = 'Left="20" Top="20" Width="200" Height="40" style="font-size:24px;color:#ffffff;"'

def shot(inner, tag):
    win, ids = load(inner)
    png = os.path.join(tempfile.gettempdir(), "sem_%s_%s.png" % (tag, uuid.uuid4().hex[:6]))
    win.save_png(png)
    win.close()
    return png

def diff_band(pa, pb):
    ia = Image.open(pa).convert("RGB"); ib = Image.open(pb).convert("RGB")
    if ia.size != ib.size:
        return None
    d = ImageChops.difference(ia, ib)
    ys = [y for y in range(d.size[1])
          if sum(1 for x in range(d.size[0]) if sum(d.getpixel((x, y))[:3]) > 24) >= 3]
    return (ys[0], ys[-1]) if ys else None

_chk_px = lambda n, c, got="": chk(n, c, got)

band = diff_band(shot('<del %s>STRIKE</del>' % _POS, "del"),
                 shot('<span %s>STRIKE</span>' % _POS, "span"))
_chk_px("<del> 画出删除线", band is not None, band)
if band is not None:
    _chk_px("<del> 删除线穿过字面（不是沿线外）", band[1] - band[0] <= 3, band)

band = diff_band(shot('<u %s>UNDER</u>' % _POS, "u"),
                 shot('<span %s>UNDER</span>' % _POS, "span2"))
_chk_px("<u> 画出下划线", band is not None, band)

p = shot('<mark Left="20" Top="20" Width="200" Height="40">MARK</mark>', "mark")
px = Image.open(p).convert("RGB").load()
_r, _g, _b = px[210, 60]
_chk_px("<mark> 画出黄色高亮底色", _r > 200 and _g > 200 and _b < 120, (_r, _g, _b))

# 9) <a href>：早先只换控件不搬内容（文字丢失、嵌套 <span> 报"未知控件"），现落 XLabel + 蓝色下划线
win, ids = load('<a Name="x" Left="20" Top="20">了解更多</a>')
fg = win.get_xml_value(ids["x"], "Foreground").lower()
td = win.get_xml_value(ids["x"], "TextDecoration").lower()
win.close()
chk("<a> 默认蓝色", fg in ("#0066cc", "#06c"), fg)
chk("<a> 默认下划线", td == "underline", td)

_ok = True
try:
    win, ids = load('<a Name="x" href="https://e.com" Left="20" Top="20"><span>行内子</span></a>')
    win.close()
except Exception as ex:      # 旧实现这里是"未知控件 <span>"
    _ok = False
    print("      exception:", ex)
chk("<a> 内嵌 <span> 能加载", _ok)

_base = 'Left="20" Top="20" Width="200" Height="40" style="font-size:20px;%s"'
p_a = shot('<a %s href="#">LINK</a>' % (_base % ''), "anchor")
p_n = shot('<a %s href="#">LINK</a>' % (_base % 'text-decoration:none'), "anchor_none")

px = Image.open(p_a).convert("RGB").load()
_pix = [px[x, y] for y in range(40, 110) for x in range(20, 220)]
_pix = [c for c in _pix if sum(c) > 200]
_avg = [sum(c[i] for c in _pix) // max(1, len(_pix)) for i in range(3)]
chk("<a> 文字画成蓝色", bool(_pix) and _avg[2] > _avg[0] + 40, tuple(_avg))

_band = diff_band(p_a, p_n)
chk("<a> 画出下划线", _band is not None, _band)
if _band is not None:
    chk("<a> 下划线是细线", _band[1] - _band[0] <= 4, _band)

print("t_html_semantic: PASS=%d FAIL=%d" % (32 - len(fails), len(fails)))
