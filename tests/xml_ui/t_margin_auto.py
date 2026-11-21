# 第26轮：CSS `margin:auto`（auto 边距先吃剩余空间，再轮到 justify-content）
import os, sys, tempfile
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def lay(inner, w=600, h=300):
    p = os.path.join(tempfile.gettempdir(), "t_margin_auto.xml")
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    return M.xml_layout(p, w, h)

# 1) margin-left:auto：按钮被顶到行尾（HTML 最常用的一招）
r = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="80" Flex="row" Gap="0">'
        '<XLabel Name="a" Width="100" Height="30" Text="A"/>'
        '<XLabel Name="b" Width="120" Height="30" style="margin-left:auto" Text="B"/>'
        '</XFlexPanel>')
chk("margin-left:auto -> b 贴右(600-120=480)", r["b"][0] == 480, r["b"])
chk("a 仍在最左", r["a"][0] == 0, r["a"])

# 2) margin:0 auto 简写：两侧 auto 均分 → 居中
r2 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="80" Flex="row" Gap="0">'
         '<XLabel Name="a" Width="200" Height="30" style="margin:0 auto" Text="A"/>'
         '</XFlexPanel>')
chk("margin:0 auto -> 居中 x=200", r2["a"][0] == 200, r2["a"])

# 3) 多个元素各自 auto，中间那个被推右
r3 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="80" Flex="row" Gap="10">'
         '<XLabel Name="a" Width="80" Height="30" Text="A"/>'
         '<XLabel Name="b" Width="80" Height="30" Text="B"/>'
         '<XLabel Name="c" Width="80" Height="30" style="margin-left:auto" Text="C"/>'
         '</XFlexPanel>')
chk("c 贴右(600-80=520)", r3["c"][0] == 520, r3["c"])
chk("a/b 未被推动", r3["a"][0] == 0 and r3["b"][0] == 90, (r3["a"], r3["b"]))

# 4) auto 与 justify-content 共存：auto 优先，justify 不再额外分配
r4 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="80" Flex="row" Gap="0" Justify="center">'
         '<XLabel Name="a" Width="100" Height="30" Text="A"/>'
         '<XLabel Name="b" Width="100" Height="30" style="margin-left:auto" Text="B"/>'
         '</XFlexPanel>')
chk("justify:center + margin-left:auto -> b 仍贴右 500", r4["b"][0] == 500, r4["b"])

# 5) column 容器：margin-top:auto 把元素压到底
r5 = lay('<XFlexPanel Name="col" Left="0" Top="0" Width="200" Height="400" Flex="column" Gap="0">'
         '<XLabel Name="a" Width="100" Height="40" Text="A"/>'
         '<XLabel Name="b" Width="100" Height="40" style="margin-top:auto" Text="B"/>'
         '</XFlexPanel>', 600, 400)
chk("column margin-top:auto -> b 贴底(400-40=360)", r5["b"][1] == 360, r5["b"])

# 6) 没有 auto 时行为不变（回归口径）
r6 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="80" Flex="row" Gap="0">'
         '<XLabel Name="a" Width="100" Height="30" Text="A"/>'
         '<XLabel Name="b" Width="100" Height="30" Text="B"/>'
         '</XFlexPanel>')
chk("无 auto -> 仍是紧排 (0,100)", (r6["a"][0], r6["b"][0]) == (0, 100), (r6["a"], r6["b"]))

# 7) 交叉轴 auto：column 容器里 margin-left/right auto == 水平居中
r7 = lay('<XFlexPanel Name="col" Left="0" Top="0" Width="400" Height="200" Flex="column" Gap="0">'
         '<XLabel Name="a" Width="100" Height="40" style="margin-left:auto;margin-right:auto" Text="A"/>'
         '</XFlexPanel>', 600, 400)
chk("column 交叉轴 auto -> 水平居中 x=150", r7["a"][0] == 150, r7["a"])

# 8) 交叉轴 auto 优先于 align-items（row 容器里 margin-top:auto 应压到底）
r8 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="400" Height="120" Flex="row" Align="start" Gap="0">'
         '<XLabel Name="a" Width="80" Height="30" style="margin-top:auto" Text="A"/>'
         '</XFlexPanel>')
chk("row 里 margin-top:auto -> 压底 y=90", r8["a"][1] == 90, r8["a"])

print("t_margin_auto: PASS=%d FAIL=%d" % (9 - len(fails), len(fails)))
