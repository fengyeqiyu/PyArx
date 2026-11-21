# 第35轮：HTML <button> 默认外观（不写样式也对）
# 浏览器原生 <button> 默认是圆角；XML 早先不写 Radius 就是直角，并排一眼看出差别。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=320, h=200):
    p = os.path.join(tempfile.gettempdir(), "bd_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

# 1) 原生 <button> 不写圆角 → 得到默认圆角
win, ids = load('<button Name="ok" Left="10" Top="10">确定</button>')
chk("<button> 默认圆角=6", win.get_xml_value(ids["ok"], "Radius") == "6",
    win.get_xml_value(ids["ok"], "Radius"))
win.close()

# 2) 显式 border-radius:0 → 不被默认值覆盖（直角仍可写出来；半径 0 在外观层归一为空）
win, ids = load('<button Name="ok" Left="10" Top="10" style="border-radius:0">确定</button>')
rv = win.get_xml_value(ids["ok"], "Radius")
chk("显式 border-radius:0 生效", rv in ("", "0"), repr(rv))
win.close()

# 3) 显式 Radius 属性 → 保留作者值
win, ids = load('<button Name="ok" Left="10" Top="10" Radius="14">确定</button>')
chk("显式 Radius=14 保留", win.get_xml_value(ids["ok"], "Radius") == "14",
    win.get_xml_value(ids["ok"], "Radius"))
win.close()

# 4) 老写法 <XButton> 不回归（不注入默认圆角）
win, ids = load('<XButton Name="ok" Left="10" Top="10" Width="88" Height="26">确定</XButton>')
chk("<XButton> 不注入默认圆角", win.get_xml_value(ids["ok"], "Radius") == "",
    repr(win.get_xml_value(ids["ok"], "Radius")))
win.close()

print("t_btn_defaults: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
