# 第18轮：CSS padding 多值 / 单边 / inset 简写
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
total = [0]
def chk(name, cond, got=""):
    total[0] += 1
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

# 1) padding: 4 16 —— 子项左起 16 而不是 4
xml = '''<XWindow Width="300" Height="200" Background="#202428">
 <div Name="box" Top="10" Left="10" Width="200" Height="120" style="display:flex;flex-direction:column;padding:4 16;gap:6">
   <div Name="a" Height="30" style="background:#334"/>
   <div Name="b" Height="30" style="background:#445"/>
 </div></XWindow>'''
n = {}; w = M.guiload_text(xml, names=n); M.pump(200)
ra = w.get_rect(n["a"]); rb = w.get_rect(n["b"]); rb_ = w.get_rect(n["box"])
print(" a=", ra, " b=", rb, " box=", rb_)
chk("padding:4 16 -> 子项左移 16", ra[0] == 26, ra)
chk("padding:4 16 -> 子项上移 14(10+4)", ra[1] == 14, ra)
chk("padding:4 16 -> 子项右留 16(宽 168)", ra[2] == 168, ra)
chk("gap:6 生效", rb[1] == ra[1] + ra[3] + 6, (ra, rb))

# 2) padding 三值：上 10 / 左右 20 / 下 5
xml2 = '''<XWindow Width="300" Height="200" Background="#202428">
 <div Name="box" Top="10" Left="10" Width="200" Height="120" style="display:flex;flex-direction:column;padding:10 20 5">
   <div Name="a" Height="30" style="background:#334"/>
 </div></XWindow>'''
n2 = {}; w2 = M.guiload_text(xml2, names=n2); M.pump(150)
ra2 = w2.get_rect(n2["a"])
chk("padding:10 20 5 -> x=30, w=160, y=20", ra2[0] == 30 and ra2[2] == 160 and ra2[1] == 20, ra2)
w2.close()

# 3) padding-left 单边
xml3 = '''<XWindow Width="300" Height="200" Background="#202428">
 <div Name="box" Top="10" Left="10" Width="200" Height="120" style="padding-left:30">
   <label Name="t" Top="0" Left="0" Width="100" Height="24" Text="X"/>
 </div></XWindow>'''
n3 = {}; w3 = M.guiload_text(xml3, names=n3); M.pump(150)
w3.close()

# 4) 声明名两种写法等价：camelCase（borderRadius / fontSize / textAlign）与 kebab-case
#    早先只做 Lower，驼峰被压成 borderradius，与 border-radius 不相等 → 写了不生效
def _alias(decl, prop):
    xml = ('<XWindow Width="300" Height="160">'
           '<label Name="t" Text="X" Width="120" Height="30" style="' + decl + '"/>'
           '</XWindow>')
    nn = {}
    ww = M.guiload_text(xml, names=nn); ww.show(); M.pump(60)
    v = ww.get_xml_value(nn["t"], prop)
    ww.close()
    return v

for kebab, camel, prop in [("border-radius:8px", "borderRadius:8px", "Radius"),
                           ("font-size:18px", "fontSize:18px", "FontSize"),
                           ("text-align:center", "textAlign:center", "TextAlign"),
                           ("background-color:#123456", "backgroundColor:#123456", "Background")]:
    chk(kebab + " 与 " + camel + " 等价", _alias(kebab, prop) == _alias(camel, prop),
        (_alias(kebab, prop), _alias(camel, prop)))

# HTML 的 `color:` 在 XML 侧内部叫 Foreground，回读别名 Color 必须等价
chk("color: 的别名 Color 与 Foreground 等价",
    _alias("color:#CC0000", "Color") == _alias("color:#CC0000", "Foreground") == "#CC0000",
    (_alias("color:#CC0000", "Color"), _alias("color:#CC0000", "Foreground")))

# 5) inset 简写尚未实现（right/bottom 反推尺寸缺失），此处不测，见 html-xml-map 待办
w4 = None
w.close()
print("t_style_box: PASS=%d FAIL=%d" % (total[0] - len(fails), len(fails)))
