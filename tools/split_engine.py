import re, pathlib

ROOT = pathlib.Path(r'G:\PyArx\MfcXmlCore')
src = (ROOT / 'XmlCore.Engine.cpp').read_text(encoding='utf-8-sig')
lines = src.splitlines(keepends=True)

head = []
body_start = 0
for i, ln in enumerate(lines):
    if ln.startswith('namespace xmlcore_impl'):
        head = lines[:i]
        body_start = i + 2   # skip namespace line and '{'
        break
body = lines[body_start:]
# drop trailing closing brace of the namespace (last non-empty line '}')
for k in range(len(body) - 1, -1, -1):
    if body[k].strip():
        if body[k].strip() == '}':
            body = body[:k]
        break

cuts = []
for i, ln in enumerate(body):
    s = ln.strip()
    if s.startswith('// 文本基线') or s.startswith('// CSS flex 布局：HTML 的默认排版方式'):
        cuts.append(i)
print('cut lines', cuts, [body[c].strip() for c in cuts])
if len(cuts) != 2:
    raise SystemExit('need two cut points')

seg = [(0, cuts[0]), (cuts[0], cuts[1]), (cuts[1], len(body))]
names = ['Util', 'Text', 'Layout']
for (a, b), nm in zip(seg, names):
    text = ''.join(head) + 'namespace xmlcore_impl\n{\n' + ''.join(body[a:b]).rstrip('\n') + '\n}\n'
    (ROOT / f'XmlCore.{nm}.cpp').write_text(text, encoding='utf-8-sig')
    print(nm, text.count('\n'))

# remove old Engine TU, register new ones
(ROOT / 'XmlCore.Engine.cpp').unlink()
p = ROOT / 'MfcXmlCore.vcxproj'
t = p.read_text(encoding='utf-8-sig')
t = t.replace('    <ClCompile Include="XmlCore.Engine.cpp" />',
              '    <ClCompile Include="XmlCore.Util.cpp" />\n'
              '    <ClCompile Include="XmlCore.Text.cpp" />\n'
              '    <ClCompile Include="XmlCore.Layout.cpp" />')
p.write_text(t, encoding='utf-8-sig')
print('vcxproj updated')
