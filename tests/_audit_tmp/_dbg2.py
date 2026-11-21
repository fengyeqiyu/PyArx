# -*- coding: utf-8 -*-
import os, sys, re, time
ROOT = r'G:\PyArx'
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
os.environ['PYARX_MFC_DEBUG'] = '1'
os.environ['PYARX_LOG_DIR'] = os.path.join(ROOT, 'tests', '_audit_tmp', 'log')
import PyMfcXml as M

LOG = os.path.join(os.environ['PYARX_LOG_DIR'], 'pymfc_shutdown.log')

def nlines():
    if not os.path.exists(LOG): return 0
    return len(re.findall(r'\[dwg-cv\]', open(LOG,'rb').read().decode('utf-8','replace')))

xml = ('<XWindow Name="root" Width="760" Height="120">'
       '<XShxText Name="shx1" x="20" y="20" w="700" h="80" '
       'Text="SHX probe 0123" FontFile="G:/PyArx/ACAD/Fonts/romans.shx" '
       'BigFontFile="G:/PyArx/ACAD/Fonts/hztxt.shx" FontHeight="20" '
       'WidthFactor="1" Oblique="0" Stroke="#1A1A1A"/></XWindow>')

n0 = nlines()
print('before load', n0)
names = {}
win = M.guiload_text(xml, context=globals(), names=names)
print('after load(no pump)', nlines()-n0, 'names=', names)
M.pump(30)
print('after pump(30)', nlines()-n0)
M.pump(30)
print('after pump(60)', nlines()-n0)
sid = names['shx1']
t0=time.perf_counter(); win.set_xml_value('shx1','FontHeight','22'); t1=time.perf_counter()
print('after 1 set', nlines()-n0, 'set ms=', round((t1-t0)*1000,3))
M.pump(20)
print('after pump(20)', nlines()-n0)
win.close()
print('closed ok')
