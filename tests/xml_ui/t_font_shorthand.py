# 第38轮：CSS `font` 简写（`font: [style] [weight] size[/line-height] family`）
# HTML 一句给出字号/字重/行高/字体族；XML 早先要写 FontSize/FontWeight/LineHeight/FontFamily 四条。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=340, h=200):
    p = os.path.join(tempfile.gettempdir(), "fn_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

def gv(win, cid, k):
    return win.get_xml_value(cid, k)

# 1) 完整简写：bold 16px/1.4 微软雅黑
win, ids = load('<label Name="a" Left="10" Top="10" style="font:bold 16px/1.4 微软雅黑">标题</label>')
chk("font 简写解析 字号", gv(win, ids["a"], "FontSize") == "16", gv(win, ids["a"], "FontSize"))
chk("font 简写解析 字重", gv(win, ids["a"], "FontWeight").lower() in ("bold", "700"),
    gv(win, ids["a"], "FontWeight"))
chk("font 简写解析 字体族", "微软雅黑" in gv(win, ids["a"], "FontFamily"),
    gv(win, ids["a"], "FontFamily"))
win.close()

# 1b) 简写与分写四条属性得到同一矩形（行高不在外观层，改用布局等价来验）
W, H = 340, 200
def lay(inner):
    p = os.path.join(tempfile.gettempdir(), "fn_%s.xml" % uuid.uuid4().hex[:8])
    open(p, "w", encoding="utf-8").write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (W, H, inner))
    return M.xml_layout(p, W, H)
A = lay('<label Name="a" Left="10" Top="10" style="font:bold 16px/1.4 微软雅黑">标题</label>')
B = lay('<label Name="b" Left="10" Top="10" style="font-size:16px;font-weight:bold;line-height:1.4;font-family:微软雅黑">标题</label>')
chk("font 简写 ≡ 分写四件套", tuple(A["a"]) == tuple(B["b"]), (A["a"], B["b"]))

# 2) 最省写：只有 size 和 family
win, ids = load('<label Name="b" Left="10" Top="10" style="font:13px Arial">x</label>')
chk("font:13px Arial", gv(win, ids["b"], "FontSize") == "13"
    and gv(win, ids["b"], "FontFamily") == "Arial", (gv(win, ids["b"], "FontSize"), gv(win, ids["b"], "FontFamily")))
win.close()

# 3) italic 前缀
win, ids = load('<label Name="c" Left="10" Top="10" style="font:italic 14px Consolas">x</label>')
chk("font italic 前缀", gv(win, ids["c"], "FontStyle").lower() == "italic",
    gv(win, ids["c"], "FontStyle"))
win.close()

# 4) 不写 font 不回归（默认字重/字号保持空或默认）
win, ids = load('<label Name="d" Left="10" Top="10">x</label>')
chk("未写 font 不回归", gv(win, ids["d"], "FontSize") == "" and gv(win, ids["d"], "FontWeight") == "",
    (gv(win, ids["d"], "FontSize"), gv(win, ids["d"], "FontWeight")))
win.close()

print("t_font_shorthand: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
