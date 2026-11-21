import re, pathlib, collections
root = pathlib.Path(r'G:\PyArx')
skip_dirs = {'.git','X64','log','docs','docs_notes','build'}
files = []
for f in root.rglob('*'):
    if not f.is_file(): continue
    if any(part in skip_dirs for part in f.parts): continue
    if f.suffix.lower() in ('.h','.cpp','.hpp','.cxx','.py','.md','.xml','.vcxproj','.filters','.inc'):
        files.append(f)
texts = {}
for f in files:
    try:
        texts[f] = f.read_text(encoding='utf-8', errors='replace')
    except Exception:
        texts[f] = ''
inc = collections.defaultdict(set)
for f, t in texts.items():
    for name in re.findall(r'#\s*include\s*[<"]([^>"]*)[>"]', t):
        inc[pathlib.PurePath(name.replace('\\','/')).name.lower()].add(f)
print(f'scanned={len(files)}')
for proj in ['MfcXmlCore','PyMfcXml']:
    print(f'--- {proj}: headers with no includer')
    for f in sorted((root/proj).rglob('*.h')):
        refs = {x for x in inc.get(f.name.lower(), ()) if x != f}
        if refs: continue
        mentions = [x for x, t in texts.items() if x != f and not str(x).startswith(str(root/proj)) and f.name in t]
        print(f'  {str(f.relative_to(root)):52s} mentions_outside={len(mentions)}' + ('  e.g. '+str(mentions[0].relative_to(root)) if mentions else ''))
