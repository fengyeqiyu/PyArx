# 第65轮：flex 的 align-content（多行容器的行组在交叉轴分布）。
# HTML 里 flex-wrap:wrap 后 align-content 是控制"整束行"位置的标准手段；
# XML 此前只有单行语义，写 align-content:center 仍从容器顶部堆行。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

# 容器 400x120，3 项各 150x40、无 gap → 折成两行（2+1），行组高 80，剩余 40
def lay(css, tag):
    p = "G:/PyArx/log/_ac2_%s.xml" % tag
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="420" Height="200" Caption="o">'
                '<div Name="box" Left="10" Top="10" Width="400" Height="120" '
                'style="display:flex;flex-wrap:wrap;%s">'
                '<div Name="a" Width="150" Height="40"/><div Name="b" Width="150" Height="40"/>'
                '<div Name="c" Width="150" Height="40"/></div></XWindow>' % css)
    r = M.xml_layout(p, 420, 200)
    return {k: (r[k][0], r[k][1]) for k in ("a", "b", "c")}

r = lay("align-content:start;", "a")
chk("start: 行贴顶（10/50）", r["a"][1] == 10 and r["c"][1] == 50, (r["a"][1], r["c"][1]))
r = lay("align-content:center;", "b")
chk("center: 行组居中（30/70）", r["a"][1] == 30 and r["c"][1] == 70, (r["a"][1], r["c"][1]))
r = lay("align-content:flex-end;", "c")
chk("flex-end: 行组贴底（50/90）", r["a"][1] == 50 and r["c"][1] == 90, (r["a"][1], r["c"][1]))
r = lay("align-content:space-between;", "d")
chk("space-between: 首行贴顶、末行贴底", r["a"][1] == 10 and r["c"][1] == 90, (r["a"][1], r["c"][1]))
r = lay("align-content:space-around;", "e")
chk("space-around: 两端各留 1/4、行间 1/2", r["a"][1] == 20 and r["c"][1] == 80, (r["a"][1], r["c"][1]))
r = lay("align-content:space-evenly;", "f")
chk("space-evenly: 三段均分", r["a"][1] == 23 and r["c"][1] == 76, (r["a"][1], r["c"][1]))

print("t_align_content.py: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))
