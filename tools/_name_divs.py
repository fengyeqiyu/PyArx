import io
import re

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read()
cnt = [0]


def rep(m):
    cnt[0] += 1
    return '<XDivider Name="div%d"' % cnt[0]


s = re.sub(r'<XDivider(?!\s+Name=)', rep, s)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('dividers named:', cnt[0])
