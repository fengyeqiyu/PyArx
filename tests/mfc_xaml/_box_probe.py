import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import pp_app, PyMfcXaml as M
r = M.powerplan.build(pp_app.sampleDoc(), M.powerplan.Style(), 'x', False)
print('boxes[0] =', r['boxes'][0])
print('links[0] =', r['links'][0])
print('nodes[0] =', r['nodes'][0])
