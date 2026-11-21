import re, pathlib, collections, json
p = pathlib.Path(r'G:\PyArx\PyMfcXml\PyMfcBind.cpp')
src = p.read_text(encoding='utf-8')
names = re.findall(r'\.def(?:_property(?:_readonly)?|_static)?\s*\(\s*"([^"]+)"', src)
cnt = collections.Counter(names)
print('defs', len(names), 'unique', len(cnt))
dups = [(n, c) for n, c in cnt.items() if c > 1]
print('duplicates:', dups)
def snake(n):
    s = re.sub(r'(?<=[a-z0-9])([A-Z])', r'_\1', n)
    s = re.sub(r'(?<=[A-Z])([A-Z][a-z])', r'_\1', s)
    return s.lower()
mapping = {n: snake(n) for n in cnt}
manual = {
 'view3DLoad':'view3d_load','view3DAppendBox':'view3d_append_box','view3DAppendLine':'view3d_append_line',
 'view3DSetCamera':'view3d_set_camera','view3DGetCamera':'view3d_get_camera','view3DFit':'view3d_fit',
 'view3DSetStyle':'view3d_set_style','addView3D':'add_view3d',
 'easingValue':'easing_value',
}
mapping.update({k: v for k, v in manual.items() if k in mapping})
print(json.dumps(mapping, ensure_ascii=False, indent=0)[:1200])
json.dump(mapping, open(r'G:\PyArx\log\bind_rename_map.json', 'w'), ensure_ascii=False, indent=1)
# consumer scan
root = pathlib.Path(r'G:\PyArx')
use = collections.Counter()
for f in root.rglob('*.py'):
    if any(x in f.parts for x in ('log','.git')): continue
    t = f.read_text(encoding='utf-8', errors='replace')
    for n in cnt:
        m = len(re.findall(r'\.' + re.escape(n) + r'\b', t))
        if m: use[n] += m
print('names used in repo py:', len(use), 'total call sites', sum(use.values()))
print('top:', use.most_common(12))
