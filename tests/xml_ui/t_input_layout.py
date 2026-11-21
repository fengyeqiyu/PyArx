# 第49轮：HTML <input> 默认左对齐 + 内边距
# 浏览器 input 的文字贴在左边（留 ~2px 内衬）；XML 早先把非 Static 一律居中，输入框里的字在正中。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def text_span(tag, xml_extra=""):
    """返回输入框内文字像素的 (最左 x, 最右 x)，用于判对齐与内边距"""
    n = {}
    w = M.guiload_text('<XWindow Width="260" Height="80">'
                       '<input Name="i" Left="20" Top="20" Width="200" Height="30" Text="Query" %s/>'
                       '</XWindow>' % xml_extra, context=globals(), names=n)
    M.pump(200)
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    w.close()
    im = Image.open(p).convert("RGB")
    wd, ht = im.size
    xs = []
    # 窗口截图含标题栏（"PyMfc" 文字会污染扫描），只扫输入框所在区域
    for y in range(34, ht):
        for x in range(20, 220):
            c = im.getpixel((x, y))
            # 文字像素：明显亮于输入框底色
            if c[0] > 170 and c[1] > 170 and c[2] > 170:
                xs.append(x)
    return (min(xs), max(xs)) if xs else (None, None)

lo, hi = text_span("ipt_left")
chk("输入框文字从左开始（不是居中）", lo is not None and lo < 20 + 200 * 0.33, (lo, hi))
chk("文字带默认内边距（离框左边 4~12px）", lo is not None and 20 < lo <= 20 + 12, (lo, hi))
print("t_input_layout: PASS=%d FAIL=%d" % (2 - len(fails), len(fails)))
