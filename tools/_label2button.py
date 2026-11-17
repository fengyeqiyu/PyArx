import io
import re

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read()

# XLabel 的底色/字色写死在宿主皮肤里（属性表只有 Text），深色主题下是一堆白条。
# 换成 XButton：同样是自绘，但底色/字色是深色主题，文字可读，视觉也统一。
# 这些按钮不绑 onClick，纯展示。
s = re.sub(r'\s*Background="#FF[0-9A-F]{6}"(?=[^>]*Height)', '', s)
s = s.replace('<XLabel', '<XButton').replace('</XLabel>', '</XButton>')

io.open(p, 'w', encoding='utf-8', newline='\n').write(s)
print('XLabel left:', s.count('<XLabel'), 'XButton:', s.count('<XButton'))
