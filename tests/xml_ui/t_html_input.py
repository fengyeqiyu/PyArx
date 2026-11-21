# 第19轮：<input> 默认值对齐 HTML（size=20 字符宽、1.3 倍行高），老 <XTextField> 不变
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def size(tagxml):
    n = {}
    w = M.guiload_text('<XWindow Width="420" Height="160" Background="#202428">' + tagxml + '</XWindow>', names=n)
    M.pump(120)
    r = w.get_rect(n["e"])
    w.close()
    return r

# 1) 原生 <input> 不写尺寸：14 号字 -> 20*14*0.55+8 = 162，高 20
r1 = size('<input Name="e" Top="10" Left="10"/>')
chk("原生 input 默认宽 = 162(20 字符)", r1[2] == 162, r1)
chk("原生 input 默认高 = 20", r1[3] == 20, r1)

# 2) 字号变大 -> 宽高跟着变
r2 = size('<input Name="e" Top="10" Left="10" style="font-size:20"/>')
chk("font-size:20 -> 宽 228", r2[2] == 228, r2)
chk("font-size:20 -> 高 26", r2[3] == 26, r2)

# 3) 显式尺寸优先，不被默认值覆盖
r3 = size('<input Name="e" Top="10" Left="10" Width="200" Height="34"/>')
chk("显式 200x34 保持", r3[2] == 200 and r3[3] == 34, r3)

# 4) 老写法 <XTextField> 仍是 160x26（不破坏既有界面）
r4 = size('<XTextField Name="e" Top="10" Left="10"/>')
chk("<XTextField> 仍 160x26", r4[2] == 160 and r4[3] == 26, r4)

print("t_html_input: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))
