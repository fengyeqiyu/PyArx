# -*- coding: utf-8 -*-
"""把 controls 下拆出来的单控件 .h/.cpp 并入 MfcXamlCore.vcxproj，并生成 .vcxproj.filters，
让所有控件文件都落在 IDE 的 "controls" 筛选器下。
"""
import os
import uuid

PROJ = r'G:\PyArx\MfcXamlCore\MfcXamlCore.vcxproj'
FILTERS = PROJ + '.filters'
CTRL = r'G:\PyArx\MfcXamlCore\controls'


def read(p):
    with open(p, 'r', encoding='utf-8-sig') as f:
        return f.read()


def write(p, t):
    with open(p, 'w', encoding='utf-8-sig', newline='\r\n') as f:
        f.write(t)


def insert_before_group_end(lines, group_open_marker, new_lines):
    """在第一个包含 group_open_marker 的 ItemGroup 的 </ItemGroup> 之前插入。"""
    for i, ln in enumerate(lines):
        if group_open_marker in ln:
            j = i + 1
            while j < len(lines) and '</ItemGroup>' not in lines[j]:
                j += 1
            lines[j:j] = new_lines
            return True
    return False


cpp = sorted(f for f in os.listdir(CTRL) if f.endswith('.cpp'))
hdr = sorted(f for f in os.listdir(CTRL) if f.endswith('.h'))
print('controls: %d 个 .cpp, %d 个 .h' % (len(cpp), len(hdr)))

raw = read(PROJ)
lines = [ln for ln in raw.split('\n')]
nl = '\r' if '\r\n' in raw else ''
if nl == '\r':
    lines = [ln[:-1] if ln.endswith('\r') else ln for ln in raw.split('\n')]

new_cpp = ['    <ClCompile Include="controls\\%s" />' % f
           for f in cpp if 'controls\\%s"' % f not in raw]
if new_cpp:
    ok = insert_before_group_end(lines, '<ClCompile', new_cpp)
    print('ClCompile 新增 %d（插入 %s）' % (len(new_cpp), ok))

new_h = ['    <ClInclude Include="controls\\%s" />' % f
         for f in hdr if 'controls\\%s"' % f not in raw]
if new_h:
    ok = insert_before_group_end(lines, '<ClInclude', new_h)
    print('ClInclude 新增 %d（插入 %s）' % (len(new_h), ok))

out = '\n'.join(lines)
if nl == '\r':
    out = out.replace('\n', '\r\n')
write(PROJ, out)

# ---- filters ----
gid = '{' + str(uuid.uuid5(uuid.NAMESPACE_URL, 'pyarx.controls')).upper() + '}'
parts = ['<?xml version="1.0" encoding="utf-8"?>',
         '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">',
         '  <ItemGroup>',
         '    <Filter Include="controls">',
         '      <UniqueIdentifier>%s</UniqueIdentifier>' % gid,
         '    </Filter>',
         '  </ItemGroup>',
         '  <ItemGroup>']
for f in cpp:
    parts += ['    <ClCompile Include="controls\\%s">' % f,
              '      <Filter>controls</Filter>',
              '    </ClCompile>']
parts += ['  </ItemGroup>', '  <ItemGroup>']
for f in hdr:
    parts += ['    <ClInclude Include="controls\\%s">' % f,
              '      <Filter>controls</Filter>',
              '    </ClInclude>']
parts += ['  </ItemGroup>', '</Project>']
write(FILTERS, '\n'.join(parts) + '\n')
print('filters: %s' % FILTERS)
