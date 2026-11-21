# 第63轮：非 flex 的块级/流式元素也要认百分比尺寸。
# HTML 里 <div style="width:50%"> 是常态；XML 此前只有 flex 子项读 pctW，
# 普通块级元素写 50% 会被当成"没写宽度"直接撑满父容器，宽度完全不对。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def lay(body, tag, w=400, h=200):
    p = "G:/PyArx/log/_pb_%s.xml" % tag
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d" Caption="p">%s</XWindow>' % (w, h, body))
    return M.xml_layout(p, w, h)["x"]

chk("style width:50% 取半宽", lay('<label Name="x" Left="0" Top="0" Height="40" style="width:50%" Text="A"/>', "a")[2] == 200,
    lay('<label Name="x" Left="0" Top="0" Height="40" style="width:50%" Text="A"/>', "a2")[2])
chk("Width=\"50%\" 属性取半宽", lay('<label Name="x" Left="0" Top="0" Width="50%" Height="40" Text="A"/>', "b")[2] == 200)
chk("style width:100% 仍铺满", lay('<label Name="x" Left="0" Top="0" Height="40" style="width:100%" Text="A"/>', "c")[2] == 400)
chk("style width:200px 不受影响", lay('<label Name="x" Left="0" Top="0" Height="40" style="width:200px" Text="A"/>', "d")[2] == 200)
chk("父宽变则百分比跟着变", lay('<label Name="x" Left="0" Top="0" Height="40" style="width:50%" Text="A"/>', "e", 640, 200)[2] == 320)

print("t_pct_block.py: PASS=%d FAIL=%d" % (5 - len(fails), len(fails)))
