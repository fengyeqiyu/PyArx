import pathlib, re

BAK = pathlib.Path(r'G:\PyArx\log\XmlCore.cpp.bak')
OUT = pathlib.Path(r'G:\PyArx\MfcXmlCore')
lines = BAK.read_text(encoding='utf-8-sig').splitlines(keepends=True)

parts = [
    ('Element', 22, 114, None),
    ('Engine', 115, 3494, 'xmlcore_impl'),
    ('Api', 3495, 3685, None),
    ('Html', 3686, 5917, None),
    ('Registry', 5918, 5966, None),
    ('Runtime', 5967, 6339, None),
    ('Style', 6340, 6750, None),
    ('Expr', 6751, 7435, None),
    ('Schema', 7436, len(lines), None),
]

common = ''.join(lines[:21]).rstrip('\n') + '\n'
(OUT / 'XmlCoreCommon.h').write_text(
    '// XmlCoreCommon.h —— XmlCore 系列 TU 共用的前置（原 XmlCore.cpp 1-21 行）。\n#pragma once\n\n' + common,
    encoding='utf-8-sig')

for name, a, b, ns in parts:
    body = ''.join(lines[a-1:b])
    if ns:
        body = re.sub(r'^namespace\s*\n\{\s*\n', f'namespace {ns}\n{{\n', body, count=1, flags=re.M)
        if f'namespace {ns}' not in body:
            raise SystemExit(f'{name}: namespace rename failed')
    text = (f'// XmlCore.{name}.cpp —— 拆自 XmlCore.cpp 第 {a}-{b} 行。\n'
            f'#include "XmlCoreCommon.h"\n#include "XmlCoreInternal.h"\n\n' + body.rstrip('\n') + '\n')
    (OUT / f'XmlCore.{name}.cpp').write_text(text, encoding='utf-8-sig')
    print(f'{name:9s} {text.count(chr(10)):5d} lines')

idx = pathlib.Path(OUT / 'XmlCoreInternal.h')
if not idx.exists():
    idx.write_text('// XmlCoreInternal.h —— 跨 TU 内部声明（xmlcore_impl 里被其它节用到的符号）。\n#pragma once\n'
                   '#include "XmlCoreCommon.h"\n\nnamespace xmlcore_impl\n{\n}\n', encoding='utf-8-sig')

(OUT / 'XmlCore.cpp').write_text(
    '// XmlCore.cpp —— 实现已按节拆到 XmlCore.<节>.cpp（9 个 TU）。\n'
    '// 公共前置：XmlCoreCommon.h；跨 TU 内部声明：XmlCoreInternal.h。\n', encoding='utf-8-sig')

p = OUT / 'MfcXmlCore.vcxproj'
t = p.read_text(encoding='utf-8-sig')
old = '    <ClCompile Include="XmlCore.cpp" />'
new = '\n'.join(f'    <ClCompile Include="XmlCore.{n}.cpp" />' for n, _, _, _ in parts)
t = t.replace(old, new)
p.write_text(t, encoding='utf-8-sig')
print('vcxproj updated:', old not in t)
