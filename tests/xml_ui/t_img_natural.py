# 第60轮：<img> 不写 width/height 时按原图自然尺寸占位（HTML intrinsic size）
# 缺口：此前一律用布局盒子的默认尺寸（100×100），40×10 的图标会被撑成 100×100 拉伸。
import os, sys, ctypes
from ctypes import wintypes
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

SRC = r'G:\PyArx\log\_t_ofit_src2.png'
im = Image.new('RGB', (40, 10), (0, 255, 0))
for y in range(10):
    for x in range(16):
        im.putpixel((x, y), (255, 0, 0))
im.save(SRC, 'PNG')

SRC400 = r'G:\PyArx\log\_t_img_400.png'
Image.new('RGB', (400, 200), (255, 0, 0)).save(SRC400, 'PNG')

XML = '''<window Width="360" Height="440" Background="#313842">
  <img src="%s" style="left:20;top:20"/>
  <img src="%s" style="left:20;top:60;max-width:100%%"/>
  <img src="%s" style="left:20;top:250;max-height:120"/>
</window>''' % (SRC, SRC400, SRC400)

u = ctypes.windll.user32
u.GetWindow.argtypes = [wintypes.HWND, ctypes.c_uint]
u.GetWindow.restype = wintypes.HWND
u.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
WM_GETTEXTLENGTH = 0x000E


def children(h):
    out, c = [], u.GetWindow(h, 5)
    while c:
        out.append(c)
        c = u.GetWindow(c, 2)
    return out


def all_desc(h):
    out, stack = [], [h]
    while stack:
        for c in children(stack.pop()):
            out.append(c)
            stack.append(c)
    return out


p = r'G:\PyArx\log\_t_img_nat.xml'
open(p, 'w', encoding='utf-8').write(XML)
w = M.guiload(p, context={}, names={})
w.show()
M.pump(400)
top = wintypes.HWND(int(w.hwnd))
kids = all_desc(top)
OK = True

rects = []
for h in kids:
    r = wintypes.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    rects.append((h, r.right - r.left, r.bottom - r.top))

nat = [x for x in rects if x[1] < 200 and x[2] < 60]
ok1 = len(nat) >= 1 and nat[0][1] == 40 and nat[0][2] == 10
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + 'img 窗口尺寸 = 原图 40×10（实际 %s）' % (str(nat[0][1:]) if nat else '空'))

w.save_png(r'G:\PyArx\log\_t_img_nat.png')
w.close()

img = Image.open(r'G:\PyArx\log\_t_img_nat.png').convert('RGB')
px = img.load()
OX, OY = 8, 31
RED = (255, 0, 0)
GREEN = (0, 255, 0)
PAGE = (49, 56, 66)


def near(p_, rgb, tol=26):
    return all(abs(p_[i] - rgb[i]) <= tol for i in range(3))


ok2 = near(px[20 + OX + 4, 20 + OY + 4], RED) and near(px[20 + OX + 34, 20 + OY + 4], GREEN)
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + '自然尺寸下落位：左段红、右段绿 %s %s'
      % (px[20 + OX + 4, 20 + OY + 4], px[20 + OX + 34, 20 + OY + 4]))

ok3 = near(px[20 + OX + 55, 20 + OY + 4], PAGE) and near(px[20 + OX + 20, 20 + OY + 20], PAGE)
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + '图片不再被拉伸到 100×100（盒外是页面底色）%s %s'
      % (px[20 + OX + 55, 20 + OY + 4], px[20 + OX + 20, 20 + OY + 20]))

ok4 = any(w_ == 344 and h_ == 172 for _, w_, h_ in rects)
OK &= ok4
print(('PASS ' if ok4 else 'FAIL ') + 'max-width:100%% 把 400×200 图夹到 344×172（保持原图比例，实际 %s）'
      % str([(w_, h_) for _, w_, h_ in rects if w_ > 40]))

ok5 = near(px[20 + OX + 40, 60 + OY + 30], RED) and near(px[20 + OX + 300, 60 + OY + 150], RED)
OK &= ok5
print(('PASS ' if ok5 else 'FAIL ') + '夹窄后图片仍铺满整个盒子 %s %s'
      % (px[20 + OX + 40, 60 + OY + 30], px[20 + OX + 300, 60 + OY + 150]))

ok6 = any(w_ == 240 and h_ == 120 for _, w_, h_ in rects)
OK &= ok6
print(('PASS ' if ok6 else 'FAIL ') + 'max-height:120 把 400×200 图夹成 240×120（按高缩宽，实际 %s）'
      % str([(w_, h_) for _, w_, h_ in rects if w_ >= 240]))

ok7 = near(px[20 + OX + 120, 250 + OY + 60], RED) and near(px[20 + OX + 260, 250 + OY + 60], PAGE)
OK &= ok7
print(('PASS ' if ok7 else 'FAIL ') + '夹低后图片宽只有 240（右侧露出页面底色）%s %s'
      % (px[20 + OX + 120, 250 + OY + 60], px[20 + OX + 260, 250 + OY + 60]))

print('t_img_natural: %s' % ('PASS=7 FAIL=0' if OK else 'FAIL'))
sys.exit(0 if OK else 1)
