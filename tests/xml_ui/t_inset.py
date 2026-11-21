# 第37轮：CSS `inset` 简写（1~4 值，顺序同 margin）
# HTML 里 `inset:0` 一句就是"撑满父"；XML 早先完全没有这个写法，
# 且直接落 Right/Bottom 会被宿主当右下锚定→元素被压成 0 尺寸，故全 0 翻译成 Left/Top + 100%。
# 注：容器 div 不建原生控件，位置一律用 xmlLayout 预览取（零成本、不开窗口）。
import os, sys, tempfile, uuid
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

W, H = 360, 220
def layout(inner):
    p = os.path.join(tempfile.gettempdir(), "ins_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (W, H, inner))
    return M.xml_layout(p, W, H)

def box(child):
    # 父用 XPanel（非 flex 容器）：inset:0 走"铺满父内框"的定位路径
    return ('<XPanel Name="g" Left="10" Top="10" Width="300" Height="120">%s</XPanel>' % child)

# 1) inset:0 撑满父：子项与父内框同框
L = layout(box('<label Name="p" style="inset:0">x</label>'))
chk("inset:0 撑满父", tuple(L["p"]) == (10, 10, 300, 120), L["p"])

# 2) 与手写锚定四件套一致（内部实现相同，用户侧只写一句 inset:0）
L = layout(box('<label Name="a" style="inset:0">x</label>'))
chk("inset:0 不产生 0 尺寸", L["a"][2] > 0 and L["a"][3] > 0, L["a"])

# 3) 非全 0 值按 CSS 顺序落四边：上4 右8 下12 左16
#    第 6 轮起同时校验尺寸：两条边都写了、又没写 Width/Height，尺寸由边反推
L = layout(box('<label Name="c" style="inset:4px 8px 12px 16px">x</label>'))
rc = L["c"]
chk("inset 四值顺序 上右下左", rc[0] == 10 + 16 and rc[1] == 10 + 4, rc)
chk("inset 四值反推尺寸", rc[2] == 300 - 16 - 8 and rc[3] == 120 - 4 - 12, rc)

# 3b) 两值简写（上下 12 / 左右 20），HTML 里最常用的写法
L = layout(box('<label Name="e" style="inset:12px 20px">x</label>'))
rc = L["e"]
chk("inset:12 20 位置", rc[0] == 10 + 20 and rc[1] == 10 + 12, rc)
chk("inset:12 20 尺寸", rc[2] == 300 - 40 and rc[3] == 120 - 24, rc)

# 3c) 只写 right/bottom（HTML 的右下角定位），宽高按声明值
L = layout(box('<label Name="f" style="right:20px;bottom:12px;width:50px;height:20px">x</label>'))
rc = L["f"]
chk("right/bottom 贴右下", rc[0] == 10 + 300 - 20 - 50 and rc[1] == 10 + 120 - 12 - 20, rc)
chk("right/bottom 不压尺寸", rc[2] == 50 and rc[3] == 20, rc)

# 3d) 百分比边值按父内框换算（CSS `inset:0 10%` = 左右各 10% → 右/左各 30，元素占 30..270）
L = layout(box('<label Name="g2" style="inset:0 10%">x</label>'))
rc = L["g2"]
chk("inset 百分比边", rc[0] == 10 + 30 and rc[2] == 300 - 60, rc)

# 4) 不写 inset 不回归（label 仍按内容宽、贴左）
L = layout(box('<label Name="d">文字</label>'))
rd = L["d"]
chk("未写 inset 不回归", rd[0] == 10 and rd[2] < 300, rd)

print("t_inset: PASS=%d FAIL=%d" % (9 - len(fails), len(fails)))
