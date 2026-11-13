import io, re, os
d = r'G:\PyArx\PyMfcXaml'
src = os.path.join(d, 'PyMfcHost.h')
lines = io.open(src, encoding='utf-8').read().split('\n')
print('PyMfcHost.h 共 %d 行' % len(lines))
# 找类和类体
ci = next(i for i, l in enumerate(lines) if l.startswith('class PyMfcHost'))
print('class 行 %d: %s' % (ci + 1, lines[ci]))
# 类体结束
depth = 0
started = False
end = None
for i in range(ci, len(lines)):
    depth += lines[i].count('{') - lines[i].count('}')
    if '{' in lines[i]: started = True
    if started and depth == 0:
        end = i
        break
print('类体结束 %d: %s' % (end + 1, lines[end]))
print('类体行数 %d' % (end - ci))
