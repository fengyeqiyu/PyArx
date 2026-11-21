import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

XML = '''<window Width="420" Height="260" Background="#313842">
  <div style="left:120;top:60;width:160;height:100;background:#2E7D32;%s"/>
</window>'''
OX, OY = 8, 31
BOX_L, BOX_T = OX + 120, OY + 60
BOX_R, BOX_B = BOX_L + 160, BOX_T + 100
BLUE = (0x00, 0xAF, 0xFF)
OK = True

def shot(style, png):
    p = r'G:\PyArx\log\_t_outdiv.xml'
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
                l = min(l, x); r = max(r, x); t = min(t, y); b = max(b, y)
    return (l, t, r, b) if r >= 0 else None

def near(a, b, tol=2):
    return a is not None and all(abs(a[i] - b[i]) <= tol for i in range(4))

img = shot('outline:2px solid #0af;outline-offset:2px', r'G:\PyArx\log\_t_outdiv_top.png')
bb = bbox(img, BLUE)
want = (BOX_L - 4, BOX_T - 4, BOX_R + 4, BOX_B + 4)
ok = near(bb, want)
OK &= ok
print(('PASS ' if ok else 'FAIL ') + 'div outline bbox=%s want=%s' % (bb, want))

imgn = shot('outline:none', r'G:\PyArx\log\_t_outdiv_none.png')
bbn = bbox(imgn, BLUE)
ok2 = bbn is None
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + 'div outline:none 无描边 bbox=%s' % (bbn,))

print('t_outline_div: %s' % ('PASS=2 FAIL=0' if OK else 'FAIL'))
