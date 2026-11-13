import io
p = r'G:\PyArx\PyMfcXaml\PyMfcXaml.vcxproj'
x = io.open(p, encoding='utf-8-sig').read()
if 'PyMfcHost.cpp' not in x:
    x = x.replace('<ClCompile Include="PyMfcProcs.cpp" />',
                  '<ClCompile Include="PyMfcProcs.cpp" />\r\n    <ClCompile Include="PyMfcHost.cpp" />')
io.open(p, 'w', encoding='utf-8-sig', newline='').write(x)
print('vcxproj 已加 PyMfcHost.cpp')
