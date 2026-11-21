# -*- coding: utf-8 -*-
import os, sys
ROOT = r'G:\PyArx'
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

x = ('<XWindow Width="400" Height="100">'
     '<XShxText Name="s1" x="10" y="10" w="380" h="80" Text="hello" '
     'FontFile="G:/PyArx/ACAD/Fonts/romans.shx" FontHeight="20"/>'
     '</XWindow>')
try:
    d = M.xml.loadString(x)
    print('ok', d.names())
except Exception as e:
    print('ERR', repr(e))
