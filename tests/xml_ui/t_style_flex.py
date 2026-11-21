# 第60轮：style 里的 flex 声明要真能落到布局上。
# HTML 里这些全是 style 写法（display:flex / justify-content:center / align-items / align-self / gap /
# flex-direction），XML 早先只有"属性写法"生效，style 写法恒等于没写 —— 书写成本差一整档。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def layout(inner, css, tag):
    xml = ('<XWindow Width="420" Height="240">'
           '<div Name="box" Left="20" Top="20" Width="380" Height="120" style="display:flex;%s">'
           '%s</div></XWindow>') % (css, inner)
    path = "G:/PyArx/log/_sf_%s.xml" % tag
    with open(path, "w", encoding="utf-8") as f:
        f.write(xml)
    return M.xml_layout(path, 420, 240)

# 1/2. 子项 align-self：容器横向 flex，交叉轴=竖直
r1 = layout('<div Name="x" Width="120" Height="40" style="align-self:flex-start"/>', "", "as")["x"]
chk("style align-self:flex-start 贴顶", r1[1] == 20, r1[1])
r2 = layout('<div Name="x" Width="120" Height="40" style="align-self:flex-end"/>', "", "ae")["x"]
chk("style align-self:flex-end 贴底", r2[1] == 100, r2[1])

# 3. 容器 justify-content:center（主轴上居中）
r3 = layout('<div Name="x" Width="100" Height="40"/>', "justify-content:center;", "jc")["x"]
chk("style justify-content:center 主轴居中", r3[0] == 160, r3[0])

# 4. 容器 align-items:flex-end（交叉轴贴底）
r4 = layout('<div Name="x" Width="100" Height="40"/>', "align-items:flex-end;", "ai")["x"]
chk("style align-items:flex-end 交叉轴贴底", r4[1] == 100, r4[1])

# 5. flex-direction:column（第二个落到下一行）
r5a = layout('<div Name="a" Width="100" Height="40"/><div Name="b" Width="100" Height="40"/>',
             "flex-direction:column;", "col")
chk("style flex-direction:column 纵排", r5a["b"][1] == 60, r5a["b"][1])

# 6. gap:10（两值简写先只测单值）
r6 = layout('<div Name="a" Width="100" Height="40"/><div Name="b" Width="100" Height="40"/>',
            "gap:10;", "gap")
chk("style gap:10 横向间距", r6["b"][0] == 130, r6["b"][0])

print("t_style_flex.py: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))
