import io, re
for f in ('02-host.inc', '03-procs.inc'):
    p = r'G:\PyArx\PyMfcXaml\parts\%s' % f
    lines = io.open(p, encoding='utf-8').read().split('\n')
    print('===== %s  %d 行 =====' % (f, len(lines)))
    for i, l in enumerate(lines):
        s = l.strip()
        if s.startswith('// ---') or (s.startswith('//') and ('====' in s or '----' in s)):
            nxt = lines[i + 1].strip() if i + 1 < len(lines) else ''
            if nxt.startswith('//'):
                print('%6d  %s' % (i + 1, nxt[:78]))
            elif s.count('-') > 20:
                continue
            else:
                print('%6d  %s' % (i + 1, s[:78]))
