# 第59轮：CSS `align-self` 的值域（flex-start / flex-end / baseline / auto / middle）
# HTML 里子项写 `align-self:flex-start` 就顶到交叉轴起点；XML 早先只认 start/center/end，
# `flex-start` 落进 else 被当成 stretch，子项在交叉轴被拉满 —— 差一整个语义。
# 注：本轮先用属性写法（`align-self="..."`）；style="align-self:..." 走不通是另一处时序缺口，记在下一轮。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def layout(xml, tag):
    path = "G:/PyArx/log/_self_%s.xml" % tag
    with open(path, "w", encoding="utf-8") as f:
        f.write(xml)
    return M.xml_layout(path, 420, 240)

def build(child):
    return ('<XWindow Width="420" Height="240">'
            '<div Name="box" Left="20" Top="20" Width="380" Height="120" '
            'style="display:flex;align-items:center">' + child + '</div></XWindow>')

# 容器 20..140（高 120），子项高 40：center → y≈60，flex-start → y≈20，flex-end → y≈100
r_start = layout(build('<div Name="x" Width="120" Height="40" align-self="flex-start"/>'), "s")["x"]
r_end = layout(build('<div Name="x" Width="120" Height="40" align-self="flex-end"/>'), "e")["x"]
r_auto = layout(build('<div Name="x" Width="120" Height="40" align-self="auto"/>'), "a")["x"]
r_center = layout(build('<div Name="x" Width="120" Height="40" align-self="center"/>'), "c")["x"]

chk("align-self=flex-start 顶到交叉轴起点（此前被当 stretch、高 120）",
    r_start[1] == 20 and r_start[3] == 40, (r_start[1], r_start[3]))
chk("align-self=flex-end 贴交叉轴末端（此前同样被当 stretch）",
    r_end[1] == 100 and r_end[3] == 40, (r_end[1], r_end[3]))
chk("align-self=auto 跟随容器 align-items:center", r_auto[1] == 60, r_auto[1])
chk("align-self=center 与 auto 同值", r_center[1] == 60, r_center[1])
print("t_align_self.py: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
