# 第58轮：CSS `gap:<row> <column>` 两值写法
# HTML 里 `gap:10px 30px` 分别是行距/列距；XML 早先只吃单值，第二个值被整段丢弃，
# 换行后行距被迫等于列距。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

XML = ('<XWindow Width="460" Height="240">'
       '<div Name="box" Left="20" Top="20" Width="400" Height="200" '
       'style="display:flex;flex-wrap:wrap;gap:10 30">'
       '<div Name="a" Width="150" Height="40"/><div Name="b" Width="150" Height="40"/>'
       '<div Name="c" Width="150" Height="40"/><div Name="d" Width="150" Height="40"/>'
       '</div></XWindow>')

def layout(xml, tag):
    """xmlLayout 只吃文件路径，这里落一份临时 xml 再算"""
    path = "G:/PyArx/log/_gap_%s.xml" % tag
    with open(path, "w", encoding="utf-8") as f:
        f.write(xml)
    return M.xml_layout(path, 460, 240)

rects = layout(XML, "two")
a, b, c = rects["a"], rects["b"], rects["c"]
colGap = b[0] - (a[0] + a[2])
rowGap = c[1] - (a[1] + a[3])

chk("列距 = 30（gap 的第二个值）", colGap == 30, colGap)
chk("行距 = 10（gap 的第一个值）", rowGap == 10, rowGap)
chk("四张卡都排进两行两列", abs(rects["d"][0] - b[0]) <= 0 and abs(rects["d"][1] - c[1]) <= 0,
    (rects["d"], b, c))

# 单值仍然行列相同（回归）
r2 = layout(XML.replace("gap:10 30", "gap:10"), "one")
chk("单值 gap 仍行列相同", (r2["b"][0] - (r2["a"][0] + r2["a"][2])) == 10 and
                         (r2["c"][1] - (r2["a"][1] + r2["a"][3])) == 10,
    (r2["b"][0] - (r2["a"][0] + r2["a"][2]), r2["c"][1] - (r2["a"][1] + r2["a"][3])))
print("t_gap_two.py: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
