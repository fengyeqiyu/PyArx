# 第43轮：CSS font-family 字体回退列表 + 引号
# HTML 写 font-family:"Segoe UI", Arial, sans-serif —— 早先整串塞进 FontFamily，
# 控件按整串去匹配字体名必然失败 → 静默回落默认字体（写了等于没写）。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def fam(css):
    n = {}
    w = M.guiload_text('<XWindow Width="300" Height="80"><label Name="t" Top="10" Left="10" '
                       'Width="280" style="font-family:%s">Hi</label></XWindow>' % css, names=n)
    M.pump(100)
    v = w.get_xml_value(n["t"], "FontFamily")
    w.close()
    return v

chk("带引号+回退列表取首个", fam('&quot;Segoe UI&quot;, Arial, sans-serif') == "Segoe UI",
    fam('&quot;Segoe UI&quot;, Arial, sans-serif'))
chk("无引号+回退列表取首个", fam("Microsoft YaHei, Arial") == "Microsoft YaHei",
    fam("Microsoft YaHei, Arial"))
chk("通用族留空（用系统默认）", fam("sans-serif") == "", repr(fam("sans-serif")))
chk("单字体名不变", fam("Consolas") == "Consolas", fam("Consolas"))

# font 简写里的 family 也走同一条，不回归
n = {}
w = M.guiload_text('<XWindow Width="300" Height="80"><label Name="t" Top="10" Left="10" '
                   'Width="280" style="font:bold 16px/1.4 Consolas">Hi</label></XWindow>', names=n)
M.pump(100)
chk("font 简写的 family 仍正确", w.get_xml_value(n["t"], "FontFamily") == "Consolas",
    w.get_xml_value(n["t"], "FontFamily"))
w.close()

print("t_font_family_list: PASS=%d FAIL=%d" % (5 - len(fails), len(fails)))
