import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

XML = '''<window Width="420" Height="260" Background="#313842">
  <label Name="box" Text="" style="left:120;top:60;width:160;height:100;background:#2E7D32;%s"/>
</window>'''
OX, OY = 8, 31          # 整窗 PNG 含标题栏的固定偏移
BOX_L, BOX_T = OX + 120, OY + 60
BOX_R, BOX_B = BOX_L + 160, BOX_T + 100
BLUE = (0x00, 0xAF, 0xFF)
GREEN = (0x2E, 0x7D, 0x32)
OK = True

def shot(style, png):
    p = r'G:\PyArx\log\_t_outline.xml'
    open(p, 'w', encoding='utf-8').write(XML % style)
    win = M.guiload(p, context={}, names={})
    win.show(); M.pump(400); win.save_png(png); win.close()
    return Image.open(png).convert('RGB')

def bbox(img, rgb, tol=40):
    l = t = 10 ** 6; r = b = -1
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            p = px[x, y]
            if all(abs(p[i] - rgb[i]) <= tol for i in range(3)):
                if x < l: l = x
                if x > r: r = x
                if y < t: t = y
                if y > b: b = y
    return (l, t, r, b) if r >= 0 else None

def near(a, b, tol=2):
    return a is not None and all(abs(a[i] - b[i]) <= tol for i in range(4))

# 1) outline:2px + outline-offset:2px → 蓝框包围盒 = 盒外扩 (2+2)=4，线宽 2
img = shot('outline:2px solid #0af;outline-offset:2px', r'G:\PyArx\log\_t_out_top.png')
bb = bbox(img, BLUE)
want = (BOX_L - 4, BOX_T - 4, BOX_R + 4, BOX_B + 4)
ok1 = near(bb, want)
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + 'outline 画出盒外蓝框 bbox=%s want=%s' % (bb, want))

# 2) outline 不占盒模型：盒内仍是绿底
p = img.getpixel((BOX_L + 80, BOX_T + 50))
ok2 = all(abs(p[i] - GREEN[i]) <= 26 for i in range(3))
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + 'outline 不侵占盒内 got=%s' % (p,))

# 3) outline-offset:0 → 框贴盒边（bbox 比 offset2 各边内缩 2）
img0 = shot('outline:2px solid #0af', r'G:\PyArx\log\_t_out_off0.png')
bb0 = bbox(img0, BLUE)
want0 = (BOX_L - 2, BOX_T - 2, BOX_R + 2, BOX_B + 2)
ok3 = near(bb0, want0)
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + 'outline-offset:0 贴盒边 bbox=%s want=%s' % (bb0, want0))

# 4) outline:none 关闭
imgn = shot('outline:2px solid #0af;outline:none', r'G:\PyArx\log\_t_out_none.png')
bbn = bbox(imgn, BLUE)
ok4 = bbn is None
OK &= ok4
print(('PASS ' if ok4 else 'FAIL ') + 'outline:none 无描边 bbox=%s' % (bbn,))

print('t_outline: %s' % ('PASS=4 FAIL=0' if OK else 'FAIL'))
