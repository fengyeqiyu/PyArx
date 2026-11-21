# 第32轮：CSS display:none（HTML 最常用的隐藏写法）
# XML 早先只认 hidden 属性；`style="display:none"` 被当成普通 display 值，元素照旧显示。
# 注：Visible 是控件属性（控件层 m_visible → 宿主 setVisible），不是外观属性，
#     getXmlValue 读不到，必须用真实 HWND 的 IsWindowVisible 判定。
import os, sys, tempfile, uuid, ctypes, ctypes.wintypes as wt
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def load(inner, w=300, h=200):
    p = os.path.join(tempfile.gettempdir(), "dn_%s.xml" % uuid.uuid4().hex[:8])
    with open(p, "w", encoding="utf-8") as f:
        f.write('<XWindow Width="%d" Height="%d">%s</XWindow>' % (w, h, inner))
    ids = {}
    return M.guiload(p, names=ids), ids

def visible_texts(win, texts):
    user32 = ctypes.windll.user32
    user32.EnumChildWindows.argtypes = [wt.HWND, ctypes.c_void_p, wt.LPARAM]
    user32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
    user32.IsWindowVisible.argtypes = [wt.HWND]
    hit = set()
    CB = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        buf = ctypes.create_unicode_buffer(64)
        user32.GetWindowTextW(h, buf, 64)
        if buf.value in texts and user32.IsWindowVisible(h):
            hit.add(buf.value)
        return True
    M.pump(150)
    user32.EnumChildWindows(wt.HWND(int(win.hwnd)), CB(cb), 0)
    return hit

# 1) display:none → 控件不可见
win, ids = load('<XLabel Name="g" Left="20" Top="20" Width="100" Height="30" '
                'style="display:none">VIS</XLabel>')
hit = visible_texts(win, {"VIS"})
chk("display:none 后控件不可见", "VIS" not in hit, hit)
win.close()

# 2) 不写 display 时仍然可见（不回归）
win, ids = load('<XLabel Name="g" Left="20" Top="20" Width="100" Height="30">VIS</XLabel>')
hit = visible_texts(win, {"VIS"})
chk("未声明 display 仍可见", "VIS" in hit, hit)
win.close()

# 3) display:block 不影响可见性
win, ids = load('<XLabel Name="g" Left="20" Top="20" Width="100" Height="30" '
                'style="display:block">VIS</XLabel>')
chk("display:block 仍可见", "VIS" in visible_texts(win, {"VIS"}))
win.close()

# 4) hidden 属性同样生效（HTML 两条写法等价）
win, ids = load('<label Name="g" Left="20" Top="20" Width="100" Height="30" hidden="">VIS</label>')
chk("hidden 属性映射为不可见", "VIS" not in visible_texts(win, {"VIS"}))
win.close()

# 5) display:flex 不被当成 none（不回归），且容器横排仍生效
win, ids = load('<div Name="g" Left="20" Top="20" Width="200" Height="100" '
                'style="display:flex">'
                '<XLabel Name="a" Width="40" Height="20">A1</XLabel>'
                '<XLabel Name="b" Width="40" Height="20">B1</XLabel></div>')
ra = win.get_rect(ids["a"]); rb = win.get_rect(ids["b"])
chk("display:flex 仍横排（b 在 a 右侧）", rb[0] > ra[0], (ra, rb))
chk("display:flex 容器子项可见", visible_texts(win, {"A1", "B1"}) == {"A1", "B1"},
    visible_texts(win, {"A1", "B1"}))
win.close()

# 6) style display:none 与属性 Visible=True 同写：CSS 更具体 → 仍隐藏
win, ids = load('<XLabel Name="g" Left="20" Top="20" Width="100" Height="30" Visible="True" '
                'style="display:none">VIS</XLabel>')
chk("style display:none 覆盖 Visible=True", "VIS" not in visible_texts(win, {"VIS"}))
win.close()

# 7) display:none 的元素**不占位**（HTML 语义）：a/b(隐藏)/c 三兄弟横排，
#    c 应紧接 a 排列（x = a.x + a.w），而不是被隐藏的 b 挤开
win, ids = load('<div Name="g" Left="10" Top="10" Width="300" Height="60" style="display:flex">'
                '<XLabel Name="a" Width="60" Height="30">A2</XLabel>'
                '<XLabel Name="b" Width="80" Height="30" style="display:none">B2</XLabel>'
                '<XLabel Name="c" Width="60" Height="30">C2</XLabel></div>')
ra = win.get_rect(ids["a"]); rc = win.get_rect(ids["c"])
chk("display:none 不占位（c 紧接 a）", rc[0] == ra[0] + ra[2], (ra, rc))
win.close()

# 8) 隐藏项不占位时也不影响 justify-content 剩余空间分配
win, ids = load('<div Name="g" Left="10" Top="10" Width="300" Height="60" '
                'style="display:flex;justify-content:flex-end">'
                '<XLabel Name="a" Width="60" Height="30">A3</XLabel>'
                '<XLabel Name="b" Width="100" Height="30" style="display:none">B3</XLabel></div>')
ra = win.get_rect(ids["a"])
chk("隐藏项不参与 justify-content 计算", ra[0] == 10 + 300 - 60, ra)
win.close()

print("t_display_none: PASS=%d FAIL=%d" % (9 - len(fails), len(fails)))




