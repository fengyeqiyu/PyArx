import io
src = r'G:\PyArx\docs\_backup_PyMfcHost.h'
lines = io.open(src, encoding='utf-8').read().split('\n')
for i, l in enumerate(lines):
    if 'PyMfcTabBodyProc' in l:
        print('L%d: %s' % (i + 1, l))
print('---- 附近 ----')
idx = next(i for i, l in enumerate(lines) if 'static LRESULT CALLBACK PyMfcTabBodyProc' in l and l.strip().endswith(')'))
for j in range(idx, idx + 34):
    print('%4d |%s' % (j + 1, lines[j]))
