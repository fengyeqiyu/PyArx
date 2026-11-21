import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image, ImageChops

XML = '''<window Width="420" Height="260" Background="#313842">
  <label Text="Shadow" style="left:120;top:60;width:200;height:60;font-size:28;color:#ffffff;%s"/>
</window>'''
OK = True

def shot(extra, png):
    p = r'G:\PyArx\log\_t_tsh.xml'
    open(p, 'w', encoding='utf-8').write(XML % extra)
    w = M.guiload(p, context={}, names={})
    w.show(); M.pump(400); w.save_png(png); w.close()
    return Image.open(png).convert('RGB')

def diff_count(a, b, tol=24):
    d = ImageChops.difference(a, b).convert('L')
    return sum(1 for v in d.getdata() if v > tol)

plain = shot('', r'G:\PyArx\log\_t_tsh_plain.png')
on = shot('text-shadow:2px 2px 0 #101010', r'G:\PyArx\log\_t_tsh_on.png')
none = shot('text-shadow:none', r'G:\PyArx\log\_t_tsh_none.png')

c1 = diff_count(plain, on)
ok1 = c1 > 100
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + 'text-shadow 改变渲染 diff=%d' % c1)

c2 = diff_count(plain, none)
ok2 = c2 == 0
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + 'text-shadow:none 与无声明一致 diff=%d' % c2)

print('t_text_shadow: %s' % ('PASS=2 FAIL=0' if OK else 'FAIL'))
