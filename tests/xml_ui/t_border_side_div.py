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

def shot(style, png):
    p = r'G:\PyArx\log\_t_bsdiv.xml'
    open(p, 'w', encoding='utf-8').write(XML % style)
    win = M.guiload(p, context={}, names={})
    win.show(); M.pump(400); win.save_png(png); win.close()
    return Image.open(png).convert('RGB')

BOX_BG = (0x2E, 0x7D, 0x32); BORDER = (0x33, 0x33, 0x33); TOL = 26

def near(px, rgb):
    return all(abs(px[i] - rgb[i]) <= TOL for i in range(3))

cases = [('border-bottom:2px solid #333', (200, 189)),
         ('border-top:2px solid #333', (200, 92)),
         ('border-left:2px solid #333', (129, 140)),
         ('border-bottom:0;border:2px solid #333', (200, 189))]
ok = True
for style, pt in cases:
    img = shot(style, r'G:\PyArx\log\_t_bsdiv.png')
    px = img.getpixel(pt)
    expect = BOX_BG if ('border-bottom:0' in style) else BORDER
    good = near(px, expect)
    ok = ok and good
    print(('PASS ' if good else 'FAIL ') + style + '  got=%s' % (px,))
print('tmp_bs_div: %s' % ('OK' if ok else 'BAD'))
