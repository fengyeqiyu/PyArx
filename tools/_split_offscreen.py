import io, os
d = r'G:\PyArx\PyMfcXaml'
parts = os.path.join(d, 'parts')

# 1) PyMfcCommon.h = 01-pre，去掉分片标题行，加 #pragma once，wicWritePng 声明改外部链接
pre = io.open(os.path.join(parts, '01-pre.inc'), encoding='utf-8').read().split('\n')
body = pre[1:]
body = [('bool wicWritePng(const std::wstring& path, int w, int h, void* bgra);'
         if l.strip().startswith('static bool wicWritePng(const std::wstring& path, int w, int h, void* bgra);')
         else l) for l in body]
hdr = ['// PyMfcCommon.h —— 公共头/宏/辅助函数/前向声明（PyMfcXaml 各编译单元共享）',
       '#pragma once', ''] + body
io.open(os.path.join(d, 'PyMfcCommon.h'), 'w', encoding='utf-8', newline='').write('\n'.join(hdr))

# 2) PyMfcOffscreen.cpp = 03c
off = io.open(os.path.join(parts, '03c-procs-off.inc'), encoding='utf-8').read().split('\n')
off = off[1:]
off = [l for l in off if not l.strip().startswith('static bool wicWritePng(const std::wstring& path, int w, int h, void* bgra);')]
off = [('bool wicWritePng(const std::wstring& path, int w, int h, void* bgra)' if l.startswith('static bool wicWritePng(') else l) for l in off]
txt = ['// PyMfcOffscreen.cpp —— 离屏渲染：BGRA→PNG(WIC) / D2D 配电图 / nanosvg SVG。',
       '// 独立编译单元：不依赖 PyMfcHost，只依赖 PyMfcCommon.h 与 MfcXamlCore。',
       '', '#include "PyMfcCommon.h"', ''] + off
io.open(os.path.join(d, 'PyMfcOffscreen.cpp'), 'w', encoding='utf-8-sig', newline='').write('\n'.join(txt))
os.remove(os.path.join(parts, '03c-procs-off.inc'))

# 3) 主装配文件：换 include
p = os.path.join(d, 'PyMfcXaml.cpp')
s = io.open(p, encoding='utf-8-sig').read()
s = s.replace('#include "parts\\\\01-pre.inc"', '#include "PyMfcCommon.h"')
s = s.replace('#include "parts\\\\03c-procs-off.inc"\n', '')
s = s.replace('//   parts\\\\01-pre.inc            公共头 / 宏 / 辅助函数 / 前向声明',
              '//   PyMfcCommon.h                公共头 / 宏 / 辅助函数 / 前向声明（各 TU 共享）')
s = s.replace('//   parts\\\\03c-procs-off.inc      离屏渲染（PNG / D2D / nanosvg）',
              '//   PyMfcOffscreen.cpp           离屏渲染（PNG / D2D / nanosvg）—— 独立编译单元')
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)

# 4) vcxproj 加编译单元
pj = os.path.join(d, 'PyMfcXaml.vcxproj')
x = io.open(pj, encoding='utf-8-sig').read()
x = x.replace('<ClCompile Include="PyMfcXaml.cpp" />',
              '<ClCompile Include="PyMfcXaml.cpp" />\r\n    <ClCompile Include="PyMfcOffscreen.cpp" />')
x = x.replace('<ClInclude Include="stdafx.h" />',
              '<ClInclude Include="stdafx.h" />\r\n    <ClInclude Include="PyMfcCommon.h" />')
io.open(pj, 'w', encoding='utf-8-sig', newline='').write(x)
print('完成：PyMfcCommon.h / PyMfcOffscreen.cpp / 装配文件 / vcxproj')
