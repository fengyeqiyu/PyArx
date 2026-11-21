import re, pathlib, collections, sys
root = pathlib.Path(r'G:\PyArx')
dirs = ['MfcXmlCore', 'PyMfcXml']
files = []
for d in dirs:
    for f in (root/d).rglob('*'):
        if f.suffix.lower() in ('.h','.cpp'):
            files.append(f)
text = {}
for f in files:
    text[f] = f.read_text(encoding='utf-8', errors='replace')
all_text = '\n'.join(text.values())
# also include other dirs' code referencing these
extra = []
for d in ['UIDesign','tests','examples','tools','PyArxCore','PyArxEngine','MfcXmlFrame']:
    p = root/d
    if p.exists():
        for f in p.rglob('*'):
            if f.suffix.lower() in ('.h','.cpp','.py','.pyi'):
                extra.append((f, f.read_text(encoding='utf-8', errors='replace')))
print('--- unreferenced by include/import (name-based)')
for f in sorted(files):
    name = f.name
    if name in ('stdafx.h',): continue
    refs = 0
    for g, t in text.items():
        if g == f: continue
        if re.search(r'#\s*include\s*[<"]' + re.escape(name) + r'[>"]', t):
            refs += 1
    ex = 0
    for g, t in extra:
        if name in t: ex += 1
    if refs == 0:
        print(f'  {str(f.relative_to(root)):55s} refs=0 ext_mentions={ex}')
