# 第64轮：flex 的 order。
# HTML 里 order 是少数"不改语义只改视觉次序"的能力（主按钮排最后、角标排最前），
# XML 此前只能靠挪 XML 节点顺序来达到，两者成本差一档。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def lay(body, tag, extra="", w=420, h=200):
    p = "G:/PyArx/log/_fo_%s.xml" % tag
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d" Caption="o">'
                '<div Name="box" Left="10" Top="10" Width="400" Height="120" style="display:flex;%s">'
                '%s</div></XWindow>' % (w, h, extra, body))
    return M.xml_layout(p, w, h)

# 1. style 写法：order:-1 排到最前
r = lay('<div Name="a" Width="100" Height="40"/><div Name="b" Width="100" Height="40" style="order:-1"/>'
        '<div Name="c" Width="100" Height="40"/>', "a")
chk("style order:-1 排到最前", r["b"][0] == 10 and r["a"][0] == 110, (r["a"][0], r["b"][0]))

# 2. 属性写法等价
r2 = lay('<div Name="a" Width="100" Height="40"/><div Name="b" Width="100" Height="40" Order="-1"/>', "b")
chk("Order=\"-1\" 属性同效", r2["b"][0] == 10 and r2["a"][0] == 110, (r2["a"][0], r2["b"][0]))

# 3. order:2 排到最后
r3 = lay('<div Name="a" Width="100" Height="40" style="order:2"/><div Name="b" Width="100" Height="40"/>'
         '<div Name="c" Width="100" Height="40"/>', "c")
chk("style order:2 排到最后", r3["a"][0] == 210, r3["a"][0])

# 4. 相同 order 保持文档顺序（默认 0 不互相插队）
r4 = lay('<div Name="a" Width="100" Height="40"/><div Name="b" Width="100" Height="40" style="order:0"/>'
         '<div Name="c" Width="100" Height="40"/>', "d")
chk("同 order 保持文档顺序", r4["a"][0] == 10 and r4["b"][0] == 110 and r4["c"][0] == 210,
    (r4["a"][0], r4["b"][0], r4["c"][0]))

print("t_flex_order.py: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
