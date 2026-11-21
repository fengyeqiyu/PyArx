# 第48轮：CSS border-radius:50% —— 百分比圆角（头像、圆点、状态指示都用它）
# 早先 `_wtoi("50%")` 直接读成 50px，方盒子写成"50% 圆角"还是方的。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def radius(attrs):
    n = {}
    w = M.guiload_text('<XWindow Width="200" Height="120"><div Name="d" Width="40" Height="40" %s/></XWindow>' % attrs,
                       context=globals(), names=n)
    M.pump(120)
    v = w.get_xml_value(n["d"], "Radius")
    w.close()
    return v

chk("style border-radius:50% 记成百分比", radius('style="background:#E5484D;border-radius:50%"') == "50%",
    radius('style="background:#E5484D;border-radius:50%"'))
chk("style border-radius:8px 仍是像素", radius('style="border-radius:8px"') == "8",
    radius('style="border-radius:8px"'))
chk("属性 Radius=50% 也认", radius('Radius="50%"') == "50%", radius('Radius="50%"'))
chk("Radius=0 仍是空（直角）", radius('Radius="0"') == "", radius('Radius="0"'))
print("t_radius_pct: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
