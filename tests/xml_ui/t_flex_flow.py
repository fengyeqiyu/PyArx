# 第34轮：CSS `flex-flow` 简写（= flex-direction + flex-wrap 一句话）
# XML 早先必须拆成 Flex="row" + Wrap="0"（或 flex-direction + flex-wrap）两处写。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=300, h=200):
    p = os.path.join(tempfile.gettempdir(), "ff_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

BOX = ('<XLabel Name="a" Width="60" Height="30">A</XLabel>'
       '<XLabel Name="b" Width="60" Height="30">B</XLabel>')

# 1) flex-flow: column → 竖排
win, ids = load('<div Name="g" Left="10" Top="10" Width="200" Height="150" '
                'style="display:flex;flex-flow:column">' + BOX + '</div>')
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("flex-flow:column 竖排", rb[1] > ra[1] and rb[0] == ra[0], (ra, rb))
win.close()

# 2) flex-flow: row-reverse → b 在 a 左侧
win, ids = load('<div Name="g" Left="10" Top="10" Width="200" Height="150" '
                'style="display:flex;flex-flow:row-reverse">' + BOX + '</div>')
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("flex-flow:row-reverse 反向", rb[0] < ra[0], (ra, rb))
win.close()

# 3) flex-flow: row wrap → 窄容器里换行
win, ids = load('<div Name="g" Left="10" Top="10" Width="100" Height="150" '
                'style="display:flex;flex-flow:row wrap">' + BOX + '</div>', w=200)
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("flex-flow:row wrap 换行", rb[1] > ra[1], (ra, rb))
win.close()

# 4) flex-flow: column nowrap → 竖排且不换行（宽度不被拆行影响）
win, ids = load('<div Name="g" Left="10" Top="10" Width="200" Height="150" '
                'style="display:flex;flex-flow:column nowrap">' + BOX + '</div>')
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("flex-flow:column nowrap 竖排", rb[1] > ra[1], (ra, rb))
win.close()

# 5) 不写 flex-flow 仍横排（不回归）
win, ids = load('<div Name="g" Left="10" Top="10" Width="200" Height="150" '
                'style="display:flex">' + BOX + '</div>')
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("未写 flex-flow 仍横排", rb[0] > ra[0] and rb[1] == ra[1], (ra, rb))
win.close()

print("t_flex_flow: PASS=%d FAIL=%d" % (5 - len(fails), len(fails)))
