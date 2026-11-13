import io
p = r'G:\PyArx\PyMfcXaml\parts\03-procs.inc'
lines = io.open(p, encoding='utf-8').read().split('\n')
for n in (1793, 1794, 1795, 1796, 2119, 2120, 2121, 2122):
    print('%6d |%s' % (n, lines[n - 1]))
print('总行 %d' % len(lines))
