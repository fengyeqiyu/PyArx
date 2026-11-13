import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
lines = io.open(P, encoding='utf-8-sig').read().split('\n')
for i, l in enumerate(lines):
    if i + 1 >= 444 and i + 1 <= 8600 and l.rstrip() in ('};', '}') and not l.startswith(' '):
        print('%6d | %s' % (i + 1, l.rstrip()))
