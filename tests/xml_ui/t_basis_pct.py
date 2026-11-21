# 第25轮：flex-basis 百分比（CSS `flex:0 0 50%` 早先被 wcstol 读成 50px，真 bug）
import os, sys, tempfile
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def lay(inner, w=600, h=300):
    p = os.path.join(tempfile.gettempdir(), "t_basis_pct.xml")
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    return M.xml_layout(p, w, h)

# 1) flex 简写里的百分比：容器 600 宽，a 应占 300
r = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="120" Flex="row" Gap="0">'
        '<XLabel Name="a" Width="100" Height="30" style="flex:0 0 50%" Text="A"/>'
        '<XLabel Name="b" Width="100" Height="30" style="flex:1" Text="B"/>'
        '</XFlexPanel>')
chk("flex:0 0 50% -> 宽 300", r["a"][2] == 300, r["a"])
chk("b 吃掉剩余 300", r["b"][2] == 300, r["b"])

# 2) 单独的 flex-basis:30%
r2 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="120" Flex="row" Gap="0">'
         '<XLabel Name="a" Width="100" Height="30" style="flex-basis:30%" Text="A"/>'
         '<XLabel Name="b" Width="100" Height="30" style="flex-grow:1" Text="B"/>'
         '</XFlexPanel>')
chk("flex-basis:30% -> 宽 180", r2["a"][2] == 180, r2["a"])

# 3) 像素写法仍按像素（不能被百分比改动牵连）
r3 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="120" Flex="row" Gap="0">'
         '<XLabel Name="a" Width="100" Height="30" style="flex:0 0 120px" Text="A"/>'
         '<XLabel Name="b" Width="100" Height="30" style="flex:1" Text="B"/>'
         '</XFlexPanel>')
chk("flex:0 0 120px -> 宽 120", r3["a"][2] == 120, r3["a"])

# 4) column 容器：百分比按高度换算（600x400，50% = 200）
r4 = lay('<XFlexPanel Name="col" Left="0" Top="0" Width="200" Height="400" Flex="column" Gap="0">'
         '<XLabel Name="a" Width="100" Height="40" style="flex:0 0 50%" Text="A"/>'
         '<XLabel Name="b" Width="100" Height="40" style="flex:1" Text="B"/>'
         '</XFlexPanel>', 600, 400)
chk("column 里 50% -> 高 200", r4["a"][3] == 200, r4["a"])

# 5) align-self 覆盖 align-items（start / center / end 三种都验）
r5 = lay('<XFlexPanel Name="row" Left="0" Top="0" Width="600" Height="120" Flex="row" Align="start" Gap="10">'
         '<XLabel Name="a" Width="80" Height="30" Text="A"/>'
         '<XLabel Name="b" Width="80" Height="30" Self="end" Text="B"/>'
         '<XLabel Name="c" Width="80" Height="30" Self="center" Text="C"/>'
         '</XFlexPanel>')
chk("align-items:start -> a 贴顶", r5["a"][1] == 0, r5["a"])
chk("Self=end -> b 贴底(120-30=90)", r5["b"][1] == 90, r5["b"])
chk("Self=center -> c 居中(45)", r5["c"][1] == 45, r5["c"])

print("t_basis_pct: PASS=%d FAIL=%d" % (8 - len(fails), len(fails)))
