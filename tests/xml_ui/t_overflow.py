import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

XML = '''<window Width="420" Height="260" Background="#313842">
  <div style="left:120;top:60;width:120;height:80;background:#2E7D32%s">
    <label Text="" style="left:80;top:20;width:60;height:40;background:#FF9800"/>
  </div>
</window>'''
CLIP_R = 8 + 120 + 120      # 容器右边界在截图坐标里
ORANGE = (0xFF, 0x98, 0x00)
OK = True

def shot(extra, png):
    p = r'G:\PyArx\log\_t_ovf.xml'
    open(p, 'w', encoding='utf-8').write(XML % extra)
    w = M.guiload(p, context={}, names={})
    w.show(); M.pump(400); w.save_png(png); w.close()
    return Image.open(png).convert('RGB')

def bbox(img, rgb, tol=30):
    l = t = 10 ** 6; r = b = -1
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            q = px[x, y]
            if all(abs(q[i] - rgb[i]) <= tol for i in range(3)):
                l = min(l, x); r = max(r, x); t = min(t, y); b = max(b, y)
    return (l, t, r, b) if r >= 0 else None

vis = bbox(shot('', r'G:\PyArx\log\_t_ovf_vis.png'), ORANGE)
ok1 = vis is not None and vis[2] > CLIP_R            # 默认 visible：溢出到容器外
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + 'overflow 默认 visible 子项溢出可见 bbox=%s' % (vis,))

hid = bbox(shot(';overflow:hidden', r'G:\PyArx\log\_t_ovf_hid.png'), ORANGE)
ok2 = hid is not None and abs(hid[2] - (CLIP_R - 1)) <= 2 and hid[0] < CLIP_R
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + 'overflow:hidden 裁到容器右边界 bbox=%s 边界=%d' % (hid, CLIP_R))

aut = bbox(shot(';overflow:auto', r'G:\PyArx\log\_t_ovf_auto.png'), ORANGE)
ok3 = aut is not None and abs(aut[2] - (CLIP_R - 1)) <= 2
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + 'overflow:auto 同样裁剪 bbox=%s' % (aut,))

print('t_overflow: %s' % ('PASS=3 FAIL=0' if OK else 'FAIL'))
