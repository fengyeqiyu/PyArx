import json, re, pathlib, sys

ROOT = pathlib.Path(r'G:\PyArx')
mapping = json.load(open(ROOT/'log/bind_rename_map.json', encoding='utf-8'))
changed = {k: v for k, v in mapping.items() if k != v}
print('renames', len(changed))

bind = ROOT/'PyMfcXml/PyMfcBind.cpp'
src = bind.read_text(encoding='utf-8')
count = 0
for old, new in sorted(changed.items(), key=lambda t: -len(t[0])):
    pat = re.compile(r'(\.def(?:_property(?:_readonly)?|_static)?\s*\(\s*")' + re.escape(old) + r'(")')
    src, n = pat.subn(lambda m: m.group(1) + new + m.group(2), src)
    count += n
bind.write_text(src, encoding='utf-8')
print('bind def renamed', count)

skip_dirs = {'log', '.git', 'docs', 'docs_notes', 'X64', 'build', '.vs'}
py_changes = 0
for f in ROOT.rglob('*.py'):
    if any(p in skip_dirs for p in f.parts):
        continue
    t = f.read_text(encoding='utf-8', errors='replace')
    orig = t
    for old, new in changed.items():
        t = re.sub(r'(?<![\w])' + re.escape(old) + r'(?=\s*\()', new, t) if False else t
        t = re.sub(r'\.' + re.escape(old) + r'\b', '.' + new, t)
    if t != orig:
        f.write_text(t, encoding='utf-8')
        py_changes += 1
print('py files changed', py_changes)
