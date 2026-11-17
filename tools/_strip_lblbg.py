import io
import re

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read()
# XLabel 上的 Background 会生成一个装饰矩形节点，在表面层和控件层各画一次，
# 两层坐标不同 → 左边缘留下 8px 宽的竖排残字。标签背景交给容器底色出。
out = []
for line in s.split('\n'):
    if '<XLabel' in line and 'Background=' in line:
        line = re.sub(r'\s*Background="[^"]*"', '', line)
    out.append(line)
s2 = '\n'.join(out)
io.open(p, 'w', encoding='utf-8', newline='\n').write(s2)
print('removed', s.count('Background="#FFE9ECF0"') - s2.count('Background="#FFE9ECF0"'))
