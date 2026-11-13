import io, os, re
d = r'G:\PyArx\PyMfcXaml'
# 删掉已并入 PyMfcHost.h / PyMfcProcs.cpp 的分片
parts = os.path.join(d, 'parts')
for f in ('02a-host-core.inc','02b-host-widgets.inc','02c-host-fill.inc','02d-host-export.inc',
          '02e-host-canvas.inc','02f-host-anim.inc','02g-host-props.inc',
          '03a-procs-skin.inc','03b-procs-canvas.inc'):
    p = os.path.join(parts, f)
    if os.path.exists(p): os.remove(p)
# vcxproj 增加 PyMfcProcs.cpp 与 PyMfcHost.h
pj = os.path.join(d, 'PyMfcXaml.vcxproj')
x = io.open(pj, encoding='utf-8-sig').read()
if 'PyMfcProcs.cpp' not in x:
    x = x.replace('<ClCompile Include="PyMfcOffscreen.cpp" />',
                  '<ClCompile Include="PyMfcOffscreen.cpp" />\r\n    <ClCompile Include="PyMfcProcs.cpp" />')
if 'PyMfcHost.h' not in x:
    x = x.replace('<ClInclude Include="PyMfcCommon.h" />',
                  '<ClInclude Include="PyMfcCommon.h" />\r\n    <ClInclude Include="PyMfcHost.h" />')
io.open(pj, 'w', encoding='utf-8-sig', newline='').write(x)
print('剩余分片：', sorted(os.listdir(parts)))
