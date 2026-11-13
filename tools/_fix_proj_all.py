import io

p = 'G:/PyArx/PyMfcXaml/PyMfcXaml.vcxproj'
x = io.open(p, encoding='utf-8-sig').read()

for name in ('PyMfcXaml.cpp', 'PyMfcWrap.cpp', 'PyMfcLoad.cpp', 'PyMfcBind.cpp'):
    x = x.replace('    <ClCompile Include="%s" />\r\n' % name, '')
    x = x.replace('    <ClCompile Include="%s" />\n' % name, '')

x = x.replace('    <ClCompile Include="PyMfcProcs.cpp" />',
              '    <ClCompile Include="PyMfcProcs.cpp" />\r\n'
              '    <ClCompile Include="PyMfcWrap.cpp" />\r\n'
              '    <ClCompile Include="PyMfcLoad.cpp" />\r\n'
              '    <ClCompile Include="PyMfcBind.cpp" />')

for name in ('PyMfcWrap.h', 'PyMfcParts.h'):
    if name not in x:
        x = x.replace('    <ClInclude Include="PyMfcHost.h" />',
                      '    <ClInclude Include="PyMfcHost.h" />\r\n'
                      '    <ClInclude Include="%s" />' % name)

io.open(p, 'w', encoding='utf-8-sig', newline='').write(x)

import re
print('ClCompile:')
for m in re.findall(r'<ClCompile Include="([^"]+)"', x):
    print('  ', m)
print('ClInclude:')
for m in re.findall(r'<ClInclude Include="([^"]+)"', x):
    print('  ', m)
