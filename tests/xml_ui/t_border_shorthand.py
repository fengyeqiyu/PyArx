# 第29轮：CSS `border` 简写（border:1px solid #x / border-width / border-color / border-style）
import os, sys, tempfile
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def run(inner, w=300, h=120):
    p = os.path.join(tempfile.gettempdir(), "t_border.xml")
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    win = M.guiload(p, names=ids)
    def val(n, prop):
        return win.get_xml_value(ids[n], prop)
    return win, ids, val

# 1) 简写：1px solid #5F6976 → 宽 1 / Single / 颜色
win, ids, val = run('<XLabel Name="a" Left="10" Top="10" Width="100" Height="30" '
                    'style="border:1px solid #5F6976" Text="A"/>')
chk("border 简写 -> BorderThickness=1", val("a", "BorderThickness") in ("1", "1px"), val("a", "BorderThickness"))
chk("border 简写 -> BorderStyle 归一为 Single", val("a", "BorderStyle") == "Single", val("a", "BorderStyle"))
chk("border 简写 -> BorderColor", val("a", "BorderColor").lower() in ("#5f6976", "5f6976"), val("a", "BorderColor"))
win.close()

# 2) border:none 归零
win, ids, val = run('<XLabel Name="b" Left="10" Top="10" Width="100" Height="30" '
                    'style="border:none" Text="B"/>')
chk("border:none -> 不存在正宽度边框", val("b", "BorderThickness") in ("", "0"), val("b", "BorderThickness"))
win.close()

# 3) 长写：border-width / border-color 各自独立
win, ids, val = run('<XLabel Name="c" Left="10" Top="10" Width="100" Height="30" '
                    'style="border-width:2;border-color:#AA0000" Text="C"/>')
chk("border-width:2", val("c", "BorderThickness") == "2", val("c", "BorderThickness"))
chk("border-color", val("c", "BorderColor").lower() in ("#aa0000", "aa0000"), val("c", "BorderColor"))
win.close()

# 4) 顺序无关：颜色在前 + 线型在后
win, ids, val = run('<XLabel Name="d" Left="10" Top="10" Width="100" Height="30" '
                    'style="border:#00FF00 dashed 3" Text="D"/>')
chk("乱序 border 仍解析宽度 3", val("d", "BorderThickness") == "3", val("d", "BorderThickness"))
chk("乱序 border 仍解析颜色", val("d", "BorderColor").lower() in ("#00ff00", "00ff00"), val("d", "BorderColor"))
chk("dashed 线型保留并落笔画虚线", val("d", "BorderStyle") == "Dashed" and val("d", "BorderThickness") == "3", val("d", "BorderStyle"))
win.close()

# 5) 老写法不回归：BorderThickness 属性直接写仍是 Single 契约
win, ids, val = run('<XLabel Name="e" Left="10" Top="10" Width="100" Height="30" '
                    'BorderThickness="1" BorderColor="#333333" BorderStyle="Single" Text="E"/>')
chk("老 XML 写法保持 Single", val("e", "BorderStyle") == "Single", val("e", "BorderStyle"))
chk("老写法宽 1", val("e", "BorderThickness") == "1", val("e", "BorderThickness"))
win.close()

print("t_border_shorthand: PASS=%d FAIL=%d" % (10 - len(fails), len(fails)))
