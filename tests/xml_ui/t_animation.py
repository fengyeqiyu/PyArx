import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

XML = '''<window Width="320" Height="160" Background="#313842">
  <label Text="" style="left:20;top:20;width:120;height:100;background:#FF0000"
         Keyframes="0:#FF0000;50:#00FF00;100:#FF0000"
         Animation="pulse 900ms linear infinite"/>
</window>'''
PX, PY = 8 + 20 + 60, 31 + 20 + 50      # 盒子中心（截图坐标）

p = r'G:\PyArx\log\_t_anim.xml'
open(p, 'w', encoding='utf-8').write(XML)
w = M.guiload(p, context={}, names={})
w.show()
M.pump(200)

samples = []
for i in range(8):
    png = r'G:\PyArx\log\_t_anim_%d.png' % i
    w.save_png(png)
    samples.append(Image.open(png).convert('RGB').load()[PX, PY])
    M.pump(300)
w.close()

OK = True
ok1 = len({s for s in samples}) >= 3
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + '关键帧动画在跑：八次采样 %s' % (samples,))

red_first = samples[0][0]
deltas = [abs(s[1] - samples[0][1]) for s in samples]
ok2 = max(deltas) >= 40
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + '绿色分量随时间变化（红→绿→红）deltas=%s' % (deltas,))

# 颜色必须落在 红(255,0,0) 与 绿(0,255,0) 的连线上（R 与 G 此消彼长）
ok3 = all(s[2] < 60 and abs(s[0] + s[1] - 255) <= 40 for s in samples)
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + '插值色在红绿连线上 %s' % (samples,))

g = [s[1] for s in samples]
fallback = any(g[i] > g[i + 1] + 20 for i in range(len(g) - 1))
OK &= fallback
print(('PASS ' if fallback else 'FAIL ') + 'infinite 循环回到红帧（绿分量先升后降）%s' % (g,))

print('t_animation: %s' % ('PASS=4 FAIL=0' if OK else 'FAIL'))
