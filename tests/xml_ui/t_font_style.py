# 第22轮：CSS font-weight 数字值 / font-family / font-style 归一（此前写了静默失效）
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def prop(inner, key, name="e"):
    n = {}
    w = M.guiload_text('<XWindow Width="420" Height="160" Background="#202428">' + inner + '</XWindow>', names=n)
    M.pump(120)
    v = w.get_xml_value(n[name], key)
    w.close()
    return v

# font-weight 数字 → 绘制端枚举（此前 _wcsicmp 比对 Bold/SemiBold/Light，全落 Normal）
chk("font-weight:600 -> SemiBold", prop('<label Name="e" style="font-weight:600">A</label>', "FontWeight") == "SemiBold")
chk("font-weight:700 -> Bold", prop('<label Name="e" style="font-weight:700">A</label>', "FontWeight") == "Bold")
chk("font-weight:900 -> Bold", prop('<label Name="e" style="font-weight:900">A</label>', "FontWeight") == "Bold")
chk("font-weight:300 -> Light", prop('<label Name="e" style="font-weight:300">A</label>', "FontWeight") == "Light")
chk("font-weight:500 -> SemiBold", prop('<label Name="e" style="font-weight:500">A</label>', "FontWeight") == "SemiBold")
# 属性直写数字也要归一（老 XML 也能享受）
chk('FontWeight="800" -> Bold', prop('<label Name="e" FontWeight="800">A</label>', "FontWeight") == "Bold")
# 关键字小写归一
chk("font-weight:bold -> Bold", prop('<label Name="e" style="font-weight:bold">A</label>', "FontWeight") == "Bold")

# font-family / font-style 此前 style 里完全不映射
chk("font-family 映射", prop('<label Name="e" style="font-family:Consolas">A</label>', "FontFamily") == "Consolas")
chk("font-style:italic 映射", prop('<label Name="e" style="font-style:italic">A</label>', "FontStyle") == "Italic")

print("t_font_style: PASS=%d FAIL=%d" % (9 - len(fails), len(fails)))
