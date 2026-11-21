import re, pathlib
root = pathlib.Path(r'G:\PyArx')
for proj in ['MfcXmlCore/MfcXmlCore.vcxproj', 'PyMfcXml/PyMfcXml.vcxproj']:
    p = root/proj
    t = p.read_text(encoding='utf-8', errors='replace')
    entries = re.findall(r'<Cl(?:Compile|Include)\s+Include="([^"]+)"', t)
    base = p.parent
    listed = {}
    for e in entries:
        rel = e.replace('\\', '/')
        full = (base/rel).resolve() if not rel.startswith('..') else (base/rel).resolve()
        listed[str(full).replace('\\','/').lower()] = e
    actual = [f for f in base.rglob('*') if f.suffix.lower() in ('.h','.cpp')]
    miss = [str(f).replace('\\','/') for f in actual if str(f.resolve()).replace('\\','/').lower() not in listed]
    stale = [e for e in entries if not (base/e.replace('\\','/')).exists()]
    print(f'== {proj}  entries={len(entries)} ondisk={len(actual)}')
    print('  not in project:')
    for m in miss: print('   -', m.replace(str(root).replace(chr(92),'/')+'/', ''))
    print('  project entries missing on disk:')
    for s in stale: print('   -', s)
