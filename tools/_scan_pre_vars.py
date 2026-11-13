import io, re
p = r'G:\PyArx\PyMfcXaml\parts\01-pre.inc'
lines = io.open(p, encoding='utf-8').read().split('\n')
print('文件级定义（col0，非 #/注释/函数）：')
for i, l in enumerate(lines):
    if l and not l[0].isspace() and not l.startswith(('#','//','/*','*','}')):
        s = l.rstrip()
        if s.endswith(';') and '(' not in s.split('=')[0]:
            print('%5d | %s' % (i + 1, s[:110]))
