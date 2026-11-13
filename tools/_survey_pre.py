import io, re
p = r'G:\PyArx\PyMfcXaml\parts\01-pre.inc'
lines = io.open(p, encoding='utf-8').read().split('\n')
print('01-pre 共 %d 行' % len(lines))
for i, l in enumerate(lines):
    s = l.strip()
    if re.match(r'(static|constexpr|const\s+wchar_t|std::|int|bool|void|HWND|LRESULT|std::map)', s) and not l.startswith(' ') and not s.startswith('//'):
        print('%5d | %s' % (i + 1, s[:100]))
