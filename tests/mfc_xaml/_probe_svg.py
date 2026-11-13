# -*- coding: utf-8 -*-
"""导出 SVG 取证：检查连线 path 是否为折线 L（不应再出现三次曲线 C）。"""
import os, sys, re
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(120)
svg = app._sceneView()['svg']
out = r'G:\PyArx\docs\_after_export.svg'
with open(out, 'w', encoding='utf-8') as f:
    f.write(svg)

paths = re.findall(r'<path d="([^"]+)"', svg)
print('path 数 =', len(paths))
c_cnt = sum(d.count(' C ') for d in paths)
l_cnt = sum(d.count(' L ') for d in paths)
print('C(三次曲线) 命令数 =', c_cnt, ' L(折线) 命令数 =', l_cnt)
for d in paths[:3]:
    print('样例:', d)
print('OK', flush=True)
os._exit(0)
