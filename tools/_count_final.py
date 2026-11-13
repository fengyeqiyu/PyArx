import io, os
d = r'G:\PyArx\PyMfcXaml'
for f in ('PyMfcXaml.cpp','PyMfcProcs.cpp','PyMfcOffscreen.cpp','PyMfcCommon.h','PyMfcHost.h','XamlBindings.cpp'):
    p = os.path.join(d, f)
    n = len(io.open(p, encoding='utf-8-sig').read().split('\n'))
    print('%-20s %5d 行' % (f, n))
print()
for f in sorted(os.listdir(os.path.join(d, 'parts'))):
    if f.endswith('.inc'):
        p = os.path.join(d, 'parts', f)
        print('parts/%-22s %5d 行' % (f, len(io.open(p, encoding='utf-8-sig').read().split('\n'))))
