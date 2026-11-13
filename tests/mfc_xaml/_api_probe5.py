import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
for n in ('setCheck','getCheck','onChecked','onValue','addCombo','addString','getXamlValue','setXamlValue'):
    f=getattr(M.Window,n,None)
    print('---',n); print((f.__doc__ or '').strip()[:200] if f else 'MISSING')
