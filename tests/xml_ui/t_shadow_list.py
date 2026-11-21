# 第 98 轮：CSS box-shadow 多阴影列表（逗号分隔）。
# 老实现把整串当一条扫：数字只取前 4 个、颜色被最后一条覆盖，多条阴影退化成一条语义错位的阴影。
# 断言口径：暗带"向下延伸多远"（对 savePng 整窗截图的客户区偏移免疫）。
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
total = [0]
def chk(name, cond, got=""):
    total[0] += 1
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond:
        fails.append(name)

PAGE = 0x20 * 3

def build(style, out):
    n = {}
    w = M.guiload_text(
        '<XWindow Width="300" Height="240" Background="#202020">'
        '<div Name="box" Left="40" Top="40" Width="160" Height="80" '
        'style="background:#3A4048;' + style + '"/>'
        '</XWindow>', names=n)
    M.pump(200)
    w.save_png(out)
    r = w.get_rect(n["box"])
    w.close()
    return r

def dark_extent(im, rect):
    """返回盒子下缘之外最远的"暗于页面底色"的行号（int）。"""
    x0 = rect[0] + 20
    x1 = rect[0] + rect[2] - 20
    y0 = rect[1] + rect[3]
    y1 = min(rect[1] + rect[3] + 80, im.size[1] - 6)
    last = None
    for y in range(max(0, y0), y1):
        for x in range(max(0, x0), x1):
            if sum(im.getpixel((x, y))[:3]) < PAGE - 6:
                last = y
                break
    return last

def dark_area(im, rect):
    """盒子下缘之外"暗于页面底色"的像素个数：多条阴影叠加必然更大。
    用面积而不是最远行号——最外圈 alpha 已经很浅（0.55/6 ≈ 0.09），
    会被 6 档阈值截掉，行号几乎不随阴影条数变化。"""
    x0 = rect[0] + 20
    x1 = rect[0] + rect[2] - 20
    y0 = rect[1] + rect[3]
    y1 = min(rect[1] + rect[3] + 80, im.size[1] - 6)
    n = 0
    for y in range(max(0, y0), y1):
        for x in range(max(0, x0), x1):
            if sum(im.getpixel((x, y))[:3]) < PAGE - 6:
                n += 1
    return n

def inner_dark(im, rect):
    """盒子内左上角一圈是否被 inset 压暗（返回该点的三通道和）。"""
    x = rect[0] + 3
    y = rect[1] + 3
    return sum(im.getpixel((x, y))[:3])

# A) 单条外投：基线
rA = build('box-shadow:0 4 10 rgba(0,0,0,0.55)', "G:/PyArx/log/shadow_list_a.png")
imA = Image.open("G:/PyArx/log/shadow_list_a.png").convert("RGB")
eA = dark_extent(imA, rA)
aA = dark_area(imA, rA)
chk("单条外投：下方出现压暗带", eA is not None, eA)

# B) 两条外投：第二条故意给大下移（22px），落在第一条（下移 4 + blur 10 → 最多到 +9）够不着的地方，
# 这样"第二条有没有画"在像素上可判别；此前用同位置的 spread 阴影会被第一条整片盖掉而看不出差别。
rB = build('box-shadow:0 4 10 rgba(0,0,0,0.55), 0 22 6 rgba(0,0,0,0.50)',
           "G:/PyArx/log/shadow_list_b.png")
imB = Image.open("G:/PyArx/log/shadow_list_b.png").convert("RGB")
eB = dark_extent(imB, rB)
aB = dark_area(imB, rB)
chk("多阴影：下方出现压暗带", eB is not None, eB)
# 口径修正（第 98 轮）：若第二条阴影与第一条位置重叠，会被先画的深色第一条整片盖住，
# 像素上看不出差别（容易被误判成"第二条没画"）。所以第二条必须放在第一条够不着的位置。
chk("多阴影：第二条真的画出来了（压暗面积显著变大）", aB > aA * 1.3, (aA, aB))

# C) rgba 内部逗号不能被当成分隔符：单条 rgba 的语义与 A 一致
rC = build('box-shadow:0 4 10 rgba(0,0,0,0.55)', "G:/PyArx/log/shadow_list_c.png")
imC = Image.open("G:/PyArx/log/shadow_list_c.png").convert("RGB")
eC = dark_extent(imC, rC)
chk("rgba(r,g,b,a) 内逗号不参与切分（与 A 等价）", eC == eA, (eA, eC))

# D) inset + 外投同串：两条都要生效（内圈压暗 且 外侧仍有暗带）
rD = build('box-shadow:inset 0 0 0 3 rgba(0,0,0,0.65), 0 5 12 rgba(0,0,0,0.55)',
           "G:/PyArx/log/shadow_list_d.png")
imD = Image.open("G:/PyArx/log/shadow_list_d.png").convert("RGB")
innerD = inner_dark(imD, rD)
eD = dark_extent(imD, rD)
fill = sum(imD.getpixel((rD[0] + rD[2] // 2, rD[1] + rD[3] // 2))[:3])
chk("inset+外投：盒内边缘被压暗", innerD < fill - 8, (innerD, fill))
chk("inset+外投：盒外仍有外投影", eD is not None, eD)

print("t_shadow_list: PASS=%d FAIL=%d TOTAL=%d" % (total[0] - len(fails), len(fails), total[0]))
sys.exit(1 if fails else 0)
