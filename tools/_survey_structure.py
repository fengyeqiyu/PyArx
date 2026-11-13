import io, re
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
lines = io.open(P, encoding='utf-8-sig').read().split('\n')
print('总行数 %d' % len(lines))
pat = re.compile(r'^(class|struct|namespace|PYBIND11_MODULE|LRESULT|static LRESULT|void |int |bool |py::|HWND |std::|const )')
for i, l in enumerate(lines):
    if re.match(r'^(class|struct)\s+\w+', l) or re.match(r'^namespace', l) or re.match(r'^PYBIND11_MODULE', l) or re.match(r'^static (LRESULT|ATOM|bool|void|int)?\s*CALLBACK', l):
        print('%6d | %s' % (i + 1, l.strip()[:110]))
