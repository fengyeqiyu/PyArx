import re, sys, collections, json
p = r'G:\PyArx\PyMfcXml\PyMfcBind.cpp'
src = open(p, encoding='utf-8', errors='replace').read()
recv = collections.Counter(); names = collections.defaultdict(list)
for m in re.finditer(r'([A-Za-z_][\w]*)\s*\.\s*(def|def_property|def_property_readonly|def_static)\s*\(\s*"([^"]+)"', src):
    r, k, n = m.group(1), m.group(2), m.group(3)
    recv[(r,k)] += 1
    names[r].append(n)
for k,v in recv.most_common():
    print(k, v)
print('---- receivers:', sorted(names.keys()))
for r in sorted(names):
    print(f'## {r} ({len(names[r])})')
    print('   ' + ', '.join(names[r]))
