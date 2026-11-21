# -*- coding: utf-8 -*-
import os, sys
ROOT = r'G:\PyArx'
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

for tag, xml in [
    ('bare_dwg', '<XWindow Width="760" Height="120"><XDwgCanvas Name="cv" x="20" y="20" w="700" h="80"/></XWindow>'),
    ('shx_nofont', '<XWindow Width="760" Height="120"><XShxText Name="s" x="20" y="20" w="700" h="80" Text="ABC"/></XWindow>'),
]:
    print('try', tag, flush=True)
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    M.pump(20)
    print('  ok', names, flush=True)
    win.close()
    print('  closed', flush=True)
