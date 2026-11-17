import io
import re

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read().split('\n')

# XLabel 的皮肤自带浅色底，浅主题下看不出来，深色主题下就成了一条条白条。
# 逐行给每个 XLabel 补上它所在区域的底色。
section = 'panel'
BG = {'tab': '#FF202327', 'cmd': '#FF202327', 'status': '#FF2C3034', 'panel': '#FF2C3034'}
out = []
for line in s:
    if 'Name="tabRow"' in line:
        section = 'tab'
    elif 'Name="cmdRow"' in line:
        section = 'cmd'
    elif 'Name="statusRow"' in line:
        section = 'status'
    elif 'Name="ribbon"' in line or 'Name="mainRow"' in line or 'Name="props"' in line \
            or 'Name="tools"' in line:
        section = 'panel'
    if '<XLabel' in line and 'Background=' not in line:
        line = re.sub(r'(<XLabel\s+Name="[^"]+"\s+Text="[^"]*")',
                      r'\1 Background="%s"' % BG[section], line, count=1)
        if 'Background=' not in line:                      # Text 属性不紧邻 Name 的写法
            line = re.sub(r'<XLabel\s', '<XLabel Background="%s" ' % BG[section], line, count=1)
    out.append(line)

io.open(p, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
print('labels patched:', sum(1 for l in out if '<XLabel' in l and 'Background=' in l))
