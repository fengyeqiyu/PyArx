# 第52轮：CSS object-fit（图片在盒子里的缩放模式）
# 缺口：原生 SS_BITMAP Static 只按原图尺寸居中+裁剪，HTML 里 <img> 写了 width/height
# 就自动缩放、object-fit 还能选 contain/cover/none —— XML 侧此前一条都表达不了。
import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

SRC = r'G:\PyArx\log\_t_ofit_src.png'
# 20×20 正方形纯红图：盒子 200×100 下，contain 应缩成 100×100 居中，cover 应铺满并裁掉
Image.new('RGB', (20, 20), (255, 0, 0)).save(SRC, 'PNG')

# 40×10 双色图（左 40% 红、右 60% 绿）：用来证明 cover 是"等比放大后裁边"，
# 而不是 fill 那样横向拉伸（拉伸时红占 40px，cropping 时红只剩 10px）。
SRC2 = r'G:\PyArx\log\_t_ofit_src2.png'
im2 = Image.new('RGB', (40, 10), (0, 255, 0))
for y in range(10):
    for x in range(16):
        im2.putpixel((x, y), (255, 0, 0))
im2.save(SRC2, 'PNG')

XML = '''<window Width="460" Height="520" Background="#313842">
  <XImage Image="%s" Left="20" Top="20" Width="200" Height="100" style="background:#0000FF;object-fit:fill"/>
  <XImage Image="%s" Left="240" Top="20" Width="200" Height="100" style="background:#0000FF;object-fit:contain"/>
  <XImage Image="%s" Left="20" Top="140" Width="200" Height="100" style="background:#0000FF;object-fit:cover"/>
  <XImage Image="%s" Left="240" Top="140" Width="200" Height="100" style="background:#0000FF" ObjectFit="none"/>
  <XImage Image="%s" Left="20" Top="260" Width="100" Height="100" style="background:#0000FF;object-fit:cover"/>
  <XImage Image="%s" Left="240" Top="260" Width="200" Height="100" style="background:#0000FF;object-fit:contain;object-position:left top"/>
  <XImage Image="%s" Left="20" Top="380" Width="100" Height="100" style="background:#0000FF;object-fit:none;object-position:30px 20px"/>
</window>''' % (SRC, SRC, SRC, SRC, SRC2, SRC, SRC)

OX, OY = 8, 31          # 截图固定偏移（含标题栏）
RED = (255, 0, 0)
BLUE = (0, 0, 255)


def near(px, rgb, tol=26):
    return all(abs(px[i] - rgb[i]) <= tol for i in range(3))


def at(px, base, dx, dy):
    return px[base[0] + OX + dx, base[1] + OY + dy]


p = r'G:\PyArx\log\_t_ofit.xml'
open(p, 'w', encoding='utf-8').write(XML)
w = M.guiload(p, context={}, names={})
w.show()
M.pump(500)
w.save_png(r'G:\PyArx\log\_t_ofit.png')
w.close()
img = Image.open(r'G:\PyArx\log\_t_ofit.png').convert('RGB')
px = img.load()
OK = True

A, B, C, D = (20, 20), (240, 20), (20, 140), (240, 140)

ok1 = near(at(px, A, 10, 10), RED) and near(at(px, A, 190, 90), RED)
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + 'fill 铺满整盒（角上也是图）%s %s' % (at(px, A, 10, 10), at(px, A, 190, 90)))

ok2 = near(at(px, B, 100, 50), RED) and near(at(px, B, 10, 50), BLUE) and near(at(px, B, 190, 50), BLUE)
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + 'contain 等比缩到 100×100 居中、左右留底色 %s %s %s'
      % (at(px, B, 100, 50), at(px, B, 10, 50), at(px, B, 190, 50)))

ok3 = near(at(px, C, 5, 5), RED) and near(at(px, C, 195, 95), RED) and near(at(px, C, 100, 50), RED)
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + 'cover 放大到 200×200 并裁掉溢出部分（整盒都是图）%s %s %s'
      % (at(px, C, 5, 5), at(px, C, 195, 95), at(px, C, 100, 50)))

ok4 = near(at(px, D, 95, 45), RED) and near(at(px, D, 85, 45), BLUE) and near(at(px, D, 5, 5), BLUE)
OK &= ok4
print(('PASS ' if ok4 else 'FAIL ') + 'none 原尺寸 20×20 居中（属性写法 ObjectFit 同样生效）%s %s %s'
      % (at(px, D, 95, 45), at(px, D, 85, 45), at(px, D, 5, 5)))

GREEN = (0, 255, 0)
E = (20, 260)

ok5 = near(at(px, E, 5, 50), RED) and near(at(px, E, 30, 50), GREEN) and near(at(px, E, 90, 50), GREEN)
OK &= ok5
print(('PASS ' if ok5 else 'FAIL ') + 'cover 等比放大裁边（红带只剩 10px，fill 会是 40px）%s %s %s'
      % (at(px, E, 5, 50), at(px, E, 30, 50), at(px, E, 90, 50)))

F = (240, 260)

ok6 = near(at(px, F, 5, 5), RED) and near(at(px, F, 95, 95), RED) \
      and near(at(px, F, 150, 50), BLUE) and near(at(px, F, 110, 5), BLUE)
OK &= ok6
print(('PASS ' if ok6 else 'FAIL ') + 'object-position:left top 把 contain 的 100×100 贴到左上角 %s %s %s %s'
      % (at(px, F, 5, 5), at(px, F, 95, 95), at(px, F, 150, 50), at(px, F, 100, 5)))

G = (20, 380)

ok7 = near(at(px, G, 35, 25), RED) and near(at(px, G, 10, 10), BLUE) and near(at(px, G, 60, 70), BLUE)
OK &= ok7
print(('PASS ' if ok7 else 'FAIL ') + 'object-position:30px 20px 长度值把 none 的 20×20 图放到 (30,20) %s %s %s'
      % (at(px, G, 35, 25), at(px, G, 10, 10), at(px, G, 60, 70)))

print('t_object_fit: %s' % ('PASS=7 FAIL=0' if OK else 'FAIL'))
