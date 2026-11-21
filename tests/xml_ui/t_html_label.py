# 第21轮：<label> 不写宽度时按内容（HTML inline 语义），老 <XLabel> 保持 100
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def size(inner, name="e"):
    n = {}
    w = M.guiload_text('<XWindow Width="420" Height="160" Background="#202428">' + inner + '</XWindow>', names=n)
    M.pump(120)
    r = w.get_rect(n[name])
    w.close()
    return r

# 1) 原生 <label> 两个汉字（14 号字）-> 2*14+4 = 32
r1 = size('<label Name="e" Top="10" Left="10">确定</label>')
chk("原生 label 按内容宽 = 32", r1[2] == 32, r1)
chk("原生 label 默认高 = 24", r1[3] == 24, r1)

# 2) 英文文本按 0.55 字号
r2 = size('<label Name="e" Top="10" Left="10">OK</label>')
chk("英文两字符 -> 2*14*0.55+4 = 19", r2[2] == 19, r2)

# 3) 字号 20 -> 2*20+4 = 44
r3 = size('<label Name="e" Top="10" Left="10" style="font-size:20">确定</label>')
chk("font-size:20 -> 44", r3[2] == 44, r3)

# 4) 显式宽度优先
r4 = size('<label Name="e" Top="10" Left="10" Width="200">确定</label>')
chk("显式 Width=200 保持", r4[2] == 200, r4)

# 5) 老写法 <XLabel> 不变：不写宽度时仍按老规则铺满可用宽（不是按内容）
r5 = size('<XLabel Name="e" Top="10" Left="10" Text="确定"/>')
chk("<XLabel> 仍走老规则(铺满)", r5[2] > 100, r5)

print("t_html_label: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))
