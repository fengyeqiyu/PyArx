import io, os
d = r'G:\PyArx\PyMfcXaml\parts'
total = 0
for f in sorted(os.listdir(d)):
    p = os.path.join(d, f)
    if not f.endswith('.inc'):
        print('非 inc 残留：', f)
        continue
    n = len(io.open(p, encoding='utf-8').read().split('\n'))
    total += n
    print('%-24s %5d 行' % (f, n))
print('合计 %d 行' % total)
