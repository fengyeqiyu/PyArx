import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
print('module:', [n for n in dir(M) if not n.startswith('_')])
print()
print('Window/ Panel 方法:')
for name in ('Window','Panel'):
    cls = getattr(M, name, None)
    if cls is None: continue
    print(' ', name, [n for n in dir(cls) if n.startswith('add') or n in ('setText','getText','setValue','getValue','onClick','onChange','onDoubleClick','show','savePng','run','loadPowerPlan')])
