# 第11轮：非 flex 流里的 CSS min-width / max-width
# HTML 里 `<div style="width:300;max-width:150">` 画出来是 150；XML 早先照样画 300（上限整条被忽略）。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def rect(inner, key="d"):
    n = {}
    w = M.guiload_text('<XWindow Width="400" Height="200">' + inner + '</XWindow>', names=n)
    M.pump(120)
    r = w.get_rect(n[key])
    w.close()
    return r

# 1) max-width 夹住显式 width
r = rect('<div Name="d" Left="10" Top="10" Width="300" Height="40" style="max-width:150"/>')
chk("max-width:150 夹住 width:300", r[2] == 150, r)

# 2) min-width 撑起显式 width
r = rect('<div Name="d" Left="10" Top="10" Width="100" Height="40" style="min-width:200"/>')
chk("min-width:200 撑起 width:100", r[2] == 200, r)

# 3) 百分比：50% of 容器内框（400 客户区 -> 192 左右，和 padding 无关）
r = rect('<div Name="d" Left="10" Top="10" Width="300" Height="40" style="max-width:50%"/>')
chk("max-width:50% 按容器宽算", 150 < r[2] <= 200, r)

# 4) 没写上下限不动（不回归）
r = rect('<div Name="d" Left="10" Top="10" Width="300" Height="40"/>')
chk("没写 min/max 保持 300", r[2] == 300, r)

# 5) 属性写法与 style 等价
r = rect('<div Name="d" Left="10" Top="10" Width="300" Height="40" MaxWidth="120"/>')
chk("属性 MaxWidth=120 同效", r[2] == 120, r)

# 6) 上限比声明的还大 -> 不放大
r = rect('<div Name="d" Left="10" Top="10" Width="100" Height="40" style="max-width:300"/>')
chk("max-width 大于 width 不放大", r[2] == 100, r)

# 7) label（内容宽）也吃上限：长文案不许把卡片撑破
r = rect('<label Name="d" Left="10" Top="10" style="max-width:200">一段很长的说明文字用来测上限</label>')
chk("label 内容宽被 max-width 200 夹住", r[2] == 200, r)

# 8) 高度同口径：max-height 夹住显式高度
r = rect('<div Name="d" Left="10" Top="10" Width="100" Height="300" style="max-height:100"/>')
chk("max-height:100 夹住 height:300", r[3] == 100, r)

# 9) min-height 撑起显式高度
r = rect('<div Name="d" Left="10" Top="10" Width="100" Height="50" style="min-height:200"/>')
chk("min-height:200 撑起 height:50", r[3] == 200, r)

# 10) 属性写法同效
r = rect('<div Name="d" Left="10" Top="10" Width="100" Height="300" MaxHeight="120"/>')
chk("属性 MaxHeight=120 同效", r[3] == 120, r)

print("t_minmax: PASS=%d FAIL=%d" % (10 - len(fails), len(fails)))
