# 第36轮：块级（非 flex）容器里的 CSS `margin: 0 auto` 水平居中
# HTML 里 <div style="width:300"><span style="margin:0 auto;width:60"> 就是居中；
# 此前这条只在 flex 容器内生效，普通块级父里被当成左对齐。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=340, h=200):
    p = os.path.join(tempfile.gettempdir(), "mab_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

def box(inner_div):
    return '<div Name="g" Left="10" Top="10" Width="300" Height="120">%s</div>' % inner_div

# 1) margin:0 auto → 水平居中：localX = 10 + (280-60)/2 = 120
win, ids = load(box('<label Name="a" Width="60" Height="24" style="margin:0 auto">居中</label>'))
ra = win.get_rect(ids["a"])
chk("块级 margin:0 auto 居中", ra[0] == 10 + 120, ra)
win.close()

# 2) 只 margin-left:auto → 靠右：localX = 10 + 300 - 10 - 60 = 240
win, ids = load(box('<label Name="a" Width="60" Height="24" style="margin-left:auto">靠右</label>'))
ra = win.get_rect(ids["a"])
chk("块级 margin-left:auto 靠右", ra[0] == 10 + 240, ra)
win.close()

# 3) 不写 auto → 左对齐（不回归；div 自身无默认 padding，贴容器左内边）
win, ids = load(box('<label Name="a" Width="60" Height="24">左</label>'))
ra = win.get_rect(ids["a"])
chk("未写 auto 仍左对齐", ra[0] == 10, ra)
win.close()

# 4) 横向流（row）不回归：两个元素依次排
win, ids = load('<div Name="g" Left="10" Top="10" Width="300" Height="60" style="display:flex">'
                '<label Name="a" Width="60" Height="24">A</label>'
                '<label Name="b" Width="60" Height="24" style="margin-left:auto">B</label></div>')
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("flex 里 margin-left:auto 仍靠右", rb[0] + rb[2] == 10 + 300 and ra[0] == 10, (ra, rb))
win.close()

print("t_margin_auto_block: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
