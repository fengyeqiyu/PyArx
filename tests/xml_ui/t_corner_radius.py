import os, sys
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M
from PIL import Image

XML = '''<window Width="420" Height="200" Background="#313842">
  <div style="left:20;top:20;width:100;height:100;background:#FF9800;border:1px solid #FF9800;border-top-left-radius:40"/>
  <div style="left:180;top:20;width:100;height:100;background:#4CAF50;border:1px solid #4CAF50;border-radius:20"/>
  <div style="left:310;top:100;width:80;height:80;background:#7B1FA2;border-radius:30"/>
</window>'''
OX, OY = 8, 31          # 截图固定偏移（含标题栏）
ORANGE = (0xFF, 0x98, 0x00)
GREEN = (0x4C, 0xAF, 0x50)
BG = (49, 56, 66)


def near(px, rgb, tol=26):
    return all(abs(px[i] - rgb[i]) <= tol for i in range(3))


p = r'G:\PyArx\log\_t_cr.xml'
open(p, 'w', encoding='utf-8').write(XML)
w = M.guiload(p, context={}, names={})
w.show()
M.pump(400)
w.save_png(r'G:\PyArx\log\_t_cr.png')
w.close()
img = Image.open(r'G:\PyArx\log\_t_cr.png').convert('RGB')
px = img.load()
OK = True

a = (20 + OX, 20 + OY)          # 第一个盒子左上角（分角 40）
def at(dx, dy, base=(20, 20)):
    return px[base[0] + OX + dx, base[1] + OY + dy]

ok1 = near(at(2, 2), BG) and near(at(3, 3), BG)
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + '左上角 40px 圆角：角内(2,2)/(3,3) 是背景 %s' % (at(3, 3),))

ok2 = near(at(98, 2), ORANGE) and near(at(2, 98), ORANGE) and near(at(98, 98), ORANGE)
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + '未声明的三个角保持直角 %s %s %s' % (at(98, 2), at(2, 98), at(98, 98)))

ok3 = near(at(50, 50), ORANGE)
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + '盒心仍是填充色 %s' % (at(50, 50),))

# 对照：统一 border-radius:20 的两个上角都该是圆角
ok4 = near(at(2, 2, (180, 20)), BG) and near(at(98, 2, (180, 20)), BG)
OK &= ok4
print(('PASS ' if ok4 else 'FAIL ') + '对照 border-radius:20 左上/右上均圆 %s %s' % (at(2, 2, (180, 20)), at(98, 2, (180, 20))))

ok5 = near(at(12, 88, (180, 20)), GREEN) and near(at(50, 50, (180, 20)), GREEN)
OK &= ok5
print(('PASS ' if ok5 else 'FAIL ') + '对照盒内近角/盒心均为填充 %s %s' % (at(12, 88, (180, 20)), at(50, 50, (180, 20))))

ok6 = near(at(2, 2, (310, 100)), BG) and near(at(40, 40, (310, 100)), (0x7B, 0x1F, 0xA2))
OK &= ok6
print(('PASS ' if ok6 else 'FAIL ') + '无边框 div border-radius:30 同样切角 %s %s' % (at(2, 2, (310, 100)), at(40, 40, (310, 100))))

print('t_corner_radius: %s' % ('PASS=6 FAIL=0' if OK else 'FAIL'))
