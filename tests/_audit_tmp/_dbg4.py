# -*- coding: utf-8 -*-
import os, sys
ROOT = r'G:\PyArx'
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
os.environ['PYARX_MFC_DEBUG'] = '1'
os.environ['PYARX_LOG_DIR'] = os.path.join(ROOT, 'tests', '_audit_tmp', 'log')
import PyMfcXml as M

scene = "<XScene Width='700' Height='80'><XGroup><XPolyline Points='20,6 350,7 680,6.5' Stroke='#1A1A1A' Width='1.2'/></XGroup></XScene>"
xml = ('<XWindow Name="root" Width="760" Height="120">'
       '<XDwgCanvas Name="cv" x="20" y="20" w="700" h="80"/></XWindow>')
names = {}
win = M.guiload_text(xml, context=globals(), names=names)
M.pump(40)
print('names', names, flush=True)
r = win.set_xml_value('cv', 'Data', scene)
print('setXmlValue ret', r, flush=True)
cid = names['cv']
try:
    print('readback Data=', repr(win.get_xml_value(cid, 'Data')[:80]), flush=True)
except Exception as e:
    print('get err', e, flush=True)
M.pump(20)
win.close()
print('closed', flush=True)
