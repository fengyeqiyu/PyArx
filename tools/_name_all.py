import io
import re

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read()
c = [0]

# 给所有缺 Name 的控件标签补一个唯一名（guiload 要求每个控件都有 x:Name/Name）
pat = re.compile(r'<(X[A-Za-z]+)(?![^>]*\bName=)(\s|/)')


def rep(m):
    c[0] += 1
    return '<%s Name="auto%d"%s' % (m.group(1), c[0], m.group(2))


s = pat.sub(rep, s)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('auto-named:', c[0])
